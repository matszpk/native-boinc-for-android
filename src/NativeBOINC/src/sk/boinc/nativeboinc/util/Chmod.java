/**
 * 
 */
package sk.boinc.nativeboinc.util;

/**
 * @author mat
 *
 */
public class Chmod {
	public static final native boolean chmod(String path, int mode);
	
	static {
		System.loadLibrary("nativeboinc_utils");
	}
}
