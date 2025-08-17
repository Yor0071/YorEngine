#ifndef VALUE_NOISE_2D_H
#define VALUE_NOISE_2D_H

#include <cmath>
#include <cstdint>
#include <algorithm>

struct ValueNoise2D
{
	uint32_t seed = 1337u;

	static inline uint32_t hash(uint32_t x)
	{
		x ^= x >> 16;
		x *= 0x7feb352d;
		x ^= x >> 15;
		x *= 0x846ca68b;
		x ^= x >> 16;
		return x;
	}

	static inline float u01(uint32_t h) { return (h & 0xFFFFFF) / float(0x1000000); }
	static inline float smooth(float t) { return t * t * (3.0f - 2.0f * t); }

    float v(int xi, int zi) const 
    {
        return u01(hash(hash(uint32_t(xi) ^ (seed * 0x9E3779B1u)) ^ (uint32_t(zi) * 0x85EBCA6Bu)));
    }

    float sample(float x, float z) const 
    {
        int xi = (int)std::floor(x), zi = (int)std::floor(z);
        float tx = x - xi, tz = z - zi;
        float v00 = v(xi, zi), v10 = v(xi + 1, zi), v01 = v(xi, zi + 1), v11 = v(xi + 1, zi + 1);
        float a = std::lerp(v00, v10, smooth(tx));
        float b = std::lerp(v01, v11, smooth(tx));
        return std::lerp(a, b, smooth(tz));
    }

    float fbm(float x, float z, int oct, float lac = 2.0f, float gain = 0.5f) const 
    {
        float amp = 1.f, freq = 1.f, sum = 0.f, norm = 0.f;
        for (int i = 0;i < oct;++i) { sum += sample(x * freq, z * freq) * amp; norm += amp; amp *= gain; freq *= lac; }
        return sum / std::max(norm, 1e-6f);
    }
};

#endif // !VALUE_NOISE_2D_H
