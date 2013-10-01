using namespace std;

#define MAX_OPENCL_PLATFORMS 16
#define MAX_COPROC_INSTANCES 64

struct GPU_NVIDIA
{
	char description[256];
	int clockRate;
	int multiProcessorCount;
	int device_num;
};

struct GPU_AMD
{
	char description[256];
	int clockRate;
	int multiProcessorCount;
	int device_num;
	int target;
};

struct GPU_OPENCL
{
	char description[256];
	char vendor[256]; 
	int clockRate;
	int multiProcessorCount;
	int device_num;
	int target;
};

struct GPU
{
	char description[256];
	char vendor[256]; 
	int clockRate;
	int multiProcessorCount;
	int device_num;
	int target;
};


void recherche_gpu_nvidia(vector<string>& warnings, vector<GPU_NVIDIA>& descs);
void recherche_gpu_amd(vector<string>& warnings, vector<GPU_AMD>& descs);
void recherche_gpu_opencl(vector<string>& warnings, vector<GPU_OPENCL>& descs);
void recherche_gpu(vector<string>& warnings, vector<GPU>& descs);
