#include <cassert>
#include <iostream>
#include <limits>

#include <embree2/rtcore.h>
#include <embree2/rtcore_ray.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "tbb/tbb.h"

#include "FullscreenQuad.h"
#include "Material.h"
#include "Mesh.h"
#include "PPMImage.h"
#include "Random.h"
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

void Foo(float& x)
{
	x *= 2.0f;
}

void SerialApplyFoo(float a[], size_t n) 
{
	ScopedTimer serial("SerialApplyFoo");

	for (size_t i = 0; i != n; ++i)
		Foo(a[i]);
}

void ParallelApplyFoo(float* a, size_t n)
{
	ScopedTimer serial("ParallelApplyFoo");

	tbb::parallel_for(tbb::blocked_range<size_t>(0, n), [=](const tbb::blocked_range<size_t>& r)
	{
		for (size_t i = r.begin(); i != r.end(); ++i)
			Foo(a[i]);
	});
}

/* size of screen tiles */
static const size_t TILE_SIZE_X{ 8 };
static const size_t TILE_SIZE_Y{ 8 };

int main(int argc, char* argv[])
{
	uint32_t width = 512;
	uint32_t height = 512;

	// don't worry about rounding for now.
	assert(width % TILE_SIZE_X == 0);
	assert(height % TILE_SIZE_X == 0);

	const size_t numTilesX = width / TILE_SIZE_X;
	const size_t numTilesY = height / TILE_SIZE_Y;

	if (argc == 1)
	{
		std::cout << "Usage: " << argv[0] << " input1.obj input2.obj input3.obj\n";
		return 1;
	}

	const size_t n = 1000000;
	float* a = new float[n];
	for (size_t i = 0; i < n; ++i)
	{
		a[i] = (float)i;
	}

	SerialApplyFoo(a, n);
	ParallelApplyFoo(a, n);

	delete[] a;

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

	GLuint texture[2];
	glGenTextures(2, texture);
	glBindTexture(GL_TEXTURE_2D, texture[0]);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, texture[1]);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	PPMImage colorAOV(width, height);

	uint32_t b = 0;
	uint32_t iteration = 1;

	{
		FullScreenQuad quad;
		while (!glfwWindowShouldClose(window))
		{
			{
				ScopedTimer TraceScene("Parallel Trace Scene");

				tbb::parallel_for(tbb::blocked_range2d<size_t>(0, height, TILE_SIZE_Y, 0, width, TILE_SIZE_X), 
					[&scene, &Materials, &colorAOV, &iteration](const tbb::blocked_range2d<size_t>& r)
				{
					RandomSample sampler(iteration);

					for (size_t y = r.rows().begin(); y != r.rows().end(); ++y)
					{
						for (size_t x = r.cols().begin(); x != r.cols().end(); ++x)
						{
							renderTile((uint32_t)x, (uint32_t)y, scene, sampler, Materials, colorAOV, iteration);
						}
					}
				});

				iteration++;
			}

			glBindTexture(GL_TEXTURE_2D, texture[b]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, static_cast<void*>(colorAOV.getPixels()));
			b = (b + 1) % 2;
			quad.draw(texture[b], iteration);
			glfwSwapBuffers(window);
			glfwPollEvents();
		}
	}

	colorAOV.Write("color.hdr", iteration);

	glDeleteTextures(2, texture);

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