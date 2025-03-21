#ifndef QUEUE_FAMILY_INDICES_H
#define QUEUE_FAMILY_INDICES_H

struct QueueFamilyIndices
{
    uint32_t graphicsFamily = UINT32_MAX;
    uint32_t presentFamily = UINT32_MAX;

    bool IsComplete() const
    {
        return graphicsFamily != UINT32_MAX && presentFamily != UINT32_MAX;
    }
};

#endif // QUEUE_FAMILY_INDICES_H