project(tests)

add_executable(basic_append_read basic_append_read.c)

target_link_libraries(basic_append_read iot_queue_static)


add_test(NAME BasicAppendRead COMMAND basic_append_read)

