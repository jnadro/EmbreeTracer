#pragma once
#include <vector>

#include <embree2/rtcore.h>

unsigned LoadMesh(const std::string& Filename, RTCScene Scene);
