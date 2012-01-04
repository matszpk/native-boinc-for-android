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
	
	public int type;
	public String projectUrl = null;
	
	public ClientEvent(int type) {
		this.type = type;
	}
	
	public ClientEvent(int type, String projectUrl) {
		this.type = type;
		this.projectUrl = projectUrl;
	}
}
