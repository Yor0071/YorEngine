#include "DomainWarpedRidged.h"

float DomainWarpedRidged::ridged(const ValueNoise2D& n, float x, float z, int oct, float scale) const
{
	float f = n.fbm(x * scale, z * scale, oct);
	float r = 1.0f - std::fabs(2.0f * f - 1.0f);
	return r * r;
}

float DomainWarpedRidged::height(float x, float z) const
{
	float wa = warpA.fbm(x * warpScale, z * warpScale, 3);
	float wb = warpB.fbm(x * warpScale * 1.3f, z * warpScale * 1.3f, 3);
	float wx = x + (wa - 0.5f) * 2.0f * warpAmp;
	float wz = z + (wb - 0.5f) * 2.0f * warpAmp;

	float r = ridged(base, wx, wz, baseOct, baseScale);
	float h = elevation * r;

	float macro = base.fbm(x * baseScale * 0.2f, z * baseScale * 0.2f, baseOct);
	h *= std::lerp(0.6f, 1.4f, macro);
	return h;
}