#CMake file for drumrox
cmake_minimum_required(VERSION 3.0)
project (drumrox VERSION 2.2.1 LANGUAGES CXX C)


set(LV2_INSTALL_DIR lib/lv2 CACHE PATH "Specifies where the LV2 libraries should be installed")

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g -O3 ")


option(USE_CLANG "Build with Clang" OFF)
option(USE_MULTI "Build with Drumrox-multi" OFF)


option(USE_NKNOB  "Use custom NKnob widgets for gain/pan instead of Gtk sliders" ON)

add_definitions ( -DPIC )

if(USE_CLANG)
  set(CMAKE_CXX_COMPILER clang++)
  set(CMAKE_C_COMPILER clang)
  message("+ Build with Clang++")
endif(USE_CLANG)


find_package(PkgConfig)
pkg_check_modules(LV2 REQUIRED lv2>=0.1.0)
pkg_check_modules(SNDFILE REQUIRED sndfile>=1.0.20)
pkg_check_modules(SAMPLERATE REQUIRED samplerate>=0.1.5)
pkg_check_modules (GTK2 REQUIRED gtk+-2.0>=2.22.0)

include_directories (${LV2_INCLUDE_DIRS} ${SNDFILE_INCLUDE_DIRS} ${SAMPLERATE_INCLUDE_DIRS} ${GTK2_INCLUDE_DIRS})
link_directories (${LV2_LIBRARY_DIRS} ${SNDFILE_LIBRARY_DIRS} ${SAMPLERATE_LIBRARY_DIRS} ${GTK2_LIBRARY_DIRS})




#add_definitions(-DDRUMROX_MULTI=1)
message("+ Build Drumrox-multi")

add_library(drumrox-multi SHARED
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

add_library(drumrox_ui-multi SHARED
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

set_target_properties(drumrox-multi PROPERTIES PREFIX "")
set_target_properties(drumrox_ui-multi PROPERTIES PREFIX "")
set_target_properties(drumrox_ui-multi
  PROPERTIES
  LINK_FLAGS "-Wl,-znodelete"
)

target_compile_definitions(drumrox-multi PRIVATE DRUMROX_MULTI=1)
target_compile_definitions(drumrox_ui-multi PRIVATE DRUMROX_MULTI=1)

target_link_libraries(drumrox-multi ${LV2_LIBRARIES} ${SNDFILE_LIBRARIES} ${SAMPLERATE_LIBRARIES})
target_link_libraries(drumrox_ui-multi ${LV2_LIBRARIES} ${GTK2_LIBRARIES} ${SNDFILE_LIBRARIES} ${SAMPLERATE_LIBRARIES})

message("+ Build Drumrox")


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



if (NOT USE_NKNOB)
add_definitions(-DNO_NKNOB=1)
endif (NOT USE_NKNOB)




# config install
install(TARGETS drumrox-multi drumrox_ui-multi
  LIBRARY
  DESTINATION ${CMAKE_INSTALL_PREFIX}/${LV2_INSTALL_DIR}/drumrox-multi.lv2
)

install (FILES ttl-multi/manifest.ttl ttl-multi/drumrox-multi.ttl knob.png led_on.png led_off.png
  DESTINATION ${CMAKE_INSTALL_PREFIX}/${LV2_INSTALL_DIR}/drumrox-multi.lv2
)



# config install
install(TARGETS drumrox drumrox_ui
  LIBRARY
  DESTINATION ${CMAKE_INSTALL_PREFIX}/${LV2_INSTALL_DIR}/drumrox.lv2
)

install (FILES ttl/manifest.ttl ttl/drumrox.ttl knob.png led_on.png led_off.png
  DESTINATION ${CMAKE_INSTALL_PREFIX}/${LV2_INSTALL_DIR}/drumrox.lv2
)

