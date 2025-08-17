#ifndef DOMAIN_WARPED_RIDGED_H
#define DOMAIN_WARPED_RIDGED_H

#include <cmath>

#include "IWorldGenerator.h"
#include "ValueNoise2D.h"

class DomainWarpedRidged : public IWorldGenerator
{
public:
	ValueNoise2D base, warpA, warpB;

	float elevation = 30.0f;
	float baseScale = 1.0f / 200.0f;
	int baseOct = 5;

	float warpScale = 1.0f / 400.0f;
	float warpAmp = 40.0f;

	float height(float x, float z) const override;

private:
	float ridged(const ValueNoise2D& n, float x, float z, int oct, float scale) const;
};

#endif // !DOMAIN_WARPED_RIDGED_H
