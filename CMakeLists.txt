cmake_minimum_required(VERSION 2.6)
project( LabelCatheter )
set(${PROJECT_NAME}_VERSION_MAJOR 0)
set(${PROJECT_NAME}_VERSION_MINOR 1)
set(${PROJECT_NAME}_VERSION ${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR})

# Overide with cmake -DCMAKE_BUILD_TYPE=Debug
if(NOT CMAKE_BUILD_TYPE)
   message("Build type not set (defaults to release)")
   message("-DCMAKE_BUILD_TYPE=Debug for debug")
   set( CMAKE_BUILD_TYPE Release )
endif(NOT CMAKE_BUILD_TYPE)

set(CMAKE_VERBOSE_MAKEFILE true)

option(BUILD_SHARED_LIBS "Build Shared Library" ON)

################################################################################
# Add local path for finding packages, set the local version first
SET(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

if( NOT MSVC )
  # Set GCC style compiler flags
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-unused-parameter")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter")

  if(${CMAKE_CXX_COMPILER_ID} STREQUAL "clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
  endif()
endif()

find_package(Eigen3 REQUIRED)
if(EIGEN3_FOUND)
    set(HAVE_EIGEN 1)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-enum-compare -Wno-unused-local-typedefs -std=c++0x")
    include_directories(${EIGEN3_INCLUDE_DIR})
    message(STATUS "Eigen Found and Enabled")
endif()

find_package(Pangolin REQUIRED)
if(Pangolin_FOUND)
    include_directories(${Pangolin_INCLUDE_DIR})    
    message(STATUS "Pangolin Found and Enabled")
endif()

list(APPEND LIB_INC_DIR  "${PROJECT_SOURCE_DIR}/include;${CMAKE_CURRENT_BINARY_DIR}/include" )
include_directories(${LIB_INC_DIR})

set(INC_DIR include)
list(APPEND HEADER ${INC_DIR}/bspline.h ${INC_DIR}/extra/pango_display.h ${INC_DIR}/extra/pango_drawer.h)

add_executable(video_exporter src/video_exporter.cpp ${HEADER})
target_link_libraries(video_exporter ${Pangolin_LIBRARY})

add_executable(label_catheter src/label_catheter.cpp ${HEADER})
target_link_libraries(label_catheter ${Pangolin_LIBRARY})
