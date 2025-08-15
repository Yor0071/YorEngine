#ifndef DESCRIPTOR_POOLS_H
#define DESCRIPTOR_POOLS_H

#include <vulkan/vulkan.h>
#include <stdexcept>

class DescriptorPools
{
public:
	DescriptorPools();
	~DescriptorPools();
	void Init(VkDevice device);
	void Destroy();

	VkDescriptorPool GetMaterialPool() const { return materialPool; }

private:
	VkDevice device{};
	VkDescriptorPool materialPool{};
};

#endif // !DESCRIPTOR_POOLS_H
