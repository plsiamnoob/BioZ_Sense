# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "")
  file(REMOVE_RECURSE
  [[GUI_App\CMakeFiles\GUI_App_autogen.dir\AutogenUsed.txt]]
  [[GUI_App\CMakeFiles\GUI_App_autogen.dir\ParseCache.txt]]
  [[GUI_App\GUI_App_autogen]]
  )
endif()
