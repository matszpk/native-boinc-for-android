/**
 * 
 */
package sk.boinc.nativeboinc.clientconnection;

import java.util.ArrayList;

/**
 * @author mat
 *
 */
public interface ClientUpdateTasksReceiver extends ClientReceiver {
	public abstract boolean updatedTasks(ArrayList<TaskInfo> tasks);
}
