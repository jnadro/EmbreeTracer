#include <iostream>

#include "Mesh.h"

#define TINYOBJLOADER_IMPLEMENTATION // define this in only *one* .cc
#include "tiny_obj_loader.h"

struct Vertex { float x, y, z, a; };
struct Triangle { int v0, v1, v2; };

unsigned LoadMesh(const std::string & Filename, RTCScene Scene)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string err;
	const bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, Filename.c_str());

	if (ret == false || shapes.size() < 1)
	{
		std::cerr << err << std::endl;
		return RTC_INVALID_GEOMETRY_ID;
	}

	// only deal with first shape.
	const tinyobj::shape_t& shape = shapes[0];

	const size_t numTriangles = shape.mesh.num_face_vertices.size();
	const size_t numVertices = attrib.vertices.size() / 3;

	unsigned geomID = rtcNewTriangleMesh2(Scene, RTC_GEOMETRY_STATIC, numTriangles, numVertices);

	{
		Vertex* vertices = (Vertex*)rtcMapBuffer(Scene, geomID, RTC_VERTEX_BUFFER);
		assert(vertices);
		for (size_t v = 0; v < numVertices; ++v)
		{
			vertices[v].x = attrib.vertices[3 * v + 0];
			vertices[v].y = attrib.vertices[3 * v + 1];
			vertices[v].z = attrib.vertices[3 * v + 2];
		}
		rtcUnmapBuffer(Scene, geomID, RTC_VERTEX_BUFFER);
	}

	{
		Triangle* triangles = (Triangle*)rtcMapBuffer(Scene, geomID, RTC_INDEX_BUFFER);
		assert(triangles);

		size_t index_offset = 0;
		for (size_t i = 0; i < numTriangles; ++i)
		{
			triangles[i].v0 = shape.mesh.indices[3 * i + 0].vertex_index;
			triangles[i].v1 = shape.mesh.indices[3 * i + 1].vertex_index;
			triangles[i].v2 = shape.mesh.indices[3 * i + 2].vertex_index;
		}

		rtcUnmapBuffer(Scene, geomID, RTC_INDEX_BUFFER);
	}

	return geomID;
}
