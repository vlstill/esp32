#pragma once

#include <limits>

struct FreqHz
{
    uint32_t value;
    constexpr explicit FreqHz( uint32_t freq ) : value( freq ) { }
};

constexpr inline FreqHz operator""_Hz(unsigned long long freq)
{
    assert( freq <= std::numeric_limits< uint32_t >::max() );
    return FreqHz( freq );
}

constexpr inline FreqHz operator"" _kHz(unsigned long long freq)
{
    return operator""_Hz( freq * 1000 );
}

constexpr inline FreqHz operator"" _MHz(unsigned long long freq)
{
    return operator""_Hz( freq * 1000 * 1000 );
}
