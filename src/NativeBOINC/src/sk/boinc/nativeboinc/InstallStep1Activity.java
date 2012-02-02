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
import sk.boinc.nativeboinc.installer.InstallerProgressListener;
import sk.boinc.nativeboinc.installer.InstallerService;
import sk.boinc.nativeboinc.installer.InstallerUpdateListener;
import sk.boinc.nativeboinc.installer.ProjectDistrib;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
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

	private final static String STATE_CLIENT_DISTRIB = "ClientDistrib";
	
	private TextView mVersionToInstall = null;

	private BoincManagerApplication mApp = null;

	private final static int DIALOG_INFO = 1;
	private final static int DIALOG_ERROR = 2;
	private final static String ARG_ERROR = "Error";

	private boolean mDelayedInstallClient = false;

	// if true, then client distrib shouldnot be updated
	private ClientDistrib mClientDistrib = null;

	private Button mInfoButton = null;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		requestWindowFeature(Window.FEATURE_INDETERMINATE_PROGRESS);

		// starting service - prevents breaking on recreating activities
		startService(new Intent(this, InstallerService.class));

		setUpService(false, false, false, false, true, true);
		super.onCreate(savedInstanceState);
		setContentView(R.layout.install_step1);

		mVersionToInstall = (TextView) findViewById(R.id.versionToInstall);

		Button nextButton = (Button) findViewById(R.id.installNext);
		Button cancelButton = (Button) findViewById(R.id.installCancel);

		mInfoButton = (Button) findViewById(R.id.clientInfo);

		if (savedInstanceState != null)
			mClientDistrib = savedInstanceState.getParcelable(STATE_CLIENT_DISTRIB);
		
		if (mClientDistrib != null) {
			mVersionToInstall.setText(getString(R.string.versionToInstall) + ": " +
					mClientDistrib.version);
	
			mInfoButton.setEnabled(true);
		} else
			mVersionToInstall.setText(getString(R.string.versionToInstall) + ": ...");

		mApp = (BoincManagerApplication) getApplication();
		mApp.installerIsRun(true);	// set as run

		nextButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				if (mInstaller != null)
					installClient();
				else
					// if installer not connected with activity
					mDelayedInstallClient = true;
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
		if (mClientDistrib == null && mInstaller != null)
			mInstaller.updateClientDistrib();
	}
	
	@Override
	protected void onSaveInstanceState(Bundle outState) {
		if (mClientDistrib != null)
			outState.putParcelable(STATE_CLIENT_DISTRIB, mClientDistrib);
		super.onSaveInstanceState(outState);
	}

	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		if (keyCode == KeyEvent.KEYCODE_BACK) {
			mApp.installerIsRun(false);
			finish();
			return true; 
		}
		return super.onKeyDown(keyCode, event);
	}

	@Override
	protected void onInstallerConnected() {
		mVersionToInstall.post(new Runnable() {
			@Override
			public void run() {
				if (mClientDistrib == null)
					// already not fetched
					mInstaller.updateClientDistrib();
			}
		});
	}

	@Override
	public Dialog onCreateDialog(int dialogId, Bundle args) {
		if (dialogId == DIALOG_ERROR)
			return new AlertDialog.Builder(this)
					.setIcon(android.R.drawable.ic_dialog_alert)
					.setTitle(R.string.installError)
					.setView(
							LayoutInflater.from(this).inflate(R.layout.dialog,
									null)).setNegativeButton(R.string.ok, null)
					.create();
		else if (dialogId == DIALOG_INFO) {
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
		if (dialogId == DIALOG_ERROR) {
			TextView textView = (TextView) dialog.findViewById(R.id.dialogText);
			textView.setText(args.getString(ARG_ERROR));
		} else if (dialogId == DIALOG_INFO) {
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
		// if progress
		setProgressBarIndeterminateVisibility(true);
	}

	@Override
	public void onOperationProgress(String distribName, String opDescription,
			int progress) {
		// if progress
		setProgressBarIndeterminateVisibility(true);
	}

	@Override
	public void onOperationError(String distribName, String errorMessage) {
		Bundle args = new Bundle();
		args.putString(ARG_ERROR, errorMessage);
		showDialog(DIALOG_ERROR, args);
		setProgressBarIndeterminateVisibility(false);
	}

	@Override
	public void onOperationCancel(String distribName) {
		setProgressBarIndeterminateVisibility(false);
	}

	@Override
	public void onOperationFinish(String distribName) {
		setProgressBarIndeterminateVisibility(false);
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

		setProgressBarIndeterminateVisibility(false);
		mVersionToInstall.setText(getString(R.string.versionToInstall) + ": " +
				clientDistrib.version);

		mInfoButton.setEnabled(true);

		/* if delayed install client (if not connected service) */
		if (mDelayedInstallClient)
			installClient();
	}

	private void installClient() {
		mInstaller.installClientAutomatically();
		finish();
		Intent intent = new Intent(this, ProgressActivity.class);
		intent.putExtra(ProgressActivity.ARG_GOTO_ACTIVITY,
				ProgressActivity.GOTO_INSTALL_STEP_2);
		startActivity(intent);
	}
}
