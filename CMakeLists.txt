cmake_minimum_required(VERSION 3.14)
project(iot_queue C)

set(CMAKE_C_STANDARD 11)
include_directories(include)
file(GLOB iot_queue_src "src/*.c")
file(GLOB iot_queue_headers "include/*.h")

enable_testing()
add_subdirectory(test build/test)

add_library(iot_queue SHARED ${iot_queue_src})
set_target_properties(iot_queue PROPERTIES PUBLIC_HEADER ${iot_queue_headers})

add_library(iot_queue_static STATIC ${iot_queue_src})
set_target_properties(iot_queue_static PROPERTIES OUTPUT_NAME iot_queue)
set_target_properties(iot_queue_static PROPERTIES PUBLIC_HEADER ${iot_queue_headers})

INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/include/
        DESTINATION include
        FILES_MATCHING PATTERN "*.h*")

INSTALL(TARGETS iot_queue
        LIBRARY DESTINATION lib)

INSTALL(TARGETS iot_queue_static
        ARCHIVE DESTINATION lib)

# packaging

SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "iot_queue")
SET(CPACK_PACKAGE_VENDOR "Baryshnikov Aleksandr (aka reddec) <owner@reddec.net>")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
SET(CPACK_PACKAGE_VERSION_MAJOR "0")
SET(CPACK_PACKAGE_VERSION_MINOR "1")
SET(CPACK_PACKAGE_VERSION_PATCH "0")
