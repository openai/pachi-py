#pragma once

#include <stdexcept>
#include <string>
#include <Python.h>

extern "C" {

// Exceptions and Python equivalents
class IllegalMove : public std::runtime_error {
public:
    explicit IllegalMove(const std::string& s) : std::runtime_error(s) { }
};

class PachiEngineError : public std::runtime_error {
public:
    explicit PachiEngineError(const std::string& s) : std::runtime_error(s) { }
};

extern PyObject* _PyIllegalMove;
extern PyObject* _PyPachiEngineError;
inline void raise_py_error() {
    try {
        throw;
    } catch (const IllegalMove& e) {
        PyErr_SetString(_PyIllegalMove, e.what());
    } catch (const PachiEngineError& e) {
        PyErr_SetString(_PyPachiEngineError, e.what());
    }
}

} // extern "C"
