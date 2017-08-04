#include <cassert>

#include <embree2/rtcore.h>
#include <embree2/rtcore_ray.h>

int main(int argc, char* argv[])
{
	RTCDevice device = rtcNewDevice();
	RTCError error = rtcDeviceGetError(nullptr);
	assert(error == RTC_NO_ERROR);

	rtcDeleteDevice(device);

    return 0;
}