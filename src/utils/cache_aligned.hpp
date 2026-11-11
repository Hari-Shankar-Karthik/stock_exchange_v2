#pragma once

#include <cstddef>

namespace exchange::utils
{
    // TODO: Does this have any type_traits?
    template <typename T, std::size_t Align = 64>
    struct alignas(Align) CacheAligned : public T
    {
    };
}