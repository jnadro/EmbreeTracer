#pragma once

#include <vector>

#include <embree2/rtcore.h>
#include <embree2/rtcore_ray.h>

#include "Material.h"
#include "VectorTypes.h"

vec3 CalculateColor(RTCScene scene, const std::vector<Material>& Materials, RTCRay& rd);
