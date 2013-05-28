/* 
 * AndroBOINC - BOINC Manager for Android
 * Copyright (C) 2010, Pavol Michalec
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */

package edu.berkeley.boinc.nativeboinc;

/**
 * @author mat
 *
 */
public class ClientEvent {
	public final static int EVENT_ATTACHED_PROJECT = 1;
	public final static int EVENT_DETACHED_PROJECT = 2;
	public final static int EVENT_SUSPEND_ALL_TASKS = 3;
	public final static int EVENT_RUN_TASKS = 4;
	public final static int EVENT_RUN_BENCHMARK = 5;
	public final static int EVENT_FINISH_BENCHMARK = 6;
	
	public final static int SUSPEND_REASON_BATTERIES = 1;
	public final static int SUSPEND_REASON_USER_ACTIVE = 2;
	public final static int SUSPEND_REASON_USER_REQ = 4;
	public final static int SUSPEND_REASON_TIME_OF_DAY = 8;
	public final static int SUSPEND_REASON_BENCHMARKS = 16;
	public final static int SUSPEND_REASON_DISK_SIZE = 32;
	public final static int SUSPEND_REASON_CPU_THROTTLE = 64;
	public final static int SUSPEND_REASON_NO_RECENT_INPUT = 128;
	public final static int SUSPEND_REASON_INITIAL_DELAY = 256;
	public final static int SUSPEND_REASON_EXCLUSIVE_APP_RUNNING = 512;
	public final static int SUSPEND_REASON_CPU_USAGE = 1024;
	public final static int SUSPEND_REASON_NETWORK_QUOTA_EXCEEDED = 2048;
	public final static int SUSPEND_REASON_OS = 4096;
    // extra reasons
	public final static int SUSPEND_REASON_WIFI_DISABLED = (1<<28);
	public final static int SUSPEND_REASON_DISCHARGE = (1<<29);
	public final static int SUSPEND_REASON_OVERHEAT = (1<<30);
	
	public int type;
	public String projectUrl = null;
	public int suspendReason = 0;
	
	public ClientEvent(int type) {
		this.type = type;
	}
	
	public ClientEvent(int type, int suspendReason) {
		this.type = type;
		this.suspendReason = suspendReason;
	}
	
	public ClientEvent(int type, String projectUrl) {
		this.type = type;
		this.projectUrl = projectUrl;
	}
}
