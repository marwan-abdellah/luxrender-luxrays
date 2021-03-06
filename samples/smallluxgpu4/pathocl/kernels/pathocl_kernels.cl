#line 2 "patchocl_kernels.cl"

/***************************************************************************
 *   Copyright (C) 1998-2010 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of LuxRays.                                         *
 *                                                                         *
 *   LuxRays is free software; you can redistribute it and/or modify       *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   LuxRays is distributed in the hope that it will be useful,            *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 *   LuxRays website: http://www.luxrender.net                             *
 ***************************************************************************/

// List of symbols defined at compile time:
//  PARAM_TASK_COUNT
//  PARAM_IMAGE_WIDTH
//  PARAM_IMAGE_HEIGHT
//  PARAM_RAY_EPSILON_MIN
//  PARAM_RAY_EPSILON_MAX
//  PARAM_MAX_PATH_DEPTH
//  PARAM_MAX_RR_DEPTH
//  PARAM_MAX_RR_CAP
//  PARAM_HAS_IMAGEMAPS
//  PARAM_HAS_PASSTHROUGH
//  PARAM_USE_PIXEL_ATOMICS
//  PARAM_HAS_BUMPMAPS
//  PARAM_HAS_NORMALMAPS
//  PARAM_ACCEL_BVH or PARAM_ACCEL_QBVH or PARAM_ACCEL_MQBVH

// To enable single material support
//  PARAM_ENABLE_MAT_MATTE
//  PARAM_ENABLE_MAT_MIRROR
//  PARAM_ENABLE_MAT_GLASS
//  PARAM_ENABLE_MAT_METAL
//  PARAM_ENABLE_MAT_ARCHGLASS
//  PARAM_ENABLE_MAT_MIX
//  PARAM_ENABLE_MAT_NULL
//  PARAM_ENABLE_MAT_MATTETRANSLUCENT

// To enable single texture support
//  PARAM_ENABLE_TEX_CONST_FLOAT
//  PARAM_ENABLE_TEX_CONST_FLOAT3
//  PARAM_ENABLE_TEX_CONST_FLOAT4
//  PARAM_ENABLE_TEX_IMAGEMAP
//  PARAM_ENABLE_TEX_SCALE

// (optional)
//  PARAM_DIRECT_LIGHT_SAMPLING
//  PARAM_DL_LIGHT_COUNT

// (optional)
//  PARAM_CAMERA_HAS_DOF

// (optional)
//  PARAM_HAS_INFINITELIGHT

// (optional, requires PARAM_DIRECT_LIGHT_SAMPLING)
//  PARAM_HAS_SUNLIGHT

// (optional)
//  PARAM_HAS_SKYLIGHT

// (optional)
//  PARAM_IMAGE_FILTER_TYPE (0 = No filter, 1 = Box, 2 = Gaussian, 3 = Mitchell)
//  PARAM_IMAGE_FILTER_WIDTH_X
//  PARAM_IMAGE_FILTER_WIDTH_Y
// (Box filter)
// (Gaussian filter)
//  PARAM_IMAGE_FILTER_GAUSSIAN_ALPHA
// (Mitchell filter)
//  PARAM_IMAGE_FILTER_MITCHELL_B
//  PARAM_IMAGE_FILTER_MITCHELL_C

// (optional)
//  PARAM_SAMPLER_TYPE (0 = Inlined Random, 1 = Metropolis, 2 = Sobol)
// (Metropolis)
//  PARAM_SAMPLER_METROPOLIS_LARGE_STEP_RATE
//  PARAM_SAMPLER_METROPOLIS_MAX_CONSECUTIVE_REJECT
//  PARAM_SAMPLER_METROPOLIS_IMAGE_MUTATION_RANGE
// (SOBOL)
//  PARAM_SAMPLER_SOBOL_MAXDEPTH

// (optional)
//  PARAM_ENABLE_ALPHA_CHANNEL

//------------------------------------------------------------------------------
// Init Kernel
//------------------------------------------------------------------------------

__kernel __attribute__((work_group_size_hint(64, 1, 1))) void Init(
		uint seedBase,
		__global GPUTask *tasks,
		__global float *samplesData,
		__global GPUTaskStats *taskStats,
		__global Ray *rays,
		__global Camera *camera
		) {
	const size_t gid = get_global_id(0);

	// Initialize the task
	__global GPUTask *task = &tasks[gid];

	// Initialize random number generator
	Seed seed;
	Rnd_Init(seedBase + gid, &seed);

	// Initialize the sample and path
	__global Sample *sample = &task->sample;
	__global float *sampleData = Sampler_GetSampleData(sample, samplesData);
	Sampler_Init(&seed, task, sampleData, camera, &rays[gid]);

	// Save the seed
	task->seed.s1 = seed.s1;
	task->seed.s2 = seed.s2;
	task->seed.s3 = seed.s3;

	__global GPUTaskStats *taskStat = &taskStats[gid];
	taskStat->sampleCount = 0;
}

//------------------------------------------------------------------------------
// InitFrameBuffer Kernel
//------------------------------------------------------------------------------

__kernel __attribute__((work_group_size_hint(64, 1, 1))) void InitFrameBuffer(
		__global Pixel *frameBuffer
#if defined(PARAM_ENABLE_ALPHA_CHANNEL)
		, __global AlphaPixel *alphaFrameBuffer
#endif
		) {
	const size_t gid = get_global_id(0);
	if (gid >= (PARAM_IMAGE_WIDTH + 2) * (PARAM_IMAGE_HEIGHT + 2))
		return;

	__global Pixel *p = &frameBuffer[gid];
	p->c.r = 0.f;
	p->c.g = 0.f;
	p->c.b = 0.f;
	p->count = 0.f;

#if defined(PARAM_ENABLE_ALPHA_CHANNEL)
	__global AlphaPixel *ap = &alphaFrameBuffer[gid];
	ap->alpha = 0.f;
#endif
}

//------------------------------------------------------------------------------
// AdvancePaths Kernel
//------------------------------------------------------------------------------

__kernel __attribute__((work_group_size_hint(64, 1, 1))) void AdvancePaths(
		__global GPUTask *tasks,
		__global GPUTaskStats *taskStats,
		__global float *samplesData,
		__global Ray *rays,
		__global RayHit *rayHits,
		__global Pixel *frameBuffer,
		__global Material *mats,
		__global Texture *texs,
		__global uint *meshMats,
		__global uint *meshIDs,
#if defined(PARAM_ACCEL_MQBVH)
		__global uint *meshFirstTriangleOffset,
		__global Mesh *meshDescs,
#endif
		__global Point *vertices,
		__global Vector *vertNormals,
		__global UV *vertUVs,
		__global Triangle *triangles,
		__global Camera *camera
#if defined(PARAM_HAS_INFINITELIGHT)
		, __global InfiniteLight *infiniteLight
#endif
#if defined(PARAM_HAS_SUNLIGHT)
		, __global SunLight *sunLight
#endif
#if defined(PARAM_HAS_SKYLIGHT)
		, __global SkyLight *skyLight
#endif
#if (PARAM_DL_LIGHT_COUNT > 0)
		, __global TriangleLight *triLightDefs
		, __global uint *meshLights
#endif
		IMAGEMAPS_PARAM_DECL
#if defined(PARAM_ENABLE_ALPHA_CHANNEL)
		, __global AlphaPixel *alphaFrameBuffer
#endif
		) {
	const size_t gid = get_global_id(0);

	__global GPUTask *task = &tasks[gid];

	// Read the path state
	PathState pathState = task->pathStateBase.state;
	const uint depth = task->pathStateBase.depth;
	__global BSDF *bsdf = &task->pathStateBase.bsdf;

	__global Sample *sample = &task->sample;
	__global float *sampleData = Sampler_GetSampleData(sample, samplesData);
	__global float *sampleDataPathBase = Sampler_GetSampleDataPathBase(sample, sampleData);
#if (PARAM_SAMPLER_TYPE != 0)
	// Used by Sampler_GetSamplePathVertex() macro
	__global float *sampleDataPathVertexBase = Sampler_GetSampleDataPathVertex(
			sample, sampleDataPathBase, depth);
#endif

	// Read the seed
	Seed seedValue;
	seedValue.s1 = task->seed.s1;
	seedValue.s2 = task->seed.s2;
	seedValue.s3 = task->seed.s3;
	// This trick is required by Sampler_GetSample() macro
	Seed *seed = &seedValue;

	__global Ray *ray = &rays[gid];
	__global RayHit *rayHit = &rayHits[gid];
	const uint currentTriangleIndex = rayHit->index;

	//--------------------------------------------------------------------------
	// Evaluation of the Path finite state machine.
	//
	// From: RT_NEXT_VERTEX
	// To: SPLAT_SAMPLE or GENERATE_DL_RAY
	//--------------------------------------------------------------------------

	if (pathState == RT_NEXT_VERTEX) {
		if (currentTriangleIndex != NULL_INDEX) {
			// Something was hit

			BSDF_Init(bsdf,
#if defined(PARAM_ACCEL_MQBVH)
					meshFirstTriangleOffset,
					meshDescs,
#endif
					meshMats, meshIDs,
#if (PARAM_DL_LIGHT_COUNT > 0)
					meshLights,
#endif
					vertices, vertNormals, vertUVs,
					triangles, ray, rayHit
#if defined(PARAM_HAS_PASSTHROUGH)
					, task->pathStateBase.bsdf.passThroughEvent
#endif
#if defined(PARAM_HAS_BUMPMAPS) || defined(PARAM_HAS_NORMALMAPS)
					MATERIALS_PARAM
#endif
					);

//#if defined(PARAM_HAS_PASSTHROUGH)
//			const float3 passThroughTrans = BSDF_GetPassThroughTransparency(bsdf
//					MATERIALS_PARAM);
//			if (!Spectrum_IsBlack(passThroughTrans)) {
//				const float3 pathThroughput = VLOAD3F(&task->pathStateBase.throughput.r) * passThroughTrans;
//				VSTORE3F(pathThroughput, &task->pathStateBase.throughput.r);
//
//				// It is a pass through point, continue to trace the ray
//				ray->mint = rayHit->t + MachineEpsilon_E(rayHit->t);
//
//				// Keep the same path state
//			}
//#endif
//#if defined(PARAM_HAS_PASSTHROUGH) && (PARAM_DL_LIGHT_COUNT > 0)
//			else
//#endif
			{
//#if (PARAM_DL_LIGHT_COUNT > 0)
//				// Check if it is a light source (note: I can hit only triangle area light sources)
//				if (bsdf->triangleLightSourceIndex != NULL_INDEX) {
//					float directPdfA;
//					const float3 emittedRadiance = BSDF_GetEmittedRadiance(bsdf,
//							triLightDefs, &directPdfA
//							MATERIALS_PARAM);
//					if (!Spectrum_IsBlack(emittedRadiance)) {
//						// Add emitted radiance
//						float weight = 1.f;
//						if (!task->directLightState.lastSpecular) {
//							const float lightPickProb = Scene_PickLightPdf();
//							const float directPdfW = PdfAtoW(directPdfA, rayHit->t,
//								fabs(dot(VLOAD3F(&bsdf->fixedDir.x), VLOAD3F(&bsdf->shadeN.x))));
//
//							// MIS between BSDF sampling and direct light sampling
//							weight = PowerHeuristic(task->directLightState.lastPdfW, directPdfW * lightPickProb);
//						}
//
//						float3 radiance = VLOAD3F(&sample->radiance.r);
//						const float3 pathThroughput = VLOAD3F(&task->pathStateBase.throughput.r);
//						const float3 le = pathThroughput * weight * emittedRadiance;
//						radiance += le;
//						VSTORE3F(radiance, &sample->radiance.r);
//					}
//				}
//#endif

//#if defined(PARAM_HAS_SUNLIGHT) || (PARAM_DL_LIGHT_COUNT > 0)
//				// Direct light sampling
//				pathState = GENERATE_DL_RAY;
//#else
				// Sample next path vertex
				pathState = GENERATE_NEXT_VERTEX_RAY;
//#endif
			}
		} else {
			//------------------------------------------------------------------
			// Nothing was hit, get environmental lights radiance
			//------------------------------------------------------------------
#if defined(PARAM_HAS_INFINITELIGHT) || defined(PARAM_HAS_SKYLIGHT) || defined(PARAM_HAS_SUNLIGHT)
			float3 radiance = VLOAD3F(&sample->radiance.r);
			const float3 pathThroughput = VLOAD3F(&task->pathStateBase.throughput.r);
			const float3 dir = -VLOAD3F(&ray->d.x);
			float3 lightRadiance = BLACK;

#if defined(PARAM_HAS_INFINITELIGHT)
			lightRadiance += InfiniteLight_GetRadiance(infiniteLight, dir
					IMAGEMAPS_PARAM);
#endif
//#if defined(PARAM_HAS_SKYLIGHT)
//			lightRadiance += SkyLight_GetRadiance(skyLight, dir);
//#endif
//#if defined(PARAM_HAS_SUNLIGHT)
//			float directPdfW;
//			const float3 sunRadiance = SunLight_GetRadiance(sunLight, dir, &directPdfW);
//			if (!Spectrum_IsBlack(sunRadiance)) {
//				// MIS between BSDF sampling and direct light sampling
//				const float weight = (task->directLightState.lastSpecular ? 1.f : PowerHeuristic(task->directLightState.lastPdfW, directPdfW));
//				lightRadiance += weight * sunRadiance;
//			}
//#endif

			radiance += pathThroughput * lightRadiance;
			VSTORE3F(radiance, &sample->radiance.r);
#endif

#if defined(PARAM_ENABLE_ALPHA_CHANNEL)
			if (depth == 1)
				sample->alpha = 0.f;
#endif

			pathState = SPLAT_SAMPLE;
		}
	}

	//--------------------------------------------------------------------------
	// Evaluation of the Path finite state machine.
	//
	// From: RT_DL
	// To: GENERATE_NEXT_VERTEX_RAY
	//--------------------------------------------------------------------------

//#if defined(PARAM_HAS_SUNLIGHT) || (PARAM_DL_LIGHT_COUNT > 0)
//	if (pathState == RT_DL) {
//		pathState = GENERATE_NEXT_VERTEX_RAY;
//
//		if (currentTriangleIndex == NULL_INDEX) {
//			// Nothing was hit, the light source is visible
//			float3 radiance = VLOAD3F(&sample->radiance.r);
//			const float3 lightRadiance = VLOAD3F(&task->directLightState.lightRadiance.r);
//			radiance += lightRadiance;
//			VSTORE3F(radiance, &sample->radiance.r);
//		}
//#if defined(PARAM_HAS_PASSTHROUGH)
//		else {
//			BSDF_Init(&task->passThroughState.passThroughBsdf,
//#if defined(PARAM_ACCEL_MQBVH)
//					meshFirstTriangleOffset,
//					meshDescs,
//#endif
//					meshMats, meshIDs,
//#if (PARAM_DL_LIGHT_COUNT > 0)
//					meshLights,
//#endif
//					vertices, vertNormals, vertUVs,
//					triangles, ray, rayHit,
//					task->passThroughState.passThroughEvent
//#if defined(PARAM_HAS_BUMPMAPS) || defined(PARAM_HAS_NORMALMAPS)
//					MATERIALS_PARAM
//#endif
//					);
//
//			const float3 passthroughTrans = BSDF_GetPassThroughTransparency(&task->passThroughState.passThroughBsdf
//					MATERIALS_PARAM);
//			if (!Spectrum_IsBlack(passthroughTrans)) {
//				const float3 lightRadiance = VLOAD3F(&task->directLightState.lightRadiance.r) * passthroughTrans;
//				VSTORE3F(lightRadiance, &task->directLightState.lightRadiance.r);
//
//				// It is a pass through point, continue to trace the ray
//				ray->mint = rayHit->t + MachineEpsilon_E(rayHit->t);
//				pathState = RT_DL;
//			}
//		}
//#endif
//	}
//#endif

	//--------------------------------------------------------------------------
	// Evaluation of the Path finite state machine.
	//
	// From: GENERATE_DL_RAY
	// To: GENERATE_NEXT_VERTEX_RAY or RT_DL
	//--------------------------------------------------------------------------

//#if defined(PARAM_HAS_SUNLIGHT) || (PARAM_DL_LIGHT_COUNT > 0)
//	if (pathState == GENERATE_DL_RAY) {
//		pathState = GENERATE_NEXT_VERTEX_RAY;
//
//		if (!BSDF_IsDelta(bsdf
//				MATERIALS_PARAM)) {
//			float3 lightRayDir;
//			float distance, directPdfW;
//			float3 lightRadiance;
//#if defined(PARAM_HAS_SUNLIGHT) && (PARAM_DL_LIGHT_COUNT > 0)
//			// Pick a light source to sample
//			const float lu0 = Sampler_GetSamplePathVertex(depth, IDX_DIRECTLIGHT_X);
//			const float lightPickPdf = Scene_PickLightPdf();
//
//			const uint lightIndex = min((uint)floor((PARAM_DL_LIGHT_COUNT + 1) * lu0), (uint)(PARAM_DL_LIGHT_COUNT));
//
//			if (lightIndex == PARAM_DL_LIGHT_COUNT) {
//				lightRadiance = SunLight_Illuminate(
//					sunLight,
//					Sampler_GetSamplePathVertex(depth, IDX_DIRECTLIGHT_Y),
//					Sampler_GetSamplePathVertex(depth, IDX_DIRECTLIGHT_Z),
//					&lightRayDir, &distance, &directPdfW);
//			} else {
//				lightRadiance = TriangleLight_Illuminate(
//					&triLightDefs[lightIndex], VLOAD3F(&bsdf->hitPoint.x),
//					Sampler_GetSamplePathVertex(depth, IDX_DIRECTLIGHT_Y),
//					Sampler_GetSamplePathVertex(depth, IDX_DIRECTLIGHT_Z),
//					Sampler_GetSamplePathVertex(depth, IDX_DIRECTLIGHT_W),
//					&lightRayDir, &distance, &directPdfW
//					MATERIALS_PARAM);
//			}
//#elif (PARAM_DL_LIGHT_COUNT > 0)
//			// Pick a light source to sample
//			const float lu0 = Sampler_GetSamplePathVertex(depth, IDX_DIRECTLIGHT_X);
//			const float lightPickPdf = Scene_PickLightPdf();
//
//			const uint lightIndex = min((uint)floor(PARAM_DL_LIGHT_COUNT * lu0), (uint)(PARAM_DL_LIGHT_COUNT - 1));
//
//			lightRadiance = TriangleLight_Illuminate(
//					&triLightDefs[lightIndex], VLOAD3F(&bsdf->hitPoint.x),
//					Sampler_GetSamplePathVertex(depth, IDX_DIRECTLIGHT_Y),
//					Sampler_GetSamplePathVertex(depth, IDX_DIRECTLIGHT_Z),
//					Sampler_GetSamplePathVertex(depth, IDX_DIRECTLIGHT_W),
//					&lightRayDir, &distance, &directPdfW
//					MATERIALS_PARAM);
//#elif defined(PARAM_HAS_SUNLIGHT)
//			// Pick a light source to sample
//			const float lightPickPdf = 1.f;
//
//			lightRadiance = SunLight_Illuminate(
//					sunLight,
//					Sampler_GetSamplePathVertex(depth, IDX_DIRECTLIGHT_Y),
//					Sampler_GetSamplePathVertex(depth, IDX_DIRECTLIGHT_Z),
//					&lightRayDir, &distance, &directPdfW);
//#endif
//
//			// Setup the shadow ray
//			if (!Spectrum_IsBlack(lightRadiance)) {
//				BSDFEvent event;
//				float bsdfPdfW;
//				const float3 bsdfEval = BSDF_Evaluate(bsdf,
//						lightRayDir, &event, &bsdfPdfW
//						MATERIALS_PARAM);
//
//				if (!Spectrum_IsBlack(bsdfEval)) {
//					const float3 pathThroughput = VLOAD3F(&task->pathStateBase.throughput.r);
//					const float cosThetaToLight = fabs(dot(lightRayDir, VLOAD3F(&bsdf->shadeN.x)));
//					const float directLightSamplingPdfW = directPdfW * lightPickPdf;
//					const float factor = cosThetaToLight / directLightSamplingPdfW;
//
//					// Russian Roulette
//					bsdfPdfW *= (depth >= PARAM_RR_DEPTH) ? fmax(Spectrum_Filter(bsdfEval), PARAM_RR_CAP) : 1.f;
//
//					// MIS between direct light sampling and BSDF sampling
//					const float weight = PowerHeuristic(directLightSamplingPdfW, bsdfPdfW);
//
//					VSTORE3F((weight * factor) * pathThroughput * bsdfEval * lightRadiance, &task->directLightState.lightRadiance.r);
//#if defined(PARAM_HAS_PASSTHROUGH)
//					task->passThroughState.passThroughEvent = Sampler_GetSamplePathVertex(depth, IDX_DIRECTLIGHT_A);
//#endif
//
//					// Setup the shadow ray
//					const float3 hitPoint = VLOAD3F(&bsdf->hitPoint.x);
//					const float epsilon = fmax(MachineEpsilon_E_Float3(hitPoint), MachineEpsilon_E(distance));
//					Ray_Init4(ray, hitPoint, lightRayDir,
//						epsilon,
//						distance - epsilon);
//					pathState = RT_DL;
//				}
//			}
//		}
//	}
//#endif

	//--------------------------------------------------------------------------
	// Evaluation of the Path finite state machine.
	//
	// From: GENERATE_NEXT_VERTEX_RAY
	// To: SPLAT_SAMPLE or RT_NEXT_VERTEX
	//--------------------------------------------------------------------------

	if (pathState == GENERATE_NEXT_VERTEX_RAY) {
		if (depth < PARAM_MAX_PATH_DEPTH) {
			// Sample the BSDF
			__global BSDF *bsdf = &task->pathStateBase.bsdf;
			float3 sampledDir;
			float lastPdfW;
			float cosSampledDir;
			BSDFEvent event;

			const float3 bsdfSample = BSDF_Sample(bsdf,
					Sampler_GetSamplePathVertex(depth, IDX_BSDF_X),
					Sampler_GetSamplePathVertex(depth, IDX_BSDF_Y),
					&sampledDir, &lastPdfW, &cosSampledDir, &event
					MATERIALS_PARAM);
//			const bool lastSpecular = ((event & SPECULAR) != 0);

			// Russian Roulette
//			const float rrProb = fmax(Spectrum_Filter(bsdfSample), PARAM_RR_CAP);
//			const bool rrEnabled = (depth >= PARAM_RR_DEPTH) && !lastSpecular;
//			const bool rrContinuePath = !rrEnabled || (Sampler_GetSamplePathVertex(depth, IDX_RR) < rrProb);

			const bool continuePath = !Spectrum_IsBlack(bsdfSample);// && rrContinuePath;
			if (continuePath) {
//				if (rrEnabled)
//					lastPdfW *= rrProb; // Russian Roulette

				float3 throughput = VLOAD3F(&task->pathStateBase.throughput.r);
				throughput *= bsdfSample * (cosSampledDir / lastPdfW);
				VSTORE3F(throughput, &task->pathStateBase.throughput.r);

				Ray_Init2(ray, VLOAD3F(&bsdf->hitPoint.x), sampledDir);

				task->pathStateBase.depth = depth + 1;
#if defined(PARAM_HAS_SUNLIGHT) || (PARAM_DL_LIGHT_COUNT > 0)
				task->directLightState.lastPdfW = lastPdfW;
				task->directLightState.lastSpecular = lastSpecular;
#endif
#if defined(PARAM_HAS_PASSTHROUGH)
				// This is a bit tricky. I store the passThroughEvent in the BSDF
				// before of the initialization because it can be use during the
				// tracing of next path vertex ray.

				// This sampleDataPathVertexBase is used inside Sampler_GetSamplePathVertex() macro
				__global float *sampleDataPathVertexBase = Sampler_GetSampleDataPathVertex(
					sample, sampleDataPathBase, depth + 1);
				task->pathStateBase.bsdf.passThroughEvent = Sampler_GetSamplePathVertex(depth + 1, IDX_PASSTHROUGH);
#endif
//				pathState = RT_NEXT_VERTEX;

				VSTORE3F(bsdfSample * cosSampledDir, &sample->radiance.r);
				pathState = SPLAT_SAMPLE;
			} else
				pathState = SPLAT_SAMPLE;
		} else
			pathState = SPLAT_SAMPLE;
	}

	//--------------------------------------------------------------------------
	// Evaluation of the Path finite state machine.
	//
	// From: SPLAT_SAMPLE
	// To: RT_NEXT_VERTEX
	//--------------------------------------------------------------------------

	if (pathState == SPLAT_SAMPLE) {
		Sampler_NextSample(task, sample, sampleData, seed, frameBuffer,
#if defined(PARAM_ENABLE_ALPHA_CHANNEL)
				alphaFrameBuffer,
#endif
				camera, ray);
		taskStats[gid].sampleCount += 1;

		// task->pathStateBase.state is set to RT_NEXT_VERTEX inside Sampler_NextSample() => GenerateCameraPath()
	} else {
		// Save the state
		task->pathStateBase.state = pathState;
	}
		

	//--------------------------------------------------------------------------

	// Save the seed
	task->seed.s1 = seed->s1;
	task->seed.s2 = seed->s2;
	task->seed.s3 = seed->s3;
}
