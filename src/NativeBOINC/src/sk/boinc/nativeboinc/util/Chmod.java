/**
 * 
 */
package sk.boinc.nativeboinc.util;

/**
 * @author mat
 *
 */
public class Chmod {
	/* returns -1 if error */
	public static final native int getmod(String path);
	public static final native boolean chmod(String path, int mode);
	
	static {
		System.loadLibrary("nativeboinc_utils");
	}
}
