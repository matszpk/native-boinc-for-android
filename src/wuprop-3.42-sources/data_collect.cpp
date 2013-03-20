#if defined _WIN32
#include "boinc_win.h"
#include <winsock.h>
#include <cstring>
#include <time.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <string>
#include <vector>
#include <iostream>
#include <map>
#include <stdlib.h>
#include <unistd.h> 
#include <cmath>
#include <list>
#include <set>
#include <time.h>
#include <sys/poll.h>
#include <sstream>
#endif
#include "boinc_api.h"
#include "filesys.h"
#include "util.h"
#include "gui_rpc_client.h"
#include "pugixml.hpp"

using namespace std;

#define CHECKPOINT_FILE "checkpoint"
#define INPUT_FILENAME "in"
#define OUTPUT_FILENAME "out"
#define CACHE_FILENAME "cache"

FILE* state;
map<string, string> dict_project;
map<string, string> dict_app;
map<string, string> dict_platform;
map<string, string> dict_delai;
map<string, string> dict_check;
map<string, string> dict_conso;
map<string, double> dict_file;
map<string, string> dict_upload;
map<string, string> dict_tmp_upload;
map<string, string> dict_download;
map<string, string> dict_duree;
map<string, string>  dict_occupation;
map<string, string> dict_result;
map<string, string> dict_dl_app;
map<string, int> dict_nb_file_result;
map<string, int> dict_new_wu;
map<string, double> dict_size_file_result;
map<string, double> dict_ecart_releve;
map<string, double> dict_prochain_releve;
map<string, string> dict_dir;
map<string, long> dict_run;
list<string> list_active_project;
int nb_cycle=0;
int port=31416;
int start=0;
int end;
int start_compute;
int last_new_wu=0;
pugi::xml_document doc_state;	//state
map<string,string>::iterator it;
map<string,int>::iterator it_int;
map<string,long>::iterator it_long;
map<string,double>::iterator it_double;
list<string>::iterator it_liste;
MFILE out;
MFILE cache;
list<string> liste;
string wu_name;
bool chargement_state;
int socket_state=0;
int socket_active_result=0;
string reponse_state;
string reponse_active_result;
string gpu;
string message;
string project_url;
string project_name;
string file_name;
string name;
int nb_gpu=0;
string plan_class;

string path_state;
double taille;
double tmp_taille;
char cache_path[512];
APP_INIT_DATA aid;
int dernier_chargement=0;
string format_state="";

char buf[256];
CC_STATUS cs;
HOST_INFO hi;
RESULTS results;
RESULTS results_all;
CC_STATE gstate;
string app_name;
string user_friendly_name;
double received_time;
string str_received_time;
double final_cpu_time;
double final_elapsed_time;
string str_final_cpu_time;
string str_final_elapsed_time;
string hostid_projet;
string str_version_num;
string platform;
int version_num;
ostringstream strs;

#ifdef ANDROID
string product_model;
string product_name;
string product_manufacturer;
#endif

#if defined _WIN32
typedef unsigned __int32 uint32;
typedef unsigned __int64 uint64;
double uint64_to_double (const uint64 x)
{
	return (double)x;
}

double RDTSC(void)
{
	unsigned long a, b;
	double x;
	__asm
	{
		RDTSC
		mov [a],eax
		mov [b],edx
	}
	x  = b;
	x *= 4294967296;
	x += a;
	return x;
}


int frequence ()
{
	uint64 Fwin;
	uint64 Twin_avant, Twin_apres;
	double Tcpu_avant, Tcpu_apres;
	double Fcpu;
	
	if (!QueryPerformanceFrequency((LARGE_INTEGER*)&Fwin)) return 0;
	
	// Avant
	Tcpu_avant = RDTSC();
	QueryPerformanceCounter((LARGE_INTEGER*)&Twin_avant);
	
	do
	{
		QueryPerformanceCounter((LARGE_INTEGER*)&Twin_apres);
	} while (Twin_apres-Twin_avant < 10000);
	
	Tcpu_apres = RDTSC();
	QueryPerformanceCounter((LARGE_INTEGER*)&Twin_apres);
	
	Fcpu  = (Tcpu_apres - Tcpu_avant);
	Fcpu *= uint64_to_double(Fwin);
	Fcpu /= uint64_to_double(Twin_apres - Twin_avant);
	return int(Fcpu/1000000);
}

int ncpu_theorical ()
{
	SYSTEM_INFO SystemInfo;
	memset( &SystemInfo, NULL, sizeof( SystemInfo ) );
	::GetSystemInfo( &SystemInfo );
	return SystemInfo.dwNumberOfProcessors;
}
#elif defined __i386 || defined __amd64
unsigned long long int rdtsc(void)
{
	unsigned long long int x;
	__asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
	return x;
}

int frequence()
{
	struct timezone tz;
	struct timeval tvstart, tvstop;
	unsigned long long int cycles[2];
	unsigned long microseconds;
	int mhz;
	memset(&tz, 0, sizeof(tz));
	gettimeofday(&tvstart, &tz);
	cycles[0] = rdtsc();
	gettimeofday(&tvstart, &tz);
	usleep(250000);
	gettimeofday(&tvstop, &tz);
	cycles[1] = rdtsc();
	gettimeofday(&tvstop, &tz);
	microseconds = ((tvstop.tv_sec-tvstart.tv_sec)*1000000) + (tvstop.tv_usec-tvstart.tv_usec);
	mhz = (int) (cycles[1]-cycles[0]) / microseconds;
	return mhz;
}

#if !defined __APPLE__
int ncpu_theorical ()
{
	return sysconf(_SC_NPROCESSORS_ONLN);
}
#else
int ncpu_theorical ()
{
	int mib[2];
  size_t len;
  int p_ncpus;
	mib[0] = CTL_HW;
	mib[1] = HW_NCPU;
	len = sizeof(p_ncpus);
  sysctl(mib, 2, &p_ncpus, &len, NULL, 0);
	return p_ncpus;
}
#endif
#else
int frequence()
{
	return 0;
}
int ncpu_theorical ()
{
	return 0;
}
#endif

int do_cache()
{
	int retval = cache.open(cache_path, "wb");
	if (retval) return 1;
	for ( it=dict_delai.begin() ; it != dict_delai.end(); it++)
	{
		cache.printf("Delai\t%s\t%s\n", (*it).first.c_str(), (*it).second.c_str());
	}
	for ( it=dict_check.begin() ; it != dict_check.end(); it++)
	{
		cache.printf("Check\t%s\t%s\n", (*it).first.c_str(), (*it).second.c_str());
	}
	for ( it=dict_conso.begin() ; it != dict_conso.end(); it++)
	{
		cache.printf("Conso\t%s\t%s\n", (*it).first.c_str(), (*it).second.c_str());
	}
	for ( it_double=dict_file.begin() ; it_double !=dict_file.end(); it_double++)
	{
		cache.printf("File\t%s\t%f\n", (*it_double).first.c_str(), (*it_double).second);
	}
	for ( it=dict_upload.begin() ; it != dict_upload.end(); it++)
	{
		cache.printf("Upload\t%s\t%s\n", (*it).first.c_str(), (*it).second.c_str());
	}
	for ( it=dict_download.begin() ; it != dict_download.end(); it++)
	{
		cache.printf( "Download\t%s\t%s\n", (*it).first.c_str(), (*it).second.c_str());
	}
	for ( it=dict_occupation.begin() ; it !=dict_occupation.end(); it++)
	{
		cache.printf( "Occupation\t%s\t%s\n", (*it).first.c_str(), (*it).second.c_str());
	}
	for ( it=dict_result.begin() ; it !=dict_result.end(); it++)
	{
		cache.printf( "Result\t%s\t%s\n", (*it).first.c_str(), (*it).second.c_str());
	}
	for ( it=dict_duree.begin() ; it !=dict_duree.end(); it++)
	{
		cache.printf( "Duree\t%s\t%s\n", (*it).first.c_str(), (*it).second.c_str());
	}
	for ( it_int=dict_nb_file_result.begin() ; it_int !=dict_nb_file_result.end(); it_int++)
	{
		cache.printf( "Nb_file_result\t%s\t%d\n", (*it_int).first.c_str(), (*it_int).second);
	}
	for ( it_double=dict_size_file_result.begin() ; it_double !=dict_size_file_result.end(); it_double++)
	{
		cache.printf( "Size_file_result\t%s\t%f\n", (*it_double).first.c_str(), (*it_double).second);
	}
	for ( it=dict_dir.begin() ; it !=dict_dir.end(); it++)
	{
		cache.printf( "Dir\t%s\t%s\n", (*it).first.c_str(), (*it).second.c_str());
	}
	for ( it=dict_dl_app.begin() ; it!=dict_dl_app.end(); it++)
	{
		cache.printf( "Dl_app\t%s\t%s\n", (*it).first.c_str(), (*it).second.c_str());
	}
	for ( it=dict_project.begin() ; it!=dict_project.end(); it++)
	{
		cache.printf( "Project\t%s\t%s\n", (*it).first.c_str(), (*it).second.c_str());
	}
	for ( it_double=dict_ecart_releve.begin() ; it_double !=dict_ecart_releve.end(); it_double++)
	{
		cache.printf( "Ecart_releve\t%s\t%f\n", (*it_double).first.c_str(), (*it_double).second);
	}
	for ( it_double=dict_prochain_releve.begin() ; it_double !=dict_prochain_releve.end(); it_double++)
	{
		cache.printf( "Prochain_releve\t%s\t%f\n", (*it_double).first.c_str(), (*it_double).second);
	}
	for ( it_int=dict_new_wu.begin() ; it_int !=dict_new_wu.end(); it_int++)
	{
		cache.printf( "New wu\t%s\t%d\n", (*it_int).first.c_str(), (*it_int).second);
	}
	cache.printf("Last_new_wu\t%d\n", last_new_wu);
	for ( it=dict_app.begin() ; it!=dict_app.end(); it++)
	{
		cache.printf( "App\t%s\t%s\n", (*it).first.c_str(), (*it).second.c_str());
	}
	for ( it=dict_platform.begin() ; it!=dict_platform.end(); it++)
	{
		cache.printf( "Platform\t%s\t%s\n", (*it).first.c_str(), (*it).second.c_str());
	}
	for ( it_liste=list_active_project.begin() ; it_liste!=list_active_project.end(); it_liste++)
	{
		cache.printf( "Active_project\t%s\n", (*it_liste).c_str());
	}
  	for ( it_long=dict_run.begin() ; it_long !=dict_run.end(); it_long++)
	{
		cache.printf( "Run\t%s\t%d\n", (*it_long).first.c_str(), (*it_long).second);
	}
	cache.flush();
	cache.close();
	return 0;
	
}

int do_checkpoint() 
{
	int retval;
	string resolved_name;
	FILE* f = fopen("temp", "w");
	if (!f) return 1;
	fprintf(f,"%d\n",nb_cycle);
	fprintf(f,"%d\n",start);
	fprintf(f,"%d\n",start_compute);
	fclose(f);
	boinc_resolve_filename_s(CHECKPOINT_FILE, resolved_name);
	retval = boinc_rename("temp", resolved_name.c_str());
	if (retval) return errno;
	retval= do_cache();
	if (retval) return 1;
	return 0;
}

void restore_cache()
{
	char *param1, *param2, *param3;
	char data[100000+1];
	while (fgets(data,100000, state))
	{
		param1=strtok(data, "\t");
		param2=strtok(NULL, "\t");
		param3=strtok(NULL, "\n");
		if (param1!=NULL && param2!=NULL)
		{
			if (strcmp(param1,"Last_new_wu")==0) last_new_wu=atoi(param2);
			if (strcmp(param1,"Active_project")==0)
			{
				param3=strtok(param2, "\n");
				if (param3!=NULL) list_active_project.push_back(param3);
			}
			if (param3!=NULL)
			{
				if (strcmp(param1,"Delai")==0) dict_delai[param2]=param3;
				if (strcmp(param1,"Check")==0) dict_check[param2]=param3;
				if (strcmp(param1,"Conso")==0) dict_conso[param2]=param3;
				if (strcmp(param1,"File")==0) dict_file[param2]=atof(param3);
				if (strcmp(param1,"Upload")==0) dict_upload[param2]=param3;
				if (strcmp(param1,"Download")==0) dict_download[param2]=param3;
				if (strcmp(param1,"Occupation")==0) dict_occupation[param2]=param3;
				if (strcmp(param1,"Result")==0) dict_result[param2]=param3;
				if (strcmp(param1,"Duree")==0) dict_duree[param2]=param3;
				if (strcmp(param1,"Nb_file_result")==0) dict_nb_file_result[param2]=atoi(param3);
				if (strcmp(param1,"Size_file_result")==0) dict_size_file_result[param2]=atof(param3);
				if (strcmp(param1,"Dir")==0) dict_dir[param2]=param3;
				if (strcmp(param1,"Dl_app")==0) dict_dl_app[param2]=param3;	
				if (strcmp(param1,"Project")==0) dict_project[param2]=param3;		
				if (strcmp(param1,"Ecart_releve")==0) dict_ecart_releve[param2]=atof(param3);
				if (strcmp(param1,"Prochain_releve")==0) dict_prochain_releve[param2]=atof(param3);
				if (strcmp(param1,"New wu")==0) dict_new_wu[param2]=atoi(param3);
				if (strcmp(param1,"Last_new_wu")==0) last_new_wu=atoi(param2);
				if (strcmp(param1,"App")==0) dict_app[param2]=param3;
				if (strcmp(param1,"Platform")==0) dict_platform[param2]=param3;
				if (strcmp(param1,"Run")==0) dict_run[param2]=atoi(param3);
			}
		}
		memset(data,0,100000);
	}
}

void restore_checkpoint()
{
	char entete[1000+1];
	memset(entete,0,1000);
	fgets(entete,1000,state);
	nb_cycle=atoi(strtok(entete, "\t"));
	fgets(entete,1000,state);
	start=atoi(strtok(entete, "\t"));
	fgets(entete,1000,state);
	start_compute=atoi(strtok(entete, "\t"));
	
}

int reception_cc_status()
{
	RPC_CLIENT rpc;
	int retval;
	if ((dernier_chargement>0) && (time(NULL)<dernier_chargement+31))
	{
		boinc_sleep(31-time(NULL)+dernier_chargement);
		fprintf(stderr, "%s Mise en veille (derniere requete date de moins de 30 secondes\n",boinc_msg_prefix(buf, sizeof(buf)));
	}
	retval = rpc.init("127.0.0.1", port);
	if (retval) {
		fprintf(stderr, "%s can't connect to localhost",boinc_msg_prefix(buf, sizeof(buf)));
		return -1;
	}
	retval = rpc.get_cc_status(cs);
	return retval;
}

int reception_host_info()
{
	RPC_CLIENT rpc;
	int retval;
	if ((dernier_chargement>0) && (time(NULL)<dernier_chargement+31))
	{
		boinc_sleep(31-time(NULL)+dernier_chargement);
		fprintf(stderr, "%s Mise en veille (derniere requete date de moins de 30 secondes\n",boinc_msg_prefix(buf, sizeof(buf)));
	}
	retval = rpc.init("127.0.0.1", port);
	if (retval) {
		fprintf(stderr, "%s can't connect to localhost",boinc_msg_prefix(buf, sizeof(buf)));
		return -1;
	}
	retval = rpc.get_host_info(hi);
	return retval;
}

int reception_active_result()
{
	RPC_CLIENT rpc;
	int retval;
	if ((dernier_chargement>0) && (time(NULL)<dernier_chargement+31))
	{
		boinc_sleep(31-time(NULL)+dernier_chargement);
		fprintf(stderr, "%s Mise en veille (derniere requete date de moins de 30 secondes\n",boinc_msg_prefix(buf, sizeof(buf)));
	}
	retval = rpc.init("127.0.0.1", port);
	if (retval) {
		fprintf(stderr, "%s can't connect to localhost",boinc_msg_prefix(buf, sizeof(buf)));
		return -1;
	}
	retval = rpc.get_results(results,true);
	return retval;
}

int reception_result()
{
	RPC_CLIENT rpc;
	int retval;
	if ((dernier_chargement>0) && (time(NULL)<dernier_chargement+31))
	{
		boinc_sleep(31-time(NULL)+dernier_chargement);
		fprintf(stderr, "%s Mise en veille (derniere requete date de moins de 30 secondes\n",boinc_msg_prefix(buf, sizeof(buf)));
	}
	retval = rpc.init("127.0.0.1", port);
	if (retval) {
		fprintf(stderr, "%s can't connect to localhost\n",boinc_msg_prefix(buf, sizeof(buf)));
		return -1;
	}
	retval = rpc.get_results(results_all,false);
	return retval;
}

int reception_state()
{
	RPC_CLIENT rpc;
	int retval;
	if ((dernier_chargement>0) && (time(NULL)<dernier_chargement+31))
	{
		boinc_sleep(31-time(NULL)+dernier_chargement);
		fprintf(stderr, "%s Mise en veille (derniere requete date de moins de 30 secondes\n",boinc_msg_prefix(buf, sizeof(buf)));
	}
	retval = rpc.init("127.0.0.1", port);
	if (retval) {
		fprintf(stderr, "%s can't connect to localhost",boinc_msg_prefix(buf, sizeof(buf)));
		return -1;
	}
	retval = rpc.get_state(gstate);
	return retval;
}

bool chargement_client_state()
{
	int retval;
	if (chargement_state==false)
	{
		pugi::xml_parse_result xml_parse_result = doc_state.load_file((path_state+"/client_state.xml").c_str());
		if (xml_parse_result)
		{
			chargement_state=true;
			format_state="file";
			return true;
		}
		else
		{
			fprintf(stderr, "%s Erreur chargement client_state.xml (%s)\n",boinc_msg_prefix(buf, sizeof(buf)),xml_parse_result.description());
			retval=reception_state();
			if (retval!=0)
			{	
				fprintf(stderr, "%s Erreur reception state\n",boinc_msg_prefix(buf, sizeof(buf)));
				dernier_chargement=time(NULL);
				return false;	
			}
			else
			{
				chargement_state=true;
				format_state="rpc";
				dernier_chargement=0;
				return true;
			}
		}
	}
	else return true;
}

void recherche_nom_projet()
{
	if (format_state=="file")
	{
		project_name=doc_state.select_single_node(("/client_state/project[master_url='"+project_url+"']").c_str()).node().child_value("project_name");
	}
	else
	{
		for (int i=0; i<gstate.projects.size(); i++) 
		{
			if (!strcmp(gstate.projects[i]->master_url, project_url.c_str()))
			{
				project_name=gstate.projects[i]->project_name;
				cout << project_name << endl;
				break;
			}
		}
	}
}

void recherche_hostid_projet()
{
	if (format_state=="file")
	{
		hostid_projet=doc_state.select_single_node(("/client_state/project[master_url='"+project_url+"']").c_str()).node().child_value("hostid");
	}
	else
	{
		int hostidprojet;
		for (int i=0; i<gstate.projects.size(); i++) 
		{
			if (!strcmp(gstate.projects[i]->master_url, project_url.c_str()))
			{
				hostidprojet=gstate.projects[i]->hostid;
				strs.str("");
				strs << hostidprojet;
				hostid_projet=strs.str();
				cout << hostid_projet << endl;
				break;
			}
		}
	}
}

void recherche_nom_application()
{
	if (format_state=="file")
	{
		app_name=doc_state.select_single_node(("/client_state/workunit[name='"+wu_name+"']").c_str()).node().child_value("app_name");
	}
	else
	{
		for (int i=0; i<gstate.wus.size(); i++) 
		{
			if (!strcmp(gstate.wus[i]->name, wu_name.c_str()))
			{
				app_name=gstate.wus[i]->app_name;
				cout << app_name << endl;
				break;
			}
		}
	}
}

void recherche_nom_courant_application()
{
	if (format_state=="file")
	{
		user_friendly_name=doc_state.select_single_node(("/client_state/app[name='"+app_name+"']").c_str()).node().child_value("user_friendly_name");
	}
	else
	{
		for (int i=0; i<gstate.apps.size(); i++) 
		{
			if (!strcmp(gstate.apps[i]->name, app_name.c_str()))
			{
				user_friendly_name=gstate.apps[i]->user_friendly_name;
				cout << user_friendly_name << endl;
				break;
			}
		}
	}
}

void recherche_hostid(int * hostid)
{
	* hostid=aid.hostid;
}

void recherche_plateforme()
{
	string path=dict_dir[name];
	int retval;
	char param[200+1];
	
	if (format_state=="file")
	{
		pugi::xml_node app_version;
		app_version = doc_state.select_single_node(("/client_state/app_version[app_name='"+app_name+"' and version_num='"+str_version_num+"']").c_str()).node();
		if (dict_dl_app.find(app_name)==dict_dl_app.end())
		{
			taille=0;
			for (pugi::xml_node file_ref = app_version.child("file_ref"); file_ref; file_ref = file_ref.next_sibling("file_ref"))
			{
				file_name=string(file_ref.child_value("file_name"));
				retval=file_size((path+"/"+file_name).c_str(),tmp_taille);
				if (retval==0) taille += tmp_taille;
			}
			if (taille!=0)
			{
				memset(param,0,200);
				string str_taille;
				sprintf(param,"%f",taille);
				str_taille=param;
				dict_dl_app[app_name]=project_name+";"+dict_app[app_name]+";"+str_taille;
			}
			else
			{
				fprintf(stderr,":%s Erreur assignation taille application\n",boinc_msg_prefix(buf, sizeof(buf)));
			}
		}
		platform = app_version.child_value("platform");
	}
	else
	{
		for (int i=0; i<gstate.app_versions.size(); i++) 
		{
			if ((!strcmp(gstate.app_versions[i]->app_name, app_name.c_str())) && (gstate.app_versions[i]->version_num==version_num))
			{
				platform=gstate.app_versions[i]->platform;
				cout << platform << endl;
				break;
			}
		}
	}
}

void recherche_new_wu()
{
	int retval;
	
	chargement_state=false;
	retval=reception_result();
	if (retval!=0)
	{
		fprintf(stderr,"%s Erreur reception result (new WU)\n",boinc_msg_prefix(buf, sizeof(buf)));
		dernier_chargement=time(NULL);
	}
	else
	{
		dernier_chargement=0;
		if (chargement_client_state()==true)
		{
			for (int i=0;i<results_all.results.size();i++)
			{
				received_time=results_all.results[i]->received_time;
				name=results_all.results[i]->name;
				wu_name=results_all.results[i]->wu_name;
				if (received_time> last_new_wu)
				{
					if (dict_delai.find(name)==dict_delai.end())
					{
						project_url=results_all.results[i]->project_url;
						if (dict_project.find(project_url)==dict_project.end())
						{
							recherche_nom_projet();
							if (project_name=="")
							{
								fprintf(stderr,"%s Erreur assignation project_name (recherche new_wu)\n",boinc_msg_prefix(buf, sizeof(buf)));
								continue;
							}
							dict_project[project_url]=project_name;
						}
						else
						{
							project_name=dict_project[project_url];
						}
						recherche_nom_application();
						if (app_name=="")
						{
							fprintf(stderr,"%s Erreur assignation app_name (recherche new_wu)\n",boinc_msg_prefix(buf, sizeof(buf)));
							continue;
						}
						if (dict_app.find(app_name)==dict_app.end())
						{
							recherche_nom_courant_application();
							if (user_friendly_name=="")
							{
								fprintf(stderr,"%s Erreur assignation user_friendly_name (recherche new_wu)\n",boinc_msg_prefix(buf, sizeof(buf)));
								continue;
							}
							dict_app[app_name]=user_friendly_name;
						}
						else
						{
							user_friendly_name=dict_app[app_name];
						}
						if (dict_new_wu.find(project_name+";"+user_friendly_name)==dict_new_wu.end())
						{
							dict_new_wu[project_name+";"+user_friendly_name]=1;
						}
						else
						{
							dict_new_wu[project_name+";"+user_friendly_name]=dict_new_wu[project_name+";"+user_friendly_name]+1;
						}
					}
				}
			}
		}
	}
	last_new_wu=time(NULL);
	for ( it_int=dict_new_wu.begin() ; it_int !=dict_new_wu.end(); it_int++)
	{
		out.printf( "%d;New_wu;%s;%d\n", last_new_wu, (*it_int).first.c_str(), (*it_int).second);
	}
	dict_new_wu.clear(); 
}

int wu_terminee(string result_name)
{
	int retval;
	if (chargement_state==false)
	{
		pugi::xml_parse_result xml_parse_result = doc_state.load_file((path_state+"/client_state.xml").c_str());
		if (xml_parse_result)
		{
			chargement_state=true;
			format_state="file";
		}
		else
		{
			fprintf(stderr, "%s Erreur chargement client_state.xml (%s)\n",boinc_msg_prefix(buf, sizeof(buf)),xml_parse_result.description());
			retval=reception_result();
			if (retval!=0)
			{
				fprintf(stderr,"%s Erreur reception state\n",boinc_msg_prefix(buf, sizeof(buf)));
				dernier_chargement=time(NULL);
			}
			else
			{
				chargement_state=true;
				format_state="rpc";
				dernier_chargement=0;
			}
		}
	}
	if (chargement_state==true)
	{
		if (format_state=="file")
		{
			pugi::xml_node node_result=doc_state.select_single_node(("/client_state/result[name='"+result_name+"']").c_str()).node();
			if (!node_result.empty())
			{
				if ((string)node_result.child_value("state")=="5")
				{
						str_final_cpu_time=(string)node_result.child_value("final_cpu_time");
						plan_class=(string)node_result.child_value("plan_class");
						if (dict_duree.find(result_name)==dict_duree.end())
						{
							if ((plan_class!="") && (nb_gpu==1))
							{
								str_final_elapsed_time=(string)node_result.child_value("final_elapsed_time");
								dict_duree[result_name]=dict_result[result_name]+";"+str_final_cpu_time.c_str()+";"+gpu+";"+str_final_elapsed_time.c_str();
							}
							else
							{
								dict_duree[result_name]=dict_result[result_name]+";"+str_final_cpu_time.c_str();
							}											
						}
						if (dict_tmp_upload.find(result_name)!=dict_tmp_upload.end())
						{
							dict_upload[result_name]=dict_result[result_name]+";"+dict_tmp_upload[result_name]+";"+str_final_cpu_time.c_str();
							dict_tmp_upload.erase(result_name);
						}
						if (dict_download.find(result_name)!=dict_download.end())
						{
							if (dict_download[result_name]!="inconnu")
							{
								if (dict_download[result_name].substr(dict_download[result_name].length()-1,1)==";")
								{	
									dict_download[result_name]=dict_download[result_name]+plan_class.c_str()+";"+str_final_cpu_time.c_str();
								}
							}
						}
				}
				else
				{
					fprintf(stderr, "%s Erreur wu_terminee (wu en cours d'upload) %s\n",boinc_msg_prefix(buf, sizeof(buf)),node_result.child_value("state"));
					return -1;
				}
			}
			else
			{
				fprintf(stderr, "%s Erreur wu_terminee (wu deja reportee)\n",boinc_msg_prefix(buf, sizeof(buf)));
				return 0;
			}
		}		
		else
		{		
			int indice=-1;			
			for (int i=0;i<results_all.results.size();i++)
			{
				if (strcmp(results_all.results[i]->name,result_name.c_str())==0)
				{
					indice=i;
					break;
				}
			}			
			if (indice>0)
			{
				if (results_all.results[indice]->ready_to_report==true)
				{
					if (results_all.results[indice]->state==5)
					{
						project_url=results_all.results[indice]->project_url;
						final_cpu_time=results_all.results[indice]->final_cpu_time;
						strs.str("");
						strs << (int)final_cpu_time;
						str_final_cpu_time=strs.str();
						plan_class=results_all.results[indice]->plan_class;
						if (dict_duree.find(result_name)==dict_duree.end())
						{
							if ((plan_class!="") && (nb_gpu==1))
							{
								final_elapsed_time=results_all.results[indice]->final_elapsed_time;
								strs.str("");
								strs << (int)final_elapsed_time;
								str_final_elapsed_time=strs.str();
								dict_duree[result_name]=dict_result[result_name]+";"+str_final_cpu_time+";"+gpu+";"+str_final_elapsed_time;
							}
							else
							{
								dict_duree[result_name]=dict_result[result_name]+";"+str_final_cpu_time;
							}											
						}
						if (dict_tmp_upload.find(result_name)!=dict_tmp_upload.end())
						{
							dict_upload[result_name]=dict_result[result_name]+";"+dict_tmp_upload[result_name]+";"+str_final_cpu_time;
							dict_tmp_upload.erase(result_name);
						}
						if (dict_download.find(result_name)!=dict_download.end())
						{
							if (dict_download[result_name]!="inconnu")
							{
								if (dict_download[result_name].substr(dict_download[result_name].length()-1,1)==";")
								{	
									dict_download[result_name]=dict_download[result_name]+plan_class+";"+str_final_cpu_time;
								}
							}
						}
					}
					else
					{
						fprintf(stderr, "%s Erreur wu_terminee (wu en erreur)\n",boinc_msg_prefix(buf, sizeof(buf)));
						return 0;
					}
				}
				else
				{
					fprintf(stderr, "%s Erreur wu_terminee (wu en cours d'upload)\n",boinc_msg_prefix(buf, sizeof(buf)));
					return -1;
				}
			}
			else
			{
				fprintf(stderr, "%s Erreur wu_terminee (wu deja reportee)\n",boinc_msg_prefix(buf, sizeof(buf)));
				return 0;
			}
		}
	}
	else
	{
		fprintf(stderr, "%s Erreur wu_terminee (chargement state)\n",boinc_msg_prefix(buf, sizeof(buf)));
		return -1;
	}
	return 0;
	
}

void path_boinc(string &path)
{
	char path_boinc[256];
	string boinc_path;
	size_t found;
	
	boinc_getcwd(path_boinc);
	strs.str("");
	strs << path_boinc;
	boinc_path=strs.str();
	found=boinc_path.find_first_of("\\");
	while (found!=string::npos)
	{
		boinc_path[found]='/';
		found=boinc_path.find_first_of("\\");
	}
	found=boinc_path.find_last_of("/");
	boinc_path=boinc_path.substr(0,found);
	found=boinc_path.find_last_of("/");
	boinc_path=boinc_path.substr(0,found);
	path=boinc_path;
	
}

void path_projet(string &project_url, string &path)
{
	string project_path;
	string boinc_path;
	size_t found;
	project_path=project_url.substr(7,project_url.length()-8);
	found=project_path.find_first_of("/");
	while (found!=string::npos)
	{
		project_path[found]='_';
		found=project_path.find_first_of("/",found+1);
	}
	
	boinc_path=path_state + "/projects/" + project_path;
	path=boinc_path;
}

void check_file()
{
	liste.clear();
	for ( it_double=dict_file.begin() ; it_double !=dict_file.end(); it_double++)
	{
		if (!boinc_file_exists((*it_double).first.c_str())) liste.push_back((*it_double).first);
	}
	for (it_liste=liste.begin();it_liste!=liste.end();it_liste++)
	{
		dict_file.erase(*it_liste);
	}
}

#ifdef ANDROID
#define BUILD_PROP_MAX_SIZE (4096)
int read_build_prop()
{
	FILE* f = fopen("/system/build.prop","rb");
	if (f==NULL)
	{
		product_manufacturer="";
		product_model="";
		product_name="";
		return -1;
	}

	char* buf = new char[BUILD_PROP_MAX_SIZE];

	bool model_is_set = false;
	bool name_is_set = false;
	bool manufacturer_is_set = false;
	while(fgets(buf,BUILD_PROP_MAX_SIZE,f)!=NULL)
	{
		if (*buf == '#' || *buf == 0) // comment
		continue;
		char* bufp = buf;
		while(isspace(*bufp)) bufp++;
		if (*bufp==0) // empty line
		continue;

		char* propname = bufp;
		char* endp = bufp;
		while(*endp != '=' && *endp != 0 && *endp != '\r' && *endp != '\n') endp++;

		if (*endp != '=')
		continue; // broken line

		*endp=0; // terminate propname
		endp++;
		char* value = endp;
		while(*endp != '\r' && *endp != '\n' && *endp != 0) endp++;

		//terminate value
		if (*endp == '\r' || *endp == '\n') *endp = 0;

		if (!strcmp(propname,"ro.product.model") && !model_is_set)
		{
			product_model = value;
			model_is_set = true;
		}
		if (!strcmp(propname,"ro.product.name") && !name_is_set)
		{
			product_name = value;
			name_is_set = true;
		}
		if (!strcmp(propname,"ro.product.manufacturer") && !manufacturer_is_set)
		{
			product_manufacturer = value;
			manufacturer_is_set = true;
		}
	}

	fclose(f);
	delete[] buf;
	return 0;
}
#endif

int main(int argc, char** argv)
{
	int retval;
	int cycle;
	int nb_cycle_max;
	double granted_credit;
	char output_path[512];
	char chkpt_path[512];
	char param[200+1];
	int socket_host_info=0;
	int socket_cc_status=0;
	string p_model;
	int p_ncpus;
	int theorical_p_ncpus;
	double report_deadline;
	int wu_state;
	int slot;
	string occupation;
	double working_set_size;
	double checkpoint_cpu_time;
	double current_cpu_time;
	string reponse_host_info;
	string reponse_cc_status;
	double estimated_cpu_time_remaining;
	double elapsed_time;
	string network_suspend_reason;
	string str_report_deadline;	
	string str_working_set_size;
	string str_checkpoint_cpu_time;
	string str_current_cpu_time;
	string str_elapsed_time;
	string str_slot;
	
	int hostid;
	double size_dir=0.0;
	char path_rel[512];
	char path_abs[512];
	char * pointeur_slash;
	#if defined _WIN32
	WSADATA WSADATA;
	WSAStartup(MAKEWORD(2,0),&WSADATA);
	#endif
	bool test=false;
	cycle=60;
	nb_cycle_max=180;
	granted_credit=7.0;
	for (int i=0; i<argc; i++) 
	{
		if (!strcmp(argv[i], "-p"))
		{
			port = atoi(argv[++i]);
		}
		if (!strcmp(argv[i], "-c"))
		{
			cycle = atoi(argv[++i]);
		}
		if (!strcmp(argv[i], "-n"))
		{
			nb_cycle_max = atoi(argv[++i]);
		}
		if (!strcmp(argv[i], "-g"))
		{
			granted_credit = atof(argv[++i]);
		}
		if (!strcmp(argv[i], "-t"))
		{
			path_state = argv[++i];
			test=true;
		}
	}
	
	//initialisation de BOINC
	retval = boinc_init();
	if (retval) 
	{
		fprintf(stderr, "%s boinc_init returned %d\n",boinc_msg_prefix(buf, sizeof(buf)), retval);
		exit(retval);
	}
	if (test==false)
	{
		boinc_get_init_data(aid);
		path_boinc(path_state);
	}
	
	boinc_resolve_filename(OUTPUT_FILENAME, output_path, sizeof(output_path));
	boinc_resolve_filename(CHECKPOINT_FILE, chkpt_path, sizeof(chkpt_path));
	if (test==false)	sprintf(cache_path, "%s/cache", aid.project_dir); else sprintf(cache_path, "cache");
	state = boinc_fopen(chkpt_path, "r");
	
	if (state) 
	{
		retval = out.open(output_path, "ab");
		restore_checkpoint();
		fclose(state);
	} 
	else 
	{
		retval = out.open(output_path, "wb");
	}
	if (retval) 
	{
		fprintf(stderr, "%s output open failed:\n",boinc_msg_prefix(buf, sizeof(buf)));
		fprintf(stderr, "%s resolved name %s, retval %s\n",boinc_msg_prefix(buf, sizeof(buf)), output_path, strerror(retval));
		perror("open");
		exit(1);
	}
	state=boinc_fopen(cache_path, "r");
	if (state)
	{
		restore_cache();
		fclose(state);
	}
	if (last_new_wu==0) last_new_wu=start_compute;
	
	out.printf("%s method 3;%d\n",boinc_msg_prefix(buf, sizeof(buf)),frequence());
	out.printf("Longueur_wu;%d\n",cycle*nb_cycle_max);
	out.printf("Credit_cycle;%f;%d\n",granted_credit,nb_cycle_max);
	p_model.assign("");	
	if (nb_cycle==0)
	{
		list_active_project.clear();
		dict_dl_app.clear();
		dict_project.clear();
		dict_app.clear();
		dict_platform.clear();
		dict_new_wu.clear();
	}
	if (nb_cycle>=nb_cycle_max)
	{
		retval=reception_cc_status();
		if (retval!=0)
		{
			fprintf(stderr,"%s Erreur reception cc_status\n",boinc_msg_prefix(buf, sizeof(buf)));
			dernier_chargement=time(NULL);
		}
		else
		{
			dernier_chargement=0;			
			if (cs.network_suspend_reason!=0)
			{
				fprintf(stderr,"%s Activite reseau suspendue\n",boinc_msg_prefix(buf, sizeof(buf)));
				nb_cycle_max=nb_cycle+1;
			}
		}
	}
	while(nb_cycle<nb_cycle_max)
	{	
		end=time(NULL);
		if (end-start<cycle)
		{
			boinc_sleep(cycle-(end-start));
		}
		else
		{
			start=time(NULL);
			doc_state.reset();
			chargement_state=false;
			nb_cycle++;
			out.printf("%s cycle%d\n",boinc_msg_prefix(buf, sizeof(buf)),nb_cycle);
			out.flush();
			if (p_model=="" || hostid==0)
			{
				if (test==false)
				{
					recherche_hostid(&hostid);
					p_model=aid.host_info.p_model;
					p_ncpus=aid.host_info.p_ncpus;
          theorical_p_ncpus = ncpu_theorical();
				}
				else
				{
					hostid=1;
					p_model="cpu";
					p_ncpus=4;
					theorical_p_ncpus=4;
				}
				retval=reception_host_info();
				if (retval!=0)
				{
					fprintf(stderr,"%s Erreur reception host_info\n",boinc_msg_prefix(buf, sizeof(buf)));
					dernier_chargement=time(NULL);
				}
				else
				{
					dernier_chargement=0;
					nb_gpu=hi.coprocs.ati.count + hi.coprocs.cuda.count;
					if (nb_gpu==1)
					{
						if (hi.have_ati()==true)
						{
							gpu=hi.coprocs.ati.name;
							out.printf("gpu_model;%s;%d;%d;%d;%d\n",gpu.c_str(),hi.coprocs.ati.attribs.numberOfSIMD,hi.coprocs.ati.attribs.wavefrontSize,hi.coprocs.ati.attribs.engineClock,hi.coprocs.ati.attribs.target);	
							
						}
						else
						{
							gpu=hi.coprocs.cuda.prop.name;
							out.printf("gpu_model;%s;;;;\n",gpu.c_str());	
						}	
					}
					else
					{
						fprintf(stderr,"%s Nombre de GPU non conforme: %d\n",boinc_msg_prefix(buf, sizeof(buf)),nb_gpu);
					}
				}
				if (p_model!="" && hostid!=0)
				{
					out.printf("hostid;%d\n",hostid);
					out.printf("p_model;%s;%d;%d\n",p_model.c_str(),p_ncpus, theorical_p_ncpus);					
				}
				else
				{
					start=time(NULL);
					continue;
				}
			}	//p_model
			#ifdef ANDROID
			if (product_model=="" || product_name == "" || product_manufacturer == "")
			{
				retval=read_build_prop();
				if (retval!=0)
				{
					fprintf(stderr,"%s Erreur reception build.prop\n",boinc_msg_prefix(buf, sizeof(buf)));
					dernier_chargement=time(NULL);
				}
				else
				{
					dernier_chargement=0;
					if (product_model!="" && product_name!="" && product_manufacturer!="")
					{
						out.printf("product_model;%s\n",product_model.c_str());
						out.printf("product_name;%s\n",product_name.c_str());
						out.printf("product_manufacturer;%s\n",product_manufacturer.c_str());
					}
				}
			}
			#endif
			out.flush();
			chargement_state=false;
			retval=reception_active_result();
			if (retval!=0)
			{
				fprintf(stderr,"%s Erreur reception active_result\n",boinc_msg_prefix(buf, sizeof(buf)));
				dernier_chargement=time(NULL);
			}
			else
			{
				dernier_chargement=0;
				for (int i=0;i<results.results.size();i++)
				{
					name=results.results[i]->name;
					project_url=results.results[i]->project_url;
					if (dict_result.find(name)==dict_result.end())
					{ //resultat non recence
						if (chargement_client_state()==true)
						{
							if (dict_project.find(project_url)==dict_project.end())
							{ //projet non recence
								recherche_nom_projet();
								if (project_name=="") 
								{
									fprintf(stderr,"%s Erreur assignation project_name (node result) - project_url: %s\n",boinc_msg_prefix(buf, sizeof(buf)), project_url.c_str());
									continue;
								}
								dict_project[project_url]=project_name;
							}
							else project_name=dict_project[project_url];
							wu_name=results.results[i]->wu_name;
							recherche_nom_application();
							if (dict_app.find(app_name)==dict_app.end())
							{
								recherche_nom_courant_application();
								if (user_friendly_name=="")
								{
									fprintf(stderr,"%s Erreur assignation user_friendly_name (node app) - app_name: %s\n",boinc_msg_prefix(buf, sizeof(buf)), app_name.c_str());
									continue;
								}
								dict_app[app_name]=user_friendly_name;
							}
							else user_friendly_name=dict_app[app_name];
							string path;
							path_projet(project_url, path);
							dict_dir[name]=path;
							dict_nb_file_result[name]=0;
							if (format_state=="file")
							{
								pugi::xml_node workunit;
								workunit = doc_state.select_single_node(("/client_state/workunit[name='"+wu_name+"']").c_str()).node();
								taille=0.0;
								bool erreur_taille=false;
								for (pugi::xml_node file_ref = workunit.child("file_ref"); file_ref; file_ref = file_ref.next_sibling("file_ref"))
								{
									file_name=path+"/"+string(file_ref.child_value("file_name"));
									if (dict_file.find(file_name)==dict_file.end())
									{
										retval=file_size(file_name.c_str(),tmp_taille);
										if (retval==0)
										{
											taille += tmp_taille;
											dict_file[file_name]=tmp_taille;
										}
										else
										{
											erreur_taille=true;
											break;
										}
									}
								}
								if (erreur_taille==false)
								{
									memset(param,0,200);
									string str_taille;
									sprintf(param,"%f",taille);
									str_taille=param;
									dict_download[name]=project_name+";"+user_friendly_name+";"+str_taille+";";
								}
								else
								{
									fprintf(stderr,"Erreur assignation taille wu (%s)\n",boinc_msg_prefix(buf, sizeof(buf)));
								}
							}
							version_num=results.results[i]->version_num;
							strs.str("");
							strs << (int)version_num;
							str_version_num=strs.str();
							if (dict_platform.find(app_name+"_"+str_version_num)==dict_platform.end())
							{
								recherche_plateforme();
								if (platform=="")
								{
									fprintf(stderr,"%s Erreur assignation platform (node app_version) - app_name: %s - version_num: %s\n",boinc_msg_prefix(buf, sizeof(buf)), app_name.c_str(), str_version_num.c_str());
									continue;
								}
								dict_platform[app_name+"_"+str_version_num]=platform;
							}
							else platform=dict_platform[app_name+"_"+str_version_num];
							plan_class=results.results[i]->plan_class;
							if (dict_delai.find(name)==dict_delai.end())
							{
								report_deadline=results.results[i]->report_deadline;
								strs.str("");
								strs << (int)report_deadline;
								str_report_deadline=strs.str();
								received_time=results.results[i]->received_time;
								strs.str("");
								strs << (int)received_time;
								str_received_time=strs.str();
								if (received_time>last_new_wu)
								{
									if (dict_new_wu.find(project_name+";"+user_friendly_name)==dict_new_wu.end())
									{
										dict_new_wu[project_name+";"+user_friendly_name]=1;
									}
									else
									{
										dict_new_wu[project_name+";"+user_friendly_name]=dict_new_wu[project_name+";"+user_friendly_name]+1;
									}
								}
								dict_delai[name]=str_received_time+";"+str_report_deadline;
								out.printf("%d;delai;%s;%s;;%s;%s\n", start, project_name.c_str(),dict_app[app_name].c_str(),str_received_time.c_str(),str_report_deadline.c_str());
							}
							dict_result[name]=p_model+";"+project_name+";"+dict_app[app_name]+";"+dict_platform[app_name+"_"+str_version_num]+";"+plan_class;
              dict_run[name+";"+dict_result[name]]=0;
						}
						else
						{
							break;
						}
					}
					else
					{
						if (chargement_client_state()==true)
						{
							if (dict_project.find(project_url)==dict_project.end())
							{ //projet non recence
								recherche_nom_projet();
								if (project_name=="") 
								{
									fprintf(stderr,"%s Erreur assignation project_name (node result)\n",boinc_msg_prefix(buf, sizeof(buf)));
									continue;
								}
								dict_project[project_url]=project_name;
							}
							else project_name=dict_project[project_url];
						}
						else
						{
							break;
						}
					}
					wu_state=results.results[i]->state;
					if (wu_state==2)
					{
						if ((!results.results[i]->suspended_via_gui) && (!results.results[i]->project_suspended_via_gui))
						{
							if (results.results[i]->active_task)
							{
								slot=results.results[i]->slot;
								strs.str("");
								strs << slot;
								str_slot=strs.str();
								if (results.results[i]->active_task_state==1)
								{
									if (find(list_active_project.begin(),list_active_project.end(),project_name)==list_active_project.end())
									{
										if (chargement_client_state()==true)
										{
											recherche_hostid_projet();
											out.printf("%d;active_project;%s;%s;%d;%d;%s\n", start, project_name.c_str(), p_model.c_str(), p_ncpus, frequence(), hostid_projet.c_str());
											out.printf("projet;%s;%s\n", project_url.c_str(), project_name.c_str());
											list_active_project.push_back(project_name);
										}
										else
										{
											break;
										}
									}
									dict_run[name+";"+dict_result[name]]++;
									if (dict_ecart_releve.find(name)==dict_ecart_releve.end())
									{
										elapsed_time=results.results[i]->elapsed_time;
										estimated_cpu_time_remaining=results.results[i]->estimated_cpu_time_remaining;
										double ecart_releve=(elapsed_time+estimated_cpu_time_remaining) / 1000;
										dict_ecart_releve[name]=ecart_releve;
										dict_prochain_releve[name]=0;
									}
									if (dict_prochain_releve[name]<end)
									{
										dict_prochain_releve[name]=end+dict_ecart_releve[name];
										memset(path_abs,0,512);
										boinc_getcwd(path_rel);
										for (unsigned int chr=0; chr<strlen(path_rel); chr++)
										{
											if (path_rel[chr]=='\\')
											{
												path_rel[chr]='/';
											}
										}
										pointeur_slash=strrchr(path_rel,'/');
										if (pointeur_slash)
										{
											strncpy(path_abs, path_rel,strlen(path_rel)-strlen(pointeur_slash));
											strcat(path_abs,("/"+str_slot).c_str());
											dir_size(path_abs,size_dir,true);
											strs.str("");
											strs << size_dir/1024;
											occupation=strs.str();
											if (dict_occupation.find(name)==dict_occupation.end())
											{
												dict_occupation[name]=dict_result[name]+";"+occupation;
											}
											else
											{
												dict_occupation[name]=dict_occupation[name]+";"+occupation;
											}
										}
										working_set_size=results.results[i]->working_set_size_smoothed;
										strs.str("");
										strs << (int)working_set_size;
										str_working_set_size=strs.str();
										if (dict_conso.find(name)==dict_conso.end())
										{
											dict_conso[name]=dict_result[name]+";"+str_working_set_size;
										}
										else
										{
											dict_conso[name]=dict_conso[name]+";"+str_working_set_size;
										}
										checkpoint_cpu_time=results.results[i]->checkpoint_cpu_time;
										strs.str("");
										strs << (int)checkpoint_cpu_time;
										str_checkpoint_cpu_time=strs.str();
										current_cpu_time=results.results[i]->current_cpu_time;
										strs.str("");
										strs << (int)current_cpu_time;
										str_current_cpu_time=strs.str();
										if (dict_check.find(name)==dict_check.end())
										{
											if (p_model!="")
											{
												dict_check[name]=dict_result[name]+";"+str_current_cpu_time+"|"+str_checkpoint_cpu_time;
											}
										}
										else
										{
											dict_check[name]=dict_check[name]+";"+str_current_cpu_time+"|"+str_checkpoint_cpu_time;
										}
									}
								}
							}
						}
					}
				}				
				liste.clear();
				chargement_state=false;
				for (it=dict_result.begin(); it!=dict_result.end(); it++)
				{
					bool termine;
					for (int i=0;i<results.results.size();i++)
					{
						termine=true;	
						if (strcmp(results.results[i]->name,(*it).first.c_str())==0)
						{
							termine=false;
							break;
						}
					}
					if (termine)
					{
						retval=wu_terminee((*it).first);
            if (retval==0)
						{
							name=(*it).first;
							wu_name=(*it).first.substr(0,(*it).first.rfind("_"));
							if (dict_upload.find(name)!=dict_upload.end())
							{
								out.printf("%d;upload;%s\n", start, dict_upload[name].c_str());
								dict_upload.erase(name);									
							}
							if (dict_download.find(name)!=dict_download.end())
							{
								if (dict_download[name]!="inconnu")
								{
									out.printf("%d;download;%s\n", start, dict_download[name].c_str());
								}
								dict_download.erase(name);									
							} 
							if (dict_conso.find(name)!=dict_conso.end())
							{
								out.printf("%d;conso|%s;%s\n", start, wu_name.c_str(), dict_conso[name].c_str());
								dict_conso.erase(name);
							}
							if (dict_check.find(name)!=dict_check.end())
							{
								out.printf("%d;check|%s;%s\n", start, wu_name.c_str(), dict_check[name].c_str());
								dict_check.erase(name);
							}
							if (dict_occupation.find(name)!=dict_occupation.end())
							{
								out.printf("%d;occupation;%s\n", start, dict_occupation[name].c_str());
								dict_occupation.erase(name);
							}
							if (dict_duree.find(name)!=dict_duree.end())
							{
								out.printf("%d;duree|%s;%s;\n", start, wu_name.c_str(), dict_duree[name].c_str());
								dict_duree.erase(name);
							}
							if (dict_run.find(name+";"+dict_result[name])!=dict_run.end())
							{
								out.printf("%d;run;%s;%s;%d\n", start, name.c_str(), dict_result[name].c_str(), dict_run[name+";"+dict_result[name]]);
								dict_run.erase(name+";"+dict_result[name]);
							}
							dict_delai.erase(name);
							dict_dir.erase(name);
							dict_ecart_releve.erase(name);
							dict_prochain_releve.erase(name);
							for (int i=0; i<dict_nb_file_result[name]+1; i++)
							{
								string num;
								sprintf(param,"%d",i);
								num=param;
								dict_size_file_result.erase(name+"_"+num);
							}
							dict_nb_file_result.erase(name);
							liste.push_back(name);															
						}
						else if (retval==-2)
						{
							break;
						}
					}
				}
				for (it_liste=liste.begin();it_liste!=liste.end();it_liste++)
				{
					dict_result.erase(*it_liste);
				}
			} //active_result
			out.flush();
			boinc_fraction_done((float)nb_cycle/(nb_cycle_max+1));
			end=time(NULL);
			
			retval = do_checkpoint();
			if (retval) 
			{
				fprintf(stderr, "%s checkpoint failed %s\n",boinc_msg_prefix(buf, sizeof(buf)), strerror(retval));
				//boinc_finish(0);
			}
			else
			{
				boinc_checkpoint_completed();
			}
			end=time(NULL);
			while (end-start<cycle-1)
			{
				for (it=dict_result.begin(); it!=dict_result.end(); it++)
				{
					name=(*it).first;
					if (dict_dir.find(name)!=dict_dir.end())
					{
						taille=0.0;
						int nb_file=0;
						for (int i=0; i<dict_nb_file_result[name]+1; i++)
						{
							string num;
							sprintf(param,"%d",i);
							num=param;
							
							retval=file_size((dict_dir[name]+"/"+name+"_"+num).c_str(),tmp_taille);
							if (retval)
							{
								if (dict_size_file_result.find(name+"_"+num)!=dict_size_file_result.end())
								{
									taille += dict_size_file_result[name+"_"+num];
								}
							}
							else
							{
								taille += tmp_taille;
								dict_size_file_result[name+"_"+num]=tmp_taille;
								nb_file=i+1;
							}
						}
						if (nb_file>0)
						{
							memset(param,0,200);
							string str_taille;
							sprintf(param,"%f",taille);
							str_taille=param;
							dict_tmp_upload[name]=str_taille;
							if (nb_file>dict_nb_file_result[name]) dict_nb_file_result[name]=nb_file;
						}
						else
						{
							if (dict_nb_file_result[name]>0)
							{
								chargement_state=false;
								retval=wu_terminee(name);
								if (retval==0)
								{
									dict_dir.erase(name);
									for (int i=0; i<dict_nb_file_result[name]+1; i++)
									{
										string num;
										sprintf(param,"%d",i);
										num=param;
										dict_size_file_result.erase(name+"_"+num);
									}
									dict_nb_file_result.erase(name);
								}
								else if (retval==-2)
								{
									break;
								}
							}
						}
					}
				}
				end=time(NULL);
				boinc_sleep(1);
			}
		}
		if (nb_cycle>=nb_cycle_max)
		{
			retval=reception_cc_status();
			if (retval!=0)
			{
				fprintf(stderr,"%s Erreur reception cc_status\n",boinc_msg_prefix(buf, sizeof(buf)));
				dernier_chargement=time(NULL);
			}
			else
			{
				dernier_chargement=0;
				if (cs.network_suspend_reason!=0)
				{
					fprintf(stderr,"%s Activite reseau suspendue\n",boinc_msg_prefix(buf, sizeof(buf)));
					nb_cycle_max=nb_cycle+1;
				}
			}
		} 
	}
	recherche_new_wu();
	for ( it=dict_dl_app.begin() ; it !=dict_dl_app.end(); it++)
	{
		out.printf( "%d;Dl_app;%s;\n", start, (*it).second.c_str());
	}
	for ( it_long=dict_run.begin() ; it_long !=dict_run.end(); it_long++)
	{
		out.printf("%d;Pending run;%s;%d\n", start, (*it_long).first.c_str(), (*it_long).second);
	}  
	check_file();
	do_checkpoint();
	out.flush();
	boinc_finish(0);
	return 0;
}
