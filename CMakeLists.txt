cmake_minimum_required(VERSION 3.13)
project(iot_queue C)
set(CMAKE_C_STANDARD 11)

add_subdirectory(tools/iot_queued)
add_subdirectory(vendor/libzmq)

file(GLOB iot_queue_headers "include/*.h")
set(iot_queue_src src/iot_queue.c src/iot_queue_partition.c)

add_library(iot_queue SHARED ${iot_queue_src} ${iot_queue_headers})

add_library(iot_queue_static STATIC ${iot_queue_src} ${iot_queue_headers})

target_include_directories(iot_queue_static PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<INSTALL_INTERFACE:include>)
target_include_directories(iot_queue PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<INSTALL_INTERFACE:include>)
set_target_properties(iot_queue_static
        PROPERTIES
        OUTPUT_NAME iot_queue
        PUBLIC_HEADER "${iot_queue_headers}")


set_target_properties(iot_queue
        PROPERTIES
        PUBLIC_HEADER "${iot_queue_headers}")

INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/include/
        DESTINATION include
        FILES_MATCHING PATTERN "*.h*")

#INSTALL(TARGETS iot_queue LIBRARY DESTINATION lib)
#INSTALL(TARGETS iot_queue_static ARCHIVE DESTINATION lib)
install(TARGETS iot_queue iot_queue_static
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        FRAMEWORK DESTINATION "Library/Frameworks"
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
# tools

INSTALL(TARGETS iot_queued RUNTIME DESTINATION bin)
# packaging

SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "iot_queue")
SET(CPACK_PACKAGE_VENDOR "Baryshnikov Aleksandr (aka reddec) <owner@reddec.net>")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
SET(CPACK_PACKAGE_VERSION_MAJOR "0")
SET(CPACK_PACKAGE_VERSION_MINOR "1")
SET(CPACK_PACKAGE_VERSION_PATCH "0")
