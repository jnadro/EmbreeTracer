#include "../Embree/include/embree2/rtcore.isph"
#include "../Embree/include/embree2/rtcore_ray.isph"

export void calculateSceneColor(uniform RTCScene scene, uniform RTCRay* uniform ray, uniform int width, uniform int height, uniform unsigned int8 gl_FragCoord[])
{
/*
	for (uniform int y = 0; y < height; ++y)
	{
		for (uniform int x = 0; x < width; ++x)
		{
			uniform int idx = (y * width + x) * 3;
			gl_FragCoord[idx] = 255;
			gl_FragCoord[idx + 1] = 0;
			gl_FragCoord[idx + 2] = 0;
		}
	}
*/

	foreach_tiled (y = 0 ... height, x = 0 ... width)
	{
		int idx = (y * width + x) * 3;
		gl_FragCoord[idx] = 255;
		gl_FragCoord[idx + 1] = 0;
		gl_FragCoord[idx + 2] = 0;
	}
}
