#include "future.hpp"
#include "iouring.hpp"
#include "unittest.hpp"

#include <array>

SIMPLE_TEST(init_test)
{
    for (auto c : cororing::get_capabilities()) {
        std::cout << c << std::endl;
    }

    cororing::iouring ring(100);
}

void poll(cororing::iouring& ring, co::future<int>& future)
{
    while (true) {
        std::cout << "Polling..." << std::endl;

        ring.poll();

        if (future.ready()) {
            break;
        }
    }
}

SIMPLE_TEST(stdout_test)
{
    cororing::iouring ring(100);

    std::string message     = "Hello, io_uring!\n";
    cororing::buffer_t buffer = { reinterpret_cast<std::byte*>(message.data()), message.size() };

    auto [future, promise] = co::create_future_promise<int>();

    ring.write(buffer, STDOUT_FILENO, [message, promise = std::move(promise)](int result) mutable {
        ASSERT_TRUE(result > 0);
        ASSERT_EQ(size_t(result), message.size());
        promise.set_value(42);
    });

    poll(ring, future);

    ASSERT_EQ(future.get(), 42);
}

SIMPLE_TEST(stdin_test)
{
    cororing::iouring ring(100);

    std::string expected = "Hello, io_uring!";
    std::string message;
    message.resize(expected.size());
    cororing::buffer_t buffer = { reinterpret_cast<std::byte*>(message.data()), message.size() };

    auto [future, promise] = co::create_future_promise<int>();

    ring.read(buffer, STDIN_FILENO, [promise = std::move(promise)](int result) mutable {
        promise.set_value(result); //
    });

    poll(ring, future);

    ASSERT_TRUE(future.get() > 0);
    ASSERT_EQ(size_t(future.get()), expected.size());
    ASSERT_EQ(expected, message);
}

TEST_MAIN()
