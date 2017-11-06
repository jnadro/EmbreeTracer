#pragma once

#include <vector>

#include <embree2/rtcore.h>
#include <embree2/rtcore_ray.h>

#include "Material.h"
#include "VectorTypes.h"

class PPMImage;

// 1. Iterates over all pixels of the image
// 2. Generates a camera ray
// 3. Traces camera ray into the scene
void traceImage(RTCScene scene, const std::vector<Material>& Materials, PPMImage& Color, uint32_t iteration);

