#include "../Embree/include/embree2/rtcore.isph"
#include "../Embree/include/embree2/rtcore_ray.isph"

export void simple(uniform float vin[], uniform float vout[],
                   uniform int count) {
    foreach (index = 0 ... count) {
        float v = vin[index];
        if (v < 3.)
            v = v * v;
        else
            v = sqrt(v);
        vout[index] = v;
    }
}
