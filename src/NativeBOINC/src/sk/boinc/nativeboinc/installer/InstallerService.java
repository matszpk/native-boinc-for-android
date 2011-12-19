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

package sk.boinc.nativeboinc.nativeclient;

import java.util.ArrayList;
import java.util.List;
import java.util.Vector;

import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.util.UpdateItem;

import android.app.Service;
import android.content.Intent;
import android.os.Binder;
import android.os.ConditionVariable;
import android.os.Handler;
import android.os.IBinder;
import android.util.Log;

/**
 * @author mat
 *
 */
public class InstallerService extends Service {
	private static final String TAG = "InstallerService";
	
	private InstallerThread mInstallerThread = null;
	private InstallerHandler mInstallerHandler = null;
	
	private List<InstallerListener> mListeners = new ArrayList<InstallerListener>();
	
	public class LocalBinder extends Binder {
		public InstallerService getService() {
			return InstallerService.this;
		}
	}
	
	private LocalBinder mBinder = new LocalBinder();
	
	@Override
	public void onCreate() {
		mListenerHandler = new ListenerHandler();
		startInstaller();
	}
	
	@Override
	public void onDestroy() {
		stopInstaller();
	}
	
	@Override
	public IBinder onBind(Intent intent) {
		startService(new Intent(this, InstallerService.class));
		return mBinder;
	}
	
	public class ListenerHandler extends Handler implements InstallerListener {
		@Override
		public void onOperation(String opDescription) {
			for (InstallerListener listener: mListeners)
				listener.onOperation(opDescription);
		}
		
		@Override
		public void onOperationProgress(String opDescription, int progress) {
			for (InstallerListener listener: mListeners)
				listener.onOperationProgress(opDescription, progress);
		}

		@Override
		public void onOperationError(String errorMessage) {
			for (InstallerListener listener: mListeners)
				listener.onOperationError(errorMessage);
		}
		
		@Override
		public void onOperationCancel() {
			for (InstallerListener listener: mListeners)
				listener.onOperationCancel();
		}
		
		@Override
		public void onOperationFinish() {
			for (InstallerListener listener: mListeners)
				listener.onOperationFinish();
		}
		
		@Override
		public void currentProjectDistribList(Vector<ProjectDistrib> projectDistribs) {
			for (InstallerListener listener: mListeners)
				listener.currentProjectDistribList(projectDistribs);
		}
		
		@Override
		public void currentClientDistrib(ClientDistrib clientDistrib) {
			for (InstallerListener listener: mListeners)
				listener.currentClientDistrib(clientDistrib);
		}
	}
	
	private ListenerHandler mListenerHandler = null;
	
	public void addInstallerListener(InstallerListener listener) {
		mListeners.add(listener);
	}
	
	public void removeInstallerListener(InstallerListener listener) {
		mListeners.remove(listener);
	}
	
	private void startInstaller() {
		if (mInstallerThread != null)
			return;	// do nothing
		
		if (Logging.DEBUG) Log.d(TAG, "Starting up Installer");
		ConditionVariable lock = new ConditionVariable(false);
		mInstallerThread = new InstallerThread(lock, this, mListenerHandler);
		mInstallerThread.start();
		boolean runningOk = lock.block(2000); // Locking until new thread fully runs
		if (!runningOk) {
			if (Logging.ERROR) Log.e(TAG, "InstallerThread did not start in 1 second");
			throw new RuntimeException("Worker thread cannot start");
		}
		if (Logging.DEBUG) Log.d(TAG, "InstallerThread started successfully");
		mInstallerHandler = mInstallerThread.getInstallerHandler();
	}
	
	public void stopInstaller() {
		mInstallerThread.stopThread();
		mInstallerThread = null;
		mListeners.clear();
		mListenerHandler = null;
	}

	/**
	 * Install client automatically (with signature checking)
	 */
	public void installClientAutomatically() {
		mInstallerThread.installClientAutomatically();
	}
	
	public void updateClientDistribList() {
		mInstallerThread.updateClientDistribList();
	}
	
	/**
	 * Installs BOINC application automatically (with signature checking)
	 * @param projectUrl
	 * @param appName
	 */
	public void installBoincApplicationAutomatically(String projectUrl) {
		mInstallerThread.installBoincApplicationAutomatically(projectUrl);
	}
	
	/**
	 * Reinstalls update item 
	 */
	public void reinstallUpdateItem(UpdateItem updateItem) {
		mInstallerThread.reinstallUpdateItem(updateItem);
	}
	
	public void updateProjectDistribList() {
		mInstallerThread.updateProjectDistribList();
	}
	
	/* remove detached projects from installed projects */
	public void synchronizeInstalledProjects() {
		mInstallerHandler.synchronizeInstalledProjects();
	}
	
	public void cancelOperation() {
		mInstallerHandler.cancelOperation();
	}
	
	/**
	 * Check whether client is installed
	 * @return
	 */
	public boolean isClientInstalled() {
		return mInstallerHandler.isClientInstalled();
	}
	
	/**
	 * 
	 * @return installed binaries info
	 */
	public InstalledBinary[] getInstalledBinaries() {
		return mInstallerHandler.getInstalledBinaries();
	}
	
	/**
	 * @param attachProjectUrls attached projects urls list
	 * @return update items
	 */
	public UpdateItem[] getBinariesToUpdateOrInstall(String[] attachedProjectUrls) {
		return mInstallerHandler.getBinariesToUpdateOrInstall(attachedProjectUrls);
	}
}
