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

package sk.boinc.androboinc.bridge;

import sk.boinc.androboinc.R;
import sk.boinc.androboinc.clientconnection.ProjectInfo;
import sk.boinc.androboinc.clientconnection.TaskInfo;
import android.content.res.Resources;
import edu.berkeley.boinc.lite.App;
import edu.berkeley.boinc.lite.Result;
import edu.berkeley.boinc.lite.Workunit;


public class TaskInfoCreator {
	public static TaskInfo create(final Result result, final Workunit workunit, final ProjectInfo pi, final App app, final Formatter formatter) {
		TaskInfo ti = new TaskInfo();
		StringBuilder sb = formatter.getStringBuilder();
		ti.taskName = result.name;
		ti.projectUrl = result.project_url;
		ti.project = pi.project;
		int appVersion = result.version_num;
		if (appVersion == 0) {
			// Older versions of client do not contain this information in Result,
			// but it is present in Workunit
			appVersion = workunit.version_num;
		}
		sb.append(app.getName());
		sb.append(" ");
		sb.append(appVersion/100);
		sb.append(".");
		sb.append(appVersion%100);
		ti.application = sb.toString();
		update(ti, result, formatter);
		return ti;
	}

	public static void update(TaskInfo ti, Result result, final Formatter formatter) {
		Resources resources = formatter.getResources();
		ti.state = formatTaskState(result.state, result.active_task_state, resources);
		ti.stateControl = 0;
		if (result.project_suspended_via_gui) {
			ti.stateControl = TaskInfo.SUSPENDED;
			ti.state = resources.getString(R.string.projectSuspendedByUser);
		}
		if (result.suspended_via_gui) {
			ti.stateControl = TaskInfo.SUSPENDED;
			ti.state = resources.getString(R.string.taskSuspendedByUser);
		}
		if (ti.stateControl == 0) {
			// Not suspended - we retrieve detailed state
			switch (result.state) {
			case 0:
			case 1:
				ti.stateControl = TaskInfo.DOWNLOADING;
				break;
			case 2:
				if (result.active_task_state == 1) {
					// Running right now
					ti.stateControl = TaskInfo.RUNNING;
				}
				else if ( (result.active_task_state != 0) || (result.fraction_done > 0.0) ) {
					// Was already running before, but now it's not running
					ti.stateControl = TaskInfo.PREEMPTED;
				}
				else if (result.active_task_state == 0) {
					// Ready to start (was not running yet)
					ti.stateControl = TaskInfo.READY_TO_START;
				}
				else if (result.active_task_state == 5) {
					// Aborted
					ti.stateControl = TaskInfo.ABORTED;
				}
				break;
			case 3:
				ti.stateControl = TaskInfo.ERROR;
				break;
			case 4:
				ti.stateControl = TaskInfo.UPLOADING;
				break;
			case 5:
				ti.stateControl = TaskInfo.READY_TO_REPORT;
				break;
			case 6:
				ti.stateControl = TaskInfo.ABORTED;
				break;
			}
		}
		double pctDone = result.fraction_done*100;
		long elapsedTime;
		if ((result.state == 4) || (result.state == 5)) {
			// The task has been completed
			pctDone = 100.0;
			elapsedTime = (long)result.final_elapsed_time;
			if (elapsedTime == 0) {
				// Older versions of client do not contain this information,
				// but they have CPU-time instead
				elapsedTime = (long)result.final_cpu_time;
			}
		}
		else {
			// Task not completed yet (or not started yet)
			elapsedTime = (long)result.elapsed_time;
			if (elapsedTime == 0) {
				// Older versions of client do not contain this information,
				// but they have CPU-time instead
				elapsedTime = (long)result.current_cpu_time;
			}
		}
		ti.elapsed = Formatter.formatElapsedTime(elapsedTime);
		ti.progInd = (int)pctDone;
		ti.progress = String.format("%.3f%%", pctDone);
		ti.toCompletion = Formatter.formatElapsedTime((long)result.estimated_cpu_time_remaining);
		ti.deadline = formatter.formatDate(result.report_deadline);
		ti.deadlineNum = result.report_deadline;
		if (result.fraction_done > 0.0) {
			// Task is running/preempted, probably using some resources
			ti.virtMemSize = formatter.formatBinSize((long)result.swap_size);
			ti.workSetSize = formatter.formatSize((long)result.working_set_size_smoothed);
			ti.cpuTime = Formatter.formatElapsedTime((long)result.current_cpu_time);
			ti.chckpntTime = Formatter.formatElapsedTime((long)result.checkpoint_cpu_time);
		}
		ti.resources = result.resources;
	}

	private static final String formatTaskState(int state, int activeTaskState, final Resources resources) {
		String result = resources.getString(R.string.unknown); // init for case something goes wrong
		String[] states = resources.getStringArray(R.array.resultStates);
		if (state < states.length) {
			result = states[state];
			if (state == 2) {
				// the task is active - we have more details to show
				String[] activeStates = resources.getStringArray(R.array.activeTaskStates);
				if (activeTaskState < activeStates.length) {
					result = activeStates[activeTaskState];
				}
			}
		}
		return result;
	}
}
