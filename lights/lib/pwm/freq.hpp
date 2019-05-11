#ifndef FREQ_HPP
#define FREQ_HPP

struct freqHz
{
    uint32_t value;
    explicit freqHz(uint32_t freq) : value(freq) {}
};

inline freqHz operator""_Hz(unsigned long long freq)
{
    assert(freq < (1ULL << 32));
    return freqHz(freq);
}

inline freqHz operator"" _kHz(unsigned long long freq)
{
    assert(freq < (1ULL << 32) / 1000);
    return freqHz(1000 * freq);
}

inline freqHz operator"" _MHz(unsigned long long freq)
{
    assert(freq < (1ULL << 32) / 1000000);
    return freqHz(1000000 * freq);
}

#endif // FREQ_HPP
