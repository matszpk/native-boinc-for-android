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
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
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
	
	private BugCatcherService mBugCatcher = null;
	
	private SharedPreferences mSharedPrefs = null;
	
	private boolean mDoRestart = false;
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
	
	private boolean mDelayedBugCatcherListenerRegistration = false;
	
	private static class SavedState {
		private final boolean doRestart;
		private final List<BugReportInfo> bugReports;
		
		public SavedState(BugCatcherActivity activity) {
			doRestart = activity.mDoRestart;
			bugReports = activity.mBugReports;
		}
		
		public void restore(BugCatcherActivity activity) {
			activity.mDoRestart = doRestart;
			activity.mBugReports = bugReports;
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
	
	private BugCatcherServiceConnection mBugCatcherServiceConnection = null;
	
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
		
		mSharedPrefs = PreferenceManager.getDefaultSharedPreferences(this);
		mBugCatcherEnable.setChecked(mSharedPrefs.getBoolean(PreferenceName.BUG_CATCHER_ENABLED, false));
		
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
					mBugCatcher.saveToSDCard();
					// disable buttons when is working
					mClearBugs.setEnabled(false);
					mBugsToSDCard.setEnabled(false);
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
		
		mBugsListView.setAdapter(new BugReportListAdapter(this));
		doBindBugCatcherService();
	}
	
	private void updateActivityState() {
		if (mRunner != null) {
			setProgressBarIndeterminateVisibility(mRunner.serviceIsWorking());
			mRunner.handlePendingErrorMessage(this);
		}
		if (mBugCatcher != null) {
			//mBugCatcher.handlePendingErrorMessage(this);
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
					}
				})
				.setNegativeButton(R.string.noText, null)
				.create();
		}
		return null;
	}
	
	@Override
	public void onPrepareDialog(int dialogId, Dialog dialog, Bundle args) {
		if (StandardDialogs.onPrepareDialog(this, dialogId, dialog, args))
			return; // if standard dialog
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
		// TODO Auto-generated method stub
		
	}

	@Override
	public void onBugReportProgress(String desc, long bugReportId,
			int count, int total) {
		// TODO Auto-generated method stub
		
	}
	
	@Override
	public void onBugReportCancel(String desc) {
		mSendBugs.setEnabled(true);
		mClearBugs.setEnabled(true);
		mBugsToSDCard.setEnabled(true);
	}

	@Override
	public void onBugReportFinish(String desc) {
		mSendBugs.setEnabled(true);
		mClearBugs.setEnabled(true);
		mBugsToSDCard.setEnabled(true);
	}

	@Override
	public boolean onBugReportError(long bugReportId) {
		mSendBugs.setEnabled(true);
		mClearBugs.setEnabled(true);
		mBugsToSDCard.setEnabled(true);
		return true;
	}
	
	@Override
	public void isBugCatcherWorking(boolean isWorking) {
		mSendBugs.setEnabled(!isWorking);
		mClearBugs.setEnabled(!isWorking);
		mBugsToSDCard.setEnabled(!isWorking);
		
		if ((mRunner != null && !mRunner.serviceIsWorking()) || isWorking)
			setProgressBarIndeterminateVisibility(true);
		else if ((mRunner == null || !mRunner.serviceIsWorking()) && !isWorking)
			setProgressBarIndeterminateVisibility(false);
	}
}
