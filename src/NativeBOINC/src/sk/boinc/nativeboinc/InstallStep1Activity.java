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

import java.util.ArrayList;

import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.installer.ClientDistrib;
import sk.boinc.nativeboinc.installer.InstallError;
import sk.boinc.nativeboinc.installer.InstallerProgressListener;
import sk.boinc.nativeboinc.installer.InstallerUpdateListener;
import sk.boinc.nativeboinc.installer.ProjectDistrib;
import sk.boinc.nativeboinc.util.ProgressState;
import sk.boinc.nativeboinc.util.StandardDialogs;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.Window;
import android.widget.Button;
import android.widget.TextView;

/**
 * @author mat
 * 
 */
public class InstallStep1Activity extends ServiceBoincActivity implements
		InstallerProgressListener, InstallerUpdateListener {
	private final static String TAG = "InstallStep1Activity";

	private TextView mVersionToInstall = null;

	private BoincManagerApplication mApp = null;

	private final static int DIALOG_INFO = 1;
	
	// if true, then client distrib shouldnot be updated
	private ClientDistrib mClientDistrib = null;
	private int mClientDistribProgressState = ProgressState.NOT_RUN;

	private Button mInfoButton = null;
	private Button mNextButton = null;
	
	private static class SavedState {
		private final ClientDistrib mClientDistrib;
		private final int mClientDistribProgressState;
		
		public SavedState(InstallStep1Activity activity) {
			mClientDistrib = activity.mClientDistrib;
			mClientDistribProgressState = activity.mClientDistribProgressState;
		}
		
		public void restore(InstallStep1Activity activity) {
			activity.mClientDistrib = mClientDistrib;
			activity.mClientDistribProgressState = mClientDistribProgressState;
		}
	}
	
	@Override
	public void onCreate(Bundle savedInstanceState) {
		requestWindowFeature(Window.FEATURE_INDETERMINATE_PROGRESS);

		SavedState savedState = (SavedState)getLastNonConfigurationInstance();
		if (savedState != null)
			savedState.restore(this);
		
		setUpService(false, false, false, false, true, true);
		super.onCreate(savedInstanceState);
		setContentView(R.layout.install_step1);

		mVersionToInstall = (TextView)findViewById(R.id.versionToInstall);
		mNextButton = (Button)findViewById(R.id.installNext);
		mInfoButton = (Button) findViewById(R.id.clientInfo);

		Button cancelButton = (Button)findViewById(R.id.installCancel);
		
		if (mClientDistrib == null)
			mVersionToInstall.setText(getString(R.string.versionToInstall) + ": ...");

		mApp = (BoincManagerApplication) getApplication();
		mApp.setInstallerStage(BoincManagerApplication.INSTALLER_CLIENT_STAGE);	// set as run

		mNextButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				if (mInstaller != null)
					installClient();
			}
		});
		cancelButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				finish();
			}
		});

		mInfoButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				showDialog(DIALOG_INFO);
			}
		});
	}

	@Override
	protected void onResume() {
		super.onResume();
		if (mInstaller != null)
			updateActivityState();
	}
	
	@Override
	public Object onRetainNonConfigurationInstance() {
		return new SavedState(this);
	}
	
	private void updateClientVersionText() {
		mVersionToInstall.setText(getString(R.string.versionToInstall) + ": " +
				mClientDistrib.version);

		mInfoButton.setEnabled(true);
		mNextButton.setEnabled(true);
	}
	
	private void updateActivityState() {
		setProgressBarIndeterminateVisibility(mInstaller.isWorking());
		
		InstallError installError = mInstaller.getPendingError();
		if (installError != null && mClientDistribProgressState == ProgressState.IN_PROGRESS) {
			handleInstallError(installError.distribName, installError.errorMessage);
			return;
		}
		
		if (mClientDistrib == null) {
			if (mClientDistribProgressState == ProgressState.IN_PROGRESS) {
				mClientDistrib = mInstaller.getPendingClientDistrib();
				
				if (mClientDistrib != null)
					updateClientVersionText();
			} else if (mClientDistribProgressState == ProgressState.NOT_RUN) {
				mClientDistribProgressState = ProgressState.IN_PROGRESS;
				mInstaller.updateClientDistrib();
			}
			// if finished but failed
		} else 
			updateClientVersionText();
	}
	
	@Override
	protected void onInstallerConnected() {
		updateActivityState();
	}
	
	@Override
	protected void onInstallerDisconnected() {
		setProgressBarIndeterminateVisibility(false);
	}
	
	@Override
	public Dialog onCreateDialog(int dialogId, Bundle args) {
		Dialog dialog = StandardDialogs.onCreateDialog(this, dialogId, args);
		if (dialog != null)
			return dialog;
		
		if (dialogId == DIALOG_INFO) {
			if (mClientDistrib == null)
				return null;
			LayoutInflater inflater = LayoutInflater.from(this);
			View view = inflater.inflate(R.layout.distrib_info, null);

			return new AlertDialog.Builder(this)
					.setIcon(android.R.drawable.ic_dialog_info)
					.setNegativeButton(R.string.dismiss, null)
					.setTitle(R.string.clientInfo).setView(view).create();
		}
		return null;
	}

	@Override
	public void onPrepareDialog(int dialogId, Dialog dialog, Bundle args) {
		if (StandardDialogs.onPrepareDialog(this, dialogId, dialog, args))
			return;
		
		if (dialogId == DIALOG_INFO) {
			TextView clientVersion = (TextView) dialog
					.findViewById(R.id.distribVersion);
			TextView clientDesc = (TextView) dialog
					.findViewById(R.id.distribDesc);
			TextView clientChanges = (TextView) dialog
					.findViewById(R.id.distribChanges);

			/* setup client info texts */
			clientVersion.setText(getString(R.string.clientVersion) + ": "
					+ mClientDistrib.version);
			clientDesc.setText(getString(R.string.description) + ":\n"
					+ mClientDistrib.description);
			clientChanges.setText(getString(R.string.changes) + ":\n"
					+ mClientDistrib.changes);
		}
	}

	@Override
	public void onOperation(String distribName, String opDescription) {
	}

	@Override
	public void onOperationProgress(String distribName, String opDescription,
			int progress) {
		// if progress
	}

	private void handleInstallError(String distribName, String errorMessage) {
		mClientDistribProgressState = ProgressState.FAILED;
		StandardDialogs.showInstallErrorDialog(this, distribName, errorMessage);
	}
	
	@Override
	public void onOperationError(String distribName, String errorMessage) {
		handleInstallError(distribName, errorMessage);
	}

	@Override
	public void onOperationCancel(String distribName) {
		mClientDistribProgressState = ProgressState.FAILED;
	}

	@Override
	public void onOperationFinish(String distribName) {
	}

	@Override
	public void currentProjectDistribList(ArrayList<ProjectDistrib> projectDistribs) {
		// do nothing
	}

	@Override
	public void currentClientDistrib(ClientDistrib clientDistrib) {
		if (Logging.DEBUG)
			Log.d(TAG, "clientDistrib");

		mClientDistrib = clientDistrib;
		mClientDistribProgressState = ProgressState.FINISHED;
		updateClientVersionText();
	}

	private void installClient() {
		mApp.setInstallerStage(BoincManagerApplication.INSTALLER_CLIENT_INSTALLING_STAGE);
		mInstaller.installClientAutomatically();
		finish();
		startActivity(new Intent(this, ProgressActivity.class));
	}

	@Override
	public void onInstallerWorking(boolean isWorking) {
		setProgressBarIndeterminateVisibility(isWorking);
	}
}
