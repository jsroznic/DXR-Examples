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

#include "Common.hlsl"

 /** Calculate diffuse shading. Normal and light direction do not need
  * to be normalized. */
float diffuseScalar(float3 normal, float3 lightDir, bool frontBackSame, int shadingMode)
{
	/* Basic equation for diffuse shading */
	float diffuse = dot(normalize(lightDir), normalize(normal));

	/* The diffuse value will be negative if the normal is pointing in
	 * the opposite direction of the light. Set diffuse to 0 in this
	 * case. Alternatively, we could take the absolute value to light
	 * the front and back the same way. Either way, diffuse should now
	 * be a value from 0 to 1. */
	if (frontBackSame)
		diffuse = abs(diffuse);
	else
		diffuse = clamp(diffuse, 0, 1);

	/* Keep diffuse value in range from .5 to 1 to prevent any object
	 * from appearing too dark. Not technically part of diffuse
	 * shading---however, you may like the appearance of this. */
	switch (shadingMode) {
	case 0: // Leave Diffuse Shading as absolute value (0 - 1)
		break;
	case 1: // Clamp Diffuse Shading to reduced range (0.2 - 1)
		diffuse = diffuse < 0.2 ? 0.2 : diffuse;
		break;
	case 2: // Scale Diffuse Shading from 0.5 - 1
		diffuse = diffuse / 2 + .5;
		break;
	default:
		break;
	}

	return diffuse;
}

// ---[ Closest Hit Shader ]---

[shader("closesthit")]
void ClosestHit(inout HitInfo payload : SV_RayPayload,
				Attributes attrib : SV_IntersectionAttributes)
{
	float3 staticPointLight = float3(-3.0f, 5.0f, -15.0f);
	uint triangleIndex = PrimitiveIndex();
	float3 barycentrics = float3((1.0f - attrib.uv.x - attrib.uv.y), attrib.uv.x, attrib.uv.y);
	VertexAttributes vertex = GetVertexAttributes(triangleIndex, barycentrics);

	//float3 color = albedo.Load(int3(coord, 0)).rgb;
	float3 color = vertex.color;

	float3 lightDir = normalize(staticPointLight - vertex.position);

	float diffuse = diffuseScalar(vertex.normal, lightDir, false, 1);

	// Launch Shadow Ray
	// Setup the ray
	RayDesc ray;
	ray.Origin = vertex.position;
	ray.TMin = 0.001f;
	ray.TMax = 1000.f;

	// Trace the shadow ray
	HitInfo rayPayload;
	rayPayload.ShadedColorAndHitT = float4(ray.Origin, payload.ShadedColorAndHitT.a + 1);
	float3 cameraPos = payload.ShadedColorAndHitT.xyz;
	float3 cameraDir = normalize(vertex.position - cameraPos);

	bool reflective = color.x < 0 && color.y < 0 && color.z < 0;

	if (reflective) {
		if (payload.ShadedColorAndHitT.a < 10) {
			ray.Direction = cameraDir - 2*vertex.normal*dot(cameraDir,vertex.normal);
			TraceRay(
				SceneBVH,
				RAY_FLAG_NONE,
				0xFF,
				0,
				0,
				0,
				ray,
				rayPayload);
			//Color With info from Reflected Color
			color = rayPayload.ShadedColorAndHitT.rgb;
			diffuse = 1;
		}
		else {
			color = float3(0, 0, 0);
		}
	}
	else {
		ray.Direction = lightDir;
		TraceRay(
			SceneBVH,
			RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
			0xFF,
			0,
			0,
			0,
			ray,
			rayPayload);
		//Ray Intersection
		if (rayPayload.ShadedColorAndHitT.a >= 0) {
			diffuse = 0.2;
		}
	}

	
	payload.ShadedColorAndHitT = float4(color * diffuse, RayTCurrent());
}