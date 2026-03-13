add_executable(coroutine-stdinout-test coroutine_stdinout.cpp)
add_executable(coroutine-accept-test coroutine_accept.cpp)

add_test(NAME coroutine-stdinout-test COMMAND
    echo "Hello, io_uring!" | ${CMAKE_CURRENT_BINARY_DIR}/coroutine-stdinout-test)
add_test(NAME coroutine-accept-test COMMAND
    bash -c "${CMAKE_CURRENT_BINARY_DIR}/coroutine-accept-test & sleep 1 && echo \"Hello, io_uring!\" | nc localhost 1234")

target_link_libraries(coroutine-stdinout-test unittest cororing)
target_link_libraries(coroutine-accept-test unittest cororing)

target_compile_options(coroutine-stdinout-test PRIVATE -Wall -Wextra -Werror)
target_compile_options(coroutine-accept-test PRIVATE -Wall -Wextra -Werror)

add_executable(echo-server echo_server.cpp)
target_link_libraries(echo-server cororing)
target_compile_options(echo-server PRIVATE -Wall -Wextra -Werror)
