#include "iouring.hpp"
#include "unittest.hpp"

#include "cppcoro/sync_wait.hpp"
#include "cppcoro/when_all_ready.hpp"

#include <coroutine>
#include <string>
#include <unistd.h>

cppcoro::task<void> read_from_stdin(cororing::iouring& ring, size_t max_size)
{
    std::vector<char> buffer(max_size);

    int size = co_await ring.read(
        cororing::buffer_t { reinterpret_cast<std::byte*>(buffer.data()), buffer.size() }, STDIN_FILENO);

    ASSERT_TRUE(size > 0);
    ASSERT_EQ(std::string(buffer.begin(), buffer.begin() + size), "Hello, io_uring!\n");
}

cppcoro::task<void> write_to_stdout(cororing::iouring& ring, std::string message)
{
    int size = co_await ring.write(
        cororing::buffer_t { reinterpret_cast<std::byte*>(message.data()), message.size() }, STDOUT_FILENO);

    ASSERT_TRUE(size > 0);
}

SIMPLE_TEST(stdout_test)
{
    cororing::iouring ring(100);

    cppcoro::sync_wait(
        cppcoro::when_all_ready(write_to_stdout(ring, "Hello, io_uring!\n"), [&]() -> cppcoro::task<void> {
            while (!ring.poll()) {
                co_await std::suspend_never {};
            }
        }()));
}

SIMPLE_TEST(stdin_test)
{
    cororing::iouring ring(100);

    cppcoro::sync_wait(cppcoro::when_all_ready(read_from_stdin(ring, 100), [&]() -> cppcoro::task<void> {
        while (!ring.poll()) {
            co_await std::suspend_never{};
        }
    }()));
}

TEST_MAIN()
