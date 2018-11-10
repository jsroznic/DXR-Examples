/* Copyright (c) 2018, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "Utils.h"

namespace std
{
	void hash_combine(size_t &seed, size_t hash)
	{
		hash += 0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= hash;
	}

	template<> struct hash<Vertex> {
		size_t operator()(Vertex const &vertex) const {
			size_t seed = 0;
			hash<float> hasher;
			hash_combine(seed, hasher(vertex.position.x));
			hash_combine(seed, hasher(vertex.position.y));
			hash_combine(seed, hasher(vertex.position.z));

			hash_combine(seed, hasher(vertex.color.x));
			hash_combine(seed, hasher(vertex.color.y));

			return seed;
		}
	};
}

namespace Utils
{

//--------------------------------------------------------------------------------------
// Command Line Parser
//--------------------------------------------------------------------------------------

HRESULT ParseCommandLine(LPWSTR lpCmdLine, ConfigInfo &config)
{
	LPWSTR* argv = NULL;
	int argc = 0;

	argv = CommandLineToArgvW(GetCommandLine(), &argc);
	if (argv == NULL)
	{
		MessageBox(NULL, L"Unable to parse command line!", L"Error", MB_OK);
		return E_FAIL;
	}

	if (argc > 1)
	{
		char str[256];
		int i = 1;
		while (i < argc)
		{
			wcstombs(str, argv[i], 256);

			if (strcmp(str, "-width") == 0)
			{
				i++;
				wcstombs(str, argv[i], 256);
				config.width = atoi(str);
				i++;
				continue;
			}

			if (strcmp(str, "-height") == 0)
			{
				i++;
				wcstombs(str, argv[i], 256);
				config.height = atoi(str);
				i++;
				continue;
			}

			if (strcmp(str, "-model") == 0)
			{
				i++;
				wcstombs(str, argv[i], 256);
				config.model = str;
				i++;
				continue;
			}

			i++;
		}
	}
	else {
		return E_FAIL;
	}

	LocalFree(argv);
	return S_OK;
}

//--------------------------------------------------------------------------------------
// Error Messaging
//--------------------------------------------------------------------------------------

void Validate(HRESULT hr, LPWSTR msg)
{
	if (FAILED(hr))
	{
		MessageBox(NULL, msg, L"Error", MB_OK);
		PostQuitMessage(EXIT_FAILURE);
	}
}

//--------------------------------------------------------------------------------------
// File Reading
//--------------------------------------------------------------------------------------

vector<char> ReadFile(const string &filename) 
{
	ifstream file(filename, ios::ate | ios::binary);

	if (!file.is_open()) 
	{
		throw std::runtime_error("Error: failed to open file!");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();

	return buffer;
}

//--------------------------------------------------------------------------------------
// Model Loading
//--------------------------------------------------------------------------------------

void LoadModel(string filepath, Model &model, Material &material) 
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;

	// Load the OBJ and MTL files
	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filepath.c_str(), "materials\\")) 
	{
		throw std::runtime_error(err);
	}

	// Get the first material
	// Only support a single material right now
	material.name = materials[0].name;
	material.texturePath = materials[0].diffuse_texname;

	// Parse the model and store the unique vertices
	unordered_map<Vertex, uint32_t> uniqueVertices = {};
	for (const auto &shape : shapes) 
	{
		for (const auto &index : shape.mesh.indices) 
		{
			Vertex vertex = {};
			vertex.position = 
			{
				attrib.vertices[3 * index.vertex_index + 2],				
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 0]								
			};

			/*vertex.uv = 
			{
				1.f - attrib.texcoords[2 * index.texcoord_index + 0],
				attrib.texcoords[2 * index.texcoord_index + 1]
			};*/

			// Fast find unique vertices using a hash
			if (uniqueVertices.count(vertex) == 0) 
			{
				uniqueVertices[vertex] = static_cast<uint32_t>(model.vertices.size());
				model.vertices.push_back(vertex);
			}

			model.indices.push_back(uniqueVertices[vertex]);
		}
	}
}

//--------------------------------------------------------------------------------------
// Load Custom Scene for Raytracing
//--------------------------------------------------------------------------------------

void LoadCustomScene(Model &model, Material &material) {

	material.name = "defaultMaterial";
	material.texturePath = "textures\\statue.jpg";
	// Initialize Vertices - Back
	model.vertices.push_back({ XMFLOAT3(-8.0f, -2.0f, -20.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) });
	model.vertices.push_back({ XMFLOAT3(8.0f, -2.0f, -20.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) });
	model.vertices.push_back({ XMFLOAT3(8.0f, 10.0f, -20.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) });
	model.vertices.push_back({ XMFLOAT3(-8.0f, 10.0f, -20.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) });

	// Floor
	model.vertices.push_back({ XMFLOAT3(-8.0f, -2.0f, -20.0f), XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) });
	model.vertices.push_back({ XMFLOAT3(8.0f, -2.0f, -20.0f), XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) });
	model.vertices.push_back({ XMFLOAT3(8.0f, -2.0f, -10.0f), XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) });
	model.vertices.push_back({ XMFLOAT3(-8.0f, -2.0f, -10.0f), XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) });

	// Side
	model.vertices.push_back({ XMFLOAT3(-8.0f, -2.0f, -20.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) });
	model.vertices.push_back({ XMFLOAT3(-8.0f, -2.0f, -10.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) });
	model.vertices.push_back({ XMFLOAT3(-8.0f, 10.0f, -20.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) });

	//Define Indicies for Triangles of Environment
	// Back
	model.indices.push_back(0);
	model.indices.push_back(1);
	model.indices.push_back(2);

	model.indices.push_back(0);
	model.indices.push_back(2);
	model.indices.push_back(3);

	// Floor
	model.indices.push_back(4);
	model.indices.push_back(6);
	model.indices.push_back(5);

	model.indices.push_back(4);
	model.indices.push_back(7);
	model.indices.push_back(6);

	//Side
	model.indices.push_back(8);
	model.indices.push_back(10);
	model.indices.push_back(9);

	LoadSphere(model, material, XMFLOAT3(0.0, 0.0, -16.0), 4, XMFLOAT3(-1, -1, -1));
	LoadSphere(model, material, XMFLOAT3(-3.0, -1.0, -14.0), 2, XMFLOAT3(-1, -1, -1));
	LoadSphere(model, material, XMFLOAT3(3.0, -1.0, -14.0), 2, XMFLOAT3(1, 0, 0));
}

void LoadSphere(Model &model, Material &material, XMFLOAT3 position, float scale, XMFLOAT3 color) {

	size_t verticalSegments = 20;
	if (verticalSegments < 3)
		verticalSegments = 3;

	float radius = scale / 2.0;
	size_t horizontalSegments = verticalSegments * 2;
	int indexOffset = model.vertices.size();

	// Create rings of vertices at progressively higher latitudes.
	for (size_t i = 0; i <= verticalSegments; i++)
	{

		float latitude = (i * XM_PI / verticalSegments) - XM_PIDIV2;
		float dy, dxz;

		XMScalarSinCos(&dy, &dxz, latitude);

		// Create a single ring of vertices at this latitude.
		for (size_t j = 0; j <= horizontalSegments; j++)
		{

			float longitude = j * XM_2PI / horizontalSegments;
			float dx, dz;

			XMScalarSinCos(&dx, &dz, longitude);

			dx *= dxz;
			dz *= dxz;

			XMFLOAT3 norm = XMFLOAT3(dx, dy, dz);

			model.vertices.push_back({ XMFLOAT3(norm.x*radius + position.x, norm.y*radius + position.y, norm.z*radius + position.z), color, norm });
		}
	}

	// Fill the index buffer with triangles joining each pair of latitude rings.
	size_t stride = horizontalSegments + 1;

	for (size_t i = 0; i < verticalSegments; i++)
	{
		for (size_t j = 0; j <= horizontalSegments; j++)
		{
			size_t nextI = i + 1;
			size_t nextJ = (j + 1) % stride;

			model.indices.push_back(indexOffset + (i * stride + j));
			model.indices.push_back(indexOffset + (nextI * stride + j));
			model.indices.push_back(indexOffset + (i * stride + nextJ));

			model.indices.push_back(indexOffset + (i * stride + nextJ));
			model.indices.push_back(indexOffset + (nextI * stride + j));
			model.indices.push_back(indexOffset + (nextI * stride + nextJ));
		}
	}

}

//--------------------------------------------------------------------------------------
// Textures
//--------------------------------------------------------------------------------------

/**
* Convert a three channel RGB texture to four channel RGBA
*/
void FormatTexture(TextureInfo &info, stbi_uc* pixels)
{
	const UINT rowPitch = info.width * 4;
	const UINT cellPitch = rowPitch >> 3;				// The width of a cell in the texture.
	const UINT cellHeight = info.height >> 3;			// The height of a cell in the texture.
	const UINT textureSize = rowPitch * info.height;
	const UINT pixelSize = info.width * 3 * info.height;

	info.pixels.resize(textureSize);
	info.stride = 4;

	UINT c = (pixelSize - 1);
	for (UINT n = 0; n < textureSize; n += 4)
	{
		info.pixels[n] = pixels[c - 2];			// R
		info.pixels[n + 1] = pixels[c - 1];		// G
		info.pixels[n + 2] = pixels[c];			// B
		info.pixels[n + 3] = 0xff;				// A
		c -= 3;
	}
}

/**
* Load an image
*/
TextureInfo LoadTexture(string filepath) 
{
	TextureInfo result;

	// Load image pixels with stb_image
	stbi_uc* pixels = stbi_load(filepath.c_str(), &result.width, &result.height, &result.stride, STBI_rgb);
	if (!pixels) 
	{
		throw runtime_error("Error: failed to load image!");
	}

	FormatTexture(result, pixels);
	stbi_image_free(pixels);
	return result;
}

}
