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
	if (materials.size() > 0) {
		material.name = materials[0].name;
		material.texturePath = materials[0].diffuse_texname;
	}
	else {
		material.name = "defaultMaterial";
		material.texturePath = "";
	}

	// Parse the model and store the unique vertices
	unordered_map<Vertex, uint32_t> uniqueVertices = {};
	for (const auto &shape : shapes) 
	{
		for (const auto &index : shape.mesh.indices) 
		{
			Vertex vertex = {
				XMFLOAT3(//Position
					attrib.vertices[3 * index.vertex_index + 2],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 0]),
				XMFLOAT3(//Color
					2,
					1.f - attrib.texcoords[2 * index.texcoord_index + 0],
					attrib.texcoords[2 * index.texcoord_index + 1]),
				index.normal_index >= 0 ?
				XMFLOAT3(//Normal
					attrib.normals[3 * index.normal_index + 2],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 0]
				)
				: XMFLOAT3(0,0,1),
				XMFLOAT3(1, 1, 0)//Material
			};

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
	material.texturePath = "";
	// Initialize Vertices - Back
	model.vertices.push_back({ XMFLOAT3(-8.0f, -2.0f, -20.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) });
	model.vertices.push_back({ XMFLOAT3(8.0f, -2.0f, -20.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) });
	model.vertices.push_back({ XMFLOAT3(8.0f, 10.0f, -20.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) });
	model.vertices.push_back({ XMFLOAT3(-8.0f, 10.0f, -20.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) });

	// Floor
	model.vertices.push_back({ XMFLOAT3(-8.0f, -2.0f, -20.0f), XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) });
	model.vertices.push_back({ XMFLOAT3(8.0f, -2.0f, -20.0f), XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) });
	model.vertices.push_back({ XMFLOAT3(8.0f, -2.0f, -10.0f), XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) });
	model.vertices.push_back({ XMFLOAT3(-8.0f, -2.0f, -10.0f), XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) });

	// Side
	model.vertices.push_back({ XMFLOAT3(-8.0f, -2.0f, -20.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) });
	model.vertices.push_back({ XMFLOAT3(-8.0f, -2.0f, -10.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) });
	model.vertices.push_back({ XMFLOAT3(-8.0f, 10.0f, -20.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) });

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

	LoadSphere(model, material, XMFLOAT3(0.0, 0.0, -16.0), 4, XMFLOAT3(1, 1, 1), XMFLOAT3(0.0f, 0.0f, 1.0f));
	LoadSphere(model, material, XMFLOAT3(-3.0, -1.0, -14.0), 2, XMFLOAT3(1, 1, 1), XMFLOAT3(0.0f, 0.0f, 1.0f));
	LoadSphere(model, material, XMFLOAT3(3.0, -1.0, -14.0), 2, XMFLOAT3(1, 0, 0), XMFLOAT3(1.0f, 0.0f, 0.0f));
}

void LoadCustomAdvancedScene(Model &model, Material &material) {

	material.name = "defaultMaterial";
	material.texturePath = "";

	//Environment Description
	// Back
	model.vertices.push_back({ XMFLOAT3(-8.0f, -2.0f, -20.0f), XMFLOAT3(0.61f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 1.0f, 0.0f) });
	model.vertices.push_back({ XMFLOAT3(8.0f, -2.0f, -20.0f), XMFLOAT3(0.61f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 1.0f, 0.0f) });
	model.vertices.push_back({ XMFLOAT3(8.0f, 10.0f, -20.0f), XMFLOAT3(0.61f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 1.0f, 0.0f) });
	model.vertices.push_back({ XMFLOAT3(-8.0f, 10.0f, -20.0f), XMFLOAT3(0.61f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 1.0f, 0.0f) });

	// Floor
	model.vertices.push_back({ XMFLOAT3(-8.0f, -2.0f, -20.0f), XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 0.5f) });
	model.vertices.push_back({ XMFLOAT3(8.0f, -2.0f, -20.0f), XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 0.5f) });
	model.vertices.push_back({ XMFLOAT3(8.0f, -2.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 0.5f) });
	model.vertices.push_back({ XMFLOAT3(-8.0f, -2.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 0.5f) });

	// Right Side
	model.vertices.push_back({ XMFLOAT3(-8.0f, -2.0f, -20.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(0.5f, 0.5f, 0.5f) });
	model.vertices.push_back({ XMFLOAT3(-8.0f, -2.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(0.5f, 0.5f, 0.5f) });
	model.vertices.push_back({ XMFLOAT3(-8.0f, 10.0f, -20.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(0.5f, 0.5f, 0.5f) });
	model.vertices.push_back({ XMFLOAT3(-8.0f, 10.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(0.5f, 0.5f, 0.5f) });

	// Left Side
	model.vertices.push_back({ XMFLOAT3(8.0f, -2.0f, -20.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(0.5f, 0.5f, 0.5f) });
	model.vertices.push_back({ XMFLOAT3(8.0f, -2.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(0.5f, 0.5f, 0.5f) });
	model.vertices.push_back({ XMFLOAT3(8.0f, 10.0f, -20.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(0.5f, 0.5f, 0.5f) });
	model.vertices.push_back({ XMFLOAT3(8.0f, 10.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(0.5f, 0.5f, 0.5f) });

	//Ears
	model.vertices.push_back({ XMFLOAT3(1.3f, 5.0f, -12.0f), XMFLOAT3(0.36f, 0.25f, 0.05f), XMFLOAT3(-1.5f, -1.0f, 1.31f), XMFLOAT3(1.0f, 0.0f, 0.0f) });
	model.vertices.push_back({ XMFLOAT3(0.3f, 3.75f, -14.0f), XMFLOAT3(0.36f, 0.25f, 0.05f), XMFLOAT3(-1.5f, -1.0f, 1.31f), XMFLOAT3(1.0f, 0.0f, 0.0f) });
	model.vertices.push_back({ XMFLOAT3(0.8f, 3.0f, -14.0f), XMFLOAT3(0.36f, 0.25f, 0.05f), XMFLOAT3(-1.5f, -1.0f, 1.31f), XMFLOAT3(1.0f, 0.0f, 0.0f) });

	model.vertices.push_back({ XMFLOAT3(-0.3f, 3.75f, -14.0f), XMFLOAT3(0.36f, 0.25f, 0.05f), XMFLOAT3(1.5f, -1.0f, 1.31f), XMFLOAT3(1.0f, 0.0f, 0.0f) });
	model.vertices.push_back({ XMFLOAT3(-1.3f, 5.0f, -12.0f), XMFLOAT3(0.36f, 0.25f, 0.05f), XMFLOAT3(1.5f, -1.0f, 1.31f), XMFLOAT3(1.0f, 0.0f, 0.0f) });
	model.vertices.push_back({ XMFLOAT3(-0.8f, 3.0f, -14.0f), XMFLOAT3(0.36f, 0.25f, 0.05f), XMFLOAT3(1.5f, -1.0f, 1.31f), XMFLOAT3(1.0f, 0.0f, 0.0f) });

	model.vertices.push_back({ XMFLOAT3(1.07f, 4.51f, -12.59f), XMFLOAT3(0.99f, 0.62f, 0.87f), XMFLOAT3(-1.5f, -1.0f, 1.31f), XMFLOAT3(1.0f, 0.0f, 0.0f) });
	model.vertices.push_back({ XMFLOAT3(0.4f, 3.60f, -13.99f), XMFLOAT3(0.99f, 0.62f, 0.87f), XMFLOAT3(-1.5f, -1.0f, 1.31f), XMFLOAT3(1.0f, 0.0f, 0.0f) });
	model.vertices.push_back({ XMFLOAT3(0.7f, 3.15f, -13.99f), XMFLOAT3(0.99f, 0.62f, 0.87f), XMFLOAT3(-1.5f, -1.0f, 1.31f), XMFLOAT3(1.0f, 0.0f, 0.0f) });

	model.vertices.push_back({ XMFLOAT3(-0.4f, 3.60f, -13.99f), XMFLOAT3(0.99f, 0.62f, 0.87f), XMFLOAT3(1.5f, -1.0f, 1.31f), XMFLOAT3(1.0f, 0.0f, 0.0f) });
	model.vertices.push_back({ XMFLOAT3(-1.07f, 4.51f, -12.59f), XMFLOAT3(0.99f, 0.62f, 0.87f), XMFLOAT3(1.5f, -1.0f, 1.31f), XMFLOAT3(1.0f, 0.0f, 0.0f) });
	model.vertices.push_back({ XMFLOAT3(-0.7f, 3.15f, -13.99f), XMFLOAT3(0.99f, 0.62f, 0.87f), XMFLOAT3(1.5f, -1.0f, 1.31f), XMFLOAT3(1.0f, 0.0f, 0.0f) });

	//Nose
	model.vertices.push_back({ XMFLOAT3(0.25f, 2.0f, -12.24f), XMFLOAT3(0.80f, 0.69f, 0.48f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) });
	model.vertices.push_back({ XMFLOAT3(-0.25f, 2.0f, -12.24f), XMFLOAT3(0.80f, 0.69f, 0.48f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) });
	model.vertices.push_back({ XMFLOAT3(0.0f, 1.56699f, -12.24f), XMFLOAT3(0.80f, 0.69f, 0.48f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) });

	//Arms
	model.vertices.push_back({ XMFLOAT3(1.5f, 1.0f, -14.0f), XMFLOAT3(0.36f, 0.25f, 0.05f), XMFLOAT3(-4.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) });
	model.vertices.push_back({ XMFLOAT3(1.5f, 0.0f, -14.0f), XMFLOAT3(0.36f, 0.25f, 0.05f), XMFLOAT3(-4.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) });
	model.vertices.push_back({ XMFLOAT3(2.5f, 1.25f, -10.0f), XMFLOAT3(0.36f, 0.25f, 0.05f), XMFLOAT3(-4.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) });

	model.vertices.push_back({ XMFLOAT3(2.5f, 1.0f, -10.0f), XMFLOAT3(0.36f, 0.25f, 0.05f), XMFLOAT3(-4.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) });
	model.vertices.push_back({ XMFLOAT3(2.5f, 1.25f, -10.0f), XMFLOAT3(0.36f, 0.25f, 0.05f), XMFLOAT3(-4.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) });
	model.vertices.push_back({ XMFLOAT3(1.5f, 0.0f, -14.0f), XMFLOAT3(0.36f, 0.25f, 0.05f), XMFLOAT3(-4.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) });

	model.vertices.push_back({ XMFLOAT3(-1.5f, 0.0f, -14.0f), XMFLOAT3(0.36f, 0.25f, 0.05f), XMFLOAT3(4.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) });
	model.vertices.push_back({ XMFLOAT3(-1.5f, 1.0f, -14.0f), XMFLOAT3(0.36f, 0.25f, 0.05f), XMFLOAT3(4.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) });
	model.vertices.push_back({ XMFLOAT3(-2.5f, 1.25f, -10.0f), XMFLOAT3(0.36f, 0.25f, 0.05f), XMFLOAT3(4.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) });

	model.vertices.push_back({ XMFLOAT3(-2.5f, 1.25f, -10.0f), XMFLOAT3(0.36f, 0.25f, 0.05f), XMFLOAT3(4.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) });
	model.vertices.push_back({ XMFLOAT3(-2.5f, 1.0f, -10.0f), XMFLOAT3(0.36f, 0.25f, 0.05f), XMFLOAT3(4.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) });
	model.vertices.push_back({ XMFLOAT3(-1.5f, 0.0f, -14.0f), XMFLOAT3(0.36f, 0.25f, 0.05f), XMFLOAT3(4.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) });

	//"Laser Swords"
	model.vertices.push_back({ XMFLOAT3(2.375f, 1.75f, -10.0f), XMFLOAT3(0.78f, 0.78f, 0.78f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.5f, 0.5f, 1.0f) });
	model.vertices.push_back({ XMFLOAT3(2.375f, 0.5f, -10.0f), XMFLOAT3(0.78f, 0.78f, 0.78f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.5f, 0.5f, 1.0f) });
	model.vertices.push_back({ XMFLOAT3(2.625f, 1.75f, -10.0f), XMFLOAT3(0.78f, 0.78f, 0.78f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.5f, 0.5f, 1.0f) });

	model.vertices.push_back({ XMFLOAT3(2.625f, 1.75f, -10.0f), XMFLOAT3(0.78f, 0.78f, 0.78f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.5f, 0.5f, 1.0f) });
	model.vertices.push_back({ XMFLOAT3(2.375f, 0.5f, -10.0f), XMFLOAT3(0.78f, 0.78f, 0.78f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.5f, 0.5f, 1.0f) });
	model.vertices.push_back({ XMFLOAT3(2.625f, 0.5f, -10.0f), XMFLOAT3(0.78f, 0.78f, 0.78f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.5f, 0.5f, 1.0f) });

	model.vertices.push_back({ XMFLOAT3(2.375f, 6.0f, -10.0f), XMFLOAT3(0.05f, 0.87f, 0.95f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 1.5f, 0.3f) });
	model.vertices.push_back({ XMFLOAT3(2.375f, 1.75f, -10.0f), XMFLOAT3(0.05f, 0.87f, 0.95f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 1.5f, 0.3f) });
	model.vertices.push_back({ XMFLOAT3(2.625f, 6.0f, -10.0f), XMFLOAT3(0.05f, 0.87f, 0.95f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 1.5f, 0.3f) });

	model.vertices.push_back({ XMFLOAT3(2.625f, 6.0f, -10.0f), XMFLOAT3(0.05f, 0.87f, 0.95f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 1.5f, 0.3f) });
	model.vertices.push_back({ XMFLOAT3(2.375f, 1.75f, -10.0f), XMFLOAT3(0.05f, 0.87f, 0.95f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 1.5f, 0.3f) });
	model.vertices.push_back({ XMFLOAT3(2.625f, 1.75f, -10.0f), XMFLOAT3(0.05f, 0.87f, 0.95f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 1.5f, 0.3f) });

	model.vertices.push_back({ XMFLOAT3(-2.375f, 1.75f, -10.0f), XMFLOAT3(0.78f, 0.78f, 0.78f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.5f, 0.5f, 1.0f) });
	model.vertices.push_back({ XMFLOAT3(-2.375f, 0.5f, -10.0f), XMFLOAT3(0.78f, 0.78f, 0.78f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.5f, 0.5f, 1.0f) });
	model.vertices.push_back({ XMFLOAT3(-2.625f, 1.75f, -10.0f), XMFLOAT3(0.78f, 0.78f, 0.78f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.5f, 0.5f, 1.0f) });

	model.vertices.push_back({ XMFLOAT3(-2.625f, 1.75f, -10.0f), XMFLOAT3(0.78f, 0.78f, 0.78f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.5f, 0.5f, 1.0f) });
	model.vertices.push_back({ XMFLOAT3(-2.375f, 0.5f, -10.0f), XMFLOAT3(0.78f, 0.78f, 0.78f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.5f, 0.5f, 1.0f) });
	model.vertices.push_back({ XMFLOAT3(-2.625f, 0.5f, -10.0f), XMFLOAT3(0.78f, 0.78f, 0.78f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.5f, 0.5f, 1.0f) });

	model.vertices.push_back({ XMFLOAT3(-2.375f, 6.0f, -10.0f), XMFLOAT3(0.42f, 0.02f, 0.68f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 1.5f, 0.3f) });
	model.vertices.push_back({ XMFLOAT3(-2.375f, 1.75f, -10.0f), XMFLOAT3(0.42f, 0.02f, 0.68f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 1.5f, 0.3f) });
	model.vertices.push_back({ XMFLOAT3(-2.625f, 6.0f, -10.0f), XMFLOAT3(0.42f, 0.02f, 0.68f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 1.5f, 0.3f) });

	model.vertices.push_back({ XMFLOAT3(-2.625f, 6.0f, -10.0f), XMFLOAT3(0.42f, 0.02f, 0.68f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 1.5f, 0.3f) });
	model.vertices.push_back({ XMFLOAT3(-2.375f, 1.75f, -10.0f), XMFLOAT3(0.42f, 0.02f, 0.68f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 1.5f, 0.3f) });
	model.vertices.push_back({ XMFLOAT3(-2.625f, 1.75f, -10.0f), XMFLOAT3(0.42f, 0.02f, 0.68f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 1.5f, 0.3f) });


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

	//Right Side
	model.indices.push_back(8);
	model.indices.push_back(10);
	model.indices.push_back(9);

	model.indices.push_back(10);
	model.indices.push_back(11);
	model.indices.push_back(9);

	//Left Side
	model.indices.push_back(14);
	model.indices.push_back(12);
	model.indices.push_back(13);

	model.indices.push_back(15);
	model.indices.push_back(14);
	model.indices.push_back(13);

	model.indices.push_back(16);
	model.indices.push_back(17);
	model.indices.push_back(18);

	model.indices.push_back(19);
	model.indices.push_back(20);
	model.indices.push_back(21);

	model.indices.push_back(22);
	model.indices.push_back(23);
	model.indices.push_back(24);

	model.indices.push_back(25);
	model.indices.push_back(26);
	model.indices.push_back(27);

	model.indices.push_back(28);
	model.indices.push_back(29);
	model.indices.push_back(30);

	model.indices.push_back(31);
	model.indices.push_back(32);
	model.indices.push_back(33);

	model.indices.push_back(34);
	model.indices.push_back(35);
	model.indices.push_back(36);

	model.indices.push_back(37);
	model.indices.push_back(38);
	model.indices.push_back(39);

	model.indices.push_back(40);
	model.indices.push_back(41);
	model.indices.push_back(42);

	model.indices.push_back(43);
	model.indices.push_back(44);
	model.indices.push_back(45);

	model.indices.push_back(46);
	model.indices.push_back(47);
	model.indices.push_back(48);

	model.indices.push_back(49);
	model.indices.push_back(50);
	model.indices.push_back(51);

	model.indices.push_back(52);
	model.indices.push_back(53);
	model.indices.push_back(54);

	model.indices.push_back(55);
	model.indices.push_back(56);
	model.indices.push_back(57);

	model.indices.push_back(58);
	model.indices.push_back(59);
	model.indices.push_back(60);

	model.indices.push_back(61);
	model.indices.push_back(62);
	model.indices.push_back(63);

	model.indices.push_back(64);
	model.indices.push_back(65);
	model.indices.push_back(66);

	//Ground Spheres
	LoadSphere(model, material, XMFLOAT3(4.5, -2.0, -12.0), 2, XMFLOAT3(1, 1, 1), XMFLOAT3(0.0f, 0.0f, 1.0f));
	LoadSphere(model, material, XMFLOAT3(-4.5, -2.0, -12.0), 2, XMFLOAT3(1, 1, 1), XMFLOAT3(0.0f, 0.0f, 1.0f));
	LoadSphere(model, material, XMFLOAT3(4.5, -2.0, -4.0), 2, XMFLOAT3(1, 1, 1), XMFLOAT3(0.0f, 0.0f, 1.0f));
	LoadSphere(model, material, XMFLOAT3(-4.5, -2.0, -4.0), 2, XMFLOAT3(1, 1, 1), XMFLOAT3(0.0f, 0.0f, 1.0f));

	//Bunny Spheres
	LoadSphere(model, material, XMFLOAT3(0.0, 0.0, -16.0), 6, XMFLOAT3(0.36, 0.25, 0.05), XMFLOAT3(1.0f, 0.0f, 0.0f));
	LoadSphere(model, material, XMFLOAT3(0.0, 2.0, -14.0), 3.5, XMFLOAT3(0.36, 0.25, 0.05), XMFLOAT3(1.0f, 0.0f, 0.0f));
	LoadSphere(model, material, XMFLOAT3(0.75, 3, -13), 1, XMFLOAT3(1.0, 1.0, 1.0), XMFLOAT3(1.0f, 1.0f, 0.5f));
	LoadSphere(model, material, XMFLOAT3(-0.75, 3, -13), 1, XMFLOAT3(1.0, 1.0, 1.0), XMFLOAT3(1.0f, 1.0f, 0.5f));
	LoadSphere(model, material, XMFLOAT3(0.75, 3, -12.6), 0.4, XMFLOAT3(0.0, 0.0, 0.0), XMFLOAT3(1.0f, 1.0f, 0.5f));
	LoadSphere(model, material, XMFLOAT3(-0.75, 3, -12.6), 0.4, XMFLOAT3(0.0, 0.0, 0.0), XMFLOAT3(1.0f, 1.0f, 0.5f));
	LoadSphere(model, material, XMFLOAT3(2.5, 1.125, -10), 0.6, XMFLOAT3(0.36, 0.25, 0.05), XMFLOAT3(1.0f, 0.0f, 0.0f));
	LoadSphere(model, material, XMFLOAT3(-2.5, 1.125, -10), 0.6, XMFLOAT3(0.36, 0.25, 0.05), XMFLOAT3(1.0f, 0.0f, 0.0f));
	LoadSphere(model, material, XMFLOAT3(2.0, -2.0, -14.25), 1.5, XMFLOAT3(0.36, 0.25, 0.05), XMFLOAT3(1.0f, 0.0f, 0.0f));
	LoadSphere(model, material, XMFLOAT3(-2.0, -2.0, -14.25), 1.5, XMFLOAT3(0.36, 0.25, 0.05), XMFLOAT3(1.0f, 0.0f, 0.0f));

}

void LoadSphere(Model &model, Material &material, XMFLOAT3 position, float scale, XMFLOAT3 color, XMFLOAT3 materialDesc) {

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

			model.vertices.push_back({ XMFLOAT3(norm.x*radius + position.x, norm.y*radius + position.y, norm.z*radius + position.z), color, norm, materialDesc });
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
