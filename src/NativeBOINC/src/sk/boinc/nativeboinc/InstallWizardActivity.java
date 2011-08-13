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

import java.io.IOException;
import java.util.Vector;

import edu.berkeley.boinc.lite.AccountIn;
import edu.berkeley.boinc.lite.AccountMgrInfo;
import edu.berkeley.boinc.lite.ProjectConfig;
import edu.berkeley.boinc.lite.ProjectListEntry;

import sk.boinc.nativeboinc.R.id;
import sk.boinc.nativeboinc.clientconnection.ClientManageReceiver;
import sk.boinc.nativeboinc.clientconnection.HostInfo;
import sk.boinc.nativeboinc.clientconnection.MessageInfo;
import sk.boinc.nativeboinc.clientconnection.ModeInfo;
import sk.boinc.nativeboinc.clientconnection.NoConnectivityException;
import sk.boinc.nativeboinc.clientconnection.ProjectInfo;
import sk.boinc.nativeboinc.clientconnection.TaskInfo;
import sk.boinc.nativeboinc.clientconnection.TransferInfo;
import sk.boinc.nativeboinc.clientconnection.VersionInfo;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.nativeclient.ClientDistrib;
import sk.boinc.nativeboinc.nativeclient.CpuType;
import sk.boinc.nativeboinc.nativeclient.InstallerListener;
import sk.boinc.nativeboinc.nativeclient.InstallerService;
import sk.boinc.nativeboinc.nativeclient.NativeBoincListener;
import sk.boinc.nativeboinc.nativeclient.NativeBoincService;
import sk.boinc.nativeboinc.nativeclient.ProjectDistrib;
import sk.boinc.nativeboinc.service.ConnectionManagerService;
import sk.boinc.nativeboinc.util.AddProjectResult;
import sk.boinc.nativeboinc.util.ClientId;
import sk.boinc.nativeboinc.util.HostListDbAdapter;
import sk.boinc.nativeboinc.util.ProjectConfigOptions;
import sk.boinc.nativeboinc.util.ProjectItem;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.Window;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

/**
 * @author mat
 *
 */
public class InstallWizardActivity extends Activity implements InstallerListener,
		NativeBoincListener, ClientManageReceiver {
	private final static String TAG = "InstallWizardActivity";
	
	private static final int ACTIVITY_SELECT_PROJECT = 1;
	private static final int ACTIVITY_ADD_PROJECT = 2;
	
	private InstallerService mInstaller = null;
	private NativeBoincService mRunner = null;
	private ConnectionManagerService mConnectionManager = null;
	
	private final static int INSTALL_STEP_1 = 1;
	private final static int INSTALL_STEP_2 = 2;
	private final static int INSTALL_STEP_3 = 3;
	private final static int INSTALL_FINISH_STEP = 4;
	private final static int INSTALL_FINISH_BUT_STEP = 5;
	
	private int mCurrentStep = 0;
	
	private Button mNext;
	private Button mFinish;
	private Button mCancel;
	private LinearLayout mInstallStep1;
	private LinearLayout mInstallStep2;
	private LinearLayout mInstallStep3;
	private LinearLayout mInstallFinishStep;
	private TextView mCurrentAccessPassword;
	private EditText mAccessPassword;
	
	private String mProjectUrl = null;
	
	private ProgressDialog mProgressDialog = null;
	
	private HostListDbAdapter mHostListDbHelper = null;
	private ClientId mClientId = null;
	
	private String mAccessPasswordString = null;
	
	private BoincManagerApplication mApp = null;
	
	private ServiceConnection mConnectionServiceConnection = new ServiceConnection() {
		@Override
		public void onServiceConnected(ComponentName name, IBinder service) {
			mConnectionManager = ((ConnectionManagerService.LocalBinder)service).getService();
			if (Logging.DEBUG) Log.d(TAG, "onServiceConnected()");
			mConnectionManager.registerStatusObserver(InstallWizardActivity.this);
		}

		@Override
		public void onServiceDisconnected(ComponentName name) {
			mConnectionManager = null;
			// This should not happen normally, because it's local service 
			// running in the same process...
			if (Logging.WARNING) Log.w(TAG, "onServiceDisconnected()");
			// We also reset client reference to prevent mess
		}
	};
	
	private ServiceConnection mInstallerServiceConnection = new ServiceConnection() {
		@Override
		public void onServiceConnected(ComponentName name, IBinder service) {
			mInstaller = ((InstallerService.LocalBinder)service).getService();
			if (Logging.DEBUG) Log.d(TAG, "installer.onServiceConnected()");
			mInstaller.addInstallerListener(InstallWizardActivity.this);
			
			/* set up cpu type text */
			TextView cpuType = (TextView)findViewById(R.id.installCPUType);
			cpuType.setText(getString(R.string.cpuType) + ": " + CpuType.getCpuDisplayName(getResources(),
					mInstaller.detectCPUType()));
		}

		@Override
		public void onServiceDisconnected(ComponentName name) {
			mInstaller = null;
			if (Logging.DEBUG) Log.d(TAG, "installer.onServiceDisconnected()");
		}
	};
	
	private ServiceConnection mRunnerServiceConnection = new ServiceConnection() {
		@Override
		public void onServiceConnected(ComponentName name, IBinder service) {
			mRunner = ((NativeBoincService.LocalBinder)service).getService();
			if (Logging.DEBUG) Log.d(TAG, "runner.onServiceConnected()");
			mRunner.addNativeBoincListener(InstallWizardActivity.this);
		}
		
		@Override
		public void onServiceDisconnected(ComponentName name) {
			mRunner = null;
			if (Logging.DEBUG) Log.d(TAG, "runner.onServiceDisconnected()");
		}
	};
	
	private void doBindConnectionManagerService() {
		bindService(new Intent(InstallWizardActivity.this, ConnectionManagerService.class),
				mConnectionServiceConnection, Context.BIND_AUTO_CREATE);
	}
	
	private void doUnbindConnectionManagerService() {
		unbindService(mConnectionServiceConnection);
		mConnectionManager = null;
	}
	
	private void doBindInstallerService() {
		bindService(new Intent(InstallWizardActivity.this, InstallerService.class),
				mInstallerServiceConnection, Context.BIND_AUTO_CREATE);
	}
	
	private void doUnbindInstallerService() {
		unbindService(mInstallerServiceConnection);
		mInstaller = null;
	}
	
	private void doBindRunnerService() {
		bindService(new Intent(InstallWizardActivity.this, NativeBoincService.class),
				mRunnerServiceConnection, Context.BIND_AUTO_CREATE);
	}
	
	private void doUnbindRunnerService() {
		unbindService(mRunnerServiceConnection);
		mRunner = null;
	}
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		requestWindowFeature(Window.FEATURE_INDETERMINATE_PROGRESS);
		super.onCreate(savedInstanceState);
		
		mApp = (BoincManagerApplication)getApplication();
		
		setContentView(R.layout.install_activity);
		
		mCurrentStep = INSTALL_STEP_1;
		
		mHostListDbHelper = new HostListDbAdapter(this);
		
		doBindInstallerService();
		doBindRunnerService();
		doBindConnectionManagerService();
		
		mInstallStep1 = (LinearLayout)findViewById(R.id.installStep1);
		mInstallStep2 = (LinearLayout)findViewById(R.id.installStep2);
		mInstallStep3 = (LinearLayout)findViewById(R.id.installStep3);
		mInstallFinishStep = (LinearLayout)findViewById(R.id.installFinishStep);
		mNext = (Button)findViewById(R.id.installNext);
		mFinish = (Button)findViewById(R.id.installFinish);
		
		mCancel = (Button)findViewById(R.id.installCancel);
		
		mCancel.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				mInstaller.cancelOperation();
				finish();
			}
		});
		
		mCurrentAccessPassword = (TextView)findViewById(id.installCurrentAccessPassword);
		mAccessPassword = (EditText)findViewById(R.id.installNewPassword);
		
		/* callbacks */
		mNext.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				switch(mCurrentStep) {
				case INSTALL_STEP_1:
					doInstallStep1();
					break;
				case INSTALL_STEP_2:
					doInstallStep2();
					break;
				case INSTALL_STEP_3:
					doInstallStep3();
					break;
				}
			}
		});
		
		mFinish.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				finish();
			}
		});
	}
	
	@Override
	protected void onResume() {
		super.onResume();
		if (mInstaller == null)
			doBindInstallerService();
		if (mRunner == null)
			doBindRunnerService();
		if (mConnectionManager == null)
			doBindConnectionManagerService();
	}

	@Override
	protected void onDestroy() {
		mHostListDbHelper = null;
		super.onDestroy();
		if (mConnectionManager != null) {
			mConnectionManager.unregisterStatusObserver(this);
			mConnectionManager = null;
		}
			
		if (mRunner != null) {
			mRunner.removeNativeBoincListener(InstallWizardActivity.this);
			mRunner = null;
		}
		
		if (mInstaller != null) {
			mInstaller.removeInstallerListener(InstallWizardActivity.this);
			mInstaller = null;
		}
		
		doUnbindInstallerService();
		doUnbindRunnerService();
		doUnbindConnectionManagerService();
		
		mApp.installerIsRun(true);
	}
	
	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		if (Logging.DEBUG) Log.d(TAG, "onActivityResult()");
		
		switch (requestCode) {
		case ACTIVITY_SELECT_PROJECT:
			if (resultCode == RESULT_OK) {
				ProjectItem project = data.getParcelableExtra(ProjectItem.TAG);
				/* contact with project */
				mProjectUrl = project.getUrl();
				boincProjectConfig(project.getUrl());
			} else
				finish();
			break;
		case ACTIVITY_ADD_PROJECT:
			if (resultCode == RESULT_OK) {
				AddProjectResult result = data.getParcelableExtra(AddProjectResult.TAG);
				// adding project
				if (result.isCreateAccount()) {
					boincCreateAccount(result.getName(), result.getEmail(), result.getPassword());
				} else {	// lookup account
					boincLookupAccount(result.getEmail(), result.getPassword());
				}
			}
		}
	}

	/*
	 * aaaaaaaaaaaa
	 */
	
	private void prepareView() {
		if (mCurrentStep == INSTALL_STEP_1)
			mInstallStep1.setVisibility(View.VISIBLE);
		else
			mInstallStep1.setVisibility(View.GONE);
		
		if (mCurrentStep == INSTALL_STEP_2)
			mInstallStep2.setVisibility(View.VISIBLE);
		else
			mInstallStep2.setVisibility(View.GONE);
		
		if (mCurrentStep == INSTALL_STEP_3)
			mInstallStep3.setVisibility(View.VISIBLE);
		else
			mInstallStep3.setVisibility(View.GONE);
		
		if (mCurrentStep == INSTALL_STEP_2) {
			mCurrentAccessPassword.setText(getString(R.string.installCurrentPassword) +
					": " + mAccessPasswordString);
			mAccessPassword.setText("");
		}
		
		if (mCurrentStep == INSTALL_FINISH_STEP || mCurrentStep == INSTALL_FINISH_BUT_STEP) {
			TextView installFinished = (TextView)findViewById(R.id.installFinishText);
			
			if (mCurrentStep == INSTALL_FINISH_BUT_STEP)	/* without project attachment */
				installFinished.setText(getString(R.string.installFinishedBut));
			
			mInstallFinishStep.setVisibility(View.VISIBLE);
			mCancel.setEnabled(false);
			mFinish.setEnabled(true);
			mNext.setEnabled(false);
		} else {
			mInstallFinishStep.setVisibility(View.GONE);
			mFinish.setEnabled(false);
			mNext.setEnabled(true);
		}
	}
	
	/* installation steps */
	
	private void doInstallStep1() {
		mNext.setEnabled(false);
		
		mInstaller.updateClientDistribList();
	}
	
	private void doInstallStep2() {
		mNext.setEnabled(false);
		
		mClientId = mHostListDbHelper.fetchHost("nativeboinc");
		
		if (mAccessPassword.getText().length() != 0) {
			String newPassword = mAccessPassword.getText().toString();
			try {
				mRunner.setAccessPassword(newPassword);
			} catch(IOException ex) {
				Toast.makeText(this, R.string.setAccessPasswordError, Toast.LENGTH_LONG).show();
				setProgressBarIndeterminateVisibility(false);
				finish();
			}
			
			/* update host password */
			mClientId.setPassword(newPassword);
			mHostListDbHelper.updateHost(mClientId);
		}
		mHostListDbHelper.close();
		/* restarting client */
		mRunner.startClient();
	}
	
	private void doInstallStep3() {
		mNext.setEnabled(false);
		/* update applications list */
		mInstaller.updateProjectDistribList();
	}
	
	private void cancelOnError() {
		if (mProgressDialog != null) {
			mProgressDialog.dismiss();
			mProgressDialog = null;
		}
		setProgressBarIndeterminateVisibility(false);
		if (mCurrentStep == INSTALL_STEP_3) {
			mCurrentStep = INSTALL_FINISH_BUT_STEP;
			prepareView();
			if (!mRunner.isRun())
				mRunner.startClient();
		} else
			InstallWizardActivity.this.finish();
	}
	
	private void showErrorDialog(String message) {
		new AlertDialog.Builder(this).setTitle(getString(R.string.error))
			.setMessage(message)
			.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
				@Override
				public void onClick(DialogInterface dialog, int which) {
					cancelOnError();
				}
			}).setOnCancelListener(new DialogInterface.OnCancelListener() {
				@Override
				public void onCancel(DialogInterface dialog) {
					cancelOnError();
				}
			}).create().show();
	}
	
	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		if (keyCode == KeyEvent.KEYCODE_BACK) {
			mInstaller.cancelOperation();
			finish();
			return false;
		} else
			return super.onKeyDown(keyCode, event);
	}
	
	/*
	 * InstallListener methods
	 */
	@Override
	public void onOperation(String opDescription) {
		setProgressBarIndeterminateVisibility(true);
		Toast.makeText(this, opDescription, Toast.LENGTH_SHORT).show();
	}
	
	@Override
	public void onOperationProgress(String opDescription, int progress) {
		if (mProgressDialog == null) {
			mProgressDialog = new ProgressDialog(this);
			mProgressDialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
		    mProgressDialog.setMessage(opDescription);
		    mProgressDialog.setMax(10000);
		    mProgressDialog.setProgress(0);
		    mProgressDialog.setCancelable(true);
		    mProgressDialog.setOnCancelListener(new DialogInterface.OnCancelListener() {
				@Override
				public void onCancel(DialogInterface dialog) {
					mInstaller.cancelOperation();
					finish();
				}
			});
		    mProgressDialog.show();
		}
		setProgressBarIndeterminateVisibility(true);
		mProgressDialog.setProgress(progress);
		if (progress == 10000) {
			mProgressDialog.dismiss();
			mProgressDialog = null;
		}
	}

	@Override
	public void onOperationError(String errorMessage) {
		showErrorDialog(errorMessage);
	}

	@Override
	public void onOperationCancel() {
		Toast.makeText(this, R.string.operationCancelled, Toast.LENGTH_LONG).show();
		if (mProgressDialog != null) {
			mProgressDialog.dismiss();
			mProgressDialog = null;
		}
		setProgressBarIndeterminateVisibility(false);
	}
	
	@Override
	public void onOperationFinish() {
		if (mProgressDialog != null) {
			mProgressDialog.dismiss();
			mProgressDialog = null;
		}
		
		setProgressBarIndeterminateVisibility(false);
		
		if (mCurrentStep == INSTALL_FINISH_STEP) {
			return;
		}
		
		if (mCurrentStep == INSTALL_STEP_1)
			mRunner.firstStartClient();
		else if (mCurrentStep == INSTALL_STEP_3) {
			mRunner.startClient();
		} else { // to next level
			mCurrentStep++;
			prepareView();
		}
	}
	
	@Override
	public void currentProjectDistribList(Vector<ProjectDistrib> projectDistribs) {
		/* start select host */
		if (mCurrentStep == INSTALL_STEP_3) {
			ProjectItem[] projectItems = new ProjectItem[projectDistribs.size()];
			
			/* prepare list */
			for (int i = 0; i < projectDistribs.size(); i++) {
				ProjectDistrib distrib = projectDistribs.get(i);
				projectItems[i] = new ProjectItem(distrib.projectName, distrib.projectUrl);
			}
			
			Intent intent = new Intent(this, ProjectListActivity.class);
			intent.putExtra(ProjectItem.TAG, projectItems);
			startActivityForResult(intent, ACTIVITY_SELECT_PROJECT);
		}
	}
	
	@Override
	public void currentClientDistrib(ClientDistrib clientDistrib) {
		mInstaller.installClientAutomatically();
	}
	
	@Override
	public void onClientStateChanged(boolean isRun) {
		if (isRun) {
			try {
				mConnectionManager.connect(mClientId, false);
			} catch(NoConnectivityException ex) {
				return;
			}
			
			if (mCurrentStep == INSTALL_STEP_2) { // to configure client
				mRunner.configureClient();
			} else if (mCurrentStep == INSTALL_STEP_3) {
				mCurrentStep++;	/* to finish */
				prepareView();
			}
		}
		else if (mCurrentStep == INSTALL_STEP_3) {
			mInstaller.installBoincApplicationAutomatically(mProjectUrl);
		}
	}
	
	@Override
	public void onClientFirstStart() {
		Toast.makeText(this, R.string.nativeClientFirstStart, Toast.LENGTH_SHORT).show();
	}
	
	@Override
	public void onAfterClientFirstKill() {
		Toast.makeText(this, R.string.nativeClientFirstKill, Toast.LENGTH_SHORT).show();
		
		if (mCurrentStep == INSTALL_STEP_1) {
			try {
				mAccessPasswordString = mRunner.getAccessPassword();
				mHostListDbHelper.open();
				mHostListDbHelper.addHost(new ClientId(0, "nativeboinc", "127.0.0.1",
					31416, mAccessPasswordString));
			} catch(IOException ex) {
				/* if error at reading access password */
				Toast.makeText(this, R.string.getAccessPasswordError, Toast.LENGTH_LONG).show();
				setProgressBarIndeterminateVisibility(false);
				finish();
			}
			
			mCurrentStep++;	// do next install step
			prepareView();
		}
	}
	
	@Override
	public void onClientConfigured() {
		Toast.makeText(this, R.string.nativeClientConfigured, Toast.LENGTH_SHORT).show();
		/* to next step */
		mCurrentStep++;
		prepareView();
	}
	
	@Override
	public void onClientError(String message) {
		showErrorDialog(message);
	}

	@Override
	public void clientConnectionProgress(int progress) {
		// do nothing
	}

	@Override
	public void clientConnected(VersionInfo clientVersion) {
		// do nothing
	}

	@Override
	public void clientDisconnected() {
		// do nothing
	}

	@Override
	public boolean clientError(int errorNum, Vector<String> messages) {
		showErrorDialog(getString(R.string.clientError));
		return false;
	}

	@Override
	public boolean updatedClientMode(ModeInfo modeInfo) {
		// do nothing
		return false;
	}

	@Override
	public boolean updatedHostInfo(HostInfo hostInfo) {
		// do nothing
		return false;
	}

	@Override
	public boolean updatedProjects(Vector<ProjectInfo> projects) {
		// do nothing
		return false;
	}

	@Override
	public boolean updatedTasks(Vector<TaskInfo> tasks) {
		// do nothing
		return false;
	}

	@Override
	public boolean updatedTransfers(Vector<TransferInfo> transfers) {
		// do nothing
		return false;
	}

	@Override
	public boolean updatedMessages(Vector<MessageInfo> messages) {
		// do nothing
		return false;
	}

	@Override
	public boolean currentBAMInfo(AccountMgrInfo accountMgrInfo) {
		// do nothing
		return false;
	}

	@Override
	public boolean currentAllProjectsList(Vector<ProjectListEntry> allProjects) {
		// do nothing
		return false;
	}

	@Override
	public boolean currentAuthCode(String authCode) {
		// attach project
		boincProjectAttach(mProjectUrl, authCode, "");
		return false;
	}

	@Override
	public boolean currentProjectConfig(ProjectConfig projectConfig) {
		// start AddProject dialog
		Intent intent = new Intent(this, AddProjectActivity.class);
		intent.putExtra(ProjectConfigOptions.TAG, new ProjectConfigOptions(projectConfig));
		startActivityForResult(intent, ACTIVITY_ADD_PROJECT);
		return false;
	}
	
	@Override
	public boolean onAfterProjectAttach() {
		/* shutting down after project attach */
		mRunner.shutdownClient();
		return false;
	}
	
	private void boincProjectConfig(String url) {
		mConnectionManager.getProjectConfig(this, url);
		Toast.makeText(this, getString(R.string.clientProjectConfig), Toast.LENGTH_LONG).show();
	}
	
	private void boincCreateAccount(String name, String email, String password) {
		AccountIn accountIn = new AccountIn();
		accountIn.url = mProjectUrl;
		accountIn.user_name = name;
		accountIn.email_addr = email;
		accountIn.passwd = password;
		mConnectionManager.createAccount(this, accountIn);
		Toast.makeText(this, getString(R.string.clientCreatingAccount), Toast.LENGTH_LONG).show();
	}
	
	private void boincLookupAccount(String email, String password) {
		AccountIn accountIn = new AccountIn();
		accountIn.url = mProjectUrl;
		accountIn.email_addr = email;
		accountIn.passwd = password;
		mConnectionManager.lookupAccount(this, accountIn);
		Toast.makeText(this, getString(R.string.clientLookingUpAccount), Toast.LENGTH_LONG).show();
	}
	
	private void boincProjectAttach(String url, String authCode, String projectName) {
		mConnectionManager.projectAttach(this, url, authCode, projectName);
		Toast.makeText(this, getString(R.string.clientAttachingToProject), Toast.LENGTH_LONG).show();
	}
}
