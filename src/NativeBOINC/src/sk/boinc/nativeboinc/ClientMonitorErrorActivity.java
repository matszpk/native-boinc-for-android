/**
 * 
 */
package sk.boinc.nativeboinc;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Bundle;

/**
 * @author mat
 *
 */
public class ClientMonitorErrorActivity extends Activity {

	private static final int DIALOG_CLIENT_MONITOR_ERROR = 1;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		showDialog(DIALOG_CLIENT_MONITOR_ERROR);
	}
	
	@Override
	protected Dialog onCreateDialog(int id) {
		if (id == DIALOG_CLIENT_MONITOR_ERROR)
			return new AlertDialog.Builder(this)
	    		.setIcon(android.R.drawable.ic_dialog_alert)
	    		.setTitle(R.string.warning)
	    		.setMessage(R.string.cantAuthorizeClientMonitorMessage)
	    		.setPositiveButton(R.string.dismiss,  new DialogInterface.OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which) {
						ClientMonitorErrorActivity.this.finish();
					}
				})
				.setOnCancelListener(new DialogInterface.OnCancelListener() {
					@Override
					public void onCancel(DialogInterface dialog) {
						ClientMonitorErrorActivity.this.finish();
					}
				})
	    		.create();
		return null;
	}
}
