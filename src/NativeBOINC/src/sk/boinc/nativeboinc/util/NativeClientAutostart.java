/**
 * 
 */
package sk.boinc.nativeboinc.util;

import android.content.SharedPreferences;

/**
 * @author mat
 *
 */
public class NativeClientAutostart {
	public static final int AUTOSTART_AT_APP_MASK = 1;
	public static final int AUTOSTART_AT_BOOT_MASK = 2;
	
	public static boolean isAutostartsAtAppStartup(SharedPreferences sharedPrefs) {
		String stringValue = sharedPrefs.getString(PreferenceName.NATIVE_AUTOSTART, "0");
		int value = Integer.parseInt(stringValue);
		
		return (value & AUTOSTART_AT_APP_MASK) != 0;
	}
	
	public static boolean isAutostartsAtOnlySystemBooting(SharedPreferences sharedPrefs) {
		String stringValue = sharedPrefs.getString(PreferenceName.NATIVE_AUTOSTART, "0");
		int value = Integer.parseInt(stringValue);
		
		return value == AUTOSTART_AT_BOOT_MASK;
	}
}
