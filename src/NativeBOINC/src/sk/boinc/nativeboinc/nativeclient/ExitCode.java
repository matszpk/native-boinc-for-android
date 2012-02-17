/**
 * 
 */
package sk.boinc.nativeboinc.nativeclient;

import sk.boinc.nativeboinc.R;
import android.content.Context;

/**
 * @author mat
 *
 */
public class ExitCode {
	public static final int EXIT_STATUS_MASK = 0xff00;
	public static final int EXIT_CODE_MASK = 0xff;
	public static final int SIGNAL_MASK = 0x7f;
	public static final int NORMAL_EXIT = 0;
	public static final int SIGNALED_EXIT = 0x100;
	
	public static final int WAIT_FAILED = -1;
	
	public static boolean isNormalExit(int exitCode) {
		return (exitCode & EXIT_STATUS_MASK) == NORMAL_EXIT; 
	}
	
	public static boolean isSignaledExit(int exitCode) {
		return (exitCode & EXIT_STATUS_MASK) == SIGNALED_EXIT; 
	}
	
	public static boolean isWaitFailed(int exitCode) {
		return exitCode == WAIT_FAILED; 
	}
	
	public static int getSignal(int exitCode) {
		return exitCode & SIGNAL_MASK; 
	}
	
	public static int getExitCode(int exitCode) {
		return exitCode & EXIT_CODE_MASK; 
	}
	
	public static String getExitCodeMessage(Context context, int exitCode,
			boolean stoppedByManager) {
		
		if (isWaitFailed(exitCode))
			return context.getString(R.string.nativeClientWaitFailed);
		else if (isNormalExit(exitCode)) {
			int code = getExitCode(exitCode);
			if (code == 0) {
				if (stoppedByManager)
					return context.getString(R.string.nativeClientShutdown);
				else
					return context.getString(R.string.nativeClientStoppedByForeign);
			} else
				return context.getString(R.string.nativeClientNormalExitWithCode) + " " + code; 
		} else if (isSignaledExit(exitCode)) {
			return context.getString(R.string.nativeClientSignaled) + " " + getSignal(exitCode);
		}
		return context.getString(R.string.nativeClientStopUnknownReason);
	}
}
