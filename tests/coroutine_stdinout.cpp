#include "ring.hpp"
#include "unittest.hpp"

#include "cppcoro/sync_wait.hpp"
#include "cppcoro/task.hpp"
#include "cppcoro/when_all_ready.hpp"

#include <string>

cppcoro::task<void> read_from_stdin(cororing::ring_t& ring, size_t max_size)
{
    std::vector<char> buffer(max_size);

    int size = co_await ring.read(cororing::buffer_t(buffer), STDIN_FILENO);

    ASSERT_TRUE(size > 0);
    ASSERT_EQ(std::string(buffer.begin(), buffer.begin() + size), "Hello, io_uring!\n");
}

cppcoro::task<void> write_to_stdout(cororing::ring_t& ring, std::string message)
{
    int size = co_await ring.write(cororing::buffer_t(message), STDOUT_FILENO);

    ASSERT_TRUE(size > 0);
}

SIMPLE_TEST(init_test)
{
    ASSERT_TRUE(cororing::is_io_uring_supported());

    cororing::ring_t ring(123);

    ASSERT_FALSE(ring.poll());
}

SIMPLE_TEST(stdout_test)
{
    cororing::ring_t ring(100);

    cppcoro::sync_wait([&]() -> cppcoro::task<void> {
        auto [task1, task2] = co_await cppcoro::when_all_ready(
            write_to_stdout(ring, "Hello, io_uring!\n"), [&]() -> cppcoro::task<void> {
                while (!ring.poll()) {
                    co_await std::suspend_never {};
                }
            }());

        task1.result();
        task2.result();
    }());
}

SIMPLE_TEST(stdin_test)
{
    cororing::ring_t ring(100);

    cppcoro::sync_wait([&]() -> cppcoro::task<void> {
        auto [task1, task2]
            = co_await cppcoro::when_all_ready(read_from_stdin(ring, 100), [&]() -> cppcoro::task<void> {
                  while (!ring.poll()) {
                      co_await std::suspend_never {};
                  }
              }());

        task1.result();
        task2.result();
    }());
}

TEST_MAIN()
