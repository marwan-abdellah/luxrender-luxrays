#include <string>
namespace slg { namespace ocl {
std::string KernelSource_pathocl_kernels = 
"#line 2 \"patchocl_kernels.cl\"\n"
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
"// List of symbols defined at compile time:\n"
"//  PARAM_TASK_COUNT\n"
"//  PARAM_IMAGE_WIDTH\n"
"//  PARAM_IMAGE_HEIGHT\n"
"//  PARAM_RAY_EPSILON_MIN\n"
"//  PARAM_RAY_EPSILON_MAX\n"
"//  PARAM_MAX_PATH_DEPTH\n"
"//  PARAM_MAX_RR_DEPTH\n"
"//  PARAM_MAX_RR_CAP\n"
"//  PARAM_HAS_IMAGEMAPS\n"
"//  PARAM_HAS_PASSTHROUGHT\n"
"//  PARAM_USE_PIXEL_ATOMICS\n"
"//  PARAM_HAS_BUMPMAPS\n"
"//  PARAM_HAS_NORMALMAPS\n"
"//  PARAM_ACCEL_BVH or PARAM_ACCEL_QBVH or PARAM_ACCEL_MQBVH\n"
"\n"
"// To enable single material support (work around for ATI compiler problems)\n"
"//  PARAM_ENABLE_MAT_MATTE\n"
"//  PARAM_ENABLE_MAT_AREALIGHT\n"
"//  PARAM_ENABLE_MAT_MIRROR\n"
"//  PARAM_ENABLE_MAT_GLASS\n"
"//  PARAM_ENABLE_MAT_MATTEMIRROR\n"
"//  PARAM_ENABLE_MAT_METAL\n"
"//  PARAM_ENABLE_MAT_MATTEMETAL\n"
"//  PARAM_ENABLE_MAT_ALLOY\n"
"//  PARAM_ENABLE_MAT_ARCHGLASS\n"
"\n"
"// (optional)\n"
"//  PARAM_DIRECT_LIGHT_SAMPLING\n"
"//  PARAM_DL_LIGHT_COUNT\n"
"\n"
"// (optional)\n"
"//  PARAM_CAMERA_HAS_DOF\n"
"\n"
"// (optional)\n"
"//  PARAM_HAS_INFINITELIGHT\n"
"\n"
"// (optional, requires PARAM_DIRECT_LIGHT_SAMPLING)\n"
"//  PARAM_HAS_SUNLIGHT\n"
"\n"
"// (optional)\n"
"//  PARAM_HAS_SKYLIGHT\n"
"\n"
"// (optional)\n"
"//  PARAM_IMAGE_FILTER_TYPE (0 = No filter, 1 = Box, 2 = Gaussian, 3 = Mitchell)\n"
"//  PARAM_IMAGE_FILTER_WIDTH_X\n"
"//  PARAM_IMAGE_FILTER_WIDTH_Y\n"
"// (Box filter)\n"
"// (Gaussian filter)\n"
"//  PARAM_IMAGE_FILTER_GAUSSIAN_ALPHA\n"
"// (Mitchell filter)\n"
"//  PARAM_IMAGE_FILTER_MITCHELL_B\n"
"//  PARAM_IMAGE_FILTER_MITCHELL_C\n"
"\n"
"// (optional)\n"
"//  PARAM_SAMPLER_TYPE (0 = Inlined Random, 1 = Metropolis)\n"
"// (Metropolis)\n"
"//  PARAM_SAMPLER_METROPOLIS_LARGE_STEP_RATE\n"
"//  PARAM_SAMPLER_METROPOLIS_MAX_CONSECUTIVE_REJECT\n"
"//  PARAM_SAMPLER_METROPOLIS_IMAGE_MUTATION_RANGE\n"
"\n"
"// TODO: IDX_BSDF_Z used only if needed\n"
"\n"
"// (optional)\n"
"//  PARAM_ENABLE_ALPHA_CHANNEL\n"
"\n"
"//------------------------------------------------------------------------------\n"
"// Init Kernel\n"
"//------------------------------------------------------------------------------\n"
"\n"
"__kernel __attribute__((work_group_size_hint(64, 1, 1))) void Init(\n"
"		uint seedBase,\n"
"		__global GPUTask *tasks,\n"
"		__global float *samplesData,\n"
"		__global GPUTaskStats *taskStats,\n"
"		__global Ray *rays,\n"
"		__global Camera *camera\n"
"		) {\n"
"	const size_t gid = get_global_id(0);\n"
"\n"
"	//if (gid == 0)\n"
"	//	printf(\"GPUTask: %d\\n\", sizeof(GPUTask));\n"
"\n"
"	// Initialize the task\n"
"	__global GPUTask *task = &tasks[gid];\n"
"	__global Sample *sample = &task->sample;\n"
"	__global float *sampleData = Sampler_GetSampleData(sample, samplesData);\n"
"\n"
"	// Initialize random number generator\n"
"	Seed seed;\n"
"	Rnd_Init(seedBase + gid, &seed);\n"
"\n"
"	// Initialize the sample\n"
"	Sampler_Init(&seed, &task->sample, sampleData);\n"
"\n"
"	// Initialize the path\n"
"	GenerateCameraPath(task, sampleData,\n"
"#if (PARAM_SAMPLER_TYPE == 0)\n"
"		&seed,\n"
"#endif\n"
"		camera, &rays[gid]);\n"
"\n"
"	// Save the seed\n"
"	task->seed.s1 = seed.s1;\n"
"	task->seed.s2 = seed.s2;\n"
"	task->seed.s3 = seed.s3;\n"
"\n"
"	__global GPUTaskStats *taskStat = &taskStats[gid];\n"
"	taskStat->sampleCount = 0;\n"
"}\n"
"\n"
"//------------------------------------------------------------------------------\n"
"// InitFrameBuffer Kernel\n"
"//------------------------------------------------------------------------------\n"
"\n"
"__kernel __attribute__((work_group_size_hint(64, 1, 1))) void InitFrameBuffer(\n"
"		__global Pixel *frameBuffer\n"
"#if defined(PARAM_ENABLE_ALPHA_CHANNEL)\n"
"		, __global AlphaPixel *alphaFrameBuffer\n"
"#endif\n"
"		) {\n"
"	const size_t gid = get_global_id(0);\n"
"	if (gid >= (PARAM_IMAGE_WIDTH + 2) * (PARAM_IMAGE_HEIGHT + 2))\n"
"		return;\n"
"\n"
"	__global Pixel *p = &frameBuffer[gid];\n"
"	p->c.r = 0.f;\n"
"	p->c.g = 0.f;\n"
"	p->c.b = 0.f;\n"
"	p->count = 0.f;\n"
"\n"
"#if defined(PARAM_ENABLE_ALPHA_CHANNEL)\n"
"	__global AlphaPixel *ap = &alphaFrameBuffer[gid];\n"
"	ap->alpha = 0.f;\n"
"#endif\n"
"}\n"
"\n"
"//------------------------------------------------------------------------------\n"
"// AdvancePaths Kernel\n"
"//------------------------------------------------------------------------------\n"
"\n"
"__kernel __attribute__((work_group_size_hint(64, 1, 1))) void AdvancePaths(\n"
"		__global GPUTask *tasks,\n"
"		__global GPUTaskStats *taskStats,\n"
"		__global float *samplesData,\n"
"		__global Ray *rays,\n"
"		__global RayHit *rayHits,\n"
"		__global Pixel *frameBuffer,\n"
"		__global Material *mats,\n"
"		__global Texture *texs,\n"
"		__global uint *meshMats,\n"
"		__global uint *meshIDs,\n"
"#if defined(PARAM_ACCEL_MQBVH)\n"
"		__global uint *meshFirstTriangleOffset,\n"
"		__global Mesh *meshDescs,\n"
"#endif\n"
"		__global Point *vertices,\n"
"		__global Vector *vertNormals,\n"
"		__global UV *vertUVs,\n"
"		__global Triangle *triangles,\n"
"		__global Camera *camera\n"
"#if defined(PARAM_HAS_INFINITELIGHT)\n"
"		, __global InfiniteLight *infiniteLight\n"
"#endif\n"
"#if defined(PARAM_HAS_IMAGEMAPS)\n"
"		, __global ImageMap *imageMapDescs\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_0)\n"
"		, __global float *imageMapBuff0\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_1)\n"
"		, __global float *imageMapBuff1\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_2)\n"
"		, __global float *imageMapBuff2\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_3)\n"
"		, __global float *imageMapBuff3\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_4)\n"
"		, __global float *imageMapBuff4\n"
"#endif\n"
"#endif\n"
"#if defined(PARAM_ENABLE_ALPHA_CHANNEL)\n"
"		, __global AlphaPixel *alphaFrameBuffer\n"
"#endif\n"
"#if (PARAM_DL_LIGHT_COUNT > 0)\n"
"		, __global TriangleLight *triLightDefs\n"
"		, __global uint *meshLights\n"
"#endif\n"
"		) {\n"
"	const size_t gid = get_global_id(0);\n"
"\n"
"	__global GPUTask *task = &tasks[gid];\n"
"	__global Sample *sample = &task->sample;\n"
"	__global float *sampleData = Sampler_GetSampleData(sample, samplesData);\n"
"\n"
"	// Read the seed\n"
"	Seed seedValue;\n"
"	seedValue.s1 = task->seed.s1;\n"
"	seedValue.s2 = task->seed.s2;\n"
"	seedValue.s3 = task->seed.s3;\n"
"	// This trick is required by Sampler_GetSample() macro\n"
"	Seed *seed = &seedValue;\n"
"\n"
"	// read the path state\n"
"	PathState pathState = task->pathStateBase.state;\n"
"\n"
"	__global Ray *ray = &rays[gid];\n"
"	__global RayHit *rayHit = &rayHits[gid];\n"
"	const uint currentTriangleIndex = rayHit->index;\n"
"\n"
"	//--------------------------------------------------------------------------\n"
"	// Evaluation of the Path finite state machine.\n"
"	//\n"
"	// From: RT_NEXT_VERTEX\n"
"	// To: SPLAT_SAMPLE or GENERATE_DL_RAY\n"
"	//--------------------------------------------------------------------------\n"
"\n"
"	if (pathState == RT_NEXT_VERTEX) {\n"
"		if (currentTriangleIndex != NULL_INDEX) {\n"
"			// Something was hit\n"
"\n"
"			__global BSDF *bsdf = &task->pathStateBase.bsdf;\n"
"			BSDF_Init(bsdf,\n"
"#if defined(PARAM_ACCEL_MQBVH)\n"
"					meshFirstTriangleOffset,\n"
"					meshDescs,\n"
"#endif\n"
"					mats, texs, meshMats, meshIDs,\n"
"#if (PARAM_DL_LIGHT_COUNT > 0)\n"
"					meshLights,\n"
"#endif\n"
"					vertices, vertNormals, vertUVs,\n"
"					triangles, ray, rayHit\n"
"#if defined(PARAM_HAS_PASSTHROUGHT)\n"
"					, Sampler_GetSample(IDX_PASSTROUGHT)\n"
"#endif\n"
"					);\n"
"\n"
"#if (PARAM_DL_LIGHT_COUNT > 0)\n"
"			// Check if it is a light source\n"
"			if (bsdf->triangleLightSourceIndex != NULL_INDEX) {\n"
"				// Add emitted radiance\n"
"				float3 radiance = vload3(0, &sample->radiance.r);\n"
"				const float3 throughput = vload3(0, &task->pathStateBase.throughput.r);\n"
"				float directPdfA;\n"
"				radiance += throughput * BSDF_GetEmittedRadiance(bsdf, mats, texs, triLightDefs, &directPdfA);\n"
"				vstore3(radiance, 0, &sample->radiance.r);\n"
"			}\n"
"#endif\n"
"			\n"
"			// Sample next path vertex\n"
"			pathState = GENERATE_NEXT_VERTEX_RAY;\n"
"		} else {\n"
"#if defined(PARAM_HAS_INFINITELIGHT)\n"
"			const float3 lightRadiance = InfiniteLight_GetRadiance(infiniteLight,\n"
"#if defined(PARAM_HAS_IMAGEMAPS)\n"
"					imageMapDescs,\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_0)\n"
"					imageMapBuff0,\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_1)\n"
"					imageMapBuff1,\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_2)\n"
"					imageMapBuff2,\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_3)\n"
"					imageMapBuff3,\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_4)\n"
"					imageMapBuff4,\n"
"#endif\n"
"#endif\n"
"					-vload3(0, &ray->d.x));\n"
"			float3 radiance = vload3(0, &sample->radiance.r);\n"
"			radiance += vload3(0, &task->pathStateBase.throughput.r) * lightRadiance;\n"
"			vstore3(radiance, 0, &sample->radiance.r);\n"
"#endif\n"
"			pathState = SPLAT_SAMPLE;\n"
"		}\n"
"	}\n"
"\n"
"	//--------------------------------------------------------------------------\n"
"	// Evaluation of the Path finite state machine.\n"
"	//\n"
"	// From: GENERATE_NEXT_VERTEX_RAY\n"
"	// To: SPLAT_SAMPLE or RT_NEXT_VERTEX\n"
"	//--------------------------------------------------------------------------\n"
"\n"
"	if (pathState == GENERATE_NEXT_VERTEX_RAY) {\n"
"		uint depth = task->pathStateBase.depth;\n"
"\n"
"		if (depth < PARAM_MAX_PATH_DEPTH) {\n"
"			// Sample the BSDF\n"
"			__global BSDF *bsdf = &task->pathStateBase.bsdf;\n"
"			float3 sampledDir;\n"
"			float lastPdfW;\n"
"			float cosSampledDir;\n"
"			BSDFEvent event;\n"
"\n"
"			const float3 bsdfSample = BSDF_Sample(bsdf, mats, texs,\n"
"#if defined(PARAM_HAS_IMAGEMAPS)\n"
"					imageMapDescs,\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_0)\n"
"					imageMapBuff0,\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_1)\n"
"					imageMapBuff1,\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_2)\n"
"					imageMapBuff2,\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_3)\n"
"					imageMapBuff3,\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_4)\n"
"					imageMapBuff4,\n"
"#endif\n"
"#endif\n"
"					Sampler_GetSample(IDX_BSDF_X), Sampler_GetSample(IDX_BSDF_Y),\n"
"					&sampledDir, &lastPdfW, &cosSampledDir, &event);\n"
"			const bool lastSpecular = ((event & SPECULAR) != 0);\n"
"\n"
"			// Russian Roulette\n"
"			const float rrProb = fmax(Spectrum_Filter(bsdfSample), PARAM_RR_CAP);\n"
"			const bool rrEnabled = (depth >= PARAM_RR_DEPTH) && !lastSpecular;\n"
"			const bool rrContinuePath = !rrEnabled || (Sampler_GetSample(IDX_RR) < rrProb);\n"
"\n"
"			const bool continuePath = !all(isequal(bsdfSample, BLACK)) && rrContinuePath;\n"
"			if (continuePath) {\n"
"				if (rrEnabled)\n"
"					lastPdfW *= rrProb; // Russian Roulette\n"
"\n"
"				float3 throughput = vload3(0, &task->pathStateBase.throughput.r);\n"
"				throughput *= bsdfSample * (cosSampledDir / lastPdfW);\n"
"				vstore3(throughput, 0, &task->pathStateBase.throughput.r);\n"
"\n"
"				Ray_Init2(ray, vload3(0, &bsdf->hitPoint.x), sampledDir);\n"
"\n"
"				task->pathStateBase.depth = depth + 1;\n"
"				pathState = RT_NEXT_VERTEX;\n"
"			} else\n"
"				pathState = SPLAT_SAMPLE;\n"
"		} else\n"
"			pathState = SPLAT_SAMPLE;\n"
"	}\n"
"\n"
"	//--------------------------------------------------------------------------\n"
"	// Evaluation of the Path finite state machine.\n"
"	//\n"
"	// From: SPLAT_SAMPLE\n"
"	// To: RT_NEXT_VERTEX\n"
"	//--------------------------------------------------------------------------\n"
"\n"
"	if (pathState == SPLAT_SAMPLE) {\n"
"		Sampler_NextSample(task, sample, sampleData, seed, frameBuffer,\n"
"#if defined(PARAM_ENABLE_ALPHA_CHANNEL)\n"
"				alphaFrameBuffer,\n"
"#endif\n"
"				camera, ray);\n"
"		taskStats[gid].sampleCount += 1;\n"
"\n"
"		pathState = RT_NEXT_VERTEX;\n"
"	}\n"
"\n"
"	//--------------------------------------------------------------------------\n"
"\n"
"	// Save the seed\n"
"	task->seed.s1 = seed->s1;\n"
"	task->seed.s2 = seed->s2;\n"
"	task->seed.s3 = seed->s3;\n"
"\n"
"	// Save the state\n"
"	task->pathStateBase.state = pathState;\n"
"}\n"
; } }
