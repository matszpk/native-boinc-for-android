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

import java.util.Locale;

import sk.boinc.nativeboinc.clientconnection.BoincOp;
import sk.boinc.nativeboinc.clientconnection.ClientPreferencesReceiver;
import sk.boinc.nativeboinc.clientconnection.VersionInfo;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.nativeclient.NativeBoincUtils;
import sk.boinc.nativeboinc.util.ClientId;
import sk.boinc.nativeboinc.util.ProgressState;
import sk.boinc.nativeboinc.util.StandardDialogs;
import sk.boinc.nativeboinc.util.TimePrefsData;
import edu.berkeley.boinc.lite.GlobalPreferences;
import edu.berkeley.boinc.lite.TimePreferences;
import android.app.Dialog;
import android.content.Intent;
import android.content.res.Resources;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.TabHost;

/**
 * @author mat
 *
 */
public class LocalPreferencesActivity extends ServiceBoincActivity implements ClientPreferencesReceiver {
	private static final String TAG = "LocalPrefsActivity";
	
	private static final int ACTIVITY_CPU_TIMES = 1;
	private static final int ACTIVITY_NET_TIMES = 2;
	
	private ClientId mConnectedClient = null;
	
	private TabHost mTabHost;
	private Button mApplyDefault;
	
	private int mGlobalPrefsFetchProgress = ProgressState.NOT_RUN;
	private boolean mGlobalPrefsSavingInProgress = false;
	private boolean mOtherGlobalPrefsSavingInProgress = false;
	
	private CheckBox mComputeOnBatteries;
	private CheckBox mRunAlwaysWhenPlugged;
	private CheckBox mComputeInUse;
	private CheckBox mUseGPUInUse;
	private EditText mComputeIdleFor;
	private EditText mComputeUsageLessThan;
	private EditText mBatteryLevelNL;
	private EditText mBatteryTempLT;
	private EditText mSwitchBetween;
	private EditText mUseAtMostCPUs;
	private EditText mUseAtMostCPUTime;
	private CheckBox mXferOnlyWhenWifi;
	private EditText mMaxDownloadRate;
	private EditText mMaxUploadRate;
	private EditText mTransferAtMost;
	private EditText mTransferPeriodDays;
	private EditText mConnectAboutEvery;
	private EditText mAdditionalWorkBuffer;
	private CheckBox mSkipVerifyImages;
	private EditText mUseAtMostDiskSpace;
	private EditText mLeaveAtLeastDiskFree;
	private EditText mUseAtMostTotalDisk;
	private EditText mCheckpointToDisk;
	private EditText mUseAtMostMemoryInUse;
	private EditText mUseAtMostMemoryInIdle;
	private CheckBox mLeaveApplications;
	
	private Button mApply;
	
	private int mSelectedTab = 0;
	
	private TimePreferences mCPUTimePreferences = null;
	private TimePreferences mNetTimePreferences = null;
	
	private static class SavedState {
		private final int globalPrefsFetchProgress;
		private final boolean globalPrefsSavingInProgress;
		private final boolean otherGlobalPrefsSavingInProgress;
		private final int selectedTab;
		private final TimePreferences cpuTimePreferences;
		private final TimePreferences netTimePreferences;
		
		public SavedState(LocalPreferencesActivity activity) {
			globalPrefsFetchProgress = activity.mGlobalPrefsFetchProgress;
			globalPrefsSavingInProgress = activity.mGlobalPrefsSavingInProgress;
			otherGlobalPrefsSavingInProgress = activity.mOtherGlobalPrefsSavingInProgress;
			selectedTab = activity.mSelectedTab;
			cpuTimePreferences = activity.mCPUTimePreferences;
			netTimePreferences = activity.mNetTimePreferences;
		}
		
		public void restore(LocalPreferencesActivity activity) {
			activity.mGlobalPrefsFetchProgress = globalPrefsFetchProgress;
			activity.mGlobalPrefsSavingInProgress = globalPrefsSavingInProgress;
			activity.mOtherGlobalPrefsSavingInProgress = otherGlobalPrefsSavingInProgress;
			activity.mSelectedTab = selectedTab;
			activity.mCPUTimePreferences = cpuTimePreferences;
			activity.mNetTimePreferences = netTimePreferences;
		}
	}
	
	private String mWrongText;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		requestWindowFeature(Window.FEATURE_INDETERMINATE_PROGRESS);
		
		final SavedState savedState = (SavedState)getLastNonConfigurationInstance();
		if (savedState != null)
			savedState.restore(this);
		
		setUpService(true, true, true, false, false, false);
		
		super.onCreate(savedInstanceState);
		setContentView(R.layout.local_prefs);
		
		mWrongText = getString(R.string.localPrefWrong);
		
		mTabHost = (TabHost)findViewById(android.R.id.tabhost);
		
		mTabHost.setup();
		
		Resources res = getResources();
		
		TabHost.TabSpec tabSpec1 = mTabHost.newTabSpec("computeOptions");
		tabSpec1.setContent(R.id.localPrefComputeOptions);
		tabSpec1.setIndicator(getString(R.string.localPrefComputeOptions),
				res.getDrawable(R.drawable.ic_tab_compute));
		mTabHost.addTab(tabSpec1);
		
		TabHost.TabSpec tabSpec2 = mTabHost.newTabSpec("networkUsage");
		tabSpec2.setContent(R.id.localPrefNetworkOptions);
		tabSpec2.setIndicator(getString(R.string.localPrefNetworkUsage),
				res.getDrawable(R.drawable.ic_tab_network));
		mTabHost.addTab(tabSpec2);
		
		TabHost.TabSpec tabSpec3 = mTabHost.newTabSpec("diskUsage");
		tabSpec3.setContent(R.id.localPrefDiskRAMOptions);
		tabSpec3.setIndicator(getString(R.string.localPrefDiskRAMUsage),
				res.getDrawable(R.drawable.ic_tab_disk));
		mTabHost.addTab(tabSpec3);
		
		mTabHost.setCurrentTab(mSelectedTab);
		
		mComputeOnBatteries = (CheckBox)findViewById(R.id.localPrefComputeOnBatteries);
		mRunAlwaysWhenPlugged = (CheckBox)findViewById(R.id.localPrefRunAlwaysWhenPlugged);
		mComputeInUse = (CheckBox)findViewById(R.id.localPrefComputeInUse);
		mUseGPUInUse = (CheckBox)findViewById(R.id.localPrefUseGPUInUse);
		mComputeIdleFor = (EditText)findViewById(R.id.localPrefComputeIdleFor);
		mComputeUsageLessThan = (EditText)findViewById(R.id.localPrefComputeUsageLessThan);
		mBatteryLevelNL = (EditText)findViewById(R.id.localPrefBatteryNL);
		mBatteryTempLT = (EditText)findViewById(R.id.localPrefTempLT);
		mSwitchBetween = (EditText)findViewById(R.id.localPrefSwitchBetween);
		mUseAtMostCPUs = (EditText)findViewById(R.id.localPrefUseAtMostCPUs);
		mUseAtMostCPUTime = (EditText)findViewById(R.id.localPrefUseAtMostCPUTime);
		mXferOnlyWhenWifi = (CheckBox)findViewById(R.id.localPrefOnlyWhenWifi);
		mMaxDownloadRate = (EditText)findViewById(R.id.localPrefMaxDownloadRate);
		mMaxUploadRate = (EditText)findViewById(R.id.localPrefMaxUploadRate);
		mTransferAtMost = (EditText)findViewById(R.id.localPrefTransferAtMost);
		mTransferPeriodDays = (EditText)findViewById(R.id.localPrefTransferPeriodDays);
		mConnectAboutEvery = (EditText)findViewById(R.id.localPrefConnectAboutEvery);
		mAdditionalWorkBuffer = (EditText)findViewById(R.id.localPrefAdditWorkBuffer);
		mSkipVerifyImages = (CheckBox)findViewById(R.id.localPrefSkipImageVerify);
		mUseAtMostDiskSpace = (EditText)findViewById(R.id.localPrefUseAtMostDiskSpace);
		mLeaveAtLeastDiskFree = (EditText)findViewById(R.id.localPrefLeaveAtLeastDiskFree);
		mUseAtMostTotalDisk = (EditText)findViewById(R.id.localPrefUseAtMostTotalDisk);
		mCheckpointToDisk = (EditText)findViewById(R.id.localPrefCheckpointToDisk);
		mUseAtMostMemoryInUse = (EditText)findViewById(R.id.localPrefUseAtMostMemoryInUse);
		mUseAtMostMemoryInIdle = (EditText)findViewById(R.id.localPrefUseAtMostMemoryInIdle);
		mLeaveApplications = (CheckBox)findViewById(R.id.localPrefLeaveApplications);
		
		mApply = (Button)findViewById(R.id.localPrefApply);
		mApply.setEnabled(false);
		mApply.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				doApplyPreferences();
			}
		});
		
		mApplyDefault = (Button)findViewById(R.id.localPrefDefault);
		mApplyDefault.setOnClickListener(new View.OnClickListener() {
			
			@Override
			public void onClick(View v) {
				if (mConnectionManager == null)
					return;
				
				mGlobalPrefsSavingInProgress = true;
				mConnectionManager.setGlobalPrefsOverride(NativeBoincUtils.INITIAL_BOINC_CONFIG);
				setApplyButtonsEnabledAndCheckPreferences();
			}
		});
		
		Button cancel = (Button)findViewById(R.id.localPrefCancel);
		cancel.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				finish();
			}
		});
		
		TextWatcher textWatcher = new TextWatcher() {
			
			@Override
			public void onTextChanged(CharSequence s, int start, int before, int count) {
				setApplyButtonsEnabledAndCheckPreferences();
			}
			
			@Override
			public void beforeTextChanged(CharSequence s, int start, int count,
					int after) {
			}
			
			@Override
			public void afterTextChanged(Editable s) {
			}
		};
		
		// hide keyboard
		getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_HIDDEN);
		
		mComputeIdleFor.addTextChangedListener(textWatcher);
		mComputeUsageLessThan.addTextChangedListener(textWatcher);
		mBatteryLevelNL.addTextChangedListener(textWatcher);
		mBatteryTempLT.addTextChangedListener(textWatcher);
		
		mSwitchBetween.addTextChangedListener(textWatcher);
		mUseAtMostCPUs.addTextChangedListener(textWatcher);
		mUseAtMostCPUTime.addTextChangedListener(textWatcher);
		
		mMaxDownloadRate.addTextChangedListener(textWatcher);
		mMaxUploadRate.addTextChangedListener(textWatcher);
		mTransferAtMost.addTextChangedListener(textWatcher);
		mTransferPeriodDays.addTextChangedListener(textWatcher);
		mConnectAboutEvery.addTextChangedListener(textWatcher);
		mAdditionalWorkBuffer.addTextChangedListener(textWatcher);
		
		mUseAtMostDiskSpace.addTextChangedListener(textWatcher);
		mUseAtMostTotalDisk.addTextChangedListener(textWatcher);
		mLeaveAtLeastDiskFree.addTextChangedListener(textWatcher);
		mCheckpointToDisk.addTextChangedListener(textWatcher);
		
		mUseAtMostMemoryInIdle.addTextChangedListener(textWatcher);
		mUseAtMostMemoryInUse.addTextChangedListener(textWatcher);
		
		// buttons
		Button cpuTimePrefs = (Button)findViewById(R.id.localPrefCPUTimePrefs);
		cpuTimePrefs.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				if (mCPUTimePreferences == null)
					return;
				
				Intent intent = new Intent(LocalPreferencesActivity.this, TimePreferencesActivity.class);
				intent.putExtra(TimePreferencesActivity.ARG_TITLE, getString(R.string.cpuTimePrefTitle));
				intent.putExtra(TimePreferencesActivity.ARG_TIME_PREFS,
						new TimePrefsData(mCPUTimePreferences));
				startActivityForResult(intent, ACTIVITY_CPU_TIMES);
			}
		});
		
		Button netTimePrefs = (Button)findViewById(R.id.localPrefNetTimePrefs);
		netTimePrefs.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				if (mNetTimePreferences == null)
					return;
				Intent intent = new Intent(LocalPreferencesActivity.this, TimePreferencesActivity.class);
				intent.putExtra(TimePreferencesActivity.ARG_TITLE, getString(R.string.netTimePrefTitle));
				intent.putExtra(TimePreferencesActivity.ARG_TIME_PREFS,
						new TimePrefsData(mNetTimePreferences));
				startActivityForResult(intent, ACTIVITY_NET_TIMES);
			}
		});
	}
	
	private void updateActivityState() {
		// 
		setProgressBarIndeterminateVisibility(mConnectionManager.isWorking());
		
		if (mGlobalPrefsFetchProgress == ProgressState.IN_PROGRESS || mGlobalPrefsSavingInProgress) {
			mConnectionManager.handlePendingClientErrors(null, this);
			
			if (mConnectedClient == null) {
				clientDisconnected(mConnectionManager.isDisconnectedByManager()); // if disconnected
				return; // really to retur5n
			}
		}
		
		if (mGlobalPrefsFetchProgress != ProgressState.FINISHED) {
			if (mGlobalPrefsFetchProgress == ProgressState.NOT_RUN)
				// try do get global prefs working
				doGetGlobalPrefsWorking();
			else if (mGlobalPrefsFetchProgress == ProgressState.IN_PROGRESS) {
				GlobalPreferences globalPrefs = (GlobalPreferences)mConnectionManager
						.getPendingOutput(BoincOp.GlobalPrefsWorking);

				if (globalPrefs != null) {	// if finished
					mGlobalPrefsFetchProgress = ProgressState.FINISHED;
					updatePreferences(globalPrefs);
				}
			}
			// if failed
		}
		
		if (!mConnectionManager.isOpBeingExecuted(BoincOp.GlobalPrefsOverride)) {
			if (mGlobalPrefsSavingInProgress) {
				// its finally overriden
				onGlobalPreferencesChanged();
			} 
			mOtherGlobalPrefsSavingInProgress = false;
		} else if (!mGlobalPrefsSavingInProgress) // if other still working
			mOtherGlobalPrefsSavingInProgress = true;
		
		setApplyButtonsEnabledAndCheckPreferences();
	}
	
	@Override
	public Object onRetainNonConfigurationInstance() {
		mSelectedTab = mTabHost.getCurrentTab();
		return new SavedState(this);
	}
	
	@Override
	protected void onConnectionManagerConnected() {
		mConnectedClient = mConnectionManager.getClientId();
		
		if (mConnectedClient != null) {
			mApplyDefault.setEnabled(mConnectionManager.isNativeConnected());
			updateActivityState();
		}
	}
	
	@Override
	protected void onConnectionManagerDisconnected() {
		mConnectedClient = null;
	}
	
	
	@Override
	protected void onResume() {
		super.onResume();
		
		if (mConnectionManager != null) {
			mConnectedClient = mConnectionManager.getClientId();
			
			updateActivityState();
		}
	}
	
	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		if (resultCode == RESULT_OK) {
			if (requestCode == ACTIVITY_CPU_TIMES) {
				TimePrefsData timePrefsData = (TimePrefsData)data.getParcelableExtra(
						TimePreferencesActivity.RESULT_TIME_PREFS);
				mCPUTimePreferences = timePrefsData.timePrefs;
			} else if (requestCode == ACTIVITY_NET_TIMES) {
				TimePrefsData timePrefsData = (TimePrefsData)data.getParcelableExtra(
						TimePreferencesActivity.RESULT_TIME_PREFS);
				mNetTimePreferences = timePrefsData.timePrefs;
			}
		}
	}
	
	@Override
	public Dialog onCreateDialog(int dialogId, Bundle args) {
		return StandardDialogs.onCreateDialog(this, dialogId, args);
	}

	@Override
	public void onPrepareDialog(int dialogId, Dialog dialog, Bundle args) {
		StandardDialogs.onPrepareDialog(this, dialogId, dialog, args);
	}
	
	private boolean checkPreferences() {
		try {
			double idle_time_to_run = Double.parseDouble(mComputeIdleFor.getText().toString());
			double suspend_cpu_usage = Double.parseDouble(mComputeUsageLessThan.getText().toString());
			
			double cpu_scheduling_period_minutes = Double.parseDouble(
					mSwitchBetween.getText().toString());
			double max_ncpus_pct = Double.parseDouble(mUseAtMostCPUs.getText().toString());
			double cpu_usage_limit = Double.parseDouble(mUseAtMostCPUTime.getText().toString());
			double battery_level_nl = Double.parseDouble(mBatteryLevelNL.getText().toString());
			double batt_temp_lt = Double.parseDouble(mBatteryTempLT.getText().toString());
			
			double max_bytes_sec_down = Double.parseDouble(mMaxDownloadRate.getText().toString());
			double max_bytes_sec_up = Double.parseDouble(mMaxUploadRate.getText().toString());
			double daily_xfer_limit_mb = Double.parseDouble(mTransferAtMost.getText().toString());
			double daily_xfer_period_days = Integer.parseInt(mTransferPeriodDays.getText().toString());
			double work_buf_min_days = Double.parseDouble(mConnectAboutEvery.getText().toString());
			double work_buf_addit_days = Double.parseDouble(mAdditionalWorkBuffer.getText().toString());
			
			double disk_max_used_gb = Double.parseDouble(mUseAtMostDiskSpace.getText().toString());
			double disk_max_used_pct = Double.parseDouble(mUseAtMostTotalDisk.getText().toString());
			double disk_min_free_gb = Double.parseDouble(mLeaveAtLeastDiskFree.getText().toString());
			double disk_interval = Double.parseDouble(mCheckpointToDisk.getText().toString());
			
			double ram_max_used_busy_frac = Double.parseDouble(
					mUseAtMostMemoryInUse.getText().toString());
			double ram_max_used_idle_frac = Double.parseDouble(
					mUseAtMostMemoryInIdle.getText().toString());
			
			/* setup color */
			boolean good = true;
			if (idle_time_to_run < 0.0) {
				mComputeIdleFor.setError(mWrongText);
				good = false;
			} else
				mComputeIdleFor.setError(null);
			
			if (suspend_cpu_usage > 100.0 || suspend_cpu_usage < 0.0) {
				mComputeUsageLessThan.setError(mWrongText);
				good = false;
			} else
				mComputeUsageLessThan.setError(null);
			
			if (battery_level_nl < 0.0 || battery_level_nl > 100.0) {
				mBatteryLevelNL.setError(mWrongText);
				good = false;
			} else
				mBatteryLevelNL.setError(null);
			
			if (batt_temp_lt > 300.0) {
				mBatteryTempLT.setError(mWrongText);
				good = false;
			} else
				mBatteryTempLT.setError(null);
			
			if (cpu_scheduling_period_minutes < 0.0) {
				mSwitchBetween.setError(mWrongText);
				good = false;
			} else
				mSwitchBetween.setError(null);
			
			if (max_ncpus_pct > 100.0 || max_ncpus_pct < 0.0) {
				mUseAtMostCPUs.setError(mWrongText);
				good = false;
			} else
				mUseAtMostCPUs.setError(null);
			
			if (cpu_usage_limit > 100.0 || cpu_usage_limit < 0.0) {
				mUseAtMostCPUTime.setError(mWrongText);
				good =false;
			} else
				mUseAtMostCPUTime.setError(null);
			
			if (max_bytes_sec_down < 0.0) {
				mMaxDownloadRate.setError(mWrongText);
				good = false;
			} else
				mMaxDownloadRate.setError(null);
			
			if (max_bytes_sec_up < 0.0) {
				mMaxUploadRate.setError(mWrongText);
				good = false;
			} else
				mMaxUploadRate.setError(null);
			
			if (daily_xfer_limit_mb < 0.0) {
				mTransferAtMost.setError(mWrongText);
				good = false;
			} else
				mTransferAtMost.setError(null);
			
			if (daily_xfer_period_days < 0) {
				mTransferPeriodDays.setError(mWrongText);
				good = false;
			} else
				mTransferPeriodDays.setError(null);
			
			if (work_buf_min_days < 0.0) {
				mConnectAboutEvery.setError(mWrongText);
				good = false;
			} else
				mConnectAboutEvery.setError(null);
			
			if (work_buf_addit_days < 0.0) {
				mAdditionalWorkBuffer.setError(mWrongText);
				good = false;
			} else
				mAdditionalWorkBuffer.setError(null);
			
			if (disk_max_used_gb < 0.0) {
				mUseAtMostDiskSpace.setError(mWrongText);
				good = false;
			} else
				mUseAtMostDiskSpace.setError(null);
			
			if (disk_max_used_pct > 100.0 || disk_max_used_pct < 0.0) {
				mUseAtMostTotalDisk.setError(mWrongText);
				good = false;
			} else
				mUseAtMostTotalDisk.setError(null);
			
			if (disk_min_free_gb < 0.0) {
				mLeaveAtLeastDiskFree.setError(mWrongText);
				good = false;
			} else
				mLeaveAtLeastDiskFree.setError(null);
			
			if (disk_interval < 0.0) {
				mCheckpointToDisk.setError(mWrongText);
				good = false;
			} else
				mCheckpointToDisk.setError(null);
			
			if (ram_max_used_busy_frac > 100.0 || ram_max_used_busy_frac < 0.0) {
				mUseAtMostMemoryInUse.setError(mWrongText);
				good = false;
			} else
				mUseAtMostMemoryInUse.setError(null);
			
			if (ram_max_used_idle_frac > 100.0 || ram_max_used_idle_frac < 0.0) {
				mUseAtMostMemoryInIdle.setError(mWrongText);
				good = false;
			} else
				mUseAtMostMemoryInIdle.setError(null);
			
			//	/* if invalids values */
			if (Logging.DEBUG) Log.d(TAG, "Set up 'apply' as enabled:"+good);
			return good;
		} catch(NumberFormatException ex) {
			return false;
		}
	}
	
	public void setApplyButtonsEnabledAndCheckPreferences() {
		boolean doEnabled = mConnectedClient != null && !mOtherGlobalPrefsSavingInProgress &&
				!mGlobalPrefsSavingInProgress &&
				(mGlobalPrefsFetchProgress != ProgressState.FAILED ||
						mGlobalPrefsFetchProgress != ProgressState.FINISHED);
		
		mApplyDefault.setEnabled((mConnectedClient != null && mConnectedClient.isNativeClient()) && doEnabled);
		mApply.setEnabled(checkPreferences() && doEnabled);
	}
	
	private static final String formatDouble(double d) {
		String out = String.format(Locale.US, "%.6f", d);
		
		int cutted;
		int length = out.length();
		for (cutted = 1; cutted < 6; cutted++)
			if (out.charAt(length-cutted) != '0')
				break;
				
		return out.substring(0, length-cutted+1);
	}
	
	private void updatePreferences(GlobalPreferences globalPrefs) {
		mComputeInUse.setChecked(globalPrefs.run_if_user_active);
		mRunAlwaysWhenPlugged.setChecked(globalPrefs.run_always_when_plugged);
		mComputeOnBatteries.setChecked(globalPrefs.run_on_batteries);
		mUseGPUInUse.setChecked(globalPrefs.run_gpu_if_user_active);
		
		mComputeIdleFor.setText(formatDouble(globalPrefs.idle_time_to_run));
		mComputeUsageLessThan.setText(formatDouble(globalPrefs.suspend_cpu_usage));
		mBatteryLevelNL.setText(formatDouble(globalPrefs.run_if_battery_nl_than));
		mBatteryTempLT.setText(formatDouble(globalPrefs.run_if_temp_lt_than));
		
		mSwitchBetween.setText(formatDouble(globalPrefs.cpu_scheduling_period_minutes));
		mUseAtMostCPUs.setText(formatDouble(globalPrefs.max_ncpus_pct));
		mUseAtMostCPUTime.setText(formatDouble(globalPrefs.cpu_usage_limit));
		
		mXferOnlyWhenWifi.setChecked(globalPrefs.xfer_only_when_wifi);
		mMaxDownloadRate.setText(formatDouble(globalPrefs.max_bytes_sec_down));
		mMaxUploadRate.setText(formatDouble(globalPrefs.max_bytes_sec_up));
		mTransferAtMost.setText(formatDouble(globalPrefs.daily_xfer_limit_mb));
		mTransferPeriodDays.setText(Integer.toString(globalPrefs.daily_xfer_period_days));
		mConnectAboutEvery.setText(formatDouble(globalPrefs.work_buf_min_days));
		mAdditionalWorkBuffer.setText(formatDouble(globalPrefs.work_buf_additional_days));
		mSkipVerifyImages.setChecked(globalPrefs.dont_verify_images);
		
		mUseAtMostDiskSpace.setText(formatDouble(globalPrefs.disk_max_used_gb));
		mUseAtMostTotalDisk.setText(formatDouble(globalPrefs.disk_max_used_pct));
		mLeaveAtLeastDiskFree.setText(formatDouble(globalPrefs.disk_min_free_gb));
		mCheckpointToDisk.setText(formatDouble(globalPrefs.disk_interval));
		
		mUseAtMostMemoryInIdle.setText(formatDouble(globalPrefs.ram_max_used_idle_frac));
		mUseAtMostMemoryInUse.setText(formatDouble(globalPrefs.ram_max_used_busy_frac));
		mLeaveApplications.setChecked(globalPrefs.leave_apps_in_memory);
		
		mCPUTimePreferences = globalPrefs.cpu_times;
		mNetTimePreferences = globalPrefs.net_times;
		
		setApplyButtonsEnabledAndCheckPreferences();
	}

	private void doApplyPreferences() {
		if (mConnectionManager == null)
			return;
		
		GlobalPreferences globalPrefs = new GlobalPreferences();
		
		try {
			globalPrefs.run_if_user_active = mComputeInUse.isChecked();
			globalPrefs.run_on_batteries = mComputeOnBatteries.isChecked();
			globalPrefs.run_always_when_plugged = mRunAlwaysWhenPlugged.isChecked();
			globalPrefs.run_gpu_if_user_active = mUseGPUInUse.isChecked();
			
			globalPrefs.idle_time_to_run = Double.parseDouble(mComputeIdleFor.getText().toString());
			globalPrefs.suspend_cpu_usage = Double.parseDouble(
					mComputeUsageLessThan.getText().toString());
			globalPrefs.run_if_battery_nl_than = Double.parseDouble(
					mBatteryLevelNL.getText().toString());
			globalPrefs.run_if_temp_lt_than = Double.parseDouble(
					mBatteryTempLT.getText().toString());
			
			globalPrefs.cpu_scheduling_period_minutes = Double.parseDouble(
					mSwitchBetween.getText().toString());
			globalPrefs.max_ncpus_pct = Double.parseDouble(mUseAtMostCPUs.getText().toString());
			globalPrefs.cpu_usage_limit = Double.parseDouble(mUseAtMostCPUTime.getText().toString());
			
			globalPrefs.xfer_only_when_wifi = mXferOnlyWhenWifi.isChecked();
			globalPrefs.max_bytes_sec_down = Double.parseDouble(mMaxDownloadRate.getText().toString());
			globalPrefs.max_bytes_sec_up = Double.parseDouble(mMaxUploadRate.getText().toString());
			globalPrefs.daily_xfer_limit_mb = Double.parseDouble(mTransferAtMost.getText().toString());
			globalPrefs.daily_xfer_period_days = Integer.parseInt(
					mTransferPeriodDays.getText().toString());
			globalPrefs.work_buf_min_days = Double.parseDouble(mConnectAboutEvery.getText().toString());
			globalPrefs.work_buf_additional_days = Double.parseDouble(
					mAdditionalWorkBuffer.getText().toString());
			globalPrefs.dont_verify_images = mSkipVerifyImages.isChecked();
			
			globalPrefs.disk_max_used_gb = Double.parseDouble(mUseAtMostDiskSpace.getText().toString());
			globalPrefs.disk_max_used_pct = Double.parseDouble(mUseAtMostTotalDisk.getText().toString());
			globalPrefs.disk_min_free_gb = Double.parseDouble(mLeaveAtLeastDiskFree.getText().toString());
			globalPrefs.disk_interval = Double.parseDouble(mCheckpointToDisk.getText().toString());
			
			globalPrefs.ram_max_used_busy_frac = Double.parseDouble(
					mUseAtMostMemoryInUse.getText().toString());
			globalPrefs.ram_max_used_idle_frac = Double.parseDouble(
					mUseAtMostMemoryInIdle.getText().toString());
			globalPrefs.leave_apps_in_memory = mLeaveApplications.isChecked();
			
			globalPrefs.cpu_times = mCPUTimePreferences;
			globalPrefs.net_times = mNetTimePreferences;
		} catch(NumberFormatException ex) {
			return;	// do nothing
		}
		mGlobalPrefsSavingInProgress = true;
		mConnectionManager.setGlobalPrefsOverrideStruct(globalPrefs);
		setApplyButtonsEnabledAndCheckPreferences();
	}
	
	@Override
	public void clientConnectionProgress(BoincOp boincOp, int progress) {
		// do nothing
	}

	@Override
	public void clientConnected(VersionInfo clientVersion) {
		mConnectedClient = mConnectionManager.getClientId();
	}

	private void doGetGlobalPrefsWorking() {
		if (Logging.DEBUG) Log.d(TAG, "get get global prefs");
		if (mConnectionManager.getGlobalPrefsWorking())
			mGlobalPrefsFetchProgress = ProgressState.IN_PROGRESS;
	}
	
	@Override
	public void clientDisconnected(boolean disconnectedByManager) {
		if (Logging.DEBUG) Log.d(TAG, "Disconnected!!!");
		mGlobalPrefsFetchProgress = ProgressState.FAILED;
		mGlobalPrefsSavingInProgress = false;
		mOtherGlobalPrefsSavingInProgress = false;
		ClientId disconnectedHost = mConnectedClient;
		mConnectedClient = null; // used by setApply..
		setApplyButtonsEnabledAndCheckPreferences();
		StandardDialogs.tryShowDisconnectedErrorDialog(this, mConnectionManager, null, disconnectedHost,
				disconnectedByManager);
	}
	
	@Override
	public void currentGlobalPreferences(GlobalPreferences globalPrefs) {
		if (Logging.DEBUG) Log.d(TAG, "on current global preferences:"+mGlobalPrefsFetchProgress);
		if (mGlobalPrefsFetchProgress == ProgressState.NOT_RUN)
			doGetGlobalPrefsWorking();
		if (mGlobalPrefsFetchProgress == ProgressState.IN_PROGRESS) {
			mGlobalPrefsFetchProgress = ProgressState.FINISHED;
			updatePreferences(globalPrefs);
		}
	}
	
	@Override
	public void onGlobalPreferencesChanged() {
		if (mOtherGlobalPrefsSavingInProgress) {
			mOtherGlobalPrefsSavingInProgress = false;
		} else {
			mGlobalPrefsSavingInProgress = false;
			// finish
			finish();
		}
	}

	@Override
	public boolean clientError(BoincOp boincOp, int errorNum, String errorMessage) {
		if (!boincOp.equals(BoincOp.GlobalPrefsWorking) && !boincOp.equals(BoincOp.GlobalPrefsOverride))
			return false;
		
		if (Logging.DEBUG) Log.d(TAG, "on error for "+boincOp);
		
		if (boincOp.equals(BoincOp.GlobalPrefsWorking)) {
			if (mGlobalPrefsFetchProgress == ProgressState.NOT_RUN)
				// if previous try to get our prefs working
				doGetGlobalPrefsWorking();
			else if (mGlobalPrefsFetchProgress == ProgressState.IN_PROGRESS)
				mGlobalPrefsFetchProgress = ProgressState.FAILED;
		}
		
		if (boincOp.equals(BoincOp.GlobalPrefsOverride)) {
			mGlobalPrefsSavingInProgress = false;
			mOtherGlobalPrefsSavingInProgress = false;
		}
		
		setApplyButtonsEnabledAndCheckPreferences();
		
		StandardDialogs.showClientErrorDialog(this, errorNum, errorMessage);
		return true;
	}

	@Override
	public void onClientIsWorking(boolean isWorking) {
		setProgressBarIndeterminateVisibility(isWorking);
	}
}
