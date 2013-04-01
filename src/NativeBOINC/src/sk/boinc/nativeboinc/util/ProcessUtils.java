/**
 * 
 */
package sk.boinc.nativeboinc.util;

/**
 * @author mat
 *
 */
public class ProcessUtils {

	public native static int exec(String program, String dirPath, String[] args);
	public native static int execSD(String program, String dirPath, String[] args);
	public native static int waitForProcess(int pid) throws InterruptedException;
	
	public native static int bugCatchExec(String program, String dirPath, String[] args);
	public native static int bugCatchInit(int pid);
	public native static int bugCatchExecSD(String program, String dirPath, String[] args);
	public native static int bugCatchWaitForProcess(int pid) throws InterruptedException;
	
	static {
		System.loadLibrary("nativeboinc_utils");
	}
}
