/**
 * @file ptr.hpp
 *
 * A stand-along general-purpose non-intrusive reference-counted pointer.
 *
 * Based on the boost::shared_ptr and yasper smart pointer libraries.
 *
 * @version 1.0
 * @author Kevin McGuinness (kevin.mcguinness {at} gmail {dot} com)
 * @date 2010-04-02 (Last Modified)
 *
 *
 * @verbatim
 * ----------------------------------------------------------------------------
 *  Copyright 2007 Kevin McGuinness
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * ----------------------------------------------------------------------------
 * @endverbatim
 */

#ifndef PTR_HPP_INCLUDED
#define PTR_HPP_INCLUDED


#include <algorithm>
#include <cassert>
#include <new>

#ifndef SMART_PTR_NO_OSTREAM_OPERATOR
	#include <iostream>
#endif


// Supress Intel C++ warning
#if defined(__ICC)
	#pragma warning(disable:981)
#endif


/// smart pointer namespace
namespace smart {


	// Implementation details for static assert
	namespace detail {

		// Templates for the static assert macro
		template <bool x> struct compile_time_check;
		template <> struct compile_time_check<true> {  };
		template <int x> struct check { };

	} /* namespace detail */


	/// Static assertation macro for compile time checks.
	#define ptr_hpp_static_assert(T) \
		typedef detail::check< sizeof( detail::compile_time_check<(bool) (T)> ) > \
		ptr_hpp_static_assertion_##__LINE__;



	// Implementation details for ptr<T>
	namespace detail {


		// Template metaprogramming stuff to check if a type is complete
		template <class T> struct is_complete
		{ static const bool value = (sizeof(T) ? true : false); };


		// Template metaprogramming stuff for compile time array notation detection
		template <class T> struct is_array { static const bool value = false; };
		template <class T> struct is_array<T[]> { static const bool value = true; };


		// Template metaprogramming stuff for extracting underlying type from an array
		template <class T> struct get_type { typedef T value; };
		template <class T> struct get_type<T[]> { typedef T value; };


		// Checked delete of normal types
		template <class T> struct deleter {
			static void free(T* v) {

				// Prevent deleting incomplete types
				ptr_hpp_static_assert(is_complete<T>::value);

				// Delete
				delete v;
			}
		};


		// Checked delete of array types
		template <class T> struct deleter<T[]> {
			template <class U> static void free(U* v) {

				// Prevent deleting incomplete types
				ptr_hpp_static_assert(is_complete<U>::value);

				// Array delete
				delete[] v;
			}
		};

	} /* namespace detail */


/**
 * @brief Stand-alone general-purpose non-intrusive reference-counted
 *        pointer template.
 *
 *
 * Based on the boost::shared_ptr and yasper libraries.
 *
 *
 * @b Requirements:
 * - Contained type destructor or delete operator must @b NEVER throw!
 * - Copy constructor must be provided if you want to use clone()
 *
 *
 * @b Features:
 * - Single header file "ptr.hpp": no dependancies on other libraries
 * - Well documented, with several examples
 * - Well tested with unit tests
 * - Safer to use than yasper (no raw pointer assignment allowed)
 * - Fast: accesses are unchecked in release mode
 * - Small: stack size of 2 pointers, heap overhead one int
 * - Handles array types correctly using the ptr<T[]> x(new T[100]) notation
 * - Handles array indexing using operator []
 * - Includes compile time checks to prevent deleting of incomplete types
 * - Compile time guards for operator -> on arrays
 * - Compile time guards for operator [] on non-arrays
 * - Debug mode run time checks for null pointer dereferencing
 * - Easily clone pointers to non array types using clone()
 * - Easy to type :-D
 *
 *
 * @b Limitations:
 * - Not thread safe
 * - Using incorrect form for array (ex. <tt>ptr< T > p(new T[100])</tt>) is
 *   not detected (language constraint).
 *
 *
 * @b Usage:
 *
 * Creating pointers:
 * @code
 * // Create a pointer to an int with value 5
 * ptr<int> p(new int(5));
 *
 * // Create a pointer to an object of type X
 * ptr<X> q(new X);
 *
 * // Create a null pointer to an object of type Y
 * ptr<Y> r;
 *
 * // Create a pointer to an array
 * ptr<char[]> s(new char[100]);
 * @endcode
 *
 *
 * Dereferencing the pointer:
 * @code
 * // Assign value of pointer to 3
 * *p = 3;
 *
 * // Increment value of pointer
 * (*p)++;
 *
 * // Print pointer value
 * cout << *p << endl;
 * @endcode
 *
 *
 * Calling member functions of objects:
 * @code
 * // Assuming X has a function func()
 * q->func();
 * @endcode
 *
 *
 * Indexing array elements:
 * @code
 * s[0] = '?';
 * @endcode
 *
 *
 * Check if pointer is null:
 * @code
 * // Explicit syntax
 * if (p.null()) {
 *   //...
 * }
 *
 * // Implicit syntax
 * if (!p) {
 *   //...
 * }
 * @endcode
 *
 *
 * Check if pointer is valid:
 * @code
 * // Explicit syntax
 * if (p.valid()) {
 *   //...
 * }
 *
 * // Implicit syntax
 * if (p) {
 *   //...
 * }
 * @endcode
 *
 *
 * Passing the raw pointer to a legacy function:
 * @code
 * // Legacy function
 * void func(X* v) {
 *   v->func();
 * }
 *
 * // Pass raw pointer
 * func(q.get());
 *
 * // Or the shorthand notation
 * func(+q);
 * @endcode
 *
 *
 * Using @b STL algorithms:
 * @code
 * // Create an array of n elements
 * int n = 100;
 * ptr<int[]> x(new int[n]);
 *
 * // Fill it with zeros
 * std::fill(+x, +x+n, 0);
 * @endcode
 *
 *
 * Aliasing the pointer value:
 * @code
 * // Create pointer alias x
 * ptr<int> x(p);
 *
 * // Create null pointer y
 * ptr<int> y;
 *
 * // Set x to y
 * y = x;
 *
 * // Reset y (set to null)
 * y.reset()
 *
 * // Ok, y is null but x is still in scope
 * cout << *x << endl;
 * @endcode
 *
 *
 * Return the pointer from a function:
 * @code
 * ptr< vector<int> > create_vector(int elements) {
 *   // Factory method simplifies creation
 *   return mkptr(new vector<int>(elements));
 * }
 * @endcode
 *
 *
 * Determine if we have a unique reference:
 * @code
 * if (p.unique()) {
 *   // ...
 * }
 * @endcode
 *
 *
 * Determine the reference count:
 * @code
 * int refs = p.count();
 * @endcode
 *
 *
 * Clone the value pointed to by the pointer:
 * @code
 * // Requires copy constructor in X
 * ptr<X> c(q.clone());
 * @endcode
 *
 *
 * Reset the pointer to own a different value:
 * @code
 * // Releases any old resource before aquiring new one
 * p.reset(new int(10));
 * @endcode
 *
 *
 * Swap pointer values:
 * @code
 * ptr<int> a(new int(1));
 * ptr<int> b(new int(2));
 *
 * // Swap pointers, now *a = 2 , *b = 1
 * swap(a, b);
 *
 * // Alternative notation
 * a.swap(b);
 * @endcode
 *
 *
 * @b Polymorphism is supported:
 * @code
 * struct A {
 *   virtual ~A() { }
 *   virtual void func() const { cout << "A" << endl; }
 * };
 *
 * struct B : public A {
 *   virtual ~B() { }
 *   virtual void func() const { cout << "B" << endl; }
 *   void bfunc() const { }
 * };
 *
 * // Create polymorphic pointer
 * ptr<A> a(new B);
 *
 * // prints "B"
 * a->func();
 *
 * // Downcast pointer
 * ptr<B> b(ptr_dynamic_cast<B>(a));
 *
 * // Call function unique to B
 * p->bfunc();
 *
 * // Can copy construct and assign too
 * ptr<A> c(b);
 * @endcode
 *
 * <b> Handling Constness </b>
 *
 * Handling constness is a little different than you might expect, but it
 * actually makes sense if you think about it. If you want a function to
 * accept a pointer to a const int, for example, declare it as the following:
 *
 * @code
 * // Use this (note that we're not passing by reference)
 * void func(ptr<const int> p) {
 *   // read only *p
 * }
 *
 * // rather than this...
 * void func(const ptr<int>& p) {
 *   // can write to *p!, the ptr<> is const!
 * }
 * @endcode
 *
 * Pass by reference may not be used in the first function declaration: the
 * compiler is unable to generate the necessary temporary. Using the first
 * function incurs a small overhead (the copy construction); for functions
 * where speed is critical functions, a raw pointer may be more appropriate:
 *
 * @code
 * // Speed critical function
 * void func(const int* p) {
 *   // read only *p
 * }
 *
 * ptr<int> x(new int(5));
 *
 * // Now use the unary + operator to get the raw pointer
 * func(+x);
 *
 * // Same technique applies for arrays
 * @endcode
 *
 *
 * @b Compilers (tested on):
 * - GNU C++ Compiler (GCC 4.1.2)
 * - Intel C++ Compiler (ICC 10.0.023)
 * - Microsoft Visual C++ Compiler 8.0 (MSVC8)
 *
 * @author
 *         Kevin McGuinness
 *
 * @version
 *         1.0.2
 *
 * @date
 *         2010-04-02 (Last Modified)
 *
 */
template <class T>
struct ptr {

	/// All types of this template are friends
	template <class U> friend class ptr;


	/// Type of contained element
	typedef typename detail::get_type< T >::value element_t;


	/// Static constant indicating whether or not the contained element is an array
	static const bool is_array = detail::is_array< T >::value;


	/**
	 * Null pointer constructor.
	 *
	 * @throws
	 *         Never throws.
	 *
	 * @note   @e Implementation: Having an extra constructor is more efficient
	 *         than defaulting the parameter of the next constructor to zero when
	 *         creating null pointers.
	 */
	ptr()
	: px(0), pn(0)
	{
	}


	/**
	 * Standard constructor.
	 *
	 * This constructor is explicit in order to prevent unintentional conversion
	 * from a raw pointer to a @c ptr<T> pointer, which could result in the raw
	 * pointer being prematurely deleted. For example, if this constructor was
	 * not explicit, and you passed a raw pointer to a function expecting a ptr
	 * object, then the raw pointer would be converted to a ptr implicitly and
	 * deleted when the function returns. This means that you must construct
	 * ptr objects using the following syntax:
	 *
	 * @code
	 * // Correct syntax
	 * ptr<T> x(new T);
	 *
	 * // Incorrect syntax - wont compile!
	 * ptr<T> x = new T;
	 * @endcode
	 *
	 *
	 * @throws
	 *         std::bad_alloc In the unlikely event that the counter is not
	 *         successfully allocated.
	 *
	 */
	explicit ptr(element_t* p)
	: px(p), pn(0)
	{
		aquire();
	}


	/**
	 * Copy constructor.
	 *
	 * Copy the pointer and increment the shared reference count if the passed
	 * pointer is not null.
	 *
	 * @throws
	 *         std::bad_alloc In the unlikely event that the counter is not
	 *         successfully allocated.
	 *
	 */
	ptr(const ptr& x)
	: px(x.px), pn(x.pn)
	{
		aquire();
	}


	/// Conversion copy constructor
	template <class U>
	ptr(const ptr<U>& x)
	: px(x.px), pn(x.pn)
	{
		aquire();
	}


	/**
	 * Destructor.
	 *
	 * If the pointer is not null, decrement the reference count and if the
	 * reference count then equals zero, free the pointer and shared count.
	 *
	 * @throws
	 *         Never throws.
	 *
	 */
	~ptr()
	{
		release();
	}


	/**
	 * Assignment.
	 *
	 * Assign the pointer to this if it is not already the same object. The
	 * assignment decrements the reference counter for the already contained
	 * object in the reciever if it is not null. The pointer and reference
	 * count are then assigned and the reference count is incremented.
	 *
	 * @throws
	 *         Never throws.
	 */
	ptr& operator = (const ptr& x)
	{
		assign(x);
		return *this;
	}


	/// Conversion assignment
	template <class U>
	ptr& operator = (const ptr<U>& x)
	{
		assign(x);
		return *this;
	}


	/**
	 * Indirection operator (non arrays only).
	 * Asserts the pointer is valid in debug mode.
	 *
	 * @throws
	 *         Never throws.
	 */
	inline element_t* operator -> () const {
		ptr_hpp_static_assert (!is_array);
		assert (px != 0);
		return px;
	}


	/**
	 * Dereference operator.
	 * Asserts the pointer is valid in debug mode.
	 *
	 * @throws
	 *         Never throws.
	 */
	inline element_t& operator * () const {
		assert (px != 0);
		return *px;
	}


	/**
	 * Array element access operator (arrays only).
	 * Asserts valid in debug mode.
	 *
	 * @throws
	 *         Never throws.
	 */
	inline element_t& operator[] (int idx) const {
		ptr_hpp_static_assert(is_array);
		assert (px != 0);
		return px[idx];
	}


	/**
	 * Unary operator+: returns the raw pointer.
	 *
	 * This allows a shorthand notation for passing the raw pointer to a function
	 * that expects a raw pointer.
	 *
	 * @note   Why the '+' operator? It looks nice and has high precedence.
	 *
	 * @throws
	 *         Never throws.
	 */
	inline element_t* operator + () const {
		return px;
	}


	/**
	 * Conversion to bool: converts to true if not null.
	 *
	 * @throws
	 *         Never throws.
	 */
	inline operator bool () const {
		return px != 0;
	}


	/**
	 * Not operator: returns false if null.
	 *
	 * @throws
	 *         Never throws.
	 */
	inline bool operator ! () const {
		return px == 0;
	}


	/**
	 * Equality operator: compares values of the raw pointers.
	 *
	 * @throws
	 *         Never throws.
	 */
	template <class U>
	bool operator == (const ptr<U>& x) const {
		return px == x.px;
	}


	/**
	 * Inequality operator: compares values of raw pointers.
	 *
	 * @throws
	 *         Never throws.
	 */
	template <class U>
	bool operator != (const ptr<U>& x) const {
		return px != x.px;
	}


	/**
	 * Less than operator (for containers).
	 *
	 * @throws
	 *         Never throws.
	 */
	template <class U>
	bool operator < (const ptr<U>& x) const {
		return px < x.px;
	}


	/**
	 * Returns the raw pointer.
	 *
	 * @throws
	 *         Never throws.
	 */
	element_t* get() const {
		return px;
	}


	/**
	 * Returns true if valid (non null)
	 *
	 * @throws
	 *         Never throws.
	 */
	bool valid() const {
		return px != 0;
	}


	/**
	 * Returns true if the pointer is null (invalid).
	 *
	 * @throws
	 *         Never throws.
	 */
	bool null() const {
		return px == 0;
	}


	/**
	 * Returns the reference count, which is always zero for a null pointer.
	 *
	 * @throws
	 *         Never throws.
	 */
	int count() const {
		return pn ? *pn : 0;
	}


	/**
	 * Returns true if reference count is one or pointer is null.
	 *
	 * @throws
	 *         Never throws.
	 */
	bool unique() const {
		return px ? count() == 1 : true;
	}


	/**
	 * Reset the pointer: set null.
	 *
	 * @throws
	 *         Never throws.
	 */
	void reset() {
		ptr<T>().swap(*this);
	}


	/**
	 * Reset the pointer to the given value.
	 *
	 * In debug mode this function asserts that if the passed pointer is not 0,
	 * it is not equal to the contained pointer in debug mode. This helps to
	 * catch self reset errors.
	 *
	 * @throws
	 *         std::bad_alloc In the unlikely event that the counter is not
	 *         successfully allocated.
	 */
	void reset(element_t* x) {
		assert(x == 0 || x != px);
		ptr<T>(x).swap(*this);
	}


	/**
	 * Clone the element held by the pointer, returning a pointer to the new
	 * element. Requires the contained element class to be copy-constructable.
	 * Does not apply to arrays.
	 *
	 * @throws
	 *         ... If the contained element type's copy constructor throws then
	 *         the exception is propagated.
	 */
	ptr<T> clone() const {
		ptr_hpp_static_assert(!is_array);
		assert(px != 0);
		return ptr<T>(new T(*px));
	}


	/**
	 * Swap the pointer with another.
	 *
	 * @throws
	 *         Never throws.
	 */
	void swap(ptr& x) {
		std::swap(px, x.px);
		std::swap(pn, x.pn);
	}


	/**
	 * Assign the pointer.
	 *
	 * @throws
	 *         Never throws.
	 */
	void assign(const ptr& x) {
		if (this != &x) {
			release();
			px = x.px;
			pn = x.pn;

			// faster than if(pn) (*pn)++
			pn && (*pn)++;
		}
	}


	/**
	 * Assign the pointer (convert version).
	 *
	 * @throws
	 *         Never throws.
	 */
	template <class U>
	void assign(const ptr<U>& x) {
		if (this != (ptr<T>*) &x) {
			release();
			px = x.px;
			pn = x.pn;

			// faster than if(pn) (*pn)++
			pn && (*pn)++;
		}
	}


	/// Cast pointer (c style cast)
	template <class U, class V>
	friend ptr<U> ptr_cast(const ptr<V>& x);


	/// Static cast pointer
	template <class U, class V>
	friend ptr<U> ptr_static_cast(const ptr<V>& x);


	/// Dynamic cast pointer
	template <class U, class V>
	friend ptr<U> ptr_dynamic_cast(const ptr<V>& x);


	/// Const cast pointer
	template <class U, class V>
	friend ptr<U> ptr_const_cast(const ptr<V>& x);


	/// Reinterpret cast pointer
	template <class U, class V>
	friend ptr<U> ptr_reinterpret_cast(const ptr<V>& x);


private:


	/// Type of the counter
	typedef unsigned int counter_t;


	/// Internal constructor
	inline ptr(element_t* x, counter_t* n)
	: px(x), pn(n)
	{
		aquire();
	}


	/// Dereference pointer
	inline void release() {
		if (pn && --(*pn) == 0) {
			delete pn;

			// Calls correct delete function
			detail::deleter<T>::free(px);
		}

		px = 0, pn = 0;
	}


	/// Refrerence pointer
	inline void aquire() {

		if (px) {
			if (pn) {
				// Counter exists
				++(*pn);

			} else {
				// Create counter
				create_pn();
			}
		}
	}


	// Create counter
	inline void create_pn() {
		// Allocate counter and surpress throw
		pn = new (std::nothrow) counter_t(1);

		if (!pn) {
			// Free element
			detail::deleter<T>::free(px);
			px = 0; pn = 0;

			// Throw
			throw std::bad_alloc();
		}
	}


private:


	// Raw pointer
	element_t* px;


	// Reference count
	counter_t* pn;
};


#ifndef SMART_PTR_NO_OSTREAM_OPERATOR

/// Stream insertion operator
template <typename T>
std::ostream& operator << (std::ostream& out, const ptr<T>& x) {
	out << "smart::ptr<T>(";
	if (x.null()) {
		out << "NULL)";
	} else {
		out << x.get() << ",refs=" << x.count() << ")";
	}
	return out;
}

#endif


/// Swap pointers
template<class T>
inline void swap(ptr<T>& a, ptr<T>& b) {
	a.swap(b);
}


/// General pointer cast
template <class U, class T>
inline ptr<U> ptr_cast(const ptr<T>& x) {
	typedef typename ptr<U>::element_t* cast_t;
	return ptr<U>((cast_t) (x.get()), x.pn);
}


/// Static pointer cast
template <class U, class T>
inline ptr<U> ptr_static_cast(const ptr<T>& x) {
	typedef typename ptr<U>::element_t* cast_t;
	return ptr<U>(static_cast<cast_t>(x.get()), x.pn);
}


/// Dynamic pointer cast
template <class U, class T>
inline ptr<U> ptr_dynamic_cast(const ptr<T>& x) {
	typedef typename ptr<U>::element_t* cast_t;
	return ptr<U>(dynamic_cast<cast_t>(x.get()), x.pn);
}


/// Const pointer cast
template <class U, class T>
inline ptr<U> ptr_const_cast(const ptr<T>& x) {
	typedef typename ptr<U>::element_t* cast_t;
	return ptr<U>(const_cast<cast_t>(x.get()), x.pn);
}


/// Reinterpret pointer cast
template <class U, class T>
inline ptr<U> ptr_reinterpret_cast(const ptr<T>& x) {
	typedef typename ptr<U>::element_t* cast_t;
	return ptr<U>(reinterpret_cast<cast_t>(x.get()), x.pn);
}


/// Factory function
template <class T>
inline ptr<T> mkptr(T* v) {
	return ptr<T>(v);
}


/// Factory function, calls default constructor
template <class T>
inline ptr<T> mkptr() {
	return ptr<T>(new T);
}


} /* end namespace smart */

#endif /* PTR_HPP_INCLUDED */
