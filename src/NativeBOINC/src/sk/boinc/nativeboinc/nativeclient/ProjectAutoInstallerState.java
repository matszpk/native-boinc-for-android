/**
 * 
 */
package sk.boinc.nativeboinc.nativeclient;

/**
 * @author mat
 *
 */
public interface ProjectAutoInstallerState {
	public static final int NOT_IN_AUTOINSTALLER = 0;
	public static final int IN_AUTOINSTALLER = 1;
	public static final int BEING_INSTALLED = 2;
}
