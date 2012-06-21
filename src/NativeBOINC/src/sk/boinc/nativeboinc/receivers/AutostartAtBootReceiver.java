/**
 * 
 */
package sk.boinc.nativeboinc.receivers;

import sk.boinc.nativeboinc.BoincManagerApplication;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.util.NativeClientAutostart;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.util.Log;

/**
 * @author mat
 *
 */
public class AutostartAtBootReceiver extends BroadcastReceiver {

	private static final String TAG = "AutostartReceiver";
	
	@Override
	public void onReceive(Context context, Intent intent) {
		if (intent == null || intent.getAction() == null) return;
		
		if (intent.getAction().equals(Intent.ACTION_BOOT_COMPLETED)) {
			BoincManagerApplication appContext = (BoincManagerApplication)context.getApplicationContext();
			
			if (Logging.DEBUG) Log.d(TAG, "Bott completed received");
			
			SharedPreferences globalPrefs = PreferenceManager.getDefaultSharedPreferences(appContext);
			if (NativeClientAutostart.isAutostartsAtOnlySystemBooting(globalPrefs)) {
				// do autostart client
				appContext.autostartClient();
			}
		}
	}
}
