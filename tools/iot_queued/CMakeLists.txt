project(iot_queued C)

set(CMAKE_C_STANDARD 11)

file(GLOB sources "src/*.c")

## ZeroMQ support
find_package(PkgConfig)
pkg_check_modules(PC_ZeroMQ QUIET zmq)
find_library(ZeroMQ_LIBRARY NAMES zmq PATHS ${PC_ZeroMQ_LIBRARY_DIRS})


add_executable(${PROJECT_NAME} ${sources})

target_link_libraries(${PROJECT_NAME} iot_queue ${ZeroMQ_LIBRARY})

INSTALL(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
