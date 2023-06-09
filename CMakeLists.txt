#CMake file for drumrox
cmake_minimum_required(VERSION 3.0)
project (drumrox VERSION 2.1.0 LANGUAGES CXX C)



set(LV2_INSTALL_DIR lib/lv2 CACHE PATH "Specifies where the LV2 libraries should be installed")
#set(CMAKE_C_FLAGS "-Wall")

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g -O3 ")


option(USE_CLANG "Build with Clang" OFF)


if(USE_CLANG)
  set(CMAKE_CXX_COMPILER clang++)
  set(CMAKE_C_COMPILER clang)
  message("+ Build with Clang++")
endif(USE_CLANG)



# Available options
option(USE_NKNOB  "Use custom NKnob widgets for gain/pan instead of Gtk sliders" ON)
#SET(SAMP_ZERO_POS "0" CACHE STRING "Default sample zero position 0=top left (tl) 1=bl 2=tr 3=br")

# check for our various libraries
find_package(PkgConfig)
pkg_check_modules(LV2 REQUIRED lv2>=0.1.0)
pkg_check_modules(SNDFILE REQUIRED sndfile>=1.0.20)
pkg_check_modules(SAMPLERATE REQUIRED samplerate>=0.1.5)
pkg_check_modules (GTK2 REQUIRED gtk+-2.0>=2.22.0)

include_directories (${LV2_INCLUDE_DIRS} ${SNDFILE_INCLUDE_DIRS} ${SAMPLERATE_INCLUDE_DIRS} ${GTK2_INCLUDE_DIRS})
link_directories (${LV2_LIBRARY_DIRS} ${SNDFILE_LIBRARY_DIRS} ${SAMPLERATE_LIBRARY_DIRS} ${GTK2_LIBRARY_DIRS})

# config libraries

add_library(drumrox SHARED
  utl.cpp
  utl.h
  kits.cpp
  kits.h
  pugiconfig.hpp
  pugixml.cpp
  pugixml.hpp
  dsp.cpp
  dsp.h
  drumrox.cpp
  drumrox.h
)

add_library(drumrox_ui SHARED
  utl.cpp
  utl.h
  kits.cpp
  kits.h
  pugiconfig.hpp
  pugixml.cpp
  pugixml.hpp
  dsp.cpp
  dsp.h
  drumrox_ui.cpp
  nknob.c
  nknob.h
)

set_target_properties(drumrox PROPERTIES PREFIX "")
set_target_properties(drumrox_ui PROPERTIES PREFIX "")
set_target_properties(drumrox_ui
  PROPERTIES
  LINK_FLAGS "-Wl,-znodelete"
)


target_link_libraries(drumrox ${LV2_LIBRARIES} ${SNDFILE_LIBRARIES} ${SAMPLERATE_LIBRARIES})
target_link_libraries(drumrox_ui ${LV2_LIBRARIES} ${GTK2_LIBRARIES} ${SNDFILE_LIBRARIES} ${SAMPLERATE_LIBRARIES})


add_definitions ( -DPIC )


if (NOT USE_NKNOB)
#  set (ui_compile_flags "${ui_compile_flags} -DNO_NKNOB")
#  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNO_NKNOB")
add_definitions(-DNO_NKNOB=1)
endif (NOT USE_NKNOB)

#if (SAMP_ZERO_POS GREATER "-1" AND SAMP_ZERO_POS LESS "4")
#   set (ui_compile_flags "${ui_compile_flags} -DDRMR_UI_ZERO_SAMP=${SAMP_ZERO_POS}")
#else (SAMP_ZERO_POS GREATER "-1" AND SAMP_ZERO_POS LESS "4")
#   message(WARNING "Invalid SAMP_ZERO_POS, defaulting to 0 \(top left\).  Valid values are 0,1,2,3")
#   set (ui_compile_flags "${ui_compile_flags} -DDRMR_UI_ZERO_SAMP=0")
#endif (SAMP_ZERO_POS GREATER "-1" AND SAMP_ZERO_POS LESS "4")

#set_target_properties (drumrox_ui
#  PROPERTIES
  #COMPILE_FLAGS ${ui_compile_flags}
#)


# config install
install(TARGETS drumrox drumrox_ui
  LIBRARY
  DESTINATION ${CMAKE_INSTALL_PREFIX}/${LV2_INSTALL_DIR}/drumrox.lv2
)

install (FILES manifest.ttl drumrox.ttl knob.png led_on.png led_off.png
  DESTINATION ${CMAKE_INSTALL_PREFIX}/${LV2_INSTALL_DIR}/drumrox.lv2
)
