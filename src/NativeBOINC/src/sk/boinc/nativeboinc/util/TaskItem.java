/**
 * 
 */
package sk.boinc.nativeboinc.util;

import sk.boinc.nativeboinc.R;
import sk.boinc.nativeboinc.bridge.Formatter;
import sk.boinc.nativeboinc.clientconnection.TaskInfo;
import edu.berkeley.boinc.lite.App;
import edu.berkeley.boinc.lite.Project;
import edu.berkeley.boinc.lite.Result;
import edu.berkeley.boinc.lite.Workunit;
import android.content.res.Resources;
import android.os.Parcel;
import android.os.Parcelable;

/**
 * @author mat
 *
 */
public class TaskItem implements Parcelable {
	public String taskName;
	public long   deadlineNum;  // Deadline in numerical form
	
	public int    stateControl; // state control in numerical form
	public int    progInd;      // Progress indication in numerical form
	
	public String project;      // Project.getName()
	public String application;  // App.getName() + Workunit.version_num converted to string
	public String elapsed;      // Result.elapsed_time converted to time-string
	public String progress;     // Result.fraction_done converted to percentage string
	public String toCompletion; // Result.estimated_cpu_time_remaining converted to time-string
	public String deadline;     // Result.report_deadline converted to date-string
	
	public static final Parcelable.Creator<TaskItem> CREATOR
		= new Parcelable.Creator<TaskItem>() {
			@Override
			public TaskItem createFromParcel(Parcel in) {
				return new TaskItem(in);
			}
	
			@Override
			public TaskItem[] newArray(int size) {
				return new TaskItem[size];
			}
	};
	
	public TaskItem(Project project, Workunit workunit, App app, Result result, Formatter formatter) {
		taskName = result.name;
		this.project = project.getName();
		this.application = workunit.app_name;
		StringBuilder sb = new StringBuilder();
		
		/**
		 * code from TaskInfoCreator
		 */
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
		sb.append((appVersion%100)/10);
		sb.append(appVersion%10);
		application = sb.toString();
		update(result, formatter);
	}
	
	public void update(Result result, final Formatter formatter) {
		/**
		 * code from TaskInfoCreator
		 */
		Resources resources = formatter.getResources();
		stateControl = 0;
		if (result.project_suspended_via_gui)
			stateControl = TaskInfo.SUSPENDED;
		
		if (result.suspended_via_gui)
			stateControl = TaskInfo.SUSPENDED;
		
		if (stateControl == 0) {
			// Not suspended - we retrieve detailed state
			switch (result.state) {
			case 0:
			case 1:
				stateControl = TaskInfo.DOWNLOADING;
				break;
			case 2:
				if (result.active_task_state == 1) {
					// Running right now
					stateControl = TaskInfo.RUNNING;
				}
				else if ( (result.active_task_state != 0) || (result.fraction_done > 0.0) ) {
					// Was already running before, but now it's not running
					stateControl = TaskInfo.PREEMPTED;
				}
				else if (result.active_task_state == 0) {
					// Ready to start (was not running yet)
					stateControl = TaskInfo.READY_TO_START;
				}
				else if (result.active_task_state == 5) {
					// Aborted
					stateControl = TaskInfo.ABORTED;
				}
				break;
			case 3:
				stateControl = TaskInfo.ERROR;
				break;
			case 4:
				stateControl = TaskInfo.UPLOADING;
				break;
			case 5:
				stateControl = TaskInfo.READY_TO_REPORT;
				break;
			case 6:
				stateControl = TaskInfo.ABORTED;
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
		
		elapsed = Formatter.formatElapsedTime(elapsedTime);
		progInd = (int)(10.0*pctDone);
		progress = String.format("%.3f%%", pctDone);
		if (result.estimated_cpu_time_remaining > 0.0)
			toCompletion = Formatter.formatElapsedTime((long)result.estimated_cpu_time_remaining);
		else
			toCompletion = resources.getString(R.string.now);
		deadline = formatter.formatDate(result.report_deadline);
		deadlineNum = result.report_deadline;
	}
	
	private TaskItem(Parcel src) {
		taskName = src.readString();
		deadlineNum = src.readLong();
		stateControl = src.readInt();
		progInd = src.readInt();
		project = src.readString();
		application = src.readString();
		elapsed = src.readString();
		progress = src.readString();
		toCompletion = src.readString();
		deadline = src.readString();
	}
	
	@Override
	public int describeContents() {
		return 0;
	}

	@Override
	public void writeToParcel(Parcel dest, int flags) {
		dest.writeString(taskName);
		dest.writeLong(deadlineNum);
		dest.writeInt(stateControl);
		dest.writeInt(progInd);
		dest.writeString(project);
		dest.writeString(application);
		dest.writeString(elapsed);
		dest.writeString(progress);
		dest.writeString(toCompletion);
		dest.writeString(deadline);
	}
}
