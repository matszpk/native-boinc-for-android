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

import sk.boinc.nativeboinc.clientconnection.ClientReceiver;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.installer.AbstractInstallerListener;
import sk.boinc.nativeboinc.installer.InstallerService;
import sk.boinc.nativeboinc.nativeclient.AbstractNativeBoincListener;
import sk.boinc.nativeboinc.nativeclient.NativeBoincService;
import sk.boinc.nativeboinc.service.ConnectionManagerService;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.util.Log;

/**
 * @author mat
 *
 */
public class ServiceBoincActivity extends AbstractBoincActivity {

	private static final String TAG = "ServiceBoincActivity";
	
	protected ConnectionManagerService mConnectionManager = null;
	protected NativeBoincService mRunner = null;
	protected InstallerService mInstaller = null;
	
	private boolean mBindToConnManager = false;
	private boolean mRegisterConnManagerListener = false;
	private boolean mBindToRunner = false;
	private boolean mRegisterRunnerListener = false;
	private boolean mBindToInstaller = false;
	private boolean mRegisterInstallerListener = false;
	
	private boolean mDelayedObserverRegistration = false;
	private boolean mDelayedInstallerListenerRegistration = false;
	private boolean mDelayedRunnerListenerRegistration = false;
		
	protected ServiceConnection mConnectionServiceConnection = new ServiceConnection() {
		@Override
		public void onServiceConnected(ComponentName name, IBinder service) {
			mConnectionManager = ((ConnectionManagerService.LocalBinder)service).getService();
			if (mRegisterConnManagerListener && mDelayedObserverRegistration) {
				if (Logging.DEBUG) Log.d(TAG, "Delayed register status observer");
				
				mConnectionManager.registerStatusObserver((ClientReceiver)ServiceBoincActivity.this);
				mDelayedObserverRegistration = false;
			}
			onConnectionManagerConnected();
			if (Logging.DEBUG) Log.d(TAG, "onServiceConnected()");
		}

		@Override
		public void onServiceDisconnected(ComponentName name) {
			if (mRegisterConnManagerListener && mConnectionManager != null)
				mConnectionManager.unregisterStatusObserver((ClientReceiver)ServiceBoincActivity.this);
			mConnectionManager = null;
			// This should not happen normally, because it's local service 
			// running in the same process...
			if (Logging.WARNING) Log.w(TAG, "onServiceDisconnected()");
			
			// We also reset client reference to prevent mess
			onConnectionManagerDisconnected();
		}
	};
	
	protected void onConnectionManagerConnected() {
		// do nothing
	}
	
	protected void onConnectionManagerDisconnected() {
		// do nothing
	}
	
	private ServiceConnection mInstallerServiceConnection = new ServiceConnection() {
		@Override
		public void onServiceConnected(ComponentName name, IBinder service) {
			mInstaller = ((InstallerService.LocalBinder)service).getService();
			if (mRegisterInstallerListener && mDelayedInstallerListenerRegistration) {
				if (Logging.DEBUG) Log.d(TAG, "Delayed register installer listener");
				
				mInstaller.addInstallerListener((AbstractInstallerListener)ServiceBoincActivity.this);
				mDelayedInstallerListenerRegistration = false;
			}
			if (Logging.DEBUG) Log.d(TAG, "installer.onServiceConnected()");
			onInstallerConnected();
		}

		@Override
		public void onServiceDisconnected(ComponentName name) {
			if (mRegisterInstallerListener && mInstaller != null)
				mInstaller.removeInstallerListener((AbstractInstallerListener)ServiceBoincActivity.this);
			mInstaller = null;
			if (Logging.DEBUG) Log.d(TAG, "installer.onServiceDisconnected()");
			onInstallerDisconnected();
		}
	};
	
	protected void onInstallerConnected() {
		// do nothing
	}
	
	protected void onInstallerDisconnected() {
		// do nothing
	}
	
	private ServiceConnection mRunnerServiceConnection = new ServiceConnection() {
		@Override
		public void onServiceConnected(ComponentName name, IBinder service) {
			mRunner = ((NativeBoincService.LocalBinder)service).getService();
			if (mRegisterRunnerListener && mDelayedRunnerListenerRegistration) {
				if (Logging.DEBUG) Log.d(TAG, "Delayed register runner listener");
					
				mRunner.addNativeBoincListener((AbstractNativeBoincListener)ServiceBoincActivity.this);
				mDelayedRunnerListenerRegistration = false;
			}
			if (Logging.DEBUG) Log.d(TAG, "runner.onServiceConnected()");
			onRunnerConnected();
		}
		
		@Override
		public void onServiceDisconnected(ComponentName name) {
			if (mRegisterRunnerListener && mRunner != null)
				mRunner.removeNativeBoincListener((AbstractNativeBoincListener)ServiceBoincActivity.this);
			mRunner = null;
			if (Logging.DEBUG) Log.d(TAG, "runner.onServiceDisconnected()");
			onRunnerDisconnected();
		}
	};
	
	protected void onRunnerConnected() {
		// do nothing
	}
	
	protected void onRunnerDisconnected() {
		// do nothing
	}
	
	protected void doBindConnectionManagerService() {
		bindService(new Intent(this, ConnectionManagerService.class),
				mConnectionServiceConnection, Context.BIND_AUTO_CREATE);
	}
	
	private void doBindInstallerService() {
		if (Logging.DEBUG) Log.d(TAG, "doBindService()");
		bindService(new Intent(this, InstallerService.class),
				mInstallerServiceConnection, Context.BIND_AUTO_CREATE);
	}
	
	private void doBindRunnerService() {
		bindService(new Intent(this, NativeBoincService.class),
				mRunnerServiceConnection, Context.BIND_AUTO_CREATE);
	}
	
	private void doUnbindConnectionManagerService() {
		unbindService(mConnectionServiceConnection);
		mConnectionManager = null;
	}
	
	private void doUnbindInstallerService() {
		if (Logging.DEBUG) Log.d(TAG, "doUnbindInstallerService()");
		unbindService(mInstallerServiceConnection);
		mInstaller = null;
	}

	private void doUnbindRunnerService() {
		unbindService(mRunnerServiceConnection);
		mRunner = null;
	}
	
	protected void setUpService(boolean bindToConnManager, boolean registerConnManagerListener,
			boolean bindToRunner, boolean registerRunnerListener,
			boolean bindToInstaller, boolean registerInstallerListener) {
		mBindToConnManager = bindToConnManager;
		mRegisterConnManagerListener = registerConnManagerListener;
		mBindToRunner = bindToRunner;
		mRegisterRunnerListener = registerRunnerListener;
		mBindToInstaller = bindToInstaller;
		mRegisterInstallerListener = registerInstallerListener;
	}
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		if (mBindToConnManager)
			doBindConnectionManagerService();
		if (mBindToInstaller)
			doBindInstallerService();
		if (mBindToRunner)
			doBindRunnerService();
	}
	
	@Override
	protected void onPause() {
		super.onPause();
		if (mConnectionManager != null && mRegisterConnManagerListener) {
			if (Logging.DEBUG) Log.d(TAG, "Unregister status observer");
			mConnectionManager.unregisterStatusObserver((ClientReceiver)this);
			mDelayedObserverRegistration = false;
		}
		if (mInstaller != null && mRegisterInstallerListener) {
			if (Logging.DEBUG) Log.d(TAG, "Unregister installer listener");
			mInstaller.removeInstallerListener((AbstractInstallerListener)this);
			mDelayedInstallerListenerRegistration = false;
		}
		if (mRunner != null && mRegisterRunnerListener) {
			if (Logging.DEBUG) Log.d(TAG, "Unregister runner listener");
			mRunner.removeNativeBoincListener((AbstractNativeBoincListener)this);
			mDelayedRunnerListenerRegistration = false;
		}
	}
	
	@Override
	protected void onResume() {
		super.onResume();
		if (mRegisterConnManagerListener) {
			if (mConnectionManager != null) {
				if (Logging.DEBUG) Log.d(TAG, "Normal register status observer");
				mConnectionManager.registerStatusObserver((ClientReceiver)this);
				mDelayedObserverRegistration = false;
			} else
				mDelayedObserverRegistration = true;
		}
		if (mRegisterInstallerListener) {
			if (mInstaller != null) {
				if (Logging.DEBUG) Log.d(TAG, "Normal register installer listener");
				mInstaller.addInstallerListener((AbstractInstallerListener)this);
				mDelayedInstallerListenerRegistration = false;
			} else
				mDelayedInstallerListenerRegistration = true;
		}
		if (mRegisterRunnerListener) {
			if (mRunner != null) {
				if (Logging.DEBUG) Log.d(TAG, "Normal register runner listener");
				mRunner.addNativeBoincListener((AbstractNativeBoincListener)this);
				mDelayedRunnerListenerRegistration = false;
			} else
				mDelayedRunnerListenerRegistration = true;
		}
	}
	
	@Override
	protected void onDestroy() {
		if (mBindToConnManager)
			doUnbindConnectionManagerService();
		if (mBindToInstaller)
			doUnbindInstallerService();
		if (mBindToRunner)
			doUnbindRunnerService();
		super.onDestroy();
	}
}
