// This file is part of BOINC.
// http://boinc.berkeley.edu
// Copyright (C) 2008 University of California
//
// BOINC is free software; you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.
//
// BOINC is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with BOINC.  If not, see <http://www.gnu.org/licenses/>.

#ifdef _WIN32
#include "boinc_win.h"
#else
#include "config.h"
#include <cstring>
#include <errno.h>
#endif

#include "parse.h"
#include "str_util.h"
#include "error_numbers.h"

#include "file_names.h"
#include "filesys.h"
#include "client_msgs.h"
#include "client_state.h"

void CLIENT_STATE::init_update_project_apps(PROJECT* project) {
    project->suspended_during_update = true;
    project->dont_preempt_suspended = false;
    project->pending_to_exit = 0;
    
    /* initializes pending_to_exit counter */
    for (size_t i=0; i<active_tasks.active_tasks.size(); i++) {
        ACTIVE_TASK* atp = active_tasks.active_tasks[i];
        if (!atp->runnable()) continue;
        if (project != atp->result->project) continue;
        project->pending_to_exit++;
    }
    request_schedule_cpus("Init update project apps");
}

int CLIENT_STATE::do_update_project_apps() {
    char projpath[1024];
    char updatepath[1024];
    char filename[256];
    
    size_t updated_projects = 0;
    for (size_t i=0; i<projects.size();i++) {
        PROJECT* p = projects[i];
        
        get_project_and_update_dir(p, projpath, 1024, updatepath, 1024);
        int plen = strlen(projpath);
        int ulen = strlen(updatepath);
        if (!p->suspended_during_update || !p->dont_preempt_suspended || p->pending_to_exit!=0)
            continue;
        if (log_flags.update_apps_debug)
            msg_printf(p, MSG_INFO, "[update_apps] Do update_project_apps:%d:%d",
                p->suspended_during_update, p->pending_to_exit);
        /* update */
        DIRREF dir = dir_open(updatepath);
        // copy to project
        updatepath[ulen]='/';
        projpath[plen]='/';
        
        bool success = true;
        
        while (!dir_scan(filename, dir, 256)) {
            if (!strcmp(filename,APP_INFO_FILE_NAME)) continue;
            strlcpy(updatepath+ulen+1,filename,1024-ulen-1);
            strlcpy(projpath+plen+1,filename,1024-plen-1);
            // move to new file
            if (log_flags.update_apps_debug)
                msg_printf(p, MSG_INFO, "[update_apps] Do copying %s to %s",
                           updatepath, projpath);
            if (::rename(updatepath,projpath)) {
                msg_printf(p, MSG_INFO, "[update_apps] Cant copy %s to %s",
                           updatepath, projpath);
                success = false;
                break;
            }
        }
        
        if (!success) {
            p->suspended_during_update = false;
            updated_projects++;
            continue;
        }
        // update client state
        strlcpy(projpath+plen+1,APP_INFO_FILE_NAME, 1024-1-plen);
        
        vector<FILE_INFO*> ofis;
        vector<APP*> oapps;
        vector<APP_VERSION*> oavps;
        
        FILE* appinfo_file=fopen(projpath,"rb");
        if (appinfo_file!=NULL) {
            get_old_app_info(p, appinfo_file, ofis, oapps, oavps);
            fclose(appinfo_file);
        }
        size_t old_apps_count = oapps.size();
        
        /* update */
        strlcpy(updatepath+ulen+1,APP_INFO_FILE_NAME, 1024-1-ulen);
        appinfo_file=fopen(updatepath,"rb");
        if (appinfo_file!=NULL) {
            update_app_info(p, appinfo_file, ofis, oapps, oavps);
            fclose(appinfo_file);
        } else {
            p->suspended_during_update = false;
            updated_projects++;
            continue;
        }
        
        // update app_info.xml
        if (log_flags.update_apps_debug)
            msg_printf(p, MSG_INFO, "[update_apps] Do copying %s to %s",
                        updatepath, projpath);
        
        if (::rename(updatepath, projpath)) {
            msg_printf(p, MSG_INFO, "[update_apps] Cant copy %s to %s",
                           updatepath, projpath);
            p->suspended_during_update = false;
            updated_projects++;
            continue;
        }
        
        // update state
        p->suspended_during_update = false;
        
        check_no_apps(p);
        
        p->anonymous_platform = true;
        
        // fill in avp->flops for anonymous platform projects
        //
        for (i=0; i<app_versions.size(); i++) {
            APP_VERSION* avp = app_versions[i];
            if (!avp->flops) {
                if (!avp->avg_ncpus) {
                    avp->avg_ncpus = 1;
                }
                avp->flops = avp->avg_ncpus * host_info.p_fpops;

                // for GPU apps, use conservative estimate:
                // assume app will run at peak CPU speed, not peak GPU
                //
                if (avp->ncudas) {
                    avp->flops += avp->ncudas * host_info.p_fpops;
                }
                if (avp->natis) {
                    avp->flops += avp->natis * host_info.p_fpops;
                }
            }
        }
        
        // update if first update (install)
        if (old_apps_count==0) {
            if (log_flags.update_apps_debug)
                msg_printf(p, MSG_INFO, "[update_apps] Updating project");
            p->sched_rpc_pending = RPC_REASON_USER_REQ;
            p->min_rpc_time = 0;
#if 1
            rss_feeds.trigger_fetch(p);
#endif
            gstate.request_work_fetch("project updated by user");
        }
        
        updated_projects++;
    }
    if (updated_projects)
        request_schedule_cpus("Finish update project apps");
    return 0;
}

int CLIENT_STATE::get_old_app_info(PROJECT* p, FILE* in,
        vector<FILE_INFO*>& ofis, vector<APP*>& oapps, vector<APP_VERSION*>& oavps) {
    char buf[256];
    MIOFILE mf;
    mf.init_file(in);

    while (fgets(buf, 256, in)) {
        if (match_tag(buf, "<app_info>")) continue;
        if (match_tag(buf, "</app_info>")) return 0;
        if (match_tag(buf, "<file_info>")) {
            FILE_INFO fip;
            if (fip.parse(mf, false))
                continue;
            
            if (log_flags.update_apps_debug)
                msg_printf(p, MSG_INFO, "[update_apps] get old file info: %s", fip.name);
            
            vector<FILE_INFO*>::iterator iter = file_infos.begin();
            while(iter!=file_infos.end()) {
                FILE_INFO* oldfip = *iter;
                if (oldfip->project==p && !strcmp(oldfip->name, fip.name))
                    ofis.push_back(oldfip);
                ++iter;
            }
            continue;
        }
        if (match_tag(buf, "<app>")) {
            APP app;
            if (app.parse(mf))
                continue;
            
            if (log_flags.update_apps_debug)
                msg_printf(p, MSG_INFO, "[update_apps] get old app: %s", app.name);
            
            vector<APP*>::iterator iter = apps.begin();
            while(iter!=apps.end()) {
                APP* oldapp = *iter;
                if (oldapp->project==p && !strcmp(oldapp->name, app.name))
                    oapps.push_back(oldapp);
                ++iter;
            }
            continue;
        }
        if (match_tag(buf, "<app_version>")) {
            APP_VERSION avp;
            if (avp.parse(mf))
                continue;
            
            avp.project = p;
            APP* app = lookup_app(p, avp.app_name);
            avp.app = app;
            
            if (log_flags.update_apps_debug)
                msg_printf(p, MSG_INFO, "[update_apps] get old app_version: %s:%d:%s",
                           app->name, avp.version_num, avp.platform);
            
            vector<APP_VERSION*>::iterator iter = app_versions.begin();
            while(iter!=app_versions.end()) {
                APP_VERSION* oldavp = *iter;
                if (oldavp->app==avp.app && oldavp->version_num==avp.version_num &&
                    !strcmp(oldavp->platform, avp.platform) &&
                    !strcmp(oldavp->plan_class, avp.plan_class))
                    oavps.push_back(oldavp);
                ++iter;
            }
            continue;
        }
        if (log_flags.unparsed_xml) {
            msg_printf(p, MSG_INFO,
                "Unparsed line in app_info.xml: %s",
                buf
            );
        }
    }
    return ERR_XML_PARSE;
}

/*
 * updates app info (updates app_info.xml and client state)
 */
int CLIENT_STATE::update_app_info(PROJECT* p, FILE* in, vector<FILE_INFO*>& oldfis,
        vector<APP*>& oldapps, vector<APP_VERSION*>& oldavps) {
    char buf[256], path[1024];
    MIOFILE mf;
    mf.init_file(in);
    
    vector<FILE_INFO*> newfis;
    vector<APP*> newapps;
    vector<APP_VERSION*> newavps;
    
    while (fgets(buf, 256, in)) {
        if (match_tag(buf, "<app_info>")) continue;
        if (match_tag(buf, "</app_info>")) break;
        if (match_tag(buf, "<file_info>")) {
            FILE_INFO* fip = new FILE_INFO;
            if (fip->parse(mf, false)) {
                delete fip;
                continue;
            }
            if (fip->urls.size()) {
                msg_printf(p, MSG_INFO,
                    "Can't specify URLs in app_info.xml"
                );
                delete fip;
                continue;
            }
            fip->project = p;
            
            if (log_flags.update_apps_debug)
                msg_printf(p, MSG_INFO, "[update_apps] get new file info: %s",
                           fip->name);
            // check that the file is actually there
            //
            get_pathname(fip, path, sizeof(path));
            if (!boinc_file_exists(path)) {
                strcpy(buf,
                    _("File referenced in app_info.xml does not exist: ")
                );
                strcat(buf, path);
                msg_printf(p, MSG_USER_ALERT, buf);
                delete fip;
                continue;
            }
            fip->status = FILE_PRESENT;
            newfis.push_back(fip);
            continue;
        }
        if (match_tag(buf, "<app>")) {
            APP* app = new APP;
            if (app->parse(mf)) {
                delete app;
                continue;
            }
            app->project = p;
            
            if (log_flags.update_apps_debug)
                msg_printf(p, MSG_INFO, "[update_apps] get new app: %s", app->name);
            
            newapps.push_back(app);
            continue;
        }
        if (match_tag(buf, "<app_version>")) {
            APP_VERSION* avp = new APP_VERSION;
            if (avp->parse(mf)) {
                delete avp;
                continue;
            }
            if (strlen(avp->platform) == 0) {
                strcpy(avp->platform, get_primary_platform());
            }
            
            if (log_flags.update_apps_debug)
                msg_printf(p, MSG_INFO, "[update_apps] get new app_version: %s:%d:%s",
                           avp->app_name, avp->version_num, avp->platform);
            
            newavps.push_back(avp);
            continue;
        }
        if (log_flags.unparsed_xml) {
            msg_printf(p, MSG_INFO,
                "Unparsed line in app_info.xml: %s",
                buf
            );
            return ERR_XML_PARSE;
        }
    }
    fclose(in);
    
    /* delete not-updated file_infos */
    unsigned int i,j;
    for (i=0;i<oldfis.size(); i++) {
        bool to_delete = true;
        FILE_INFO* oldfi = oldfis[i];
        FILE_INFO* newfi = NULL;
        for (j=0;j<newfis.size(); j++) {
            newfi = newfis[j];
            if (newfi->project==oldfi->project && !strcmp(newfi->name, oldfi->name)) {
                to_delete = false;
                break;
            }
        }
        if (to_delete) { // to delete
            if (log_flags.update_apps_debug)
                msg_printf(p, MSG_INFO, "[update_apps] delete old file: %s",
                           oldfi->name);
            oldfi->delete_file();
        }
        else {
            if (log_flags.update_apps_debug)
                msg_printf(p, MSG_INFO, "[update_apps] update file info: %s",
                           newfi->name);
            
            memcpy(oldfi,newfi,sizeof(FILE_INFO));
        }
    }
    /* add new file_infos */
    for (i=0;i<newfis.size(); i++) {
        bool is_new = true;
        FILE_INFO* newfi = newfis[i];
        FILE_INFO* oldfi = NULL;
        for (j=0;j<oldfis.size(); j++) {
            oldfi = oldfis[j];
            if (newfi->project==oldfi->project && !strcmp(newfi->name, oldfi->name)) {
                is_new = false;
                break;
            }
        }
        if (is_new) {
            if (log_flags.update_apps_debug)
                msg_printf(p, MSG_INFO, "[update_apps] push new file info: %s",
                           newfi->name);
            file_infos.push_back(newfi);
        }
        else
            delete newfi;
    }
    
    /* updating (to update) apps */
    for (i=0;i<oldapps.size(); i++) {
        bool to_update = false;
        APP* oldapp = oldapps[i];
        APP* newapp = NULL;
        for (j=0;j<newapps.size(); j++) {
            newapp = newapps[j];
            if (newapp->project==oldapp->project && !strcmp(newapp->name, oldapp->name)) {
                to_update = true;
                break;
            }
        }
        if (to_update) {
            // merge info
            if (log_flags.update_apps_debug)
                msg_printf(p, MSG_INFO, "[update_apps] update app: %s", newapp->name);
            strcpy(oldapp->user_friendly_name, newapp->user_friendly_name);
            oldapp->non_cpu_intensive = newapp->non_cpu_intensive;
        }
    }
    /* add new file_infos */
    for (i=0;i<newapps.size(); i++) {
        bool is_new = true;
        APP* newapp = newapps[i];
        APP* oldapp = NULL;
        for (j=0;j<oldapps.size(); j++) {
            oldapp = oldapps[j];
            if (newapp->project==oldapp->project && !strcmp(newapp->name, oldapp->name)) {
                is_new = false;
                break;
            }
        }
        if (is_new) {
            if (log_flags.update_apps_debug)
                msg_printf(p, MSG_INFO, "[update_apps] push new app: %s", newapp->name);
            apps.push_back(newapp);
        }
        else
            delete newapp;
    }
    
    for (i=0;i<newavps.size(); i++) {
        APP_VERSION* newavp = newavps[i];
        apply_new_app_version(p, newavp);
    }
    
    if (log_flags.update_apps_debug) {
        msg_printf(p, MSG_INFO, "[update_apps] newavps.size=%d",newavps.size());
        msg_printf(p, MSG_INFO, "[update_apps] oldavps.size=%d",oldavps.size());
    }
    
    for (i=0;i<oldavps.size(); i++) {
        bool to_update = false;
        APP_VERSION* oldavp = oldavps[i];
        APP_VERSION* newavp = NULL;
        for (j=0;j<newavps.size(); j++) {
            newavp = newavps[j];
            if (newavp->app==oldavp->app &&
                    oldavp->version_num==newavp->version_num &&
                    !strcmp(oldavp->platform, newavp->platform) &&
                    !strcmp(oldavp->plan_class, newavp->plan_class)) {
                to_update = true;
                break;
            }
        }
        if (to_update) { // merge info
            if (log_flags.update_apps_debug)
                msg_printf(p, MSG_INFO, "[update_apps] update app_version: %s:%d:%s",
                           newavp->app->name, newavp->version_num, newavp->platform);
            strcpy(oldavp->app_name, newavp->app_name);
            strcpy(oldavp->api_version, newavp->api_version);
            oldavp->avg_ncpus = newavp->avg_ncpus;
            oldavp->max_ncpus = newavp->max_ncpus;
            oldavp->ncudas = newavp->ncudas;
            oldavp->natis = newavp->natis;
            oldavp->gpu_ram = newavp->gpu_ram;
            oldavp->flops = newavp->flops;
            strcpy(oldavp->cmdline, newavp->cmdline);
            oldavp->app_files = newavp->app_files;
            oldavp->max_working_set_size = newavp->max_working_set_size;
            strcpy(oldavp->graphics_exec_path, newavp->graphics_exec_path);
            strcpy(oldavp->graphics_exec_file, newavp->graphics_exec_file);
        }
    }
    for (i=0;i<newavps.size(); i++) {
        bool is_new = true;
        APP_VERSION* newavp = newavps[i];
        APP_VERSION* oldavp = NULL;
        for (j=0;j<oldavps.size(); j++) {
            oldavp = oldavps[j];
            if (newavp->app==oldavp->app &&
                oldavp->version_num==newavp->version_num &&
                    !strcmp(oldavp->platform, newavp->platform) &&
                    !strcmp(oldavp->plan_class, newavp->plan_class)) {
                is_new = false;
                break;
            }
        }
        if (is_new) {
            if (log_flags.update_apps_debug)
                msg_printf(p, MSG_INFO, "[update_apps] push new app_version: %s:%d:%s",
                           newavp->app->name, newavp->version_num, newavp->platform);
            app_versions.push_back(newavp);
        }
        else
            delete newavp;
    }
    
    return 0;
}

int CLIENT_STATE::apply_new_app_version(PROJECT* p, APP_VERSION* avp) {
    /* copy of link_app_version (without checking uniqueness) */
    APP* app;
    FILE_INFO* fip;
    unsigned int i;

    avp->project = p;
    app = lookup_app(p, avp->app_name);
    if (!app) {
        msg_printf(p, MSG_INTERNAL_ERROR,
            "State file error: bad application name %s",
            avp->app_name
        );
        return ERR_NOT_FOUND;
    }
    avp->app = app;

    strcpy(avp->graphics_exec_path, "");
    strcpy(avp->graphics_exec_file, "");

    for (i=0; i<avp->app_files.size(); i++) {
        FILE_REF& file_ref = avp->app_files[i];
        fip = lookup_file_info(p, file_ref.file_name);
        if (!fip) {
            msg_printf(p, MSG_INTERNAL_ERROR,
                "State file error: missing application file %s",
                file_ref.file_name
            );
            return ERR_NOT_FOUND;
        }

        if (!strcmp(file_ref.open_name, GRAPHICS_APP_FILENAME)) {
            char relpath[512], path[512];
            get_pathname(fip, relpath, sizeof(relpath));
            relative_to_absolute(relpath, path);
            strlcpy(avp->graphics_exec_path, path, sizeof(avp->graphics_exec_path));
            strcpy(avp->graphics_exec_file, fip->name);
        }

        // any file associated with an app version must be signed
        //
        if (!config.unsigned_apps_ok) {
            fip->signature_required = true;
        }

        file_ref.file_info = fip;
    }
    return 0;
}

void CLIENT_STATE::dont_preempt_suspended_in_projects() {
    for (size_t i=0; i<projects.size(); i++) {
        PROJECT* p = projects[i];
        if (p->suspended_during_update && !p->dont_preempt_suspended) {
            msg_printf(p, MSG_INFO, "[update_apps] dont preempt suspended");
            p->dont_preempt_suspended = true;
        }
    }
}