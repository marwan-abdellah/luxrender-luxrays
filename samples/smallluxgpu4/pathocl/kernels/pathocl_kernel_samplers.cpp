#include <string>
namespace slg { namespace ocl {
std::string KernelSource_PathOCL_Samplers = 
"#line 2 \"patchocl_kernel_samplers.cl\"\n"
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
"#if (PARAM_SAMPLER_TYPE == 0)\n"
"#define Sampler_GetSample(index) (Rnd_FloatValue(seed))\n"
"#elif (PARAM_SAMPLER_TYPE == 1)\n"
"#define Sampler_GetSample(index) (sampleData[index])\n"
"#endif\n"
"\n"
"void GenerateCameraRay(\n"
"		__global Camera *camera,\n"
"		__global Sample *sample,\n"
"		__global float *sampleData,\n"
"		Seed *seed,\n"
"		__global Ray *ray) {\n"
"#if (PARAM_SAMPLER_TYPE == 0)\n"
"	const uint pixelIndex = sample->pixelIndex;\n"
"\n"
"	const float scrSampleX = sampleData[IDX_SCREEN_X];\n"
"	const float scrSampleY = sampleData[IDX_SCREEN_Y];\n"
"\n"
"	const float screenX = pixelIndex % PARAM_IMAGE_WIDTH + scrSampleX - .5f;\n"
"	const float screenY = pixelIndex / PARAM_IMAGE_WIDTH + scrSampleY - .5f;\n"
"#elif (PARAM_SAMPLER_TYPE == 1)\n"
"	const float screenX = min(sampleData[IDX_SCREEN_X] * PARAM_IMAGE_WIDTH, (float)(PARAM_IMAGE_WIDTH - 1));\n"
"	const float screenY = min(sampleData[IDX_SCREEN_Y] * PARAM_IMAGE_HEIGHT, (float)(PARAM_IMAGE_HEIGHT - 1));\n"
"#endif\n"
"\n"
"	float3 Pras = (float3)(screenX, PARAM_IMAGE_HEIGHT - screenY - 1.f, 0.f);\n"
"	float3 rayOrig = Transform_ApplyPoint(&camera->rasterToCamera, Pras);\n"
"	float3 rayDir = rayOrig;\n"
"\n"
"	const float hither = camera->hither;\n"
"\n"
"#if defined(PARAM_CAMERA_HAS_DOF)\n"
"	const float dofSampleX = Sampler_GetSample(IDX_DOF_X);\n"
"	const float dofSampleY = Sampler_GetSample(IDX_DOF_Y);\n"
"\n"
"	// Sample point on lens\n"
"	float lensU, lensV;\n"
"	ConcentricSampleDisk(dofSampleX, dofSampleY, &lensU, &lensV);\n"
"	const float lensRadius = camera->lensRadius;\n"
"	lensU *= lensRadius;\n"
"	lensV *= lensRadius;\n"
"\n"
"	// Compute point on plane of focus\n"
"	const float focalDistance = camera->focalDistance;\n"
"	const float dist = focalDistance - hither;\n"
"	const float ft = dist / rayDir.z;\n"
"	float3 Pfocus;\n"
"	Pfocus = rayOrig + rayDir * ft;\n"
"\n"
"	// Update ray for effect of lens\n"
"	const float k = dist / focalDistance;\n"
"	rayOrig.x += lensU * k;\n"
"	rayOrig.y += lensV * k;\n"
"\n"
"	rayDir = Pfocus - rayOrig;\n"
"#endif\n"
"\n"
"	rayDir = normalize(rayDir);\n"
"	ray->mint = PARAM_RAY_EPSILON;\n"
"	ray->maxt = (camera->yon - hither) / rayDir.z;\n"
"\n"
"	// Transform ray in world coordinates\n"
"	rayOrig = Transform_ApplyPoint(&camera->cameraToWorld, rayOrig);\n"
"	rayDir = Transform_ApplyVector(&camera->cameraToWorld, rayDir);\n"
"\n"
"	vstore3(rayOrig, 0, &ray->o.x);\n"
"	vstore3(rayDir, 0, &ray->d.x);\n"
"\n"
"	/*printf(\"(%f, %f, %f) (%f, %f, %f) [%f, %f]\\n\",\n"
"		ray->o.x, ray->o.y, ray->o.z, ray->d.x, ray->d.y, ray->d.z,\n"
"		ray->mint, ray->maxt);*/\n"
"}\n"
"\n"
"void GenerateCameraPath(\n"
"		__global GPUTask *task,\n"
"		__global float *sampleData,\n"
"		Seed *seed,\n"
"		__global Camera *camera,\n"
"		__global Ray *ray) {\n"
"	__global Sample *sample = &task->sample;\n"
"\n"
"	GenerateCameraRay(camera, sample, sampleData, seed, ray);\n"
"\n"
"	sample->radiance.r = 0.f;\n"
"	sample->radiance.g = 0.f;\n"
"	sample->radiance.b = 0.f;\n"
"\n"
"	// Initialize the path state\n"
"	task->pathStateBase.state = RT_NEXT_VERTEX;\n"
"	task->pathStateBase.depth = 0;\n"
"	task->pathStateBase.throughput.r = 1.f;\n"
"	task->pathStateBase.throughput.g = 1.f;\n"
"	task->pathStateBase.throughput.b = 1.f;\n"
"#if defined(PARAM_DIRECT_LIGHT_SAMPLING)\n"
"	task->pathStateDirectLight.bouncePdf = 1.f;\n"
"	task->pathStateDirectLight.specularBounce = TRUE;\n"
"#endif\n"
"#if defined(PARAM_ENABLE_ALPHA_CHANNEL)\n"
"	task->pathStateAlphaChannel.vertexCount = 0;\n"
"	task->pathStateAlphaChannel.alpha = 1.f;\n"
"#endif\n"
"}\n"
"\n"
"//------------------------------------------------------------------------------\n"
"// Random Sampler Kernel\n"
"//------------------------------------------------------------------------------\n"
"\n"
"#if (PARAM_SAMPLER_TYPE == 0)\n"
"\n"
"__global float *Sampler_GetSampleData(__global Sample *sample, __global float *sampleData) {\n"
"	const size_t gid = get_global_id(0);\n"
"	return &sampleData[gid * TOTAL_U_SIZE];\n"
"}\n"
"\n"
"void Sampler_Init(Seed *seed, __global Sample *sample, __global float *sampleData) {\n"
"	const size_t gid = get_global_id(0);\n"
"\n"
"	sample->radiance.r = 0.f;\n"
"	sample->radiance.g = 0.f;\n"
"	sample->radiance.b = 0.f;\n"
"	sample->pixelIndex = InitialPixelIndex(gid);\n"
"\n"
"	sampleData[IDX_SCREEN_X] = Rnd_FloatValue(seed);\n"
"	sampleData[IDX_SCREEN_Y] = Rnd_FloatValue(seed);\n"
"}\n"
"\n"
"void Sampler_NextSample(\n"
"		__global GPUTask *task,\n"
"		__global Sample *sample,\n"
"		__global float *sampleData,\n"
"		Seed *seed,\n"
"		__global Pixel *frameBuffer,\n"
"#if defined(PARAM_ENABLE_ALPHA_CHANNEL)\n"
"		__global AlphaPixel *alphaFrameBuffer,\n"
"#endif\n"
"		__global Camera *camera,\n"
"		__global Ray *ray\n"
"		) {\n"
"	const uint pixelIndex = sample->pixelIndex;\n"
"	Spectrum radiance = sample->radiance;\n"
"	SplatSample(frameBuffer,\n"
"#if defined(PARAM_ENABLE_ALPHA_CHANNEL)\n"
"				alphaFrameBuffer,\n"
"#endif\n"
"				pixelIndex, &radiance,\n"
"#if defined(PARAM_ENABLE_ALPHA_CHANNEL)\n"
"				contribAlpha,\n"
"#endif\n"
"				1.f);\n"
"\n"
"	// Move to the next assigned pixel\n"
"	sample->pixelIndex = NextPixelIndex(pixelIndex);\n"
"\n"
"	sampleData[IDX_SCREEN_X] = Rnd_FloatValue(seed);\n"
"	sampleData[IDX_SCREEN_Y] = Rnd_FloatValue(seed);\n"
"\n"
"	GenerateCameraPath(task, sampleData, seed, camera, ray);\n"
"}\n"
"\n"
"#endif\n"
"\n"
"//------------------------------------------------------------------------------\n"
"// Metropolis Sampler Kernel\n"
"//------------------------------------------------------------------------------\n"
"\n"
"#if (PARAM_SAMPLER_TYPE == 1)\n"
"\n"
"__global float *Sampler_GetSampleData(__global Sample *sample, __global float *sampleData) {\n"
"	const size_t gid = get_global_id(0);\n"
"	return &sampleData[2 * gid * TOTAL_U_SIZE];\n"
"}\n"
"\n"
"void LargeStep(Seed *seed, const uint largeStepCount, __global float *proposedU) {\n"
"	for (int i = 0; i < TOTAL_U_SIZE; ++i)\n"
"		proposedU[i] = Rnd_FloatValue(seed);\n"
"}\n"
"\n"
"float Mutate(Seed *seed, const float x) {\n"
"	const float s1 = 1.f / 512.f;\n"
"	const float s2 = 1.f / 16.f;\n"
"\n"
"	const float randomValue = Rnd_FloatValue(seed);\n"
"\n"
"	const float dx = s1 / (s1 / s2 + fabs(2.f * randomValue - 1.f)) -\n"
"		s1 / (s1 / s2 + 1.f);\n"
"\n"
"	float mutatedX = x;\n"
"	if (randomValue < 0.5f) {\n"
"		mutatedX += dx;\n"
"		mutatedX = (mutatedX < 1.f) ? mutatedX : (mutatedX - 1.f);\n"
"	} else {\n"
"		mutatedX -= dx;\n"
"		mutatedX = (mutatedX < 0.f) ? (mutatedX + 1.f) : mutatedX;\n"
"	}\n"
"\n"
"	return mutatedX;\n"
"}\n"
"\n"
"float MutateScaled(Seed *seed, const float x, const float range) {\n"
"	const float s1 = 32.f;\n"
"\n"
"	const float randomValue = Rnd_FloatValue(seed);\n"
"\n"
"	const float dx = range / (s1 / (1.f + s1) + (s1 * s1) / (1.f + s1) *\n"
"		fabs(2.f * randomValue - 1.f)) - range / s1;\n"
"\n"
"	float mutatedX = x;\n"
"	if (randomValue < 0.5f) {\n"
"		mutatedX += dx;\n"
"		mutatedX = (mutatedX < 1.f) ? mutatedX : (mutatedX - 1.f);\n"
"	} else {\n"
"		mutatedX -= dx;\n"
"		mutatedX = (mutatedX < 0.f) ? (mutatedX + 1.f) : mutatedX;\n"
"	}\n"
"\n"
"	return mutatedX;\n"
"}\n"
"\n"
"void SmallStep(Seed *seed, __global float *currentU, __global float *proposedU) {\n"
"	proposedU[IDX_SCREEN_X] = MutateScaled(seed, currentU[IDX_SCREEN_X],\n"
"			PARAM_SAMPLER_METROPOLIS_IMAGE_MUTATION_RANGE);\n"
"	proposedU[IDX_SCREEN_Y] = MutateScaled(seed, currentU[IDX_SCREEN_Y],\n"
"			PARAM_SAMPLER_METROPOLIS_IMAGE_MUTATION_RANGE);\n"
"\n"
"	for (int i = IDX_SCREEN_Y + 1; i < TOTAL_U_SIZE; ++i)\n"
"		proposedU[i] = Mutate(seed, currentU[i]);\n"
"}\n"
"\n"
"void Sampler_Init(const size_t gid, Seed *seed, __global Sample *sample) {\n"
"	sample->totalI = 0.f;\n"
"	sample->largeMutationCount = 0.f;\n"
"\n"
"	sample->current = 0xffffffffu;\n"
"	sample->proposed = 1;\n"
"\n"
"	sample->smallMutationCount = 0;\n"
"	sample->consecutiveRejects = 0;\n"
"\n"
"	sample->weight = 0.f;\n"
"	sample->currentRadiance.r = 0.f;\n"
"	sample->currentRadiance.g = 0.f;\n"
"	sample->currentRadiance.b = 0.f;\n"
"#if defined(PARAM_ENABLE_ALPHA_CHANNEL)\n"
"	sample->currentAlpha = 0.f;\n"
"#endif\n"
"\n"
"	LargeStep(seed, 0, &sample->u[1][0]);\n"
"}\n"
"\n"
"//__kernel void Sampler(\n"
"//		__global GPUTask *tasks,\n"
"//		__global GPUTaskStats *taskStats,\n"
"//		__global Ray *rays,\n"
"//		__global Camera *camera) {\n"
"//	const size_t gid = get_global_id(0);\n"
"//\n"
"//	// Initialize the task\n"
"//	__global GPUTask *task = &tasks[gid];\n"
"//\n"
"//	__global Sample *sample = &task->sample;\n"
"//	const uint current = sample->current;\n"
"//\n"
"//	// Check if it is a complete path and not the very first sample\n"
"//	if ((current != 0xffffffffu) && (task->pathState.state == PATH_STATE_DONE)) {\n"
"//		// Read the seed\n"
"//		Seed seed;\n"
"//		seed.s1 = task->seed.s1;\n"
"//		seed.s2 = task->seed.s2;\n"
"//		seed.s3 = task->seed.s3;\n"
"//\n"
"//		const uint proposed = sample->proposed;\n"
"//		__global float *proposedU = &sample->u[proposed][0];\n"
"//\n"
"//		if (Rnd_FloatValue(&seed) < PARAM_SAMPLER_METROPOLIS_LARGE_STEP_RATE) {\n"
"//			LargeStep(&seed, sample->largeMutationCount, proposedU);\n"
"//			sample->smallMutationCount = 0;\n"
"//		} else {\n"
"//			__global float *currentU = &sample->u[current][0];\n"
"//\n"
"//			SmallStep(&seed, currentU, proposedU);\n"
"//			sample->smallMutationCount += 1;\n"
"//		}\n"
"//\n"
"//		taskStats[gid].sampleCount += 1;\n"
"//\n"
"//		GenerateCameraPath(task, &rays[gid], &seed, camera);\n"
"//\n"
"//		// Save the seed\n"
"//		task->seed.s1 = seed.s1;\n"
"//		task->seed.s2 = seed.s2;\n"
"//		task->seed.s3 = seed.s3;\n"
"//	}\n"
"//}\n"
"\n"
"//void Sampler_MLT_SplatSample(__global Pixel *frameBuffer,\n"
"//#if defined(PARAM_ENABLE_ALPHA_CHANNEL)\n"
"//		__global AlphaPixel *alphaFrameBuffer,\n"
"//#endif\n"
"//		Seed *seed, __global Sample *sample\n"
"//#if defined(PARAM_ENABLE_ALPHA_CHANNEL)\n"
"//		, const float alpha\n"
"//#endif\n"
"//		) {\n"
"//	uint current = sample->current;\n"
"//	uint proposed = sample->proposed;\n"
"//\n"
"//	Spectrum radiance = sample->radiance;\n"
"//\n"
"//	if (current == 0xffffffffu) {\n"
"//		// It is the very first sample, I have still to initialize the current\n"
"//		// sample\n"
"//\n"
"//		sample->currentRadiance = radiance;\n"
"//#if defined(PARAM_ENABLE_ALPHA_CHANNEL)\n"
"//		sample->currentAlpha = alpha;\n"
"//#endif\n"
"//		sample->totalI = Spectrum_Y(&radiance);\n"
"//\n"
"//		// The following 2 lines could be moved in the initialization code\n"
"//		sample->largeMutationCount = 1;\n"
"//		sample->weight = 0.f;\n"
"//\n"
"//		current = proposed;\n"
"//		proposed ^= 1;\n"
"//	} else {\n"
"//		const Spectrum currentL = sample->currentRadiance;\n"
"//#if defined(PARAM_ENABLE_ALPHA_CHANNEL)\n"
"//		const float currentAlpha = sample->currentAlpha;\n"
"//#endif\n"
"//		const float currentI = Spectrum_Y(&currentL);\n"
"//\n"
"//		const Spectrum proposedL = radiance;\n"
"//#if defined(PARAM_ENABLE_ALPHA_CHANNEL)\n"
"//		const float proposedAlpha = alpha;\n"
"//#endif\n"
"//		float proposedI = Spectrum_Y(&proposedL);\n"
"//		proposedI = isinf(proposedI) ? 0.f : proposedI;\n"
"//\n"
"//		float totalI = sample->totalI;\n"
"//		uint largeMutationCount = sample->largeMutationCount;\n"
"//		uint smallMutationCount = sample->smallMutationCount;\n"
"//		if (smallMutationCount == 0) {\n"
"//			// It is a large mutation\n"
"//			totalI += Spectrum_Y(&proposedL);\n"
"//			largeMutationCount += 1;\n"
"//\n"
"//			sample->totalI = totalI;\n"
"//			sample->largeMutationCount = largeMutationCount;\n"
"//		}\n"
"//\n"
"//		const float meanI = (totalI > 0.f) ? (totalI / largeMutationCount) : 1.f;\n"
"//\n"
"//		// Calculate accept probability from old and new image sample\n"
"//		uint consecutiveRejects = sample->consecutiveRejects;\n"
"//\n"
"//		float accProb;\n"
"//		if ((currentI > 0.f) && (consecutiveRejects < PARAM_SAMPLER_METROPOLIS_MAX_CONSECUTIVE_REJECT))\n"
"//			accProb = min(1.f, proposedI / currentI);\n"
"//		else\n"
"//			accProb = 1.f;\n"
"//\n"
"//		const float newWeight = accProb + ((smallMutationCount == 0) ? 1.f : 0.f);\n"
"//		float weight = sample->weight;\n"
"//		weight += 1.f - accProb;\n"
"//\n"
"//		const float rndVal = Rnd_FloatValue(seed);\n"
"//\n"
"//		/*if (get_global_id(0) == 0)\n"
"//			printf(\\\"[%d] Current: (%f, %f, %f) [%f] Proposed: (%f, %f, %f) [%f] accProb: %f <%f>\\\\n\\\",\n"
"//					consecutiveRejects,\n"
"//					currentL.r, currentL.g, currentL.b, weight,\n"
"//					proposedL.r, proposedL.g, proposedL.b, newWeight,\n"
"//					accProb, rndVal);*/\n"
"//\n"
"//		Spectrum contrib;\n"
"//#if defined(PARAM_ENABLE_ALPHA_CHANNEL)\n"
"//		float contribAlpha;\n"
"//#endif\n"
"//		float norm;\n"
"//		float scrX, scrY;\n"
"//\n"
"//		if ((accProb == 1.f) || (rndVal < accProb)) {\n"
"//			/*if (get_global_id(0) == 0)\n"
"//				printf(\\\"\\\\t\\\\tACCEPTED !\\\\n\\\");*/\n"
"//\n"
"//			// Add accumulated contribution of previous reference sample\n"
"//			norm = weight / (currentI / meanI + PARAM_SAMPLER_METROPOLIS_LARGE_STEP_RATE);\n"
"//			contrib = currentL;\n"
"//#if defined(PARAM_ENABLE_ALPHA_CHANNEL)\n"
"//			contribAlpha = currentAlpha;\n"
"//#endif\n"
"//			scrX = sample->u[current][IDX_SCREEN_X];\n"
"//			scrY = sample->u[current][IDX_SCREEN_Y];\n"
"//\n"
"//#if defined(PARAM_SAMPLER_METROPOLIS_DEBUG_SHOW_SAMPLE_DENSITY)\n"
"//			// Debug code: to check sample distribution\n"
"//			contrib.r = contrib.g = contrib.b = (consecutiveRejects + 1.f)  * .01f;\n"
"//			const uint pixelIndex = PPixelIndexFloat2D(scrX, scrY);\n"
"//			SplatSample(frameBuffer,\n"
"//#if defined(PARAM_ENABLE_ALPHA_CHANNEL)\n"
"//				alphaFrameBuffer,\n"
"//#endif\n"
"//				pixelIndex, &contrib,\n"
"//#if defined(PARAM_ENABLE_ALPHA_CHANNEL)\n"
"//				contribAlpha,\n"
"//#endif\n"
"//				1.f);\n"
"//#endif\n"
"//\n"
"//			current ^= 1;\n"
"//			proposed ^= 1;\n"
"//			consecutiveRejects = 0;\n"
"//\n"
"//			weight = newWeight;\n"
"//\n"
"//			sample->currentRadiance = proposedL;\n"
"//#if defined(PARAM_ENABLE_ALPHA_CHANNEL)\n"
"//			sample->currentAlpha = proposedAlpha;\n"
"//#endif\n"
"//		} else {\n"
"//			/*if (get_global_id(0) == 0)\n"
"//				printf(\\\"\\\\t\\\\tREJECTED !\\\\n\\\");*/\n"
"//\n"
"//			// Add contribution of new sample before rejecting it\n"
"//			norm = newWeight / (proposedI / meanI + PARAM_SAMPLER_METROPOLIS_LARGE_STEP_RATE);\n"
"//			contrib = proposedL;\n"
"//#if defined(PARAM_ENABLE_ALPHA_CHANNEL)\n"
"//			contribAlpha = proposedAlpha;\n"
"//#endif\n"
"//\n"
"//			scrX = sample->u[proposed][IDX_SCREEN_X];\n"
"//			scrY = sample->u[proposed][IDX_SCREEN_Y];\n"
"//\n"
"//			++consecutiveRejects;\n"
"//\n"
"//#if defined(PARAM_SAMPLER_METROPOLIS_DEBUG_SHOW_SAMPLE_DENSITY)\n"
"//			// Debug code: to check sample distribution\n"
"//			contrib.r = contrib.g = contrib.b = 1.f * .01f;\n"
"//			const uint pixelIndex = PixelIndexFloat2D(scrX, scrY);\n"
"//			SplatSample(frameBuffer,\n"
"//#if defined(PARAM_ENABLE_ALPHA_CHANNEL)\n"
"//				alphaFrameBuffer,\n"
"//#endif\n"
"//				pixelIndex, &contrib,\n"
"//#if defined(PARAM_ENABLE_ALPHA_CHANNEL)\n"
"//				contribAlpha,\n"
"//#endif\n"
"//				1.f);\n"
"//#endif\n"
"//		}\n"
"//\n"
"//#if !defined(PARAM_SAMPLER_METROPOLIS_DEBUG_SHOW_SAMPLE_DENSITY)\n"
"//		if (norm > 0.f) {\n"
"//			/*if (get_global_id(0) == 0)\n"
"//				printf(\\\"\\\\t\\\\tContrib: (%f, %f, %f) [%f] consecutiveRejects: %d\\\\n\\\",\n"
"//						contrib.r, contrib.g, contrib.b, norm, consecutiveRejects);*/\n"
"//\n"
"//#if (PARAM_IMAGE_FILTER_TYPE == 0)\n"
"//			const uint pixelIndex = PixelIndexFloat2D(scrX, scrY);\n"
"//			SplatSample(frameBuffer,\n"
"//#if defined(PARAM_ENABLE_ALPHA_CHANNEL)\n"
"//				alphaFrameBuffer,\n"
"//#endif\n"
"//				pixelIndex, &contrib,\n"
"//#if defined(PARAM_ENABLE_ALPHA_CHANNEL)\n"
"//				contribAlpha,\n"
"//#endif\n"
"//				norm);\n"
"//#else\n"
"//			float sx, sy;\n"
"//			const uint pixelIndex = PixelIndexFloat2DWithOffset(scrX, scrY, &sx, &sy);\n"
"//			SplatSample(frameBuffer,\n"
"//#if defined(PARAM_ENABLE_ALPHA_CHANNEL)\n"
"//				alphaFrameBuffer,\n"
"//#endif\n"
"//				pixelIndex, sx, sy, &contrib,\n"
"//#if defined(PARAM_ENABLE_ALPHA_CHANNEL)\n"
"//				contribAlpha,\n"
"//#endif\n"
"//				norm);\n"
"//#endif\n"
"//		}\n"
"//#endif\n"
"//\n"
"//		sample->weight = weight;\n"
"//		sample->consecutiveRejects = consecutiveRejects;\n"
"//	}\n"
"//\n"
"//	sample->current = current;\n"
"//	sample->proposed = proposed;\n"
"//}\n"
"\n"
"#endif\n"
; } }