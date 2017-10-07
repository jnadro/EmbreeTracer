#include <cassert>
#include <iostream>
#include <limits>

#include <embree2/rtcore.h>
#include <embree2/rtcore_ray.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "FullscreenQuad.h"
#include "Material.h"
#include "Mesh.h"
#include "PPMImage.h"
#include "Renderer.h"
#include "RenderKernels/RenderKernels.h"
#include "ScopedTimer.h"
#include "VectorTypes.h"

static void EmbreeErrorHandler(void* userPtr, const RTCError code, const char* str)
{
	if (code != RTC_NO_ERROR)
	{
		switch (code)
		{
		case RTC_UNKNOWN_ERROR: std::cout << "RTC_UNKNOWN_ERROR"; break;
		case RTC_INVALID_ARGUMENT: std::cout << "RTC_INVALID_ARGUMENT"; break;
		case RTC_INVALID_OPERATION: std::cout << "RTC_INVALID_OPERATION"; break;
		case RTC_OUT_OF_MEMORY: std::cout << "RTC_OUT_OF_MEMORY"; break;
		case RTC_UNSUPPORTED_CPU: std::cout << "RTC_UNSUPPORTED_CPU"; break;
		case RTC_CANCELLED: std::cout << "RTC_CANCELLED"; break;
		default: std::cout << "Invalid Error Code"; break;
		}

		if (str)
		{
			std::cout << " (" << str << ")\n";
		}
	}
}

struct Triangle { int v0, v1, v2; };

vec4 colors[4] = {
	{ 0.0f, 0.0f, 0.0f, 1.0f },
	{ 1.0f, 1.0f, 0.0f, 1.0f },
	{ 0.0f, 1.0f, 0.0f, 1.0f },
	{ 1.0f, 0.0f, 0.0f, 1.0f } };

int main(int argc, char* argv[])
{
	uint32_t width = 1024;
	uint32_t height = 1024;

	if (argc == 1)
	{
		std::cout << "Usage: " << argv[0] << " input1.obj input2.obj input3.obj\n";
		return 1;
	}

	if (!glfwInit())
	{
		std::cout << "Failed to init GLFW.";
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	GLFWwindow* window = glfwCreateWindow(width, height, "EmbreeTracer", nullptr, nullptr);
	if (!window)
	{
		glfwTerminate();
		std::cout << "Unable to create GLFW Window.";
		return -1;
	}

	glfwMakeContextCurrent(window);

	if (!gladLoadGL()) 
	{
		std::cout << "Failed to load OpenGL functions.";
		return -1;
	}
	glfwSwapInterval(1);

	HMODULE dll = LoadLibrary(L"RenderKernels.dll");
	assert(dll != nullptr);

	typedef void(*SimpleFunc)(float* in, float* out, int count);
	SimpleFunc TestSimple = (SimpleFunc)GetProcAddress(dll, "Simple");
	float inFloats[5] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
	float outFloats[5] = { 0.0f };
	TestSimple(inFloats, outFloats, 5);

	typedef void(*CalculateSceneColorFunc)(RTCScene scene, RTCRay* ray, int width, int height, unsigned char* gl_FragCoord);
	CalculateSceneColorFunc CalculateSceneColor = (CalculateSceneColorFunc)GetProcAddress(dll, "CalculateSceneColor");

	RTCDevice device = rtcNewDevice();
	EmbreeErrorHandler(nullptr, rtcDeviceGetError(nullptr), nullptr);

	rtcDeviceSetErrorFunction2(device, EmbreeErrorHandler, nullptr);

	RTCScene scene = rtcDeviceNewScene(device, RTC_SCENE_STATIC, RTC_INTERSECT1 | RTC_INTERPOLATE);

	std::vector<TriangleMesh*> Meshes;
	std::vector<Material> Materials;

	{
		ScopedTimer MeshLoading("Loading Meshes");
		const int numObjFiles = argc - 1;
		for (int i = 0; i < numObjFiles; ++i)
		{
			LoadObjMesh(argv[i + 1], scene, Meshes, Materials);
		}
		assert(Meshes.size() == Materials.size());
	}

	{
		ScopedTimer BuildBVH("Building BVH");
		rtcCommit(scene);
	}

	PPMImage colorAOV(width, height);

	// start tracing
	{
		ScopedTimer TraceScene("Tracing Scene");
		traceImage(scene, Materials, colorAOV);
	}

	colorAOV.Write("color.hdr");

	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_FLOAT, static_cast<void*>(colorAOV.getPixels()));

	{
		FullScreenQuad quad;
		while (!glfwWindowShouldClose(window))
		{
			quad.draw(texture);
			glfwSwapBuffers(window);
			glfwPollEvents();
		}
	}

	glDeleteTextures(1, &texture);

	glfwDestroyWindow(window);
	glfwTerminate();

	for (TriangleMesh* Mesh : Meshes)
	{
		delete Mesh;
	}
	rtcDeleteScene(scene);
	rtcDeleteDevice(device);

    return 0;
}