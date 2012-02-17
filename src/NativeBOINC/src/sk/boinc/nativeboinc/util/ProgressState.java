/**
 * 
 */
package sk.boinc.nativeboinc.util;

/**
 * @author mat
 *
 */
public interface ProgressState {
	public static final int NOT_RUN = 0;
	public static final int IN_PROGRESS = 1;
	public static final int FINISHED = 2;
	public static final int FAILED = 3;
}
