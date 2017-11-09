#pragma once

#include <vector>

#include <embree2/rtcore.h>
#include <embree2/rtcore_ray.h>

#include "Material.h"
#include "VectorTypes.h"

class PPMImage;

void renderPixel(uint32_t x, uint32_t y, RTCScene scene, RandomSample& sampler, const std::vector<Material>& Materials, PPMImage& Color, uint32_t iteration);
