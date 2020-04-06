cmake_minimum_required (VERSION 2.6)
project (lfetch)

include_directories(${CMAKE_BINARY_DIR}/modules/mbedtls/include)
file(GLOB lfetch_sources ./src/**.c)
SET(lfetch ${lfetch_sources})

ADD_LIBRARY (lfetch ${lfetch})
TARGET_LINK_LIBRARIES (lfetch mbedtls)