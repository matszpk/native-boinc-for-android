/**
 * 
 */
package sk.boinc.nativeboinc;

import sk.boinc.nativeboinc.installer.InstallerService;
import android.os.Bundle;
import android.widget.EditText;

/**
 * @author mat
 *
 */
public class BoincLogsActivity extends AbstractBoincActivity {

	private static final String STATE_LOADED = "Loaded";
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.boinc_logs);
		
		boolean isLoaded = false;
		if (savedInstanceState != null)
			isLoaded = savedInstanceState.getBoolean(STATE_LOADED, false);
		
		// load logs
		if (!isLoaded) {
			String logs = InstallerService.getBoincLogs(this);
			EditText logsText = (EditText)findViewById(R.id.text);
			if (logs != null)
				logsText.setText(logs);
		}
	}
	
	@Override
	protected void onSaveInstanceState(Bundle outState) {
		super.onSaveInstanceState(outState);
		outState.putBoolean(STATE_LOADED, true);
	}
}
