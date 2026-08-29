# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/home/hamza/ad5940-examples/build/_deps/cmsis_core-src")
  file(MAKE_DIRECTORY "/home/hamza/ad5940-examples/build/_deps/cmsis_core-src")
endif()
file(MAKE_DIRECTORY
  "/home/hamza/ad5940-examples/build/_deps/cmsis_core-build"
  "/home/hamza/ad5940-examples/build/_deps/cmsis_core-subbuild/cmsis_core-populate-prefix"
  "/home/hamza/ad5940-examples/build/_deps/cmsis_core-subbuild/cmsis_core-populate-prefix/tmp"
  "/home/hamza/ad5940-examples/build/_deps/cmsis_core-subbuild/cmsis_core-populate-prefix/src/cmsis_core-populate-stamp"
  "/home/hamza/ad5940-examples/build/_deps/cmsis_core-subbuild/cmsis_core-populate-prefix/src"
  "/home/hamza/ad5940-examples/build/_deps/cmsis_core-subbuild/cmsis_core-populate-prefix/src/cmsis_core-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/hamza/ad5940-examples/build/_deps/cmsis_core-subbuild/cmsis_core-populate-prefix/src/cmsis_core-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/hamza/ad5940-examples/build/_deps/cmsis_core-subbuild/cmsis_core-populate-prefix/src/cmsis_core-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
