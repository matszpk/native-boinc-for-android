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
#include "cal_boinc.h"
#include "gpu_detect.h"

using namespace std;

#ifdef _WIN32
typedef int (__stdcall *ATI_ATTRIBS) (CALdeviceattribs *attribs, CALuint ordinal);
typedef int (__stdcall *ATI_CLOSE)(void);
typedef int (__stdcall *ATI_GDC)(CALuint *numDevices);
typedef int (__stdcall *ATI_GDI)(void);
typedef int (__stdcall *ATI_INFO) (CALdeviceinfo *info, CALuint ordinal);
typedef int (__stdcall *ATI_VER) (CALuint *cal_major, CALuint *cal_minor, CALuint *cal_imp);
typedef int (__stdcall *ATI_STATUS) (CALdevicestatus*, CALdevice);
typedef int (__stdcall *ATI_DEVICEOPEN) (CALdevice*, CALuint);
typedef int (__stdcall *ATI_DEVICECLOSE) (CALdevice);

ATI_ATTRIBS __calDeviceGetAttribs = NULL;
ATI_CLOSE   __calShutdown = NULL;
ATI_GDC     __calDeviceGetCount = NULL;
ATI_GDI     __calInit = NULL;
ATI_INFO    __calDeviceGetInfo = NULL;
ATI_VER     __calGetVersion = NULL;
ATI_STATUS  __calDeviceGetStatus = NULL;
ATI_DEVICEOPEN  __calDeviceOpen = NULL;
ATI_DEVICECLOSE  __calDeviceClose = NULL;

#else

int (*__calInit)();
int (*__calGetVersion)(CALuint*, CALuint*, CALuint*);
int (*__calDeviceGetCount)(CALuint*);
int (*__calDeviceGetAttribs)(CALdeviceattribs*, CALuint);
int (*__calShutdown)();
int (*__calDeviceGetInfo)(CALdeviceinfo*, CALuint);
int (*__calDeviceGetStatus)(CALdevicestatus*, CALdevice);
int (*__calDeviceOpen)(CALdevice*, CALuint);
int (*__calDeviceClose)(CALdevice);

#endif

void recherche_gpu_amd(vector<string>& warnings, vector<GPU_AMD>& descs_amd)
{
	CALuint numDevices;
	bool atirt_detected=false;
	bool amdrt_detected=false;
	CALdeviceattribs attribs;
    char buf[256];
    int retval;
	string gpu_name;
	GPU_AMD gpu_temp;
	attribs.struct_size = sizeof(CALdeviceattribs);
	
#ifdef _WIN32
#if defined _M_X64
    const char* atilib_name = "aticalrt64.dll";
    const char* amdlib_name = "amdcalrt64.dll";
#else
    const char* atilib_name = "aticalrt.dll";
    const char* amdlib_name = "amdcalrt.dll";
#endif

	HINSTANCE callib = LoadLibrary(amdlib_name);
    if (!callib) {
        callib = LoadLibrary(amdlib_name);
    }

    if (!callib) {
        warnings.push_back("No ATI library found.\n");
        return;
    }

	__calInit = (ATI_GDI)GetProcAddress(callib, "calInit" );
    __calGetVersion = (ATI_VER)GetProcAddress(callib, "calGetVersion" );
    __calDeviceGetCount = (ATI_GDC)GetProcAddress(callib, "calDeviceGetCount" );
    __calDeviceGetAttribs =(ATI_ATTRIBS)GetProcAddress(callib, "calDeviceGetAttribs" );
    __calShutdown = (ATI_CLOSE)GetProcAddress(callib, "calShutdown" );
    __calDeviceGetInfo = (ATI_INFO)GetProcAddress(callib, "calDeviceGetInfo" );
    __calDeviceGetStatus = (ATI_STATUS)GetProcAddress(callib, "calDeviceGetStatus" );
    __calDeviceOpen = (ATI_DEVICEOPEN)GetProcAddress(callib, "calDeviceOpen" );
    __calDeviceClose = (ATI_DEVICECLOSE)GetProcAddress(callib, "calDeviceClose" );

#else
	void* callib;

    callib = dlopen("libaticalrt.so", RTLD_NOW);
    if (!callib) {
        warnings.push_back("No ATI library found\n");
        return;
    }

    __calInit = (int(*)()) dlsym(callib, "calInit");
    __calGetVersion = (int(*)(CALuint*, CALuint*, CALuint*)) dlsym(callib, "calGetVersion");
    __calDeviceGetCount = (int(*)(CALuint*)) dlsym(callib, "calDeviceGetCount");
    __calDeviceGetAttribs = (int(*)(CALdeviceattribs*, CALuint)) dlsym(callib, "calDeviceGetAttribs");
    __calShutdown = (int(*)()) dlsym(callib, "calShutdown");
    __calDeviceGetInfo = (int(*)(CALdeviceinfo*, CALuint)) dlsym(callib, "calDeviceGetInfo");
    __calDeviceGetStatus = (int(*)(CALdevicestatus*, CALdevice)) dlsym(callib, "calDeviceGetStatus");
    __calDeviceOpen = (int(*)(CALdevice*, CALuint)) dlsym(callib, "calDeviceOpen");
    __calDeviceClose = (int(*)(CALdevice)) dlsym(callib, "calDeviceClose");

#endif

	if (!__calInit) {
        warnings.push_back("calInit() missing from CAL library\n");
        return;
    }
    if (!__calDeviceGetCount) {
        warnings.push_back("calDeviceGetCount() missing from CAL library\n");
        return;
    }
    if (!__calDeviceGetAttribs) {
        warnings.push_back("calDeviceGetAttribs() missing from CAL library\n");
        return;
    }
    
    retval = (*__calInit)();
    if (retval != CAL_RESULT_OK) {
        sprintf(buf, "calInit() returned %d\n", retval);
        warnings.push_back(buf);
        return;
    }
    retval = (*__calDeviceGetCount)(&numDevices);
    if (retval != CAL_RESULT_OK) {
        sprintf(buf, "calDeviceGetCount() returned %d\n", retval);
        warnings.push_back(buf);
        return;
    }
    
    if (!numDevices) {
        warnings.push_back("No usable CAL devices found\n");
        return;
    }
	for (CALuint i=0; i<numDevices; i++) {
		retval = (*__calDeviceGetAttribs)(&attribs, i);
        if (retval != CAL_RESULT_OK) {
            sprintf(buf, "calDeviceGetAttribs() returned %d\n", retval);
            warnings.push_back(buf);
            return;
        }
        switch ((int)attribs.target) {
		case 0:
            gpu_name="ATI Radeon HD 2900 (RV600)";
            break;
        case 1:
            gpu_name="ATI Radeon HD 2300/2400/3200/4200 (RV610)";
            attribs.numberOfSIMD=1;        // set correct values (reported wrong by driver)
            attribs.wavefrontSize=32;
            break;
        case 2:
            gpu_name="ATI Radeon HD 2600/3650 (RV630/RV635)";
            // set correct values (reported wrong by driver)
            attribs.numberOfSIMD=3;
            attribs.wavefrontSize=32;
            break;
        case 3:
            gpu_name="ATI Radeon HD 3800 (RV670)";
            break;
        case 4:
            gpu_name="ATI Radeon HD 4350/4550 (R710)";
            break;
        case 7:
            gpu_name="ATI Radeon HD 4600 series (R730)";
            break;
        case 6:
            gpu_name="ATI Radeon (RV700 class)";
            break;
        case 5:
            gpu_name="ATI Radeon HD 4700/4800 (RV740/RV770)";
            break;
        case 8:
            gpu_name="ATI Radeon HD 5800/5900 series (Cypress/Hemlock)";
            break;
        case 9:
            gpu_name="ATI Radeon HD 5700 series (Juniper)";
            break;
        case 10:
            gpu_name="ATI Radeon HD 5500/5600 series (Redwood)";
			// AMD Radeon HD 6390/7510 (OEM rebranded)
			break;
        case 11:
            gpu_name="ATI Radeon HD 5400 series (Cedar)";
			// real names would be AMD Radeon HD 6290/6350/7350/8350 (OEM rebranded)
			break;
// AMD CAL Chaos - see http://developer.amd.com/download/AMD_Accelerated_Parallel_Processing_OpenCL_Programming_Guide.pdf @page 230
		case 12:
            gpu_name="AMD Radeon HD 6370D/6380G/6410D/6480G (Sumo)"; // OpenCL-Name = WinterPark
			break;
        case 13:
            gpu_name="AMD Radeon HD 6520G/6530D/6550D/6620G (SuperSumo)"; // OpenCL-Name = BeaverCreek
			break;
        case 14:
            gpu_name="AMD Radeon HD 6200/6300/7300 series (Wrestler)"; // OpenCL-Name = Loveland
			// real names would be AMD Radeon HD 6250/6290/6310/6320/7310/7340 series (Wrestler)
            break;
        case 15:
            gpu_name="AMD Radeon HD 6900 series (Cayman)";
            break;
        case 16:
            gpu_name="AMD Radeon HD (Kauai)";
			break;
        case 17:
            gpu_name="AMD Radeon HD 6790/6850/6870 series (Barts)";
			// AMD Radeon 7720 (OEM rebranded)
            break;
        case 18:
            gpu_name="AMD Radeon HD 6570/6670/7570/7670 series (Turks)";
            break;
        case 19:
            gpu_name="AMD Radeon HD 6350/6450/7450/7470 series (Caicos)";
			// AMD Radeon HD 7450/7470/8450/8470/8490 (OEM rebranded)
            break;
        case 20:
            gpu_name="AMD Radeon HD 7870/7950/7970 series (Tahiti)";
            break;
        case 21:
            gpu_name="AMD Radeon HD 7850/7870 series (Pitcairn)";
            break;
        case 22:
            gpu_name="AMD Radeon HD 7700 series (Capeverde)";
            break;
        case 23:
            gpu_name="AMD Radeon HD 7500/7600/8500/8600 series (Devastator)";
			// higher GPUs inside Trinity/Richland APUs
			// PCI Device IDs 9900h to 991Fh 
            break;
		case 24:
            gpu_name="AMD Radeon HD 7400/7500/8300/8400 series (Scrapper)";
			// (s)lower GPUs of Trinity/Richland APUs
			// PCI Device IDs 9990h to 99AFh
            break;
        case 25:
            gpu_name="AMD Radeon HD 8600/8790M (Oland)";
            break;
		case 26:
            gpu_name="AMD Radeon HD 7790 series (Bonaire)"; 
            break;
		case 27:
			gpu_name="AMD Radeon HD (Casper)";
			break;
		case 28:
			gpu_name="AMD Radeon HD (Slimer)";
			break;
		case 29:
			gpu_name="AMD Radeon HD 8200/8300/8400 series (Kalindi)";
			// GPUs inside AMD Family 16h aka Kabini/Temash
			break;
		case 30:
			gpu_name="AMD Radeon HD 8600M (Hainan)";
			break;
		case 31:
			gpu_name="AMD Radeon HD (Curacao)";
			break;
        default:
            gpu_name="AMD Radeon HD (unknown)";
            break;
        }
		sprintf(gpu_temp.description,gpu_name.c_str());
		gpu_temp.clockRate=attribs.engineClock;
		gpu_temp.device_num=i;
		gpu_temp.multiProcessorCount=attribs.numberOfSIMD;
		gpu_temp.target=attribs.target;
		descs_amd.push_back(gpu_temp);
	}
}
