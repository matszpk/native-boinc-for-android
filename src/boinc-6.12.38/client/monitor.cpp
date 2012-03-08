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

#include "cpp.h"

#ifdef _WIN32
#include "boinc_win.h"
#else
#include "config.h"
#include <cstdio>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <cerrno>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#endif

#include "filesys.h"
#include "md5_file.h"
#include "error_numbers.h"
#include "client_state.h"
#include "client_msgs.h"
#include "monitor.h"

MONITOR_CONN::MONITOR_CONN(int sock) {
    this->sock = sock;
    auth_code = NULL;
}

MONITOR_CONN::~MONITOR_CONN() {
    boinc_close_socket(sock);
}

void MONITOR_CONN::send_event(const MONITOR_EVENT* event) {
    MIOFILE mf;
    MFILE m;
    mf.init_mfile(&m);
    m.printf("<reply>\n  <type>%d</type>\n",event->type);
    if (event->project_url!=NULL)
        m.printf("  <project>%s</project>\n",event->project_url);
    m.puts("</reply>\n\003");
    char* buf;
    int n;
    m.get_buf(buf, n);
    // write out
    send(sock,buf,n,0);
}

/*
 * MONITOR_EVENT constructor and destructor 
 */
MONITOR_EVENT::MONITOR_EVENT(int type, const char* project_url) {
    this->type = type;
    if (project_url!=NULL)
        this->project_url = strdup(project_url);
    else
        this->project_url = NULL;
}

MONITOR_EVENT::~MONITOR_EVENT() {
    if (project_url!=NULL)
        free(project_url);
}

/*
 * MONITOR_INSTANCE constructor and destructor 
 */
MONITOR_INSTANCE::MONITOR_INSTANCE() {
    lsock = -1;
}

MONITOR_INSTANCE::~MONITOR_INSTANCE() {
    close();
}

int MONITOR_INSTANCE::init(bool lasttime) {
    sockaddr_in addr;
    
    int retval;
    retval = boinc_socket(lsock);
    if (retval) {
        if (lasttime)
            msg_printf(NULL, MSG_INTERNAL_ERROR, "Monitor failed to create socket %d", lsock);
        return retval;
    }
    
    memset(&addr,0,sizeof(addr));
    addr.sin_port = htons(MONITOR_PORT);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // only localhost
    
    /* reusing tcp address */
    int one = 1;
    setsockopt(lsock, SOL_SOCKET, SO_REUSEADDR, (char*)&one, 4);
    
    retval = bind(lsock, (struct sockaddr*)&addr,sizeof(addr));
    if (retval) {
#ifndef _WIN32
        retval = errno;     // Display the real error code
#endif
        if (lasttime)
            msg_printf(NULL, MSG_INTERNAL_ERROR, "Monitor failed to bind socket %d", lsock);
        boinc_close_socket(lsock);
        return retval;
    }
    
    retval = listen(lsock, 999);
    if (retval) {
        if (lasttime)
            msg_printf(NULL, MSG_INTERNAL_ERROR, "Monitor failed to listen socket %d", lsock);
        boinc_close_socket(lsock);
        return retval;
    }
    
    return 0;
}

void MONITOR_INSTANCE::close() {
    int conns_num = conns.size();
    int auth_codes_num = auth_codes.size();
    int events_num = pending_events.size();
    
    for (int i=0; i < conns_num; i++)
        delete conns[i];
    conns.clear();
    
    for (int i=0; i < auth_codes_num; i++)
        free(auth_codes[i]);
    auth_codes.clear();
    
    for (int i=0; i < events_num; i++)
        delete pending_events[i];
    pending_events.clear();
}

void MONITOR_INSTANCE::get_fdset(FDSET_GROUP& fg, FDSET_GROUP& all) {
    int conns_num = conns.size();
    if (lsock<0)
        return;
    
    for (int i=0; i < conns_num; i++) {
        MONITOR_CONN* conn = conns[i];
        int sock = conn->sock;
        bool authorized = (conn->auth_code!=NULL);
        
        if (!authorized)
            FD_SET(sock, &fg.read_fds);
        FD_SET(sock, &fg.exc_fds);
        if (sock > fg.max_fd) fg.max_fd = sock;
        
        if (!authorized)
            FD_SET(sock, &all.read_fds);
        FD_SET(sock, &all.exc_fds);
        if (sock > all.max_fd) all.max_fd = sock;
    }
    
    FD_SET(lsock, &fg.read_fds);
    if (lsock > fg.max_fd) fg.max_fd = lsock;
    FD_SET(lsock, &all.read_fds);
    if (lsock > all.max_fd) all.max_fd = lsock;
}


void MONITOR_INSTANCE::show_connect_error(sockaddr_storage* addr) {
    static int lasttime = -1;
    static int conn_count = 0;
    
    if (lasttime == -1) {
        conn_count = 1;
        lasttime = gstate.now;
    } else {
        if (lasttime - gstate.now < CONNECT_ERROR_PERIOD) {
            conn_count++;
            return;
        }
        lasttime = gstate.now;
    }
    char addr_str[256];
    
#ifdef _WIN32
    sockaddr_in* sin = (sockaddr_in*)addr;
    strcpy(buf, inet_ntoa(sin->sin_addr));
#else
    inet_ntop(addr->ss_family, addr, addr_str, 256);
#endif
    if (conn_count<=1)
        msg_printf(NULL, MSG_INFO,
                   "Monitor request from non-allowed address %s", addr_str);
    else
        msg_printf(NULL, MSG_INFO,
                   "%d connections rejected in last 10 minutes", conn_count);
    
    conn_count = 0;
}

/* handling server socket and client sockets for monitor */
void MONITOR_INSTANCE::got_select(FDSET_GROUP& fg) {
    if (FD_ISSET(lsock,&fg)) {
        sockaddr_storage addr;
        socklen_t addrlen;
        int sock = accept(lsock,(sockaddr*)&addr,&addrlen);
        if (sock < 0)
            return;
#ifndef _WIN32
            fcntl(sock, F_SETFD, FD_CLOEXEC);
#endif
        if (is_localhost(addr)) {
            MONITOR_CONN* conn = new MONITOR_CONN(sock);
            conns.push_back(conn);
        } else {
            show_connect_error(&addr);
            boinc_close_socket(sock);
        }
    }
    
    std::vector<MONITOR_CONN*>::iterator iter = conns.begin();
    
    /* handling exceptions: by deleting connections */
    while (iter != conns.end()) {
        MONITOR_CONN* conn = *iter;
        if (FD_ISSET(conn->sock,&fg.exc_fds)) {
            msg_printf(NULL, MSG_INFO, "Error at %d socket",conn->sock);
            if (conn->auth_code) {
                std::remove(auth_codes.begin(), auth_codes.end(), conn->auth_code);
                free(conn->auth_code);
            }
            delete conn;
            iter = conns.erase(iter);
        } else
            ++iter;
    }
        
    iter = conns.begin();
    while (iter != conns.end()) {
        MONITOR_CONN* conn = *iter;
        // skip authorized connections
        if (conn->auth_code != NULL) {
            ++iter;
            continue;
        }
        
        int sock = conn->sock;
        if (!FD_ISSET(sock,&fg.read_fds)) {
            ++iter;
            continue;
        }

        // authorize connection
        int n;
        char buf[257];
        char auth_code[33];
#ifdef _WIN32
        n = recv(sock, buf, 256, 0);
#else
        n = read(sock, buf, 256);
#endif
        
        int error = 0;
        bool auth_failed = false;
        if (n < 0) {
            auth_failed = true;
            error = ERR_READ;
        }
        else
            buf[n+1] = 0;
        
        if (!auth_failed && parse_str(buf, "authorize",auth_code,33)) {
            int auth_codes_num = auth_codes.size();
            bool matched = false;
            for (int j=0; j < auth_codes_num; j++) {
                if (strcmp(auth_code, auth_codes[j])==0) {
                    // if match
                    // set pointer to matched auth code
                    conn->set_auth_code(auth_codes[j]);
                    // write to socket
                    send(sock, "<success/>\003", 11, 0);
                    matched = true;
                    break;
                }
            }
            // if authorization failed
            if (!matched) auth_failed = true;
        } else
            auth_failed = true;
        
        if (auth_failed) {
            send(sock, "<failed/>\003", 10, 0);
            if (conn->auth_code) {
                std::remove(auth_codes.begin(), auth_codes.end(), conn->auth_code);
                free(conn->auth_code);
            }
            delete conn;
            iter = conns.erase(iter);
            if (error == ERR_READ)
                msg_printf(NULL,MSG_INFO, "Error at reading from monitor connection");
            else
                msg_printf(NULL,MSG_INFO, "Monitor connection authorization failed");
            ++iter;
        } else {
            ++iter;
        }
    }
}

char* MONITOR_INSTANCE::create_auth_code() {
    char* buf = (char*)malloc(33);
    make_weak_random_string(buf);
    auth_codes.push_back(buf);
    return buf;
}

void MONITOR_INSTANCE::push_event(int event_type, const char* project_url) {
    //msg_printf(NULL,MSG_INFO, "[monitor] Pushing event:%d", event_type);
    MONITOR_EVENT* event = new MONITOR_EVENT(event_type, project_url);
    pending_events.push_back(event);
}

void MONITOR_INSTANCE::broadcast_events() {
    int conns_num = conns.size();
    int events_num = pending_events.size();
    
    if (conns_num==0 || events_num == 0)
        return;
    
    for (int i=0; i < conns_num; i++)
        for (int j=0; j < events_num; j++)
            conns[i]->send_event(pending_events[j]);
        
    // delete sent (all) events
    for (int i=0; i < events_num; i++)
        delete pending_events[i];
    pending_events.clear();
}

bool MONITOR_INSTANCE::is_event_pushed(int event_type) {
    int events_num = pending_events.size();
    for (int i=0; i < events_num; i++)
        if (pending_events[i]->type==event_type)
            return true;
    return false;
}
