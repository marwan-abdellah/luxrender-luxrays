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

#include <cstdlib>
#include <cassert>
#include <iosfwd>
#include <sstream>
#include <stdexcept>

#include "luxrays/core/context.h"
#ifdef LUXRAYS_DISABLE_OPENCL
#include "luxrays/core/intersectiondevice.h"
#else
#include "luxrays/opencl/intersectiondevice.h"
#endif
#include "luxrays/core/virtualdevice.h"

using namespace luxrays;

Context::Context(LuxRaysDebugHandler handler, const int openclPlatformIndex) {
	debugHandler = handler;
	currentDataSet = NULL;
	started = false;

	// Get the list of devices available on the platform
	NativeThreadDeviceDescription::AddDeviceDescs(deviceDescriptions);

#if !defined(LUXRAYS_DISABLE_OPENCL)
	// Platform info
	VECTOR_CLASS<cl::Platform> platforms;
	cl::Platform::get(&platforms);
	for (size_t i = 0; i < platforms.size(); ++i)
		LR_LOG(this, "OpenCL Platform " << i << ": " << platforms[i].getInfo<CL_PLATFORM_VENDOR>().c_str());

	if (openclPlatformIndex < 0) {
		if (platforms.size() > 0) {
			// Just use all the platforms available
			for (size_t i = 0; i < platforms.size(); ++i)
				OpenCLDeviceDescription::AddDeviceDescs(
					platforms[i], DEVICE_TYPE_OPENCL_ALL,
					deviceDescriptions);
		} else
			LR_LOG(this, "No OpenCL platform available");
	} else {
		if ((platforms.size() == 0) || (openclPlatformIndex >= (int)platforms.size()))
			throw std::runtime_error("Unable to find an appropiate OpenCL platform");
		else {
			OpenCLDeviceDescription::AddDeviceDescs(
				platforms[openclPlatformIndex],
				DEVICE_TYPE_OPENCL_ALL, deviceDescriptions);
		}
	}
#endif

	// Print device info
	for (size_t i = 0; i < deviceDescriptions.size(); ++i) {
		DeviceDescription *desc = deviceDescriptions[i];
		LR_LOG(this, "Device " << i << " name: " <<
			desc->GetName());

		LR_LOG(this, "Device " << i << " type: " <<
			DeviceDescription::GetDeviceType(desc->GetType()));

		LR_LOG(this, "Device " << i << " compute units: " <<
			desc->GetComputeUnits());

		LR_LOG(this, "Device " << i << " max allocable memory: " <<
			desc->GetMaxMemory() / (1024 * 1024) << "MBytes");

		LR_LOG(this, "Device " << i << " max allocable memory block size: " <<
			desc->GetMaxMemoryAllocSize() / (1024 * 1024) << "MBytes");
	}
}

Context::~Context() {
	if (started)
		Stop();

	for (size_t i = 0; i < idevices.size(); ++i) {
		// Virtual devices are deleted below
		if (idevices[i]->GetType() != DEVICE_TYPE_VIRTUAL)
			delete idevices[i];
	}
	for (size_t i = 0; i < m2mDevices.size(); ++i)
		delete m2mDevices[i];
	for (size_t i = 0; i < m2oDevices.size(); ++i)
		delete m2oDevices[i];

	for (size_t i = 0; i < deviceDescriptions.size(); ++i)
		delete deviceDescriptions[i];
}

void Context::SetDataSet(DataSet *dataSet) {
	assert (!started);
	assert ((dataSet == NULL) || ((dataSet != NULL) && dataSet->IsPreprocessed()));

	currentDataSet = dataSet;

	for (size_t i = 0; i < idevices.size(); ++i)
		idevices[i]->SetDataSet(currentDataSet);
}

void Context::UpdateDataSet() {
	assert (started);

	if (currentDataSet->GetAcceleratorType() != ACCEL_MQBVH)
		throw std::runtime_error("Context::UpdateDataSet supported only with MQBVH accelerator");

	// Update the data set
	currentDataSet->UpdateMeshes();

#if !defined(LUXRAYS_DISABLE_OPENCL)
	// Update all OpenCL devices
	for (unsigned int i = 0; i < oclDevices.size(); ++i)
		oclDevices[i]->UpdateDataSet();
#endif
}

void Context::Start() {
	assert (!started);

	for (size_t i = 0; i < idevices.size(); ++i)
		idevices[i]->Start();

	started = true;
}

void Context::Interrupt() {
	assert (started);

	for (size_t i = 0; i < idevices.size(); ++i)
		idevices[i]->Interrupt();
}

void Context::Stop() {
	assert (started);

	Interrupt();

	for (size_t i = 0; i < idevices.size(); ++i)
		idevices[i]->Stop();

	started = false;
}

const std::vector<DeviceDescription *> &Context::GetAvailableDeviceDescriptions() const {
	return deviceDescriptions;
}

const std::vector<IntersectionDevice *> &Context::GetIntersectionDevices() const {
	return idevices;
}

const std::vector<VirtualM2OHardwareIntersectionDevice *> &Context::GetVirtualM2OIntersectionDevices() const {
	return m2oDevices;
}

const std::vector<VirtualM2MHardwareIntersectionDevice *> &Context::GetVirtualM2MIntersectionDevices() const {
	return m2mDevices;
}

std::vector<IntersectionDevice *> Context::CreateIntersectionDevices(std::vector<DeviceDescription *> &deviceDesc) {
	assert (!started);

	LR_LOG(this, "Creating " << deviceDesc.size() << " intersection device(s)");

	std::vector<IntersectionDevice *> newDevices;
	for (size_t i = 0; i < deviceDesc.size(); ++i) {
		LR_LOG(this, "Allocating intersection device " << i << ": " << deviceDesc[i]->GetName() <<
				" (Type = " << DeviceDescription::GetDeviceType(deviceDesc[i]->GetType()) << ")");

		IntersectionDevice *device;
		if (deviceDesc[i]->GetType() == DEVICE_TYPE_NATIVE_THREAD) {
			// Nathive thread devices
			const NativeThreadDeviceDescription *ntvDeviceDesc = (const NativeThreadDeviceDescription *)deviceDesc[i];
			device = new NativeThreadIntersectionDevice(this, ntvDeviceDesc->GetThreadIndex(), i);
		}
#if !defined(LUXRAYS_DISABLE_OPENCL)
		else if (deviceDesc[i]->GetType() & DEVICE_TYPE_OPENCL_ALL) {
			// OpenCL devices
			OpenCLDeviceDescription *oclDeviceDesc = (OpenCLDeviceDescription *)deviceDesc[i];

			device = new OpenCLIntersectionDevice(this, oclDeviceDesc, i);
			oclDevices.push_back((OpenCLIntersectionDevice *)device);
		}
#endif
		else
			assert (false);

		newDevices.push_back(device);
	}

	return newDevices;
}

std::vector<IntersectionDevice *> Context::AddIntersectionDevices(std::vector<DeviceDescription *> &deviceDesc) {
	assert (!started);

	std::vector<IntersectionDevice *> newDevices = CreateIntersectionDevices(deviceDesc);
	for (size_t i = 0; i < newDevices.size(); ++i)
		idevices.push_back(newDevices[i]);

	return newDevices;
}

std::vector<IntersectionDevice *> Context::AddVirtualM2MIntersectionDevices(const unsigned int count,
		std::vector<DeviceDescription *> &deviceDescs) {
	assert (!started);
	assert (deviceDescs.size() > 0);

	std::vector<IntersectionDevice *> realDevices = CreateIntersectionDevices(deviceDescs);
	// OpenCL are the only HardwareIntersectionDevice supported at the moment
	std::vector<HardwareIntersectionDevice *> realHDevices;
	for (size_t i = 0; i < realDevices.size(); ++i)
		realHDevices.push_back((HardwareIntersectionDevice *)realDevices[i]);

	VirtualM2MHardwareIntersectionDevice *m2mDevice = new VirtualM2MHardwareIntersectionDevice(count, realHDevices);

	m2mDevices.push_back(m2mDevice);
	for (unsigned int i = 0; i < count; ++i)
		idevices.push_back(m2mDevice->GetVirtualDevice(i));

	return realDevices;
}

std::vector<IntersectionDevice *> Context::AddVirtualM2OIntersectionDevices(const unsigned int count,
		std::vector<DeviceDescription *> &deviceDescs) {
	assert (!started);
	assert (deviceDescs.size() == 1);

	std::vector<IntersectionDevice *> realDevices = CreateIntersectionDevices(deviceDescs);
	VirtualM2OHardwareIntersectionDevice *m2oDevice = new VirtualM2OHardwareIntersectionDevice(count, (HardwareIntersectionDevice *)realDevices[0]);

	m2oDevices.push_back(m2oDevice);
	for (unsigned int i = 0; i < count; ++i)
		idevices.push_back(m2oDevice->GetVirtualDevice(i));

	return realDevices;
}
