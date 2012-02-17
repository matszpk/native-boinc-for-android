/**
 * 
 */
package sk.boinc.nativeboinc.nativeclient;

/**
 * @author mat
 *
 */
public interface NativeBoincUpdateListener extends NativeBoincServiceListener {
	public abstract void updatedProjectApps(String projectUrl);
	public abstract void updateProjectAppsError(String projectUrl);
}
