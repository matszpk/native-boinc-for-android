/* 
 * NativeBOINC - Native BOINC Client with Manager
 * Copyright (C) 2011, Mateusz Szpakowski
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */
package sk.boinc.nativeboinc;

import java.io.FileReader;
import java.io.IOException;

import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.nativeclient.AbstractNativeBoincListener;
import sk.boinc.nativeboinc.nativeclient.NativeBoincUtils;
import sk.boinc.nativeboinc.util.FileUtils;
import sk.boinc.nativeboinc.util.StandardDialogs;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.os.Environment;
import android.text.Editable;
import android.text.InputType;
import android.text.TextWatcher;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.Window;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

/**
 * @author mat
 *
 */
public class ConfigFileActivity extends ServiceBoincActivity implements AbstractNativeBoincListener {
	
	private static final String TAG = "ConfigFileActivity";
	
	public static final String RESULT_RESTARTED = "Restarted";
	
	private static final int DIALOG_CONFIRM_DELETE = 1;
	private static final int DIALOG_ENTER_FROM_SDCARD = 2;
	
	private EditText mConfigContent;
	private Button mCancelButton;
	private Button mOkButton;
	private Button mDeleteButton;
	private Button mFromSDCardButton;
	
	private boolean mConfigLoaded = false;
	
	private String mExternalPath = null;
	
	private static class SavedState {
		private final boolean configLoaded;
		
		public SavedState(ConfigFileActivity activity) {
			configLoaded = activity.mConfigLoaded;
		}
		
		public void restore(ConfigFileActivity activity) {
			activity.mConfigLoaded = configLoaded;
		}
	}
	
	@Override
	public void onCreate(Bundle savedInstanceState) {
		requestWindowFeature(Window.FEATURE_INDETERMINATE_PROGRESS);
		
		final SavedState savedState = (SavedState)getLastNonConfigurationInstance();
		if (savedState != null)
			savedState.restore(this);
		
		setUpService(false, false, true, true, false, false);
		super.onCreate(savedInstanceState);
		setContentView(R.layout.config_file);
		
		mExternalPath = Environment.getExternalStorageDirectory().toString();
		
		mConfigContent = (EditText)findViewById(R.id.configContent);
		mDeleteButton = (Button)findViewById(R.id.deleteConfig);
		mFromSDCardButton = (Button)findViewById(R.id.configFromSDCard);
		mCancelButton = (Button)findViewById(R.id.cancel);
		mOkButton = (Button)findViewById(R.id.ok);
		
		if (!mConfigLoaded) { // if config not loaded
			String content = null;
			
			try { // load from boinc directory
				content = NativeBoincUtils.readCcConfigFile(this);
			} catch(IOException ex) { // if error happens
				Toast.makeText(this, R.string.configFileReadError, Toast.LENGTH_LONG).show();
			}
			if (content != null)
				mConfigContent.setText(content);
			// set as loaded
			mConfigLoaded = true;
		}
		
		mCancelButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				finish();
			}
		});
		
		mDeleteButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				showDialog(DIALOG_CONFIRM_DELETE);
			}
		});
		
		mFromSDCardButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				showDialog(DIALOG_ENTER_FROM_SDCARD);
			}
		});
		
		mOkButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				try {
					if (Logging.DEBUG) Log.d(TAG, "Writing cc_config.xml");
					NativeBoincUtils.writeCcConfigFile(ConfigFileActivity.this,
							mConfigContent.getText().toString());
				} catch(IOException ex) {
					if (Logging.DEBUG) Log.d(TAG, "Failed writing cc_config.xml");
					Toast.makeText(ConfigFileActivity.this, R.string.configFileWriteError,
							Toast.LENGTH_LONG).show();
				}
				
				Intent data = new Intent();
				data.putExtra(RESULT_RESTARTED, true);
				setResult(RESULT_OK, data);
				finish();
			}
		});
	}
	
	private void updateActivityState() {
		if (mRunner == null)
			return;
		
		setProgressBarIndeterminateVisibility(mRunner.serviceIsWorking());
		mRunner.handlePendingErrorMessage(this);
	}
	
	@Override
	public void onRunnerConnected() {
		updateActivityState();
	}
	
	@Override
	public Object onRetainNonConfigurationInstance() {
		return new SavedState(this);
	}
	
	@Override
	public void onResume() {
		super.onResume();
		updateActivityState();
	}
	
	@Override
	public Dialog onCreateDialog(int dialogId, Bundle args) {
		Dialog dialog = StandardDialogs.onCreateDialog(this, dialogId, args);
		if (dialog != null)
			return dialog;
		
		switch (dialogId) {
		case DIALOG_ENTER_FROM_SDCARD: {
			View view = LayoutInflater.from(this).inflate(R.layout.dialog_edit, null);
			final EditText edit = (EditText)view.findViewById(android.R.id.edit);
			edit.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_NORMAL);
			
			return new AlertDialog.Builder(this)
				.setIcon(android.R.drawable.ic_input_get)
				.setTitle(R.string.enterConfigPath)
				.setView(view)
				.setPositiveButton(R.string.ok, new Dialog.OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which) {
						String ccConfigFile = edit.getText().toString();
						if (ccConfigFile.length() != 0) {
							// load from sdcard
							String content = loadConfigFromSDCard(ccConfigFile);
							if (content != null) {
								if (Logging.DEBUG) Log.d(TAG, "setting content from sdcard");
								mConfigContent.setText(content);
							} else // if load failed
								Toast.makeText(ConfigFileActivity.this, R.string.configFileReadError,
										Toast.LENGTH_LONG).show();
						}
					}
				})
				.setNegativeButton(R.string.cancel, null)
				.create();
		}
		case DIALOG_CONFIRM_DELETE:
			return new AlertDialog.Builder(this)
				.setIcon(android.R.drawable.ic_dialog_alert)
				.setTitle(R.string.warning)
				.setMessage(R.string.confirmConfigDelete)
				.setPositiveButton(R.string.yesText, new DialogInterface.OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which) {
						NativeBoincUtils.removeCcConfigFile(ConfigFileActivity.this);
						if (Logging.DEBUG) Log.d(TAG, "remove cc_config.xml");
						// force restarting
						Intent data = new Intent();
						data.putExtra(RESULT_RESTARTED, true);
						setResult(RESULT_OK, data);
						finish();
					}
				})
				.setNegativeButton(R.string.noText, null)
				.create();
		}
		return null;
	}
	
	@Override
	protected void onPrepareDialog(int dialogId, Dialog dialog, Bundle args) {
		if (StandardDialogs.onPrepareDialog(this, dialogId, dialog, args))
			return; // if standard dialog
		
		if (dialogId == DIALOG_ENTER_FROM_SDCARD) {
			final EditText edit = (EditText)dialog.findViewById(android.R.id.edit);
			edit.setText("");
		}
	}
	
	/* load cc config from sdcard */
	public String loadConfigFromSDCard(String configPath) {
		FileReader inReader = null;
		StringBuilder content = null;
		
		try {
			inReader = new FileReader(FileUtils.joinBaseAndPath(mExternalPath, configPath));
			content = new StringBuilder();
			
			char[] buf = new char[1024];
			while(true) {
				int readed = inReader.read(buf);
				if (readed == -1)
					break;
				content.append(buf, 0, readed);
			}
		} catch(IOException ex) { // load failed
			if (Logging.DEBUG) Log.d(TAG, "IO exception in  loading content from sdcard");
			return null;
		} finally {
			if (inReader != null) {
				try {
					inReader.close();
				} catch(IOException ex) { }
			}
		}
		
		return (content!=null) ? content.toString() : null;
	}
	
	@Override
	public boolean onNativeBoincClientError(String message) {
		StandardDialogs.showErrorDialog(this, message);
		return true;
	}

	@Override
	public void onChangeRunnerIsWorking(boolean isWorking) {
		setProgressBarIndeterminateVisibility(isWorking);
	}
}
