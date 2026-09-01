# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "C:/Users/tecse/OneDrive/Documents/GitHub/BioZ_Sense/build/firmware/_deps/cmsis_core-src")
  file(MAKE_DIRECTORY "C:/Users/tecse/OneDrive/Documents/GitHub/BioZ_Sense/build/firmware/_deps/cmsis_core-src")
endif()
file(MAKE_DIRECTORY
  "C:/Users/tecse/OneDrive/Documents/GitHub/BioZ_Sense/build/firmware/_deps/cmsis_core-build"
  "C:/Users/tecse/OneDrive/Documents/GitHub/BioZ_Sense/build/firmware/_deps/cmsis_core-subbuild/cmsis_core-populate-prefix"
  "C:/Users/tecse/OneDrive/Documents/GitHub/BioZ_Sense/build/firmware/_deps/cmsis_core-subbuild/cmsis_core-populate-prefix/tmp"
  "C:/Users/tecse/OneDrive/Documents/GitHub/BioZ_Sense/build/firmware/_deps/cmsis_core-subbuild/cmsis_core-populate-prefix/src/cmsis_core-populate-stamp"
  "C:/Users/tecse/OneDrive/Documents/GitHub/BioZ_Sense/build/firmware/_deps/cmsis_core-subbuild/cmsis_core-populate-prefix/src"
  "C:/Users/tecse/OneDrive/Documents/GitHub/BioZ_Sense/build/firmware/_deps/cmsis_core-subbuild/cmsis_core-populate-prefix/src/cmsis_core-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/tecse/OneDrive/Documents/GitHub/BioZ_Sense/build/firmware/_deps/cmsis_core-subbuild/cmsis_core-populate-prefix/src/cmsis_core-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/tecse/OneDrive/Documents/GitHub/BioZ_Sense/build/firmware/_deps/cmsis_core-subbuild/cmsis_core-populate-prefix/src/cmsis_core-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
