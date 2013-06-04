// This file is part of BOINC.
// http://boinc.berkeley.edu
// Author: Mateusz Szpakowski
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

#ifndef _MONITOR_H_
#define _MONITOR_H_

#include "network.h"
#include "miofile.h"

#define MONITOR_EVENT_ATTACH_PROJECT 1
#define MONITOR_EVENT_DETACH_PROJECT 2
#define MONITOR_EVENT_SUSPEND_ALL_TASKS 3
#define MONITOR_EVENT_RUN_TASKS 4
#define MONITOR_EVENT_RUN_BENCHMARK 5
#define MONITOR_EVENT_FINISH_BENCHMARK 6
#define MONITOR_EVENT_BEGIN_DOWNLOAD 7
#define MONITOR_EVENT_FINISH_DOWNLOAD 8
#define MONITOR_EVENT_BATTERY_NOT_DETECTED 9

struct MONITOR_EVENT {
    int type;
    char* project_url;
    int suspend_reason;
    
    MONITOR_EVENT(int type, const char* project_url);
    MONITOR_EVENT(int type, int suspend_reason);
    ~MONITOR_EVENT();
};

class MONITOR_CONN {
public:
    int sock;
    char* auth_code;
public:
    MONITOR_CONN(int sock);
    ~MONITOR_CONN();

    void set_auth_code(char* code) {
        auth_code = code;
    }
    void send_event(const MONITOR_EVENT* event);
};

class MONITOR_INSTANCE {
private:
    std::vector<MONITOR_EVENT*> pending_events;
    std::vector<char*> auth_codes;
    std::vector<MONITOR_CONN*> conns;
    
    void show_connect_error(sockaddr_storage* addr);
public:
    int lsock;
    
    MONITOR_INSTANCE();
    ~MONITOR_INSTANCE();
    
    int init(bool last_time);
    void close();
    
    char* create_auth_code();
    
    void get_fdset(FDSET_GROUP&, FDSET_GROUP&);
    void got_select(FDSET_GROUP&);
    void push_event(int event_type, const char* project_url);
    void push_event(int event_type, int suspend_reason);
    void broadcast_events();
    bool is_event_pushed(int event_type);
};

#endif /* __MONITOR_H__ */
