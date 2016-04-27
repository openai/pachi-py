# For building Pachi as a library (libpachi)

cmake_minimum_required(VERSION 2.8)
project(pachi)

# http://cmake.3232098.n2.nabble.com/Default-value-for-CMAKE-BUILD-TYPE-td7550756.html
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    CACHE PATH "Output directory for static libraries.")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    CACHE PATH "Output directory for shared libraries.")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    CACHE PATH "Output directory for executables and DLL's.")

list(APPEND CMAKE_C_FLAGS "-std=c99")
list(APPEND CMAKE_CXX_FLAGS "-std=c++11")

# list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
# include(${CMAKE_SOURCE_DIR}/cmake/UseCython.cmake)

# find_package(PythonLibs REQUIRED)
# find_package(Numpy REQUIRED)

if (APPLE)
  set(CMAKE_MACOSX_RPATH 0)
endif()


set(PACHI_DIR ${CMAKE_SOURCE_DIR})
add_library(pachi STATIC
    ${PACHI_DIR}/board.c
    ${PACHI_DIR}/chat.c
    ${PACHI_DIR}/distributed/distributed.c
    ${PACHI_DIR}/distributed/merge.c
    ${PACHI_DIR}/distributed/protocol.c
    ${PACHI_DIR}/fbook.c
    ${PACHI_DIR}/gtp.c
    ${PACHI_DIR}/joseki/base.c
    ${PACHI_DIR}/joseki/joseki.c
    ${PACHI_DIR}/libpachi_globals.c
    ${PACHI_DIR}/montecarlo/montecarlo.c
    ${PACHI_DIR}/move.c
    ${PACHI_DIR}/network.c
    ${PACHI_DIR}/ownermap.c
    # ${PACHI_DIR}/pachi.c
    ${PACHI_DIR}/pattern3.c
    ${PACHI_DIR}/pattern.c
    ${PACHI_DIR}/patternplay/patternplay.c
    ${PACHI_DIR}/patternprob.c
    ${PACHI_DIR}/patternscan/patternscan.c
    ${PACHI_DIR}/patternsp.c
    ${PACHI_DIR}/playout.c
    ${PACHI_DIR}/playout/light.c
    ${PACHI_DIR}/playout/moggy.c
    ${PACHI_DIR}/probdist.c
    ${PACHI_DIR}/random.c
    ${PACHI_DIR}/random/random.c
    ${PACHI_DIR}/replay/replay.c
    ${PACHI_DIR}/stone.c
    ${PACHI_DIR}/tactics/1lib.c
    ${PACHI_DIR}/tactics/2lib.c
    ${PACHI_DIR}/tactics/ladder.c
    ${PACHI_DIR}/tactics/nakade.c
    ${PACHI_DIR}/tactics/nlib.c
    ${PACHI_DIR}/tactics/selfatari.c
    ${PACHI_DIR}/tactics/util.c
    ${PACHI_DIR}/timeinfo.c
    # ${PACHI_DIR}/t-play/autotest/rc
    ${PACHI_DIR}/t-unit/test.c
    ${PACHI_DIR}/uct/dynkomi.c
    # ${PACHI_DIR}/uct/plugin/example.c
    ${PACHI_DIR}/uct/plugins.c
    # ${PACHI_DIR}/uct/plugin/wolf.c
    ${PACHI_DIR}/uct/policy/generic.c
    ${PACHI_DIR}/uct/policy/ucb1amaf.c
    ${PACHI_DIR}/uct/policy/ucb1.c
    ${PACHI_DIR}/uct/prior.c
    ${PACHI_DIR}/uct/search.c
    ${PACHI_DIR}/uct/slave.c
    ${PACHI_DIR}/uct/tree.c
    ${PACHI_DIR}/uct/uct.c
    ${PACHI_DIR}/uct/walk.c
    ${PACHI_DIR}/libpachi_globals.c
)
target_include_directories(pachi PRIVATE ${PACHI_DIR})
# -D_GNU_SOURCE for weird hostent issue, see https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=522007
target_compile_options(pachi PRIVATE "-fPIC" "-D_GNU_SOURCE -Wall -Wextra -Wno-unused-function -Wno-sign-compare -Wno-unused-parameter -Wno-unused-variable")
#set(CMAKE_C_FLAGS "-Wall -Wextra -Wno-unused-function -Wno-sign-compare -Wno-unused-parameter -Wno-unused-variable ${CMAKE_C_FLAGS}")
