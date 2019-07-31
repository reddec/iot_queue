cmake_minimum_required(VERSION 3.13)
project(iot_queued C)

set(CMAKE_C_STANDARD 11)

file(GLOB sources "src/*.c")

add_executable(${PROJECT_NAME} ${sources})
## ZeroMQ support
target_link_libraries(${PROJECT_NAME} iot_queue_static libzmq-static -lm -lstdc++)

INSTALL(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
