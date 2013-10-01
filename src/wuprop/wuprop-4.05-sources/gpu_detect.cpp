#ifdef _WIN32
#include "boinc_win.h"
#else
#include <string>
#include <string.h>
#include <stdio.h>
#include <vector>
#include <setjmp.h>
 #include <signal.h>
#endif
#include "gpu_detect.h"

using namespace std;

#ifndef _WIN32
jmp_buf resume;

void segv_handler(int) {
    longjmp(resume, 1);
}
#endif

void recherche_gpu(vector<string>& warnings, vector<GPU>& descs)
{
	vector<GPU_NVIDIA> descs_nvidia;
	vector<string> warnings_nvidia;
	vector<GPU_AMD> descs_amd;
	vector<string> warnings_amd;
	vector<GPU_OPENCL> descs_opencl;
	vector<string> warnings_opencl;
	int amd_device_num=0;
	int nvidia_device_num=0;
	int intel_device_num=0;
	char buf[256];
	bool gpu_recence=false;
	GPU gpu_temp;
	
#ifdef _WIN32
    try {
        recherche_gpu_nvidia(warnings_nvidia, descs_nvidia);
    }
    catch (...) {
        warnings.push_back("Caught SIGSEGV in NVIDIA GPU detection\n");
    }
    try {
        recherche_gpu_amd(warnings_amd, descs_amd);
    } 
    catch (...) {
        warnings.push_back("Caught SIGSEGV in ATI GPU detection\n");
    }
    try {
        recherche_gpu_opencl(warnings_opencl, descs_opencl);
    } 
    catch (...) {
        warnings.push_back("Caught SIGSEGV in OpenCL detection\n");
    }
#else
    void (*old_sig)(int) = signal(SIGSEGV, segv_handler);
    if (setjmp(resume)) {
        warnings.push_back("Caught SIGSEGV in NVIDIA GPU detection\n");
    } else {
        recherche_gpu_nvidia(warnings_nvidia, descs_nvidia);
    }
#ifndef __APPLE__       // ATI does not yet support CAL on Macs
    if (setjmp(resume)) {
        warnings.push_back("Caught SIGSEGV in ATI GPU detection\n");
    } else {
        recherche_gpu_amd(warnings_amd, descs_amd);
    }
#endif
    if (setjmp(resume)) {
        warnings.push_back("Caught SIGSEGV in OpenCL detection\n");
    } else {
        recherche_gpu_opencl(warnings_opencl, descs_opencl);
    }
    signal(SIGSEGV, old_sig);
#endif
	
	if (descs_nvidia.size()==0)	{
		for(vector<string>::iterator it = warnings_nvidia.begin(); it != warnings_nvidia.end(); ++it) {
			warnings.push_back(*it);
		}
	}
	else {
		for(vector<GPU_NVIDIA>::iterator it = descs_nvidia.begin(); it != descs_nvidia.end(); ++it) {
			strcpy(gpu_temp.vendor,"NVIDIA");
			strcpy(gpu_temp.description,(*it).description);
			gpu_temp.clockRate=(*it).clockRate/1000;
			gpu_temp.device_num=(*it).device_num;
			gpu_temp.multiProcessorCount=(*it).multiProcessorCount;
			gpu_temp.target=-1;
			descs.push_back(gpu_temp);
			nvidia_device_num++;
		}
	}

	if (descs_amd.size()==0)	{
		for(vector<string>::iterator it = warnings_amd.begin(); it != warnings_amd.end(); ++it) {
			warnings.push_back(*it);
		}
	}
	else {
		for(vector<GPU_AMD>::iterator it = descs_amd.begin(); it != descs_amd.end(); ++it) {
			strcpy(gpu_temp.vendor,"ATI/AMD");
			strcpy(gpu_temp.description,(*it).description);
			gpu_temp.clockRate=(*it).clockRate;
			gpu_temp.device_num=(*it).device_num;
			gpu_temp.multiProcessorCount=(*it).multiProcessorCount;
			gpu_temp.target=(*it).target;
			descs.push_back(gpu_temp);
			amd_device_num++;
		}
	}
	
	if (descs_opencl.size()==0)	{
		for(vector<string>::iterator it = warnings_opencl.begin(); it != warnings_opencl.end(); ++it) {
			warnings.push_back(*it);
		}
	}
	else {
		for(vector<GPU_OPENCL>::iterator it = descs_opencl.begin(); it != descs_opencl.end(); ++it) {
			if (strstr((*it).vendor,"NVIDIA")){
				gpu_recence=false;
				for(vector<GPU_NVIDIA>::iterator it_nvidia = descs_nvidia.begin(); it_nvidia!=descs_nvidia.end();it_nvidia++) {
					if(strstr((*it).description,(*it_nvidia).description)){
						sprintf(buf, "Device %s already detected as NVIDIA %s\n",(*it).description,(*it_nvidia).description);
						warnings.push_back(buf);
						gpu_recence=true;
						break;
					}
				}
				if (!gpu_recence) {
					strcpy(gpu_temp.vendor,(*it).vendor);
					strcpy(gpu_temp.description,(*it).description);
					gpu_temp.clockRate=(*it).clockRate;
					gpu_temp.device_num=nvidia_device_num;
					gpu_temp.multiProcessorCount=(*it).multiProcessorCount;
					gpu_temp.target=-1;
					descs.push_back(gpu_temp);
					nvidia_device_num++;					
				}
			}
			else if (strstr((*it).vendor,"ATI") || strstr((*it).vendor,"AMD") || strstr((*it).vendor,"Advanced Micro Devices, Inc.")){
				gpu_recence=false;
				for(vector<GPU_AMD>::iterator it_amd = descs_amd.begin(); it_amd!=descs_amd.end();it_amd++) {
					if(((*it).target==(*it_amd).target) && ((*it).multiProcessorCount==(*it_amd).multiProcessorCount)){
						sprintf(buf, "Device %s already detected as AMD %s\n",(*it).description,(*it_amd).description);
						warnings.push_back(buf);
						gpu_recence=true;
						break;
					}
				}
				if (!gpu_recence) {
					strcpy(gpu_temp.vendor,"ATI/AMD");
					strcpy(gpu_temp.description,(*it).description);
					gpu_temp.clockRate=(*it).clockRate;
					gpu_temp.device_num=amd_device_num;
					gpu_temp.multiProcessorCount=(*it).multiProcessorCount;
					gpu_temp.target=(*it).target;
					descs.push_back(gpu_temp);
					amd_device_num++;					
				}
			}
			else if (strstr((*it).vendor,"intel_gpu")){
				strcpy(gpu_temp.vendor,"Intel");
				strcpy(gpu_temp.description,(*it).description);
				gpu_temp.clockRate=(*it).clockRate;
				gpu_temp.device_num=intel_device_num;
				gpu_temp.multiProcessorCount=(*it).multiProcessorCount;
				gpu_temp.target=-1;
				descs.push_back(gpu_temp);
				intel_device_num++;		
			}
		}
	}
}
