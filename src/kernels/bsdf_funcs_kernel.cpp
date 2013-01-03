#include <string>
namespace luxrays { namespace ocl {
std::string KernelSource_bsdf_funcs = 
"#line 2 \"bsdf_funcs.cl\"\n"
"\n"
"/***************************************************************************\n"
" *   Copyright (C) 1998-2010 by authors (see AUTHORS.txt )                 *\n"
" *                                                                         *\n"
" *   This file is part of LuxRays.                                         *\n"
" *                                                                         *\n"
" *   LuxRays is free software; you can redistribute it and/or modify       *\n"
" *   it under the terms of the GNU General Public License as published by  *\n"
" *   the Free Software Foundation; either version 3 of the License, or     *\n"
" *   (at your option) any later version.                                   *\n"
" *                                                                         *\n"
" *   LuxRays is distributed in the hope that it will be useful,            *\n"
" *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *\n"
" *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *\n"
" *   GNU General Public License for more details.                          *\n"
" *                                                                         *\n"
" *   You should have received a copy of the GNU General Public License     *\n"
" *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *\n"
" *                                                                         *\n"
" *   LuxRays website: http://www.luxrender.net                             *\n"
" ***************************************************************************/\n"
"\n"
"void BSDF_Init(\n"
"		__global BSDF *bsdf,\n"
"		//const bool fromL,\n"
"#if defined(PARAM_ACCEL_MQBVH)\n"
"		__global uint *meshFirstTriangleOffset,\n"
"		__global Mesh *meshDescs,\n"
"#endif\n"
"		__global Material *mats,\n"
"		__global Texture *texs,\n"
"		__global uint *meshMats,\n"
"		__global uint *meshIDs,\n"
"#if (PARAM_DL_LIGHT_COUNT > 0)\n"
"		__global uint *meshLights,\n"
"#endif\n"
"		__global Point *vertices,\n"
"		__global Vector *vertNormals,\n"
"		__global UV *vertUVs,\n"
"		__global Triangle *triangles,\n"
"		__global Ray *ray,\n"
"		__global RayHit *rayHit\n"
"#if defined(PARAM_HAS_PASSTHROUGHT)\n"
"		, const float u0\n"
"#endif\n"
"		) {\n"
"	//bsdf->fromLight = fromL;\n"
"#if defined(PARAM_HAS_PASSTHROUGHT)\n"
"	bsdf->passThroughEvent = u0;\n"
"#endif\n"
"\n"
"	const float3 rayOrig = vload3(0, &ray->o.x);\n"
"	const float3 rayDir = vload3(0, &ray->d.x);\n"
"	vstore3(rayOrig + rayHit->t * rayDir, 0, &bsdf->hitPoint.x);\n"
"	vstore3(-rayDir, 0, &bsdf->fixedDir.x);\n"
"\n"
"	const uint currentTriangleIndex = rayHit->index;\n"
"	const uint meshIndex = meshIDs[currentTriangleIndex];\n"
"\n"
"#if defined(PARAM_ACCEL_MQBVH)\n"
"	TODO\n"
"	__global Mesh *meshDesc = &meshDescs[meshIndex];\n"
"	__global Point *iVertices = &vertices[meshDesc->vertsOffset];\n"
"	__global Vector *iVertNormals = &vertNormals[meshDesc->vertsOffset];\n"
"#if defined(PARAM_HAS_TEXTUREMAPS)\n"
"	__global UV *iVertUVs = &vertUVs[meshDesc->vertsOffset];\n"
"#endif\n"
"	bsdf->triangles = &triangles[meshDesc->trisOffset];\n"
"	bsdf->triIndex = currentTriangleIndex - meshFirstTriangleOffset[meshIndex];\n"
"#endif\n"
"\n"
"	// Get the material\n"
"	const uint matIndex = meshMats[meshIndex];\n"
"	bsdf->materialIndex = matIndex;\n"
"\n"
"	// Interpolate face normal\n"
"	vstore3(Mesh_GetGeometryNormal(vertices, triangles, currentTriangleIndex), 0, &bsdf->geometryN.x);\n"
"#if defined(PARAM_ACCEL_MQBVH)\n"
"	Mesh_InterpolateNormal(iVertNormals, iTriangles, triangleID, hitPointB1, hitPointB2, &N);\n"
"	TransformNormal(meshDesc->invTrans, &N);\n"
"#else\n"
"	float3 shadeN = Mesh_InterpolateNormal(vertNormals, triangles, currentTriangleIndex, rayHit->b1, rayHit->b2);\n"
"#endif\n"
"\n"
"#if (PARAM_DL_LIGHT_COUNT > 0)\n"
"	// Check if it is a light source\n"
"	bsdf->triangleLightSourceIndex = meshLights[currentTriangleIndex];\n"
"#endif\n"
"\n"
"	// Interpolate UV coordinates\n"
"	vstore2(Mesh_InterpolateTriUV(vertUVs, triangles, currentTriangleIndex, rayHit->b1, rayHit->b2), 0, &bsdf->hitPointUV.u);\n"
"\n"
"//	// Check if I have to apply bump mapping\n"
"//	if (material->HasNormalTex()) {\n"
"//		// Apply normal mapping\n"
"//		const Texture *nm = material->GetNormalTexture();\n"
"//		const Spectrum color = nm->GetColorValue(hitPointUV);\n"
"//\n"
"//		const float x = 2.f * (color.r - 0.5f);\n"
"//		const float y = 2.f * (color.g - 0.5f);\n"
"//		const float z = 2.f * (color.b - 0.5f);\n"
"//\n"
"//		Vector v1, v2;\n"
"//		CoordinateSystem(Vector(shadeN), &v1, &v2);\n"
"//		shadeN = Normalize(Normal(\n"
"//				v1.x * x + v2.x * y + shadeN.x * z,\n"
"//				v1.y * x + v2.y * y + shadeN.y * z,\n"
"//				v1.z * x + v2.z * y + shadeN.z * z));\n"
"//	}\n"
"\n"
"//	// Check if I have to apply normal mapping\n"
"//	if (material->HasBumpTex()) {\n"
"//		// Apply normal mapping\n"
"//		const Texture *bm = material->GetBumpTexture();\n"
"//		const UV &dudv = bm->GetDuDv();\n"
"//\n"
"//		const float b0 = bm->GetGreyValue(hitPointUV);\n"
"//\n"
"//		const UV uvdu(hitPointUV.u + dudv.u, hitPointUV.v);\n"
"//		const float bu = bm->GetGreyValue(uvdu);\n"
"//\n"
"//		const UV uvdv(hitPointUV.u, hitPointUV.v + dudv.v);\n"
"//		const float bv = bm->GetGreyValue(uvdv);\n"
"//\n"
"//		const Vector bump(bu - b0, bv - b0, 1.f);\n"
"//\n"
"//		Vector v1, v2;\n"
"//		CoordinateSystem(Vector(shadeN), &v1, &v2);\n"
"//		shadeN = Normalize(Normal(\n"
"//				v1.x * bump.x + v2.x * bump.y + shadeN.x * bump.z,\n"
"//				v1.y * bump.x + v2.y * bump.y + shadeN.y * bump.z,\n"
"//				v1.z * bump.x + v2.z * bump.y + shadeN.z * bump.z));\n"
"//	}\n"
"\n"
"	\n"
"	Frame_SetFromZ(&bsdf->frame, shadeN);\n"
"\n"
"	vstore3(shadeN, 0, &bsdf->shadeN.x);\n"
"}\n"
"\n"
"float3 BSDF_Sample(\n"
"		__global BSDF *bsdf,\n"
"		__global Material *mats,\n"
"		__global Texture *texs,\n"
"#if defined(PARAM_HAS_IMAGEMAPS)\n"
"		__global ImageMap *imageMapDescs,\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_0)\n"
"		__global float *imageMapBuff0,\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_1)\n"
"		__global float *imageMapBuff1,\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_2)\n"
"		__global float *imageMapBuff2,\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_3)\n"
"		__global float *imageMapBuff3,\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_4)\n"
"		__global float *imageMapBuff4,\n"
"#endif\n"
"#endif\n"
"		const float u0, const float u1,\n"
"		float3 *sampledDir, float *pdfW, float *cosSampledDir, BSDFEvent *event) {\n"
"	const float3 fixedDir = vload3(0, &bsdf->fixedDir.x);\n"
"	const float3 localFixedDir = Frame_ToLocal(&bsdf->frame, fixedDir);\n"
"	float3 localSampledDir;\n"
"\n"
"	const float3 result = Material_Sample(\n"
"			&mats[bsdf->materialIndex], texs,\n"
"			(float2)(0.f, 0.f), localFixedDir, &localSampledDir,\n"
"			u0, u1,\n"
"#if defined(PARAM_HAS_PASSTHROUGHT)\n"
"			bsdf->passThroughEvent,\n"
"#endif\n"
"			pdfW, cosSampledDir, event);\n"
"	if (all(isequal(result, BLACK)))\n"
"		return 0.f;\n"
"\n"
"	*sampledDir = Frame_ToWorld(&bsdf->frame, localSampledDir);\n"
"\n"
"//	// Adjoint BSDF\n"
"//	if (fromLight) {\n"
"//		const float absDotFixedDirNS = fabsf(localFixedDir.z);\n"
"//		const float absDotSampledDirNS = fabsf(localSampledDir.z);\n"
"//		const float absDotFixedDirNG = AbsDot(fixedDir, geometryN);\n"
"//		const float absDotSampledDirNG = AbsDot(*sampledDir, geometryN);\n"
"//		return result * ((absDotFixedDirNS * absDotSampledDirNG) / (absDotSampledDirNS * absDotFixedDirNG));\n"
"//	} else\n"
"		return result;\n"
"}\n"
"\n"
"#if (PARAM_DL_LIGHT_COUNT > 0)\n"
"float3 BSDF_GetEmittedRadiance(__global BSDF *bsdf, __global Material *mats,\n"
"		__global Texture *texs, __global TriangleLight *triLightDefs, float *directPdfA) {\n"
"	return TriangleLight_GetRadiance(&triLightDefs[bsdf->triangleLightSourceIndex], mats, texs,\n"
"			vload3(0, &bsdf->fixedDir.x), vload3(0, &bsdf->geometryN.x), vload2(0, &bsdf->hitPointUV.u), directPdfA);\n"
"}\n"
"#endif\n"
; } }
