cmake_minimum_required( VERSION 3.8 )

set( MAIN_SRCS scan.cpp )
set( CMAKE_CXX_STANDARD 17 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )

include_directories( SYSTEM "${CMAKE_SOURCE_DIR}/config/" )
include_directories( SYSTEM "${CMAKE_SOURCE_DIR}/ext/libcxx/include/" )

include( $ENV{IDF_PATH}/tools/cmake/project.cmake )
project( scan )

