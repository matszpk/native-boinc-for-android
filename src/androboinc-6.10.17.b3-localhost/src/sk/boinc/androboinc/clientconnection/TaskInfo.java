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

package sk.boinc.androboinc.clientconnection;


/**
 * Description of BOINC task for AndroBOINC purpose
 * Reflects the classes of BOINC-library: Result, Project, Workunit, App
 */
public class TaskInfo {
	public String taskName;     // Result.name - unique ID
	public String projectUrl;   // Result.project_url
	public int    stateControl; // state control in numerical form
	public static final int DOWNLOADING = 1;
	public static final int READY_TO_START = 2;
	public static final int RUNNING = 3;
	public static final int PREEMPTED = 4;
	public static final int UPLOADING = 5;
	public static final int READY_TO_REPORT = 6;
	public static final int SUSPENDED = 7;
	public static final int ABORTED = 8;
	public static final int ERROR = 9;
	
	public int    progInd;      // Progress indication in numerical form
	public long   deadlineNum;  // Deadline in numerical form
	
	public String project;      // Project.getName()
	public String application;  // App.getName() + Workunit.version_num converted to string
	public String elapsed;      // Result.elapsed_time converted to time-string
	public String progress;     // Result.fraction_done converted to percentage string
	public String toCompletion; // Result.estimated_cpu_time_remaining converted to time-string
	public String deadline;     // Result.report_deadline converted to date-string
	public String virtMemSize;  // Result.swap_size converted to size-string (base 2) (can be null)
	public String workSetSize;  // Result.working_set_size_smoothed converted to size-string (base 10) (can be null)
	public String cpuTime;      // Result.current_cpu_time converted to time-string (can be null)
	public String chckpntTime;  // Result.checkpoint_cpu_time converted to time-string (can be null)
	public String resources;    // Result.resources (can be null)
	public String state;        // Result.state combined with Result.active_task_state converted to string
}
