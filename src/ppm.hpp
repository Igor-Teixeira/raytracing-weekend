#pragma once
#include <cstdint>

namespace RT
{
    bool WritePPM(const char* filename, unsigned int width, unsigned int height, const float* pixels);
}
