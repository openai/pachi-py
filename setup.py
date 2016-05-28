from setuptools.command.build_ext import build_ext as _build_ext
from setuptools import setup, Extension
import subprocess

# For building Pachi as a library
class BuildLibPachi(_build_ext):
    def run(self):
        try:
            subprocess.check_call("cd pachi_py; mkdir -p build && cd build && cmake ../pachi && make -j4", shell=True)
        except subprocess.CalledProcessError as e:
            print("Could not build pachi-py: %s" % e)
            raise
        # Prevent numpy from trying to setup
        __builtins__.__NUMPY_SETUP__ = False
        import numpy
        self.include_dirs.append(numpy.get_include())
        _build_ext.run(self)

# Cython recommands checking in the Cython-generated C files
# (cf. http://stackoverflow.com/a/19138055).
# After changing pachi_py/cypachi.pyx, recompile using:
#
#     $ cython --cplus pachi_py/cypachi.pyx
#
ext = Extension(
    name="pachi_py.cypachi",
    sources=["pachi_py/cypachi.cpp", "pachi_py/goutil.cpp"],
    language="c++",
    include_dirs=["pachi_py/pachi"],
    libraries=["pachi"],
    library_dirs=["pachi_py/build/lib"], # this is the output dir of BuildLibPachi
    extra_compile_args=["-std=c++11"],
)

setup(name='pachi-py',
      version='0.0.20',
      description='Python bindings to Pachi',
      url='https://github.com/openai/pachi-py',
      author='OpenAI',
      author_email='info@openai.com',
      packages=['pachi_py'],
      cmdclass={'build_ext': BuildLibPachi},
      setup_requires=['numpy'],
      install_requires=['numpy'],
      tests_require=['nose2'],
      classifiers=['License :: OSI Approved :: GNU General Public License v2 (GPLv2)'],
      ext_modules=[ext],
      include_package_data=True,
)
