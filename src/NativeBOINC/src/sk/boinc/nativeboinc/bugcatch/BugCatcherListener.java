/**
 * 
 */
package sk.boinc.nativeboinc.bugcatch;

/**
 * @author mat
 *
 */
public interface BugCatcherListener {
	public abstract void onBugReportBegin(String message);
	public abstract void onBugReportProgress(String message, String bugReportId, int count, int total);
	public abstract void onBugReportFinish(String message);
	public abstract void onBugReportError(String bugReportId);
}
