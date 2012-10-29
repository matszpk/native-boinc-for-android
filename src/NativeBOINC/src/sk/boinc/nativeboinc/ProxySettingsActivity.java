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

import edu.berkeley.boinc.lite.ProxyInfo;
import sk.boinc.nativeboinc.clientconnection.BoincOp;
import sk.boinc.nativeboinc.clientconnection.ClientProxyReceiver;
import sk.boinc.nativeboinc.clientconnection.VersionInfo;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.util.ClientId;
import sk.boinc.nativeboinc.util.ProgressState;
import sk.boinc.nativeboinc.util.StandardDialogs;
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
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.TabHost;

/**
 * @author mat
 *
 */
public class ProxySettingsActivity extends ServiceBoincActivity implements ClientProxyReceiver {
	private static final String TAG = "ProxySettingsActivity";
	
	private ClientId mConnectedClient = null;
	
	private TabHost mTabHost;
	
	private int mProxySettingsFetchProgress = ProgressState.NOT_RUN;
	private boolean mProxySettingsSavingInProgress = false;
	private boolean mOtherProxySettingsSavingInProgress = false;

	private Button mApply;
	
	private CheckBox mUseHttpProxy;
	private EditText mHttpProxyAddress;
	private EditText mHttpProxyPort;
	private EditText mHttpProxyUser;
	private EditText mHttpProxyPassword;
	private EditText mHttpNoForHosts;

	private CheckBox mUseSocksProxy;
	private EditText mSocksProxyAddress;
	private EditText mSocksProxyPort;
	private EditText mSocksProxyUser;
	private EditText mSocksProxyPassword;
	private EditText mSocksNoForHosts;

	private int mSelectedTab = 0;

	private static class SavedState {
		private final int proxySettingsFetchProgress;
		private final boolean proxySettingsSavingInProgress;
		private final boolean otherProxySettingsSavingInProgress;
		private int selectedTab;
		
		public SavedState(ProxySettingsActivity activity) {
			proxySettingsFetchProgress = activity.mProxySettingsFetchProgress;
			proxySettingsSavingInProgress = activity.mProxySettingsSavingInProgress;
			otherProxySettingsSavingInProgress = activity.mOtherProxySettingsSavingInProgress;
			selectedTab = activity.mSelectedTab;
		}
		
		public void restore(ProxySettingsActivity activity) {
			activity.mProxySettingsFetchProgress = proxySettingsFetchProgress;
			activity.mProxySettingsSavingInProgress = proxySettingsSavingInProgress;
			activity.mOtherProxySettingsSavingInProgress = otherProxySettingsSavingInProgress;
			activity.mSelectedTab = selectedTab;
		}
	}
	
	private boolean mDontChangeNoProxy = false;
	
	@Override
	public void onCreate(Bundle savedInstanceState) {
		requestWindowFeature(Window.FEATURE_INDETERMINATE_PROGRESS);
		
		final SavedState savedState = (SavedState)getLastNonConfigurationInstance();
		if (savedState != null)
			savedState.restore(this);
		
		setUpService(true, true, true, false, false, false);
		
		super.onCreate(savedInstanceState);
		setContentView(R.layout.proxy_settings);
		
		mTabHost = (TabHost)findViewById(android.R.id.tabhost);
		
		mTabHost.setup();
		
		Resources res = getResources();
		
		TabHost.TabSpec tabSpec1 = mTabHost.newTabSpec("computeOptions");
		tabSpec1.setContent(R.id.proxyHttpServer);
		tabSpec1.setIndicator(getString(R.string.localPrefComputeOptions),
				res.getDrawable(R.drawable.ic_tab_network));
		mTabHost.addTab(tabSpec1);
		
		TabHost.TabSpec tabSpec2 = mTabHost.newTabSpec("networkUsage");
		tabSpec2.setContent(R.id.proxySocksServer);
		tabSpec2.setIndicator(getString(R.string.localPrefNetworkUsage),
				res.getDrawable(R.drawable.ic_tab_network));
		mTabHost.addTab(tabSpec2);
		
		mTabHost.setCurrentTab(mSelectedTab);
		
		mUseHttpProxy = (CheckBox)findViewById(R.id.proxyUseHttpProxy);
		mHttpProxyAddress = (EditText)findViewById(R.id.proxyHttpServerAddress);
		mHttpProxyPort = (EditText)findViewById(R.id.proxyHttpServerPort);
		mHttpProxyUser = (EditText)findViewById(R.id.proxyHttpServerUser);
		mHttpProxyPassword = (EditText)findViewById(R.id.proxyHttpServerPassword);
		mHttpNoForHosts = (EditText)findViewById(R.id.proxyHttpNoForHosts);
		mUseSocksProxy = (CheckBox)findViewById(R.id.proxyUseSocksProxy);
		mSocksProxyAddress = (EditText)findViewById(R.id.proxySocksServerAddress);
		mSocksProxyPort = (EditText)findViewById(R.id.proxySocksServerPort);
		mSocksProxyUser = (EditText)findViewById(R.id.proxySocksServerUser);
		mSocksProxyPassword = (EditText)findViewById(R.id.proxySocksServerPassword);
		mSocksNoForHosts = (EditText)findViewById(R.id.proxySocksNoForHosts);
		
		mUseHttpProxy.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
			@Override
			public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
				updateEditsEnabled();
			}
		});
		mUseSocksProxy.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
			@Override
			public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
				updateEditsEnabled();
			}
		});
		
		mApply = (Button)findViewById(R.id.proxyApply);
		mApply.setEnabled(false);
		mApply.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				doApplySettings();
			}
		});
		
		Button cancel = (Button)findViewById(R.id.proxyCancel);
		cancel.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				finish();
			}
		});
		
		TextWatcher httpNoProxyWatcher = new TextWatcher() {
			@Override
			public void onTextChanged(CharSequence s, int start, int before, int count) {
				if (!mDontChangeNoProxy) {
					mDontChangeNoProxy = true;
					mSocksNoForHosts.setText(mHttpNoForHosts.getText());
				} else
					mDontChangeNoProxy = false;
			}
			
			@Override
			public void beforeTextChanged(CharSequence s, int start, int count,
					int after) {
			}
			
			@Override
			public void afterTextChanged(Editable s) {
			}
		};
		
		TextWatcher socksNoProxyWatcher = new TextWatcher() {
			@Override
			public void onTextChanged(CharSequence s, int start, int before, int count) {
				if (!mDontChangeNoProxy) {
					mDontChangeNoProxy = true;
					mHttpNoForHosts.setText(mSocksNoForHosts.getText());
				} else
					mDontChangeNoProxy = false;
			}
			
			@Override
			public void beforeTextChanged(CharSequence s, int start, int count,
					int after) {
			}
			
			@Override
			public void afterTextChanged(Editable s) {
			}
		};
		
		mHttpNoForHosts.addTextChangedListener(httpNoProxyWatcher);
		mSocksNoForHosts.addTextChangedListener(socksNoProxyWatcher);
		
		// hide keyboard
		getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_HIDDEN);
		
		updateEditsEnabled();
	}
	
	private void updateActivityState() {
		// 
		setProgressBarIndeterminateVisibility(mConnectionManager.isWorking());
		
		if (mProxySettingsFetchProgress == ProgressState.IN_PROGRESS || mProxySettingsSavingInProgress) {
			mConnectionManager.handlePendingClientErrors(null, this);
			
			if (mConnectedClient == null) {
				clientDisconnected(mConnectionManager.isDisconnectedByManager()); // if disconnected
				return; // really to retur5n
			}
		}
		
		if (mProxySettingsFetchProgress != ProgressState.FINISHED) {
			if (mProxySettingsFetchProgress == ProgressState.NOT_RUN)
				// try do get global prefs working
				doGetProxySettings();
			else if (mProxySettingsFetchProgress == ProgressState.IN_PROGRESS) {
				ProxyInfo proxyInfo = (ProxyInfo)mConnectionManager
						.getPendingOutput(BoincOp.GetProxySettings);
				
				if (proxyInfo != null) {	// if finished
					mProxySettingsFetchProgress = ProgressState.FINISHED;
					updateProxySettings(proxyInfo);
				}
			}
			// if failed
		}
		
		if (!mConnectionManager.isOpBeingExecuted(BoincOp.SetProxySettings)) {
			if (mProxySettingsSavingInProgress) {
				// its finally overriden
				onProxySettingsChanged();
			} 
			mOtherProxySettingsSavingInProgress = false;
		} else if (!mProxySettingsSavingInProgress) // if other still working
			mOtherProxySettingsSavingInProgress = true;
		
		setApplyButtonEnabled();
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
	
	private void updateEditsEnabled() {
		boolean enabled = mUseHttpProxy.isChecked();
		mHttpProxyAddress.setEnabled(enabled);
		mHttpProxyPort.setEnabled(enabled);
		mHttpProxyUser.setEnabled(enabled);
		mHttpProxyPassword.setEnabled(enabled);
		mHttpNoForHosts.setEnabled(enabled);
		
		enabled = mUseSocksProxy.isChecked();
		mSocksProxyAddress.setEnabled(enabled);
		mSocksProxyPort.setEnabled(enabled);
		mSocksProxyUser.setEnabled(enabled);
		mSocksProxyPassword.setEnabled(enabled);
		mSocksNoForHosts.setEnabled(enabled);
	}
	
	private void doApplySettings() {
		if (mConnectionManager == null)
			return;
		
		ProxyInfo proxyInfo = new ProxyInfo();
		
		try {
			proxyInfo.use_http_auth = true;
			proxyInfo.use_http_proxy = mUseHttpProxy.isChecked();
			proxyInfo.http_server_name = mHttpProxyAddress.getText().toString();
			proxyInfo.http_server_port = Integer.parseInt(mHttpProxyPort.getText().toString());
			proxyInfo.http_user_name = mHttpProxyUser.getText().toString();
			proxyInfo.http_user_passwd = mHttpProxyPassword.getText().toString();
			
			proxyInfo.use_socks_proxy = mUseSocksProxy.isChecked();
			proxyInfo.socks_server_name = mSocksProxyAddress.getText().toString();
			proxyInfo.socks_server_port = Integer.parseInt(mSocksProxyPort.getText().toString());
			proxyInfo.socks5_user_name = mSocksProxyUser.getText().toString();
			proxyInfo.socks5_user_passwd = mSocksProxyPassword.getText().toString();
			
			proxyInfo.noproxy_hosts = mHttpNoForHosts.getText().toString();
		} catch(NumberFormatException ex) {
			return;	// do nothing
		}
		
		mProxySettingsSavingInProgress = true;
		mConnectionManager.setProxySettings(proxyInfo);
		setApplyButtonEnabled();
	}
	
	private void updateProxySettings(ProxyInfo proxyInfo) {
		mUseHttpProxy.setChecked(proxyInfo.use_http_proxy);
		mHttpProxyAddress.setText(proxyInfo.http_server_name);
		mHttpProxyPort.setText(Integer.toString(proxyInfo.http_server_port));
		mHttpProxyUser.setText(proxyInfo.http_user_name);
		mHttpProxyPassword.setText(proxyInfo.http_user_passwd);
		mHttpNoForHosts.setText(proxyInfo.noproxy_hosts);
		
		mUseSocksProxy.setChecked(proxyInfo.use_socks_proxy);
		mSocksProxyAddress.setText(proxyInfo.socks_server_name);
		mSocksProxyPort.setText(Integer.toString(proxyInfo.socks_server_port));
		mSocksProxyUser.setText(proxyInfo.socks5_user_name);
		mSocksProxyPassword.setText(proxyInfo.socks5_user_passwd);
		mSocksNoForHosts.setText(proxyInfo.noproxy_hosts);
		
		updateEditsEnabled();
		setApplyButtonEnabled();
	}
	
	private void setApplyButtonEnabled() {
		boolean doEnabled = mConnectedClient != null && !mOtherProxySettingsSavingInProgress &&
				!mProxySettingsSavingInProgress &&
				(mProxySettingsFetchProgress == ProgressState.FAILED ||
						mProxySettingsFetchProgress == ProgressState.FINISHED);
		mApply.setEnabled(doEnabled);
	}

	private void doGetProxySettings() {
		if (Logging.DEBUG) Log.d(TAG, "get proxy settings");
		if (mConnectionManager.getProxySettings())
			mProxySettingsFetchProgress = ProgressState.IN_PROGRESS;
	}
	
	@Override
	public void clientConnectionProgress(BoincOp boincOp, int progress) {
	}

	@Override
	public void clientConnected(VersionInfo clientVersion) {
		mConnectedClient = mConnectionManager.getClientId();
	}

	@Override
	public void clientDisconnected(boolean disconnectedByManager) {
		if (Logging.DEBUG) Log.d(TAG, "Disconnected!!!");
		mProxySettingsFetchProgress = ProgressState.FAILED;
		mProxySettingsSavingInProgress = false;
		mOtherProxySettingsSavingInProgress = false;
		ClientId disconnectedHost = mConnectedClient;
		mConnectedClient = null; // used by setApply..
		setApplyButtonEnabled();
		StandardDialogs.tryShowDisconnectedErrorDialog(this, mConnectionManager, null, disconnectedHost,
				disconnectedByManager);
	}
	
	@Override
	public boolean clientError(BoincOp boincOp, int errorNum, String errorMessage) {
		if (!boincOp.equals(BoincOp.GetProxySettings) && !boincOp.equals(BoincOp.SetProxySettings))
			return false;
		
		if (Logging.DEBUG) Log.d(TAG, "on error for "+boincOp);
		
		if (boincOp.equals(BoincOp.GetProxySettings)) {
			if (mProxySettingsFetchProgress == ProgressState.NOT_RUN)
				// if previous try to get our prefs working
				doGetProxySettings();
			else if (mProxySettingsFetchProgress == ProgressState.IN_PROGRESS)
				mProxySettingsFetchProgress = ProgressState.FAILED;
		}
		
		if (boincOp.equals(BoincOp.SetProxySettings)) {
			mProxySettingsSavingInProgress = false;
			mOtherProxySettingsSavingInProgress = false;
		}
		
		setApplyButtonEnabled();
		
		StandardDialogs.showClientErrorDialog(this, errorNum, errorMessage);
		return true;
	}

	@Override
	public void onClientIsWorking(boolean isWorking) {
		setProgressBarIndeterminateVisibility(isWorking);
	}

	@Override
	public void currentProxySettings(ProxyInfo proxyInfo) { 
		if (Logging.DEBUG) Log.d(TAG, "on current proxy settings:"+mProxySettingsFetchProgress);
		if (mProxySettingsFetchProgress == ProgressState.NOT_RUN)
			doGetProxySettings();
		if (mProxySettingsFetchProgress == ProgressState.IN_PROGRESS) {
			mProxySettingsFetchProgress = ProgressState.FINISHED;
			updateProxySettings(proxyInfo);
		}
	}

	@Override
	public void onProxySettingsChanged() {
		if (mOtherProxySettingsSavingInProgress) {
			mOtherProxySettingsSavingInProgress = false;
		} else {
			mProxySettingsSavingInProgress = false;
			// finish
			finish();
		}
	}
}
