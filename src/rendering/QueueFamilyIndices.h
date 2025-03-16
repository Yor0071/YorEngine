#ifndef QUEUE_FAMILY_INDICES_H
#define QUEUE_FAMILY_INDICES_H

struct QueueFamilyIndices
{
    int graphicsFamily = -1;
    int presentFamily = -1;

    bool IsComplete() const
    {
        return graphicsFamily != -1 && presentFamily != -1;
    }
};

#endif // QUEUE_FAMILY_INDICES_H