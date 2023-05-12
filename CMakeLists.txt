#CMake file for drumrox
cmake_minimum_required(VERSION 3.0)
project (drumrox VERSION 1.0.0 LANGUAGES CXX C)

set(LV2_INSTALL_DIR lib/lv2 CACHE PATH "Specifies where the LV2 libraries should be installed")
set(CMAKE_C_FLAGS "-Wall")

# Available options
option(USE_NKNOB  "Use custom NKnob widgets for gain/pan instead of Gtk sliders" ON)
SET(SAMP_ZERO_POS "0" CACHE STRING "Default sample zero position 0=top left (tl) 1=bl 2=tr 3=br")

# check for our various libraries
find_package(PkgConfig)
pkg_check_modules(LV2 REQUIRED lv2>=0.1.0)
pkg_check_modules(SNDFILE REQUIRED sndfile>=1.0.20)
pkg_check_modules(SAMPLERATE REQUIRED samplerate>=0.1.5)
pkg_check_modules (GTK2 REQUIRED gtk+-2.0>=2.22.0)
find_package (EXPAT REQUIRED)

include_directories (${LV2_INCLUDE_DIRS} ${EXPAT_INCLUDE_DIRS} ${SNDFILE_INCLUDE_DIRS} ${SAMPLERATE_INCLUDE_DIRS} ${GTK2_INCLUDE_DIRS})
link_directories (${LV2_LIBRARY_DIRS} ${EXPAT_LIBRARY_DIRS} ${SNDFILE_LIBRARY_DIRS} ${SAMPLERATE_LIBRARY_DIRS} ${GTK2_LIBRARY_DIRS})

# config libraries

add_library(drumrox SHARED
  drumrox.cpp
  drumrox_hydrogen.cpp
  drumrox.h
  drumrox_hydrogen.h
)

add_library(drumrox_ui SHARED
  drumrox_ui.cpp
  drumrox_hydrogen.cpp
  nknob.c
  drumrox_hydrogen.h
  nknob.h
)

set_target_properties(drumrox PROPERTIES PREFIX "")
set_target_properties(drumrox_ui PROPERTIES PREFIX "")
set_target_properties(drumrox_ui
  PROPERTIES
  LINK_FLAGS "-Wl,-znodelete"
)

target_link_libraries(drumrox ${LV2_LIBRARIES} ${SNDFILE_LIBRARIES} ${SAMPLERATE_LIBRARIES} ${EXPAT_LIBRARIES} m)
target_link_libraries(drumrox_ui ${LV2_LIBRARIES} ${GTK2_LIBRARIES} ${SNDFILE_LIBRARIES} ${SAMPLERATE_LIBRARIES} ${EXPAT_LIBRARIES} m)


add_definitions ( -DPIC )

#set_target_properties (drumrox
#  PROPERTIES
#  COMPILE_FLAGS "-std=gnu99"
#)

#set (ui_compile_flags "-std=gnu99")

if (NOT USE_NKNOB)
  set (ui_compile_flags "${ui_compile_flags} -DNO_NKNOB")
endif (NOT USE_NKNOB)

if (SAMP_ZERO_POS GREATER "-1" AND SAMP_ZERO_POS LESS "4")
   set (ui_compile_flags "${ui_compile_flags} -DDRMR_UI_ZERO_SAMP=${SAMP_ZERO_POS}")
else (SAMP_ZERO_POS GREATER "-1" AND SAMP_ZERO_POS LESS "4")
   message(WARNING "Invalid SAMP_ZERO_POS, defaulting to 0 \(top left\).  Valid values are 0,1,2,3")
   set (ui_compile_flags "${ui_compile_flags} -DDRMR_UI_ZERO_SAMP=0")
endif (SAMP_ZERO_POS GREATER "-1" AND SAMP_ZERO_POS LESS "4")

set_target_properties (drumrox_ui
  PROPERTIES
  COMPILE_FLAGS ${ui_compile_flags}
)

# Custom test commands
add_executable ( htest
  EXCLUDE_FROM_ALL
  drumrox_hydrogen.cpp
)

add_executable ( knobt
  EXCLUDE_FROM_ALL
  nknob.c
)

# config test executables
target_link_libraries(htest ${LV2_LIBRARIES} ${GTK2_LIBRARIES} ${SNDFILE_LIBRARIES} ${SAMPLERATE_LIBRARIES} ${EXPAT_LIBRARIES} m)
set_target_properties ( htest
  PROPERTIES
  COMPILE_FLAGS "-D_TEST_HYDROGEN_PARSER"
)
target_link_libraries(knobt ${LV2_LIBRARIES} ${GTK2_LIBRARIES} ${SNDFILE_LIBRARIES} ${SAMPLERATE_LIBRARIES} ${EXPAT_LIBRARIES} m)
set_target_properties ( knobt
  PROPERTIES
  COMPILE_FLAGS "-D_TEST_N_KNOB"
)

# config install
install(TARGETS drumrox drumrox_ui
  LIBRARY
  DESTINATION ${CMAKE_INSTALL_PREFIX}/${LV2_INSTALL_DIR}/drumrox.lv2
)

install (FILES manifest.ttl drumrox.ttl knob.png led_on.png led_off.png
  DESTINATION ${CMAKE_INSTALL_PREFIX}/${LV2_INSTALL_DIR}/drumrox.lv2
)
