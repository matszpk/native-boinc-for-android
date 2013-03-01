/**
 * 
 */
package sk.boinc.nativeboinc;

import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStreamWriter;

import sk.boinc.nativeboinc.installer.InstallerService;
import sk.boinc.nativeboinc.util.FileUtils;
import sk.boinc.nativeboinc.util.StandardDialogs;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.os.Environment;
import android.text.ClipboardManager;
import android.text.InputType;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.EditText;
import android.widget.ListView;

/**
 * @author mat
 *
 */
public class BoincLogsActivity extends AbstractBoincActivity {

	private static final int LOGS_FILENAME_DIALOG = 1;
	
	private String[] mLogsArray = null;
	private String mLogsString = null;
	
	private static final class SavedState {
		private final String logsString;
		private final String[] logsArray;
		
		public SavedState(BoincLogsActivity activity) {
			logsString = activity.mLogsString;
			logsArray = activity.mLogsArray;
		}
		
		public void restore(BoincLogsActivity activity) {
			activity.mLogsString = logsString;
			activity.mLogsArray = logsArray;
		}
	}
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.boinc_logs);
		
		// load logs
		final SavedState savedState = (SavedState)getLastNonConfigurationInstance();
		if (savedState != null)
			savedState.restore(this);
		
		if (mLogsArray == null) {
			mLogsString = InstallerService.getBoincLogs(this);
			if (mLogsString != null) {
				mLogsArray = mLogsString.split("\n");
			} else
				mLogsArray = new String[0];
		}
		
		ListView logsList = (ListView)findViewById(R.id.logsList);
		logsList.setAdapter(new ArrayAdapter<String>(this,
				R.layout.boinc_logs_item, mLogsArray));
	}
	
	@Override
	public Object onRetainNonConfigurationInstance() {
		return new SavedState(this); 
	}
	
	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		super.onCreateOptionsMenu(menu);
		MenuInflater inflater = getMenuInflater();
		inflater.inflate(R.menu.logs_menu, menu);
		return true;
	}
	
	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		switch (item.getItemId()) {
		case R.id.copyText: 
			((ClipboardManager)getSystemService(CLIPBOARD_SERVICE)).setText(mLogsString);
			return true;
		case R.id.saveToSDCard:
			showDialog(LOGS_FILENAME_DIALOG);
			return true;
		}
		return false;
	}
	
	@Override
	protected Dialog onCreateDialog(int dialogId, Bundle args) {
		Dialog dialog = StandardDialogs.onCreateDialog(this, dialogId, args);
		if (dialog != null)
			return dialog;
		
		if (dialogId == LOGS_FILENAME_DIALOG) {
			View view = LayoutInflater.from(this).inflate(R.layout.dialog_edit, null);
			final EditText edit = (EditText)view.findViewById(android.R.id.edit);
			edit.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_NORMAL);
			
			return new AlertDialog.Builder(this)
				.setIcon(android.R.drawable.ic_input_get)
				.setTitle(R.string.enterOutputFilePath)
				.setView(view)
				.setPositiveButton(R.string.ok, new Dialog.OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which) {
						String base = Environment.getExternalStorageDirectory().getAbsolutePath();
						String filePath = edit.getText().toString();
						String outPath = FileUtils.joinBaseAndPath(base, filePath);
						
						saveToFile(outPath);
					}
				})
				.setNegativeButton(R.string.cancel, null)
				.create();
		}
		return null;
	}
	
	@Override
	protected void onPrepareDialog(int dialogId, Dialog dialog, Bundle args) {
		StandardDialogs.onPrepareDialog(this, dialogId, dialog, args);
	}
	
	private void saveToFile(String path) {
		OutputStreamWriter outWriter = null;
		if (mLogsString == null)
			return;
		try {
			outWriter = new OutputStreamWriter(new FileOutputStream(path), "UTF-8");
			outWriter.write(mLogsString);
			
			outWriter.flush();
		} catch(IOException ex) {
			StandardDialogs.showErrorDialog(this, getString(R.string.boincLogsSaveError));
		} finally {
			try {
				if (outWriter != null)
					outWriter.close();
			} catch(IOException ex2) { }
		}
	}
}
