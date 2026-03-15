#pragma once

#include <cstddef>
#include <type_traits>

/** @file */

namespace cororing {

/**
 * @brief  Non-owning pointer and size wrapper with usefull constructors defined
 */
class buffer_t {
public:
    template <typename T>
        requires(sizeof(T) == sizeof(std::byte) && std::is_trivial_v<T>)
    buffer_t(T* begin, std::size_t size)
        : begin_(begin)
        , size_(size)
    {
    }

    template <typename T>
        requires(sizeof(typename T::value_type) == sizeof(std::byte) && std::is_trivial_v<typename T::value_type>)
    explicit buffer_t(T& container)
        : begin_(reinterpret_cast<std::byte*>(container.data()))
        , size_(container.size())
    {
    }

    size_t size() const
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
    std::byte* begin_;
    const size_t size_;
};

}
