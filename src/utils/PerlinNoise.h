#ifndef PERLIN_NOISE_H
#define PERLIN_NOISE_H

#include <cmath>
#include <numeric>
#include <vector>
#include <random>

class PerlinNoise
{
public:
	PerlinNoise();
	PerlinNoise(unsigned int seed);
	float noise(float x, float y) const;

private:
	std::vector<int> p;
	float fade(float t) const;
	float lerp(float a, float b, float t) const;
	float grad(int hash, float x, float y) const;
};

#endif // !PERLIN_NOISE_H
