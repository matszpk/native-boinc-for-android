#ifdef _WIN32
#include "boinc_win.h"
#else
#ifdef __APPLE__
// Suppress obsolete warning when building for OS 10.3.9
#define DLOPEN_NO_WARN
#include <mach-o/dyld.h>
#endif
#include "config.h"
#include <dlfcn.h>
#include <string>
#include <string.h>
#include <stdio.h>
#include <vector>
#endif
#include "cl_boinc.h"
#include "gpu_detect.h"

using namespace std;

#ifdef _WIN32
HMODULE opencl_lib = NULL;

typedef cl_int (__stdcall *CL_PLATFORMIDS) (cl_uint, cl_platform_id*, cl_uint*);
typedef cl_int (__stdcall *CL_PLATFORMINFO) (cl_platform_id, cl_platform_info, size_t, void*, size_t*);
typedef cl_int (__stdcall *CL_DEVICEIDS)(cl_platform_id, cl_device_type, cl_uint, cl_device_id*, cl_uint*);
typedef cl_int (__stdcall *CL_INFO) (cl_device_id, cl_device_info, size_t, void*, size_t*);

CL_PLATFORMIDS  __clGetPlatformIDs = NULL;
CL_PLATFORMINFO __clGetPlatformInfo = NULL;
CL_DEVICEIDS    __clGetDeviceIDs = NULL;
CL_INFO         __clGetDeviceInfo = NULL;

#else

void* opencl_lib = NULL;

cl_int (*__clGetPlatformIDs)(
    cl_uint,         // num_entries,
    cl_platform_id*, // platforms
    cl_uint *        // num_platforms
);
cl_int (*__clGetPlatformInfo)(
    cl_platform_id,  // platform
    cl_platform_info, // param_name
    size_t,          // param_value_size
    void*,           // param_value
    size_t*          // param_value_size_ret
);
cl_int (*__clGetDeviceIDs)(
    cl_platform_id,  // platform
    cl_device_type,  // device_type
    cl_uint,         // num_entries
    cl_device_id*,   // devices
    cl_uint*         // num_devices
);
cl_int (*__clGetDeviceInfo)(
    cl_device_id,    // device
    cl_device_info,  // param_name
    size_t,          // param_value_size
    void*,           // param_value
    size_t*          // param_value_size_ret
);

#endif

void recherche_gpu_opencl(vector<string>& warnings, vector<GPU_OPENCL>& descs_opencl)
{
	cl_int ciErrNum;
    cl_platform_id platforms[MAX_OPENCL_PLATFORMS];
    cl_uint num_platforms, platform_index, num_devices, device_index;
	cl_device_id devices[MAX_COPROC_INSTANCES];
	char platform_version[256];
	GPU_OPENCL gpu_temp;
	
#ifdef _WIN32
	opencl_lib = LoadLibrary("OpenCL.dll");
    if (!opencl_lib) {
        warnings.push_back("No OpenCL library found\n");
        return;
    }

	__clGetPlatformIDs = (CL_PLATFORMIDS)GetProcAddress( opencl_lib, "clGetPlatformIDs" );
    __clGetPlatformInfo = (CL_PLATFORMINFO)GetProcAddress( opencl_lib, "clGetPlatformInfo" );
    __clGetDeviceIDs = (CL_DEVICEIDS)GetProcAddress( opencl_lib, "clGetDeviceIDs" );
    __clGetDeviceInfo = (CL_INFO)GetProcAddress( opencl_lib, "clGetDeviceInfo" );

#else
#ifdef __APPLE__
    opencl_lib = dlopen("/System/Library/Frameworks/OpenCL.framework/Versions/Current/OpenCL", RTLD_NOW);
#else
//TODO: Is this correct?
    opencl_lib = dlopen("libOpenCL.so", RTLD_NOW);
#endif
    if (!opencl_lib) {
        warnings.push_back("No OpenCL library found\n");
        return;
    }
    __clGetPlatformIDs = (cl_int(*)(cl_uint, cl_platform_id*, cl_uint*)) dlsym( opencl_lib, "clGetPlatformIDs" );
    __clGetPlatformInfo = (cl_int(*)(cl_platform_id, cl_platform_info, size_t, void*, size_t*)) dlsym( opencl_lib, "clGetPlatformInfo" );
    __clGetDeviceIDs = (cl_int(*)(cl_platform_id, cl_device_type, cl_uint, cl_device_id*, cl_uint*)) dlsym( opencl_lib, "clGetDeviceIDs" );
    __clGetDeviceInfo = (cl_int(*)(cl_device_id, cl_device_info, size_t, void*, size_t*)) dlsym( opencl_lib, "clGetDeviceInfo" );
#endif

	if (!__clGetPlatformIDs) {
        warnings.push_back("clGetPlatformIDs() missing from OpenCL library\n");
        return;
    }
    if (!__clGetPlatformInfo) {
        warnings.push_back("clGetPlatformInfo() missing from OpenCL library\n");
        return;
    }
    if (!__clGetDeviceIDs) {
        warnings.push_back("clGetDeviceIDs() missing from OpenCL library\n");
        return;
    }
    if (!__clGetDeviceInfo) {
        warnings.push_back("clGetDeviceInfo() missing from OpenCL library\n");
        return;
    }

	ciErrNum = (*__clGetPlatformIDs)(MAX_OPENCL_PLATFORMS, platforms, &num_platforms);
    if (ciErrNum != CL_SUCCESS) {
        warnings.push_back("clGetPlatformIDs() failed to return any OpenCL platforms\n");
        return;
    }

	for (platform_index=0; platform_index<num_platforms; ++platform_index) {
        ciErrNum = (*__clGetPlatformInfo)(
            platforms[platform_index], CL_PLATFORM_VERSION,
            sizeof(platform_version), &platform_version, NULL
        );
        if (ciErrNum != CL_SUCCESS) continue;
        ciErrNum = (*__clGetDeviceIDs)(
            platforms[platform_index], CL_DEVICE_TYPE_GPU,
            MAX_COPROC_INSTANCES, devices, &num_devices
        );
        if (ciErrNum == CL_DEVICE_NOT_FOUND) continue;  // No devices
        if (ciErrNum != CL_SUCCESS) continue;

		for (device_index=0; device_index<num_devices; ++device_index) {
			ciErrNum = (*__clGetDeviceInfo)(devices[device_index], CL_DEVICE_NAME, sizeof(gpu_temp.description), gpu_temp.description, NULL);
			if ((ciErrNum != CL_SUCCESS) || (gpu_temp.description[0] == 0)) continue;
			
			ciErrNum = (*__clGetDeviceInfo)(devices[device_index], CL_DEVICE_VENDOR, sizeof(gpu_temp.vendor), gpu_temp.vendor, NULL);
			if ((ciErrNum != CL_SUCCESS) || (gpu_temp.vendor[0] == 0)) continue;
			
			ciErrNum = (*__clGetDeviceInfo)(
				devices[device_index], CL_DEVICE_MAX_COMPUTE_UNITS,
				sizeof(gpu_temp.multiProcessorCount), &gpu_temp.multiProcessorCount, NULL
			);
			if (ciErrNum != CL_SUCCESS) continue;
			
			ciErrNum = (*__clGetDeviceInfo)(
				devices[device_index], CL_DEVICE_MAX_CLOCK_FREQUENCY,
				sizeof(gpu_temp.clockRate), &gpu_temp.clockRate, NULL
			);
			if (ciErrNum != CL_SUCCESS) continue;

			gpu_temp.target=-1;
			if (strstr(gpu_temp.description,"RV770")) gpu_temp.target=5;
			if (strstr(gpu_temp.description,"RV710")) gpu_temp.target=6;
			if (strstr(gpu_temp.description,"RV730")) gpu_temp.target=7;
			if (strstr(gpu_temp.description,"Cypress")) gpu_temp.target=8;
			if (strstr(gpu_temp.description,"Juniper")) gpu_temp.target=9;
			if (strstr(gpu_temp.description,"Redwood")) gpu_temp.target=10;
			if (strstr(gpu_temp.description,"Cedar")) gpu_temp.target=11;
			if (strstr(gpu_temp.description,"SUMO")) gpu_temp.target=12;
			if (strstr(gpu_temp.description,"WinterPark")) gpu_temp.target=12;
			if (strstr(gpu_temp.description,"SUPERSUMO")) gpu_temp.target=13;
			if (strstr(gpu_temp.description,"BeaverCreek")) gpu_temp.target=13;
			if (strstr(gpu_temp.description,"Wrestler")) gpu_temp.target=14;
			if (strstr(gpu_temp.description,"Loveland")) gpu_temp.target=14;
			if (strstr(gpu_temp.description,"Cayman")) gpu_temp.target=15;
			if (strstr(gpu_temp.description,"Kuai")) gpu_temp.target=16;
			if (strstr(gpu_temp.description,"Barts")) gpu_temp.target=17;
			if (strstr(gpu_temp.description,"Turks")) gpu_temp.target=18;
			if (strstr(gpu_temp.description,"Caicos")) gpu_temp.target=19;
			if (strstr(gpu_temp.description,"Tahiti")) gpu_temp.target=20;
			if (strstr(gpu_temp.description,"Pitcairn")) gpu_temp.target=21;
			if (strstr(gpu_temp.description,"Capeverde")) gpu_temp.target=22;
			if (strstr(gpu_temp.description,"Devastator")) gpu_temp.target=23;
			if (strstr(gpu_temp.description,"Scrapper")) gpu_temp.target=24;
			if (strstr(gpu_temp.description,"Oland")) gpu_temp.target=25;
			if (strstr(gpu_temp.description,"Bonaire")) gpu_temp.target=26;
			if (strstr(gpu_temp.description,"Casper")) gpu_temp.target=27;
			if (strstr(gpu_temp.description,"Slimer")) gpu_temp.target=28;
			if (strstr(gpu_temp.description,"Kalindi")) gpu_temp.target=29;
			if (strstr(gpu_temp.description,"Hainan")) gpu_temp.target=30;
			if (strstr(gpu_temp.description,"Curacao")) gpu_temp.target=31;

			descs_opencl.push_back(gpu_temp);
		}
	}
}	
