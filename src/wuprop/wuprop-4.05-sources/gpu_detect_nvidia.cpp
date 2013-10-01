#ifdef _WIN32
#include "boinc_win.h"
#include "nvapi.h"
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
#include "gpu_detect.h"

using namespace std;

typedef int CUdevice;

#ifdef _WIN32
typedef int (__stdcall *CUDA_GDC)(int *count);
typedef int (__stdcall *CUDA_GDV)(int* version);
typedef int (__stdcall *CUDA_GDI)(unsigned int);
typedef int (__stdcall *CUDA_GDG)(int*, int);
typedef int (__stdcall *CUDA_GDA)(int*, int, int);
typedef int (__stdcall *CUDA_GDN)(char*, int, int);
typedef int (__stdcall *CUDA_GDM)(size_t*, int);
typedef int (__stdcall *CUDA_GDCC)(int*, int*, int);
typedef int (__stdcall *CUDA_CC)(void**, unsigned int, unsigned int);
typedef int (__stdcall *CUDA_CD)(void*);
typedef int (__stdcall *CUDA_MA)(unsigned int*, size_t);
typedef int (__stdcall *CUDA_MF)(unsigned int);
typedef int (__stdcall *CUDA_MGI)(size_t*, size_t*);


CUDA_GDC __cuDeviceGetCount = NULL;
CUDA_GDV __cuDriverGetVersion = NULL;
CUDA_GDI __cuInit = NULL;
CUDA_GDG __cuDeviceGet = NULL;
CUDA_GDA __cuDeviceGetAttribute = NULL;
CUDA_GDN __cuDeviceGetName = NULL;
CUDA_GDM __cuDeviceTotalMem = NULL;
CUDA_GDCC __cuDeviceComputeCapability = NULL;
CUDA_CC __cuCtxCreate = NULL;
CUDA_CD __cuCtxDestroy = NULL;
CUDA_MA __cuMemAlloc = NULL;
CUDA_MF __cuMemFree = NULL;
CUDA_MGI __cuMemGetInfo = NULL;
#else
void* cudalib;
int (*__cuInit)(unsigned int);
int (*__cuDeviceGetCount)(int*);
int (*__cuDriverGetVersion)(int*);
int (*__cuDeviceGet)(int*, int);
int (*__cuDeviceGetAttribute)(int*, int, int);
int (*__cuDeviceGetName)(char*, int, int);
int (*__cuDeviceTotalMem)(size_t*, int);
int (*__cuDeviceComputeCapability)(int*, int*, int);
int (*__cuCtxCreate)(void**, unsigned int, unsigned int);
int (*__cuCtxDestroy)(void*);
int (*__cuMemAlloc)(unsigned int*, size_t);
int (*__cuMemFree)(unsigned int);
int (*__cuMemGetInfo)(size_t*, size_t*);
#endif

enum CUdevice_attribute_enum {
    CU_DEVICE_ATTRIBUTE_MAX_THREADS_PER_BLOCK = 1,
    CU_DEVICE_ATTRIBUTE_MAX_BLOCK_DIM_X = 2,
    CU_DEVICE_ATTRIBUTE_MAX_BLOCK_DIM_Y = 3,
    CU_DEVICE_ATTRIBUTE_MAX_BLOCK_DIM_Z = 4,
    CU_DEVICE_ATTRIBUTE_MAX_GRID_DIM_X = 5,
    CU_DEVICE_ATTRIBUTE_MAX_GRID_DIM_Y = 6,
    CU_DEVICE_ATTRIBUTE_MAX_GRID_DIM_Z = 7,
    CU_DEVICE_ATTRIBUTE_SHARED_MEMORY_PER_BLOCK = 8,
    CU_DEVICE_ATTRIBUTE_TOTAL_CONSTANT_MEMORY = 9,
    CU_DEVICE_ATTRIBUTE_WARP_SIZE = 10,
    CU_DEVICE_ATTRIBUTE_MAX_PITCH = 11,
    CU_DEVICE_ATTRIBUTE_REGISTERS_PER_BLOCK = 12,
    CU_DEVICE_ATTRIBUTE_CLOCK_RATE = 13,
    CU_DEVICE_ATTRIBUTE_TEXTURE_ALIGNMENT = 14,
    CU_DEVICE_ATTRIBUTE_GPU_OVERLAP = 15,
    CU_DEVICE_ATTRIBUTE_MULTIPROCESSOR_COUNT = 16,
    CU_DEVICE_ATTRIBUTE_KERNEL_EXEC_TIMEOUT = 17,
    CU_DEVICE_ATTRIBUTE_INTEGRATED = 18,
    CU_DEVICE_ATTRIBUTE_CAN_MAP_HOST_MEMORY = 19,
    CU_DEVICE_ATTRIBUTE_PCI_BUS_ID = 33,
    CU_DEVICE_ATTRIBUTE_PCI_DEVICE_ID = 34,
    CU_DEVICE_ATTRIBUTE_PCI_DOMAIN_ID = 50
};


void recherche_gpu_nvidia(vector<string>& warnings, vector<GPU_NVIDIA>& descs_nvidia)
{
	int cuda_ndevs, retval;
	char buf[256];
	GPU_NVIDIA gpu_temp;

#ifdef _WIN32
	HMODULE cudalib = LoadLibrary("nvcuda.dll");
	if (!cudalib) {
        warnings.push_back("No NVIDIA library found\n");
        return;
	}
	__cuDeviceGetCount = (CUDA_GDC)GetProcAddress( cudalib, "cuDeviceGetCount" );
    __cuDriverGetVersion = (CUDA_GDV)GetProcAddress( cudalib, "cuDriverGetVersion" );
    __cuInit = (CUDA_GDI)GetProcAddress( cudalib, "cuInit" );
    __cuDeviceGet = (CUDA_GDG)GetProcAddress( cudalib, "cuDeviceGet" );
    __cuDeviceGetAttribute = (CUDA_GDA)GetProcAddress( cudalib, "cuDeviceGetAttribute" );
    __cuDeviceGetName = (CUDA_GDN)GetProcAddress( cudalib, "cuDeviceGetName" );
    __cuDeviceTotalMem = (CUDA_GDM)GetProcAddress( cudalib, "cuDeviceTotalMem" );
    __cuDeviceComputeCapability = (CUDA_GDCC)GetProcAddress( cudalib, "cuDeviceComputeCapability" );
    __cuCtxCreate = (CUDA_CC)GetProcAddress( cudalib, "cuCtxCreate" );
    __cuCtxDestroy = (CUDA_CD)GetProcAddress( cudalib, "cuCtxDestroy" );
    __cuMemAlloc = (CUDA_MA)GetProcAddress( cudalib, "cuMemAlloc" );
    __cuMemFree = (CUDA_MF)GetProcAddress( cudalib, "cuMemFree" );
    __cuMemGetInfo = (CUDA_MGI)GetProcAddress( cudalib, "cuMemGetInfo" );

#else

#ifdef __APPLE__
    cudalib = dlopen("/usr/local/cuda/lib/libcuda.dylib", RTLD_NOW);
#else
    cudalib = dlopen("libcuda.so", RTLD_NOW);
#endif
    if (!cudalib) {
        warnings.push_back("No NVIDIA library found\n");
        return;
    }
    __cuDeviceGetCount = (int(*)(int*)) dlsym(cudalib, "cuDeviceGetCount");
    __cuDriverGetVersion = (int(*)(int*)) dlsym( cudalib, "cuDriverGetVersion" );
    __cuInit = (int(*)(unsigned int)) dlsym( cudalib, "cuInit" );
    __cuDeviceGet = (int(*)(int*, int)) dlsym( cudalib, "cuDeviceGet" );
    __cuDeviceGetAttribute = (int(*)(int*, int, int)) dlsym( cudalib, "cuDeviceGetAttribute" );
    __cuDeviceGetName = (int(*)(char*, int, int)) dlsym( cudalib, "cuDeviceGetName" );
    __cuDeviceTotalMem = (int(*)(size_t*, int)) dlsym( cudalib, "cuDeviceTotalMem" );
    __cuDeviceComputeCapability = (int(*)(int*, int*, int)) dlsym( cudalib, "cuDeviceComputeCapability" );
    __cuCtxCreate = (int(*)(void**, unsigned int, unsigned int)) dlsym( cudalib, "cuCtxCreate" );
    __cuCtxDestroy = (int(*)(void*)) dlsym( cudalib, "cuCtxDestroy" );
    __cuMemAlloc = (int(*)(unsigned int*, size_t)) dlsym( cudalib, "cuMemAlloc" );
    __cuMemFree = (int(*)(unsigned int)) dlsym( cudalib, "cuMemFree" );
    __cuMemGetInfo = (int(*)(size_t*, size_t*)) dlsym( cudalib, "cuMemGetInfo" );
#endif

	if (!__cuInit) {
        warnings.push_back("cuInit() missing from NVIDIA library\n");
        return;
    }
    if (!__cuDeviceGetCount) {
        warnings.push_back("cuDeviceGetCount() missing from NVIDIA library\n");
        return;
    }
    if (!__cuDeviceGet) {
        warnings.push_back("cuDeviceGet() missing from NVIDIA library\n");
        return;
    }
    if (!__cuDeviceGetAttribute) {
        warnings.push_back("cuDeviceGetAttribute() missing from NVIDIA library\n");
        return;
    }

    retval = (*__cuInit)(0);
    if (retval) {
        warnings.push_back("NVIDIA drivers present but no GPUs found\n");
        return;
    }

    retval = (*__cuDeviceGetCount)(&cuda_ndevs);
    if (retval) {
        sprintf(buf, "cuDeviceGetCount() returned %d\n", retval);
		warnings.push_back(buf);
        return;
    }

	sprintf(buf, "NVIDIA library reports %d GPU%s\n", cuda_ndevs, (cuda_ndevs==1)?"":"s");
	warnings.push_back(buf);

	for (int j=0; j<cuda_ndevs; j++) 
	{
		CUdevice device;
		gpu_temp.device_num=j;
        retval = (*__cuDeviceGet)(&device, j);
        if (retval) {
            sprintf(buf, "cuDeviceGet(%d) returned %d\n", j, retval);
            warnings.push_back(buf);
            return;
        }
        (*__cuDeviceGetName)(gpu_temp.description, 256, device);
        if (retval) {
            sprintf(buf, "cuDeviceGetName(%d) returned %d\n", j, retval);
            warnings.push_back(buf);
            return;
        }
		(*__cuDeviceGetAttribute)(&gpu_temp.clockRate, CU_DEVICE_ATTRIBUTE_CLOCK_RATE, device);
		(*__cuDeviceGetAttribute)(&gpu_temp.multiProcessorCount, CU_DEVICE_ATTRIBUTE_MULTIPROCESSOR_COUNT, device);
		descs_nvidia.push_back(gpu_temp);
	}
}
