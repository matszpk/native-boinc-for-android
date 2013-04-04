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

import java.util.List;

import sk.boinc.nativeboinc.bugcatch.BugCatchProgress;
import sk.boinc.nativeboinc.bugcatch.BugCatcherListener;
import sk.boinc.nativeboinc.bugcatch.BugCatcherService;
import sk.boinc.nativeboinc.bugcatch.BugReportInfo;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.nativeclient.AbstractNativeBoincListener;
import sk.boinc.nativeboinc.util.PreferenceName;
import sk.boinc.nativeboinc.util.StandardDialogs;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.IBinder;
import android.preference.PreferenceManager;
import android.text.InputType;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.TextView;

/**
 * @author mat
 *
 */
public class BugCatcherActivity extends ServiceBoincActivity implements AbstractNativeBoincListener,
		BugCatcherListener {

	private static final String TAG = "BugCatcherActivity";
	
	public static final String RESULT_RESTARTED = "Restarted";
	
	private static final int DIALOG_CLEAR_QUESTION = 1;
	private static final int DIALOG_BUG_SDCARD_DIRECTORY = 2;
	private static final int DIALOG_APPLY_AFTER_RESTART = 3;
	
	private BugCatcherService mBugCatcher = null;
	
	private SharedPreferences mSharedPrefs = null;
	
	private boolean mPrevBugCatcherIsEnabled = false;
	private List<BugReportInfo> mBugReports = null;
	
	private CheckBox mBugCatcherEnable = null;
	private Button mSendBugs = null;
	private Button mClearBugs = null;
	private Button mBugsToSDCard = null;
	private ListView mBugsListView = null;
	private View mProgressItem = null;
	private ProgressBar mProgressBar = null;
	private TextView mProgressInfo = null;
	private Button mProgressCancel = null;
	
	private boolean mDisplayProgress = false;
	
	private boolean mDelayedBugCatcherListenerRegistration = false;
	
	private static class SavedState {
		private final boolean prevBugCatcherIsEnabled;
		private final List<BugReportInfo> bugReports;
		private final boolean displayProgress;
		
		public SavedState(BugCatcherActivity activity) {
			prevBugCatcherIsEnabled = activity.mPrevBugCatcherIsEnabled;
			bugReports = activity.mBugReports;
			displayProgress = activity.mDisplayProgress;
		}
		
		public void restore(BugCatcherActivity activity) {
			activity.mPrevBugCatcherIsEnabled = prevBugCatcherIsEnabled;
			activity.mBugReports = bugReports;
			activity.mDisplayProgress = displayProgress;
		}
	}
	
	private class BugReportListAdapter extends BaseAdapter {

		private Context mContext;
		
		public BugReportListAdapter(Context context) {
			mContext = context;
		}
		
		@Override
		public int getCount() {
			if (mBugReports == null)
				return 0;
			return mBugReports.size();
		}

		@Override
		public Object getItem(int position) {
			return mBugReports.get(position);
		}

		@Override
		public long getItemId(int position) {
			return position;
		}

		@Override
		public View getView(int position, View convertView, ViewGroup parent) {
			View view = null;
			if (convertView == null) {
				LayoutInflater inflater = LayoutInflater.from(mContext);
				view = inflater.inflate(android.R.layout.simple_list_item_2, null);
			} else
				view = convertView;
			
			TextView text1 = (TextView)view.findViewById(android.R.id.text1);
			TextView text2 = (TextView)view.findViewById(android.R.id.text2);
			text1.setText(mBugReports.get(position).getTime());
			text2.setText(mBugReports.get(position).getContent());
			return view;
		}
	}
	
	private BugReportListAdapter mBugReportsListAdapter = null;
	
	private class BugCatcherServiceConnection implements ServiceConnection {
		@Override
		public void onServiceConnected(ComponentName name, IBinder service) {
			mBugCatcher = ((BugCatcherService.LocalBinder)service).getService();
			if (mDelayedBugCatcherListenerRegistration) {
				if (Logging.DEBUG) Log.d(TAG, "Delayed register bugCatcher listener");
				
				mBugCatcher.addBugCatcherListener(BugCatcherActivity.this);
				mDelayedBugCatcherListenerRegistration = false;
			}
			updateActivityState();
		}

		@Override
		public void onServiceDisconnected(ComponentName name) {
			if (mBugCatcher != null)
				mBugCatcher.removeBugCatcherListener(BugCatcherActivity.this);
			mBugCatcher = null;
		}		
	};
	
	private BugCatcherServiceConnection mBugCatcherServiceConnection = new BugCatcherServiceConnection();
	
	private void doBindBugCatcherService() {
		bindService(new Intent(this, BugCatcherService.class),
				mBugCatcherServiceConnection, Context.BIND_AUTO_CREATE);
	}
	
	private void doUnbindBugCatcherService() {
		unbindService(mBugCatcherServiceConnection);
		mBugCatcher = null;
	}
	
	@Override
	public void onCreate(Bundle savedInstanceState) {
		requestWindowFeature(Window.FEATURE_INDETERMINATE_PROGRESS);
		
		final SavedState savedState = (SavedState)getLastNonConfigurationInstance();
		if (savedState != null)
			savedState.restore(this);
		
		setUpService(false, false, true, true, false, false);
		super.onCreate(savedInstanceState);
		setContentView(R.layout.bug_catch);
		
		mBugCatcherEnable = (CheckBox)findViewById(R.id.enableBugCatcher);
		mSendBugs = (Button)findViewById(R.id.sendBugs);
		mClearBugs = (Button)findViewById(R.id.clear);
		mBugsToSDCard = (Button)findViewById(R.id.saveToSDCard);
		mBugsListView = (ListView)findViewById(R.id.bugsList);
		mProgressItem = findViewById(R.id.progressItem);
		mProgressInfo = (TextView)findViewById(R.id.progressInfo);
		mProgressBar = (ProgressBar)findViewById(R.id.progressBar);
		mProgressCancel = (Button)findViewById(R.id.progressCancel);
		
		if (mBugReports == null) // load from source
			mBugReports = BugCatcherService.loadBugReports(this);
		
		mSharedPrefs = PreferenceManager.getDefaultSharedPreferences(this);
		boolean bugCatcherIsEnabled = mSharedPrefs.getBoolean(PreferenceName.BUG_CATCHER_ENABLED, false);
		
		if (savedState == null) // if really created, we save previous state
			mPrevBugCatcherIsEnabled = bugCatcherIsEnabled;
		
		mBugCatcherEnable.setChecked(bugCatcherIsEnabled);
		
		mBugCatcherEnable.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
			@Override
			public void onCheckedChanged(CompoundButton buttonView,
					boolean isChecked) {
				mSharedPrefs.edit().putBoolean(PreferenceName.BUG_CATCHER_ENABLED, isChecked).commit();
			}
		});
		 
		mSendBugs.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				if (mBugCatcher != null) {
					mBugCatcher.sendBugsToAuthor();
					// disable buttons when is working
					mClearBugs.setEnabled(false);
					mSendBugs.setEnabled(false);
					mBugsToSDCard.setEnabled(false);
					mDisplayProgress = true;
				}
			}
		});
		
		mClearBugs.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				showDialog(DIALOG_CLEAR_QUESTION);
			}
		});
		
		mBugsToSDCard.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				if (mBugCatcher != null) {
					showDialog(DIALOG_BUG_SDCARD_DIRECTORY);
				}
			}
		});
		
		mProgressCancel.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				if (mBugCatcher != null)
					mBugCatcher.cancelOperation();
			}
		});
		
		mBugReportsListAdapter = new BugReportListAdapter(this);
		mBugsListView.setAdapter(mBugReportsListAdapter);
		
		doBindBugCatcherService();
	}
	
	private void updateActivityState() {
		if (mRunner != null) {
			if (mBugCatcher == null)
				setProgressBarIndeterminateVisibility(mRunner.serviceIsWorking());
			mRunner.handlePendingErrorMessage(this);
		}
		
		mBugReports = BugCatcherService.loadBugReports(this);
		mBugReportsListAdapter.notifyDataSetChanged();
		
		// if bug catcher initialized
		if (mBugCatcher != null) {
			boolean bugCatcherIsWorking = mBugCatcher.serviceIsWorking();
			setProgressBarIndeterminateVisibility(bugCatcherIsWorking &&
					mRunner.serviceIsWorking());
			// if no error and error occured, then error message retrieved from progress
			boolean isError = mBugCatcher.handlePendingErrors(this);
				
			updateButtonsWorking(bugCatcherIsWorking);
			
			if (bugCatcherIsWorking || mDisplayProgress) {
				BugCatchProgress progress = mBugCatcher.getOpProgress();
				if (progress != null)
					updateProgress(progress.desc, progress.count, progress.total, !bugCatcherIsWorking);
				else
					hideProgress();
			} else if (!isError) // no error no progress
				hideProgress();
		}
	}
	
	@Override
	public void onRunnerConnected() {
		updateActivityState();
	}
	
	@Override
	public Object onRetainNonConfigurationInstance() {
		return new SavedState(this);
	}
	
	private void updateProgress(String desc, int count, int total, boolean finished) {
		mProgressItem.setVisibility(View.VISIBLE);
		mProgressInfo.setText(desc);
		if (!finished) {
			mProgressBar.setVisibility(View.VISIBLE);
			mProgressBar.setProgress(count);
			mProgressBar.setMax(total);
		} else
			mProgressBar.setVisibility(View.GONE);
		mProgressCancel.setEnabled(!finished);
		mDisplayProgress = true;
	}
	
	private void hideProgress() {
		mProgressCancel.setEnabled(false);
		mProgressItem.setVisibility(View.GONE);
	}
	
	private void updateButtonsWorking(boolean working) {
		boolean empty = (mBugReports == null || mBugReports.size()==0);
		mSendBugs.setEnabled(!working && !empty);
		mClearBugs.setEnabled(!working && !empty);
		mBugsToSDCard.setEnabled(!working && !empty);
	}
	
	@Override
	public void onPause() {
		super.onPause();
		if (mBugCatcher!= null) {
			if (Logging.DEBUG) Log.d(TAG, "Unregister bugcatcher listener");
			mBugCatcher.removeBugCatcherListener(this);
			mDelayedBugCatcherListenerRegistration = false;
		}
	}
	
	@Override
	public void onResume() {
		super.onResume();
		if (mBugCatcher != null) {
			if (Logging.DEBUG) Log.d(TAG, "Normal register bugCatcher listener");
			mBugCatcher.addBugCatcherListener(this);
			mDelayedBugCatcherListenerRegistration = false;
		} else
			mDelayedBugCatcherListenerRegistration = true;
		// update runner error (show)
		updateActivityState();
	}
	
	@Override
	public void onDestroy() {
		doUnbindBugCatcherService();
		super.onDestroy();
	}
	
	@Override
	public void onBackPressed() {
		boolean bugCatcherIsEnabled = mSharedPrefs.getBoolean(PreferenceName.BUG_CATCHER_ENABLED, false);
		if (mPrevBugCatcherIsEnabled != bugCatcherIsEnabled) {
			if (isTaskRoot() && mRunner != null && mRunner.isRun()) {
				showDialog(DIALOG_APPLY_AFTER_RESTART);
				return;
			}
			// force restart
			Intent data = new Intent();
			data.putExtra(RESULT_RESTARTED, true);
			setResult(RESULT_OK, data);
			finish();
		} else // normal back pressed
			super.onBackPressed();
	}
	
	@Override
	public Dialog onCreateDialog(int dialogId, Bundle args) {
		Dialog dialog = StandardDialogs.onCreateDialog(this, dialogId, args);
		if (dialog != null)
			return dialog;
		
		switch (dialogId) {
		case DIALOG_CLEAR_QUESTION:
			return new AlertDialog.Builder(this)
				.setIcon(android.R.drawable.ic_dialog_alert)
				.setTitle(R.string.warning)
				.setMessage(R.string.bugCatchClearQuestion)
				.setPositiveButton(R.string.yesText, new DialogInterface.OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which) {
						BugCatcherService.clearBugReports(BugCatcherActivity.this);
						
						mBugReports = BugCatcherService.loadBugReports(BugCatcherActivity.this);
						mBugReportsListAdapter.notifyDataSetChanged();
						
						boolean bugCatcherWorking = (mBugCatcher != null && mBugCatcher.serviceIsWorking());
						updateButtonsWorking(bugCatcherWorking);
					}
				})
				.setNegativeButton(R.string.noText, null)
				.create();
		case DIALOG_BUG_SDCARD_DIRECTORY: {
			View view = LayoutInflater.from(this).inflate(R.layout.dialog_edit, null);
			final EditText edit = (EditText)view.findViewById(android.R.id.edit);
			edit.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_NORMAL);
			
			return new AlertDialog.Builder(this)
				.setIcon(android.R.drawable.ic_input_get)
				.setTitle(R.string.bugsSDCardDirectory)
				.setView(view)
				.setPositiveButton(R.string.ok, new Dialog.OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which) {
						String sdcardDir = edit.getText().toString();
						if (sdcardDir.length() != 0) {
							mBugCatcher.saveToSDCard(sdcardDir);
							mDisplayProgress = true;
						}
					}
				})
				.setNegativeButton(R.string.cancel, null)
				.create();
		}
		case DIALOG_APPLY_AFTER_RESTART: {
			return new AlertDialog.Builder(this)
				.setIcon(android.R.drawable.ic_dialog_alert)
				.setTitle(R.string.warning)
				.setMessage(R.string.applyAfterRestart)
				.setPositiveButton(R.string.restart, new DialogInterface.OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which) {
						if (mRunner != null) {
							mRunner.restartClient();
							// finish activity with notification
							Intent data = new Intent();
							setResult(RESULT_OK, data);
							finish();
						}
					}
				})
				.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which) {
						// finish activity
						finish();
					}
				})
				.setOnCancelListener(new DialogInterface.OnCancelListener() {
					@Override
					public void onCancel(DialogInterface dialog) {
						// finish activity
						finish();
					}
				})
				.create();
		}
		}
		return null;
	}
	
	@Override
	public void onPrepareDialog(int dialogId, Dialog dialog, Bundle args) {
		if (StandardDialogs.onPrepareDialog(this, dialogId, dialog, args))
			return; // if standard dialog
		
		if (dialogId == DIALOG_BUG_SDCARD_DIRECTORY) {
			EditText edit = (EditText)dialog.findViewById(android.R.id.edit);
			edit.setText("boincbugs");
		}
	}
	
	@Override
	public boolean onNativeBoincClientError(String message) {
		StandardDialogs.showErrorDialog(this, message);
		return true;
	}

	@Override
	public void onChangeRunnerIsWorking(boolean isWorking) {
		if ((mBugCatcher != null && !mBugCatcher.serviceIsWorking()) || isWorking)
			setProgressBarIndeterminateVisibility(true);
		else if ((mBugCatcher == null || !mBugCatcher.serviceIsWorking()) && !isWorking)
			setProgressBarIndeterminateVisibility(false);
	}

	@Override
	public void onBugReportBegin(String desc) {
		// disable buttons when is working
		mClearBugs.setEnabled(false);
		mSendBugs.setEnabled(false);
		mBugsToSDCard.setEnabled(false);
	}

	@Override
	public void onBugReportProgress(String desc, long bugReportId,
			int count, int total) {
		updateProgress(desc, count, total, false);
	}
	
	@Override
	public void onBugReportCancel(String desc) {
		BugCatchProgress progress = mBugCatcher.getOpProgress();
		if (progress != null)
			updateProgress(desc, progress.count, progress.total, true);
		else
			updateProgress(desc, 0, 1, true);
		
		// update bugs list
		mBugReports = BugCatcherService.loadBugReports(BugCatcherActivity.this);
		mBugReportsListAdapter.notifyDataSetChanged();
		updateButtonsWorking(false);
	}

	@Override
	public void onBugReportFinish(String desc) {
		BugCatchProgress progress = mBugCatcher.getOpProgress();
		if (progress != null)
			updateProgress(desc, progress.count, progress.total, true);
		else
			updateProgress(desc, 1, 1, true);
		// update bugs list
		mBugReports = BugCatcherService.loadBugReports(BugCatcherActivity.this);
		mBugReportsListAdapter.notifyDataSetChanged();
		updateButtonsWorking(false);
	}

	@Override
	public boolean onBugReportError(String desc, long bugReportId) {
		BugCatchProgress progress = mBugCatcher.getOpProgress();
		if (progress != null)
			updateProgress(desc, progress.count, progress.total, true);
		else
			updateProgress(desc, 0, 1, true);
		
		// update bugs list
		mBugReports = BugCatcherService.loadBugReports(BugCatcherActivity.this);
		mBugReportsListAdapter.notifyDataSetChanged();
		updateButtonsWorking(false);
		return true;
	}
	
	@Override
	public void onNewBugReport(long bugReportId) {
		// update bugs list
		mBugReports = BugCatcherService.loadBugReports(BugCatcherActivity.this);
		mBugReportsListAdapter.notifyDataSetChanged();
		boolean bugCatcherWorking = (mBugCatcher != null && mBugCatcher.serviceIsWorking()); 
		updateButtonsWorking(bugCatcherWorking);
	}
	
	@Override
	public void isBugCatcherWorking(boolean isWorking) {
		updateButtonsWorking(isWorking);
		
		if ((mRunner != null && !mRunner.serviceIsWorking()) || isWorking)
			setProgressBarIndeterminateVisibility(true);
		else if ((mRunner == null || !mRunner.serviceIsWorking()) && !isWorking)
			setProgressBarIndeterminateVisibility(false);
	}
}
