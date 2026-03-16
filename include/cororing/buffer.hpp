#pragma once

#include <cstddef>
#include <type_traits>

/** @file */

namespace cororing {

/**
 * @brief Non-owning pointer and size wrapper with useful constructors defined. Used for reading into the buffer from
 * source.
 */
class buffer_t {
public:
    template <typename T>
        requires(sizeof(T) == sizeof(std::byte) && std::is_trivial_v<T>)
    buffer_t(T* begin, std::size_t size)
        : begin_(reinterpret_cast<std::byte*>(begin))
        , size_(size)
    {
    }

    template <typename T>
        requires(sizeof(typename T::value_type) == sizeof(std::byte) && std::is_trivial_v<typename T::value_type>)
    explicit buffer_t(T&& container) = delete;

    template <typename T>
        requires(sizeof(typename T::value_type) == sizeof(std::byte) && std::is_trivial_v<typename T::value_type>)
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
        requires(sizeof(T) == sizeof(std::byte) && std::is_trivial_v<T>)
    const_buffer_t(const T* begin, std::size_t size)
        : begin_(reinterpret_cast<const std::byte*>(begin))
        , size_(size)
    {
    }

    template <typename T>
        requires(sizeof(typename T::value_type) == sizeof(std::byte) && std::is_trivial_v<typename T::value_type>)
    explicit const_buffer_t(T&& container) = delete;

    template <typename T>
        requires(sizeof(typename T::value_type) == sizeof(std::byte) && std::is_trivial_v<typename T::value_type>)
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
