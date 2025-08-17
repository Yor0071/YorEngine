#ifndef I_WORLD_GENERATOR_H
#define I_WORLD_GENERATOR_H

class IWorldGenerator
{
public:
	virtual ~IWorldGenerator() = default;
	virtual float height(float x, float z) const = 0;
};

#endif // !I_WORLD_GENERATOR_H
