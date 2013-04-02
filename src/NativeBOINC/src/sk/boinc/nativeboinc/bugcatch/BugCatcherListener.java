/**
 * 
 */
package sk.boinc.nativeboinc.bugcatch;

/**
 * @author mat
 *
 */
public interface BugCatcherListener {
	public abstract void onBugReportBegin(String desc);
	public abstract void onBugReportProgress(String desc, long bugReportId, int count, int total);
	public abstract void onBugReportCancel(String desc);
	public abstract void onBugReportFinish(String desc);
	public abstract boolean onBugReportError(long bugReportId);
	
	public abstract void isBugCatcherWorking(boolean isWorking);
}
