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
import sk.boinc.nativeboinc.installer.InstallOp;
import sk.boinc.nativeboinc.installer.InstallerService;
import sk.boinc.nativeboinc.installer.InstallerUpdateListener;
import sk.boinc.nativeboinc.installer.ProjectDistrib;
import sk.boinc.nativeboinc.util.ProgressState;
import sk.boinc.nativeboinc.util.StandardDialogs;
import sk.boinc.nativeboinc.util.UpdateItem;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.Window;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.Spinner;
import android.widget.TextView;

/**
 * @author mat
 * 
 */
public class InstallStep1Activity extends ServiceBoincActivity implements InstallerUpdateListener {
	private final static String TAG = "InstallStep1Activity";

	private TextView mVersionToInstall = null;

	private BoincManagerApplication mApp = null;
	
	private boolean mReUpdateClientDistrib = false;
	// if true, then client distrib shouldnot be updated
	private ClientDistrib mClientDistrib = null;
	private int mClientDistribProgressState = ProgressState.NOT_RUN; 
	
	private Button mInfoButton = null;
	private Button mNextButton = null;
	private CheckBox mSelectOlder = null;
	
	private Spinner mInstallPlaceSpinner = null;
	
	private static final int DIALOG_INSTALL_CANCEL = 1;
	
	@Override
	public int getInstallerChannelId() {
		return InstallerService.DEFAULT_CHANNEL_ID;
	}
	
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
		mInfoButton = (Button)findViewById(R.id.clientInfo);
		
		mInstallPlaceSpinner = (Spinner)findViewById(R.id.placeToInstall);

		Button cancelButton = (Button)findViewById(R.id.installCancel);
		
		mSelectOlder = (CheckBox)findViewById(R.id.selectOlderVersion);
		mSelectOlder.setChecked(BoincManagerApplication.isClientOlderVersion(this));
		
		if (mClientDistrib == null)
			mVersionToInstall.setText(getString(R.string.versionToInstall) + ": ...");
		
		mApp = (BoincManagerApplication) getApplication();
		mApp.setInstallerStage(BoincManagerApplication.INSTALLER_CLIENT_STAGE);	// set as run
		
		mSelectOlder.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
			@Override
			public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
				// update client older version
				BoincManagerApplication.setClientOlderVersion(InstallStep1Activity.this, isChecked);
				
				if (mInstaller != null) {
					reUpdateClientDistrib();
				}
				else
					mReUpdateClientDistrib = true;
			}
		});
		
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
				showDialog(DIALOG_INSTALL_CANCEL);
			}
		});

		mInfoButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				StandardDialogs.showDistribInfoDialog(InstallStep1Activity.this,
						InstallerService.BOINC_CLIENT_ITEM_NAME, mClientDistrib.version,
						mClientDistrib.description, mClientDistrib.changes);
			}
		});
	}
	
	private void reUpdateClientDistrib() {
		mVersionToInstall.setText(R.string.versionToInstall);

		mSelectOlder.setEnabled(false);
		mInfoButton.setEnabled(false);
		mNextButton.setEnabled(false);
		
		mClientDistrib = null;
		if (mClientDistribProgressState == ProgressState.IN_PROGRESS)
			mInstaller.cancelSimpleOperation();
		mClientDistribProgressState = ProgressState.IN_PROGRESS;
		// ignore if doesnt run, because we reusing previous result
		mInstaller.updateClientDistrib();
	}

	@Override
	protected void onResume() {
		super.onResume();
		if (mInstaller != null)
			updateActivityState();
	}
	
	@Override
	public void onBackPressed() {
		if (mInstaller != null)
			mInstaller.cancelSimpleOperation();
		finish();
	}
	
	@Override
	public Object onRetainNonConfigurationInstance() {
		return new SavedState(this);
	}
	
	private void updateClientVersionText() {
		mVersionToInstall.setText(getString(R.string.versionToInstall) + ": " +
				mClientDistrib.version);
		mSelectOlder.setEnabled(true);

		mInfoButton.setEnabled(true);
		mNextButton.setEnabled(true);
	}
	
	private void updateActivityState() {
		setProgressBarIndeterminateVisibility(mInstaller.isWorking());
		
		if (mInstaller.handlePendingErrors(InstallOp.UpdateClientDistrib, this))
			return;
		
		if (mClientDistrib == null) {
			if (mClientDistribProgressState == ProgressState.IN_PROGRESS) {
				mClientDistrib = (ClientDistrib)mInstaller.getPendingOutput(InstallOp.UpdateClientDistrib);
				
				if (mClientDistrib != null)
					updateClientVersionText();
			} else if (mClientDistribProgressState == ProgressState.NOT_RUN) {
				mClientDistribProgressState = ProgressState.IN_PROGRESS;
				// ignore if doesnt run, because we reusing previous result
				mInstaller.updateClientDistrib();
			}
			// if finished but failed
		} else 
			updateClientVersionText();
	}
	
	@Override
	protected void onInstallerConnected() {
		if (mReUpdateClientDistrib) {
			mReUpdateClientDistrib = false;
			reUpdateClientDistrib();
		}
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
		
		if (dialogId == DIALOG_INSTALL_CANCEL) {
			return new AlertDialog.Builder(this)
				.setIcon(android.R.drawable.ic_dialog_alert)
				.setTitle(R.string.warning)
				.setMessage(R.string.installBoincCancelQuestion)
				.setPositiveButton(R.string.yesText, new DialogInterface.OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which) {
						mApp.setNoBoincInstallation();
						mApp.setInstallerStage(BoincManagerApplication.INSTALLER_FINISH_STAGE);
						// finish activity
						onBackPressed();
					}
				})
				.setNegativeButton(R.string.noText, new DialogInterface.OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which) {
						// finish activity
						onBackPressed();
					}
				})
				.create();
		}
		return null;
	}

	@Override
	public void onPrepareDialog(int dialogId, Dialog dialog, Bundle args) {
		StandardDialogs.onPrepareDialog(this, dialogId, dialog, args);
	}
	
	@Override
	public boolean onOperationError(InstallOp installOp, String distribName, String errorMessage) {
		if (installOp.equals(InstallOp.UpdateClientDistrib) &&
				mClientDistribProgressState == ProgressState.IN_PROGRESS) {
			mClientDistribProgressState = ProgressState.FAILED;
			StandardDialogs.showInstallErrorDialog(this, distribName, errorMessage);
			return true;
		}
		return false;
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
		if (mInstaller == null)
			return;
		
		mApp.setInstallerStage(BoincManagerApplication.INSTALLER_CLIENT_INSTALLING_STAGE);
		
		if (mInstallPlaceSpinner.getSelectedItemPosition() == 0) //: first internal memory
			BoincManagerApplication.setBoincPlace(this, false);
		else
			BoincManagerApplication.setBoincPlace(this, true);
		
		mInstaller.installClientAutomatically();
		finish();
		
		startActivity(new Intent(this, ProgressActivity.class));
	}

	@Override
	public void onChangeInstallerIsWorking(boolean isWorking) {
		setProgressBarIndeterminateVisibility(isWorking);
	}

	@Override
	public void binariesToUpdateOrInstall(UpdateItem[] updateItems) {
		
	}

	@Override
	public void binariesToUpdateFromSDCard(String[] projectNames) {
		
	}
}
