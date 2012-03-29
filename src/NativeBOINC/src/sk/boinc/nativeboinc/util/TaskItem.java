/**
 * 
 */
package sk.boinc.nativeboinc.util;

import sk.boinc.nativeboinc.R;
import sk.boinc.nativeboinc.bridge.Formatter;
import sk.boinc.nativeboinc.bridge.TaskInfoCreator;
import sk.boinc.nativeboinc.clientconnection.ProjectInfo;
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
	
	public TaskInfo taskInfo = new TaskInfo();
	
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
		ProjectInfo projectInfo = new ProjectInfo(); // dependency
		projectInfo.project = project.getName();
		taskInfo = TaskInfoCreator.create(result, workunit, projectInfo, app, formatter);
	}
	
	public void update(Result result, final Formatter formatter) {
		TaskInfoCreator.update(taskInfo, result, formatter);
	}
	
	private TaskItem(Parcel src) {
		taskInfo.taskName = src.readString();
		taskInfo.projectUrl = src.readString();
		taskInfo.stateControl = src.readInt();
		taskInfo.progInd = src.readInt();
		taskInfo.deadlineNum = src.readLong();
		taskInfo.project = src.readString();
		taskInfo.application = src.readString();
		taskInfo.elapsed = src.readString();
		taskInfo.progress = src.readString();
		taskInfo.toCompletion = src.readString();
		taskInfo.deadline = src.readString();
		taskInfo.virtMemSize = src.readString();
		taskInfo.workSetSize = src.readString();
		taskInfo.cpuTime = src.readString();
		taskInfo.chckpntTime = src.readString();
		taskInfo.resources = src.readString();
		taskInfo.state = src.readString();
		taskInfo.pid = src.readInt();
		taskInfo.received_time = src.readString();
		taskInfo.rsc_fpops_est = src.readString();
		taskInfo.rsc_memory_bound = src.readString();
		taskInfo.directory = src.readString();
	}
	
	@Override
	public int describeContents() {
		return 0;
	}

	@Override
	public void writeToParcel(Parcel dest, int flags) {
		dest.writeString(taskInfo.taskName);
		dest.writeString(taskInfo.projectUrl);
		dest.writeInt(taskInfo.stateControl);
		dest.writeInt(taskInfo.progInd);
		dest.writeLong(taskInfo.deadlineNum);
		dest.writeString(taskInfo.project);
		dest.writeString(taskInfo.application);
		dest.writeString(taskInfo.elapsed);
		dest.writeString(taskInfo.progress);
		dest.writeString(taskInfo.toCompletion);
		dest.writeString(taskInfo.deadline);
		dest.writeString(taskInfo.virtMemSize);
		dest.writeString(taskInfo.workSetSize);
		dest.writeString(taskInfo.cpuTime);
		dest.writeString(taskInfo.chckpntTime);
		dest.writeString(taskInfo.resources);
		dest.writeString(taskInfo.state);
		dest.writeInt(taskInfo.pid);
		dest.writeString(taskInfo.received_time);
		dest.writeString(taskInfo.rsc_fpops_est);
		dest.writeString(taskInfo.rsc_memory_bound);
		dest.writeString(taskInfo.directory);
	}
}
