#include "coroutine.hpp"
#include "iouring.hpp"
#include "unittest.hpp"

#include <string>
#include <unistd.h>

co::coroutine<std::string> read_from_stdin(cororing::iouring& ring, size_t max_size)
{
    std::vector<char> buffer(max_size);

    int size = co_await ring.read(
        cororing::buffer_t { reinterpret_cast<std::byte*>(buffer.data()), buffer.size() }, STDIN_FILENO);

    if (size < 0) {
        std::abort();
    }

    co_return std::string(buffer.begin(), buffer.begin() + size);
}

co::coroutine<void> write_to_stdout(cororing::iouring& ring, std::string message)
{
    int size = co_await ring.write(
        cororing::buffer_t { reinterpret_cast<std::byte*>(message.data()), message.size() }, STDOUT_FILENO);

    if (size < 0) {
        std::abort();
    }
}

void poll(cororing::iouring& ring, auto& coro)
{
    while (true) {
        ring.poll();

        if (coro.done()) {
            return;
        }
    }
}

SIMPLE_TEST(stdout_test)
{
    cororing::iouring ring(100);

    co::coroutine<void> coro = write_to_stdout(ring, "Hello, io_uring!\n");

    poll(ring, coro);

    ASSERT_EQ(coro.done(), true);
}

SIMPLE_TEST(stdin_test)
{
    cororing::iouring ring(100);

    co::coroutine<std::string> coro = read_from_stdin(ring, 100);

    poll(ring, coro);

    ASSERT_EQ(coro.done(), true);
    std::string result = coro.get();
    ASSERT_EQ(result, "Hello, io_uring!\n");
}

TEST_MAIN()
