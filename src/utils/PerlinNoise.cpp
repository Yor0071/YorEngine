#include "PerlinNoise.h"

PerlinNoise::PerlinNoise()
{
	p.resize(256);
	std::iota(p.begin(), p.end(), 0);
	std::default_random_engine engine(std::random_device{}());
	std::shuffle(p.begin(), p.end(), engine);
	p.insert(p.end(), p.begin(), p.end());
}

PerlinNoise::PerlinNoise(unsigned int seed)
{
	p.resize(256);
	std::iota(p.begin(), p.end(), 0);
	std::default_random_engine engine(seed);
	std::shuffle(p.begin(), p.end(), engine);
	p.insert(p.end(), p.begin(), p.end());
}

float PerlinNoise::fade(float t) const
{
	return t * t * t * (t * (t * 6 - 15) + 10);
}

float PerlinNoise::lerp(float a, float b, float t) const
{
	return a + t * (b - a);
}

float PerlinNoise::grad(int hash, float x, float y) const
{
	int h = hash & 15;
	float u = h < 8 ? x : y;
	float v = h < 4 ? y : (h == 12 || h == 14 ? x : 0);
	return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
}

float PerlinNoise::noise(float x, float y) const
{
	int xi = static_cast<int>(std::floor(x)) & 255;
	int yi = static_cast<int>(std::floor(y)) & 255;

	float xf = x - std::floor(x);
	float yf = y - std::floor(y);

	float u = fade(xf);
	float v = fade(yf);

	int aa = p[p[xi] + yi];
	int ab = p[p[xi] + yi + 1];
	int ba = p[p[xi + 1] + yi];
	int bb = p[p[xi + 1] + yi + 1];

	float x1 = lerp(grad(aa, xf, yf), grad(ab, xf, yf - 1), v);
	float x2 = lerp(grad(ba, xf - 1, yf), grad(bb, xf - 1, yf - 1), v);

	return (lerp(x1, x2, v) + 1.0f) / 2.0f;
}