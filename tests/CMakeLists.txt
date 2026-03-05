add_executable(iouring-stdinout-test iouring_stdinout_test.cpp)
add_executable(iouring-accept-test iouring_accept_test.cpp)
add_executable(coroutine-stdinout-test coroutine_stdinout.cpp)

add_test(NAME iouring-stdinout-test COMMAND
    echo "Hello, io_uring!" | ${CMAKE_CURRENT_BINARY_DIR}/iouring-stdinout-test)
add_test(NAME iouring-accept-test COMMAND
    bash -c "${CMAKE_CURRENT_BINARY_DIR}/iouring-accept-test & sleep 1 && echo \"Hello, io_uring!\" | nc localhost 1234")
add_test(NAME coroutine-stdinout-test COMMAND
    echo "Hello, io_uring!" | ${CMAKE_CURRENT_BINARY_DIR}/coroutine-stdinout-test)

target_link_libraries(iouring-stdinout-test unittest cororing)
target_link_libraries(iouring-accept-test unittest cororing)
target_link_libraries(coroutine-stdinout-test unittest cororing)

target_compile_options(iouring-stdinout-test PRIVATE -Wall -Wextra -Werror)
target_compile_options(iouring-accept-test PRIVATE -Wall -Wextra -Werror)
target_compile_options(coroutine-stdinout-test PRIVATE -Wall -Wextra -Werror)
