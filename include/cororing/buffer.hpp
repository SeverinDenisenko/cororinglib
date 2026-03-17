#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

/** @file */

namespace cororing {

template <class T>
concept is_byte = std::is_same_v<std::remove_cv_t<T>, std::byte> || std::is_same_v<std::remove_cv_t<T>, unsigned char>
    || std::is_same_v<std::remove_cv_t<T>, char> || std::is_same_v<std::remove_cv_t<T>, std::uint8_t>
    || std::is_same_v<std::remove_cv_t<T>, std::int8_t>;

/**
 * @brief Non-owning pointer and size wrapper with useful constructors defined. Used for reading into the buffer from
 * source.
 */
class buffer_t {
public:
    template <typename T>
        requires is_byte<T>
    buffer_t(T* begin, std::size_t size)
        : begin_(reinterpret_cast<std::byte*>(begin))
        , size_(size)
    {
    }

    template <typename T>
        requires is_byte<typename T::value_type>
    explicit buffer_t(T&& container) = delete;

    template <typename T>
        requires is_byte<typename T::value_type>
    explicit buffer_t(T& container)
        : begin_(reinterpret_cast<std::byte*>(container.data()))
        , size_(container.size())
    {
    }

    std::size_t size() const
    {
        return size_;
    }

    std::byte* data()
    {
        return begin_;
    }

    const std::byte* data() const
    {
        return begin_;
    }

private:
    std::byte* begin_ { nullptr };
    std::size_t size_ { 0 };
};

/**
 * @brief Non-owning const pointer and size wrapper with useful constructors defined. Used for writing from buffer to
 * destination.
 */
class const_buffer_t {
public:
    template <typename T>
        requires is_byte<T>
    const_buffer_t(const T* begin, std::size_t size)
        : begin_(reinterpret_cast<const std::byte*>(begin))
        , size_(size)
    {
    }

    template <typename T>
        requires is_byte<typename T::value_type>
    explicit const_buffer_t(T&& container) = delete;

    template <typename T>
        requires is_byte<typename T::value_type>
    explicit const_buffer_t(const T& container)
        : begin_(reinterpret_cast<const std::byte*>(container.data()))
        , size_(container.size())
    {
    }

    const_buffer_t(const buffer_t& buffer)
        : begin_(buffer.data())
        , size_(buffer.size())
    {
    }

    std::size_t size() const
    {
        return size_;
    }

    const std::byte* data() const
    {
        return begin_;
    }

private:
    const std::byte* begin_ { nullptr };
    std::size_t size_ { 0 };
};

}
