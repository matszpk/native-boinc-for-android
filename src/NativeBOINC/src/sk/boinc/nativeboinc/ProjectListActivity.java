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
import java.util.Collections;
import java.util.Comparator;

import edu.berkeley.boinc.lite.ProjectListEntry;

import sk.boinc.nativeboinc.clientconnection.BoincOp;
import sk.boinc.nativeboinc.clientconnection.ClientAllProjectsListReceiver;
import sk.boinc.nativeboinc.clientconnection.VersionInfo;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.installer.ClientDistrib;
import sk.boinc.nativeboinc.installer.InstallOp;
import sk.boinc.nativeboinc.installer.InstallerProgressListener;
import sk.boinc.nativeboinc.installer.InstallerService;
import sk.boinc.nativeboinc.installer.InstallerUpdateListener;
import sk.boinc.nativeboinc.installer.ProjectDistrib;
import sk.boinc.nativeboinc.util.ClientId;
import sk.boinc.nativeboinc.util.ProgressState;
import sk.boinc.nativeboinc.util.ProjectItem;
import sk.boinc.nativeboinc.util.StandardDialogs;
import sk.boinc.nativeboinc.util.UpdateItem;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.text.InputType;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.TextView;

/**
 * @author mat
 *
 */
public class ProjectListActivity extends ServiceBoincActivity implements InstallerProgressListener,
	InstallerUpdateListener, ClientAllProjectsListReceiver {
	private static final String TAG = "ProjectListActivity";
	
	private static final String ARG_FORCE_PROJECT_LIST = "ForceProjectList";
	
	private static final int DIALOG_ENTER_URL = 1;
	
	public static final int ACTIVITY_ADD_PROJECT = 1;
	
	private ArrayList<ProjectItem> mProjectsList = null;
	private ArrayList<ProjectDistrib> mProjectDistribs = null;
	private boolean mGetFromInstaller = false;
	private int mDataDownloadProgressState = ProgressState.NOT_RUN;

	
	private ClientId mConnectedClient = null;
	/* if add project finish successfully */
	private boolean mEarlyAddProjectGoodFinish = false;
	
	@Override
	public int getInstallerChannelId() {
		return InstallerService.DEFAULT_CHANNEL_ID;
	}
	
	private static class SavedState {
		private final ArrayList<ProjectItem> mProjectsList;
		private final ArrayList<ProjectDistrib> mProjectDistribs;
		private final boolean mEarlyAddProjectGoodFinish;
		
		private int mDataDownloadProgressState;
		
		public SavedState(ProjectListActivity activity) {
			mProjectDistribs = activity.mProjectDistribs;
			mProjectsList = activity.mProjectsList;
			mDataDownloadProgressState = activity.mDataDownloadProgressState; 
			mEarlyAddProjectGoodFinish = activity.mEarlyAddProjectGoodFinish;
		}
		
		public void restore(ProjectListActivity activity) {
			activity.mProjectDistribs = mProjectDistribs;
			activity.mProjectsList = mProjectsList;
			activity.mDataDownloadProgressState = mDataDownloadProgressState;
			activity.mEarlyAddProjectGoodFinish = mEarlyAddProjectGoodFinish;
		}
	}
	
	private class ProjectsListAdapter extends BaseAdapter {
		private Context mContext;
		
		public ProjectsListAdapter(Context context) {
			mContext = context;
		}
		
		@Override
		public int getCount() {
			if (mProjectsList == null)
				return 0;
			return mProjectsList.size();
		}

		@Override
		public Object getItem(int index) {
			if (mProjectsList == null || mProjectsList.size() <= index)
				return null;
			return mProjectsList.get(index);
		}

		@Override
		public long getItemId(int index) {
			return index;
		}

		@Override
		public View getView(int pos, View inView, ViewGroup parent) {
			View view = inView;
			if (view == null) {
				LayoutInflater inflater = (LayoutInflater)mContext.getSystemService(LAYOUT_INFLATER_SERVICE);
				view = inflater.inflate(android.R.layout.simple_list_item_2, null);
			}
			
			TextView text1 = (TextView)view.findViewById(android.R.id.text1);
			TextView text2 = (TextView)view.findViewById(android.R.id.text2);
			
			if (mProjectsList != null) {
				ProjectItem projectEntry = mProjectsList.get(pos);
				text1.setText(projectEntry.getName());
				text2.setText(projectEntry.getUrl());
			}
			return view;
		}
	}
	
	private class ProjectDistribsAdapter extends BaseAdapter {
		private Context mContext;
		
		public ProjectDistribsAdapter(Context context) {
			mContext = context;
		}
		
		@Override
		public int getCount() {
			if (mProjectDistribs == null)
				return 0;
			return mProjectDistribs.size();
		}

		@Override
		public Object getItem(int index) {
			if (mProjectDistribs == null)
				return null;
			return mProjectDistribs.get(index);
		}

		@Override
		public long getItemId(int index) {
			return index;
		}

		@Override
		public View getView(int pos, View inView, ViewGroup parent) {
			View view = inView;
			if (view == null) {
				LayoutInflater inflater = (LayoutInflater)mContext
						.getSystemService(LAYOUT_INFLATER_SERVICE);
				view = inflater.inflate(android.R.layout.simple_list_item_2, null);
			}
			
			TextView text1 = (TextView)view.findViewById(android.R.id.text1);
			TextView text2 = (TextView)view.findViewById(android.R.id.text2);
			
			if (mProjectDistribs != null && pos < mProjectDistribs.size()) {
				ProjectDistrib projectDistrib = mProjectDistribs.get(pos);
				text1.setText(projectDistrib.projectName + " " + projectDistrib.version);
				text2.setText(projectDistrib.projectUrl);
			}
			return view;
		}
	}
	
	
	private TextView mListEmptyText = null;
	private ListView mListView = null;
	private BaseAdapter mListAdapter = null;
	
	@Override
	public void onCreate(Bundle savedInstanceState) {
		requestWindowFeature(Window.FEATURE_INDETERMINATE_PROGRESS);
		
		final SavedState savedState = (SavedState)getLastNonConfigurationInstance();
		if (savedState != null)
			savedState.restore(this);
		
		setUpService(true, true, false, false, true, true);
		
		super.onCreate(savedInstanceState);
		setContentView(R.layout.project_list);
		
		mListView = (ListView)findViewById(android.R.id.list);
		mListEmptyText = (TextView)findViewById(android.R.id.empty);
		
		Button cancelButton = (Button)findViewById(R.id.cancel);
		cancelButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				onBackPressed();
			}
		});
		
		Button otherProjectButton = (Button)findViewById(R.id.otherProject);
		otherProjectButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				if (!mGetFromInstaller)
					showDialog(DIALOG_ENTER_URL);
				else {
					finish();
					Intent intent = new Intent(ProjectListActivity.this, ProjectListActivity.class);
					intent.putExtra(ARG_FORCE_PROJECT_LIST, true);
					// with boinc project list 
					startActivity(intent);
				}
			}
		});
		
		mListView.setOnItemLongClickListener(new AdapterView.OnItemLongClickListener() {

			@Override
			public boolean onItemLongClick(AdapterView<?> parent, View view, int position,
					long id) {
				Log.d(TAG, "Long click in "+ position);
				if (!mGetFromInstaller) // if not installer
					return false;
				
				if (mProjectDistribs == null || position >= mProjectDistribs.size())
					return false;
				
				ProjectDistrib distrib = mProjectDistribs.get(position);
				
				StandardDialogs.showDistribInfoDialog(ProjectListActivity.this,
						distrib.projectName, distrib.version,
						distrib.description, distrib.changes);
				return true;
			}
		});
		
		mListView.setOnItemClickListener(new AdapterView.OnItemClickListener() {
			@Override
			public void onItemClick(AdapterView<?> parent, View view, int position,
					long id) {
				if (!mGetFromInstaller) { // if from gett
					if (position < mProjectsList.size()) {
						Intent intent = new Intent(ProjectListActivity.this, AddProjectActivity.class);
						ProjectItem entry = mProjectsList.get(position);
						intent.putExtra(ProjectItem.TAG, entry);
						
						startActivityForResult(intent, ACTIVITY_ADD_PROJECT);	// add project activity
					}
				} else { // if from installer
					if (position < mProjectDistribs.size()) {
						Intent intent = new Intent(ProjectListActivity.this, AddProjectActivity.class);
						ProjectDistrib distrib = mProjectDistribs.get(position);
						ProjectItem projectItem = new ProjectItem(distrib.projectName,
								distrib.projectUrl, distrib.version);
						intent.putExtra(ProjectItem.TAG, projectItem);
						
						// reseting all pendings
						startActivityForResult(intent, ACTIVITY_ADD_PROJECT);	// add project activity
					}
				}
			}
		});		
	}
	
	@Override
	protected void onResume() {
		super.onResume();
		
		if (mConnectionManager != null)
			mConnectedClient = mConnectionManager.getClientId();
		
		updateActivityState();
	}
	
	@Override
	public void onBackPressed() {
		if (mInstaller != null)
			mInstaller.cancelSimpleOperation();
		finish();
	}
	
	/**
	 * call when add project activity finished successfully
	 */
	private void onAddProjectActivityGoodFinish() {
		mEarlyAddProjectGoodFinish = false;
		
		if (mConnectionManager.isNativeConnected()) { // if native client
			finish(); // if ok then go to progress activity
			startActivity(new Intent(this, ProgressActivity.class));
		} else // if normal client
			finish();
	}
	
	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		if (requestCode == ACTIVITY_ADD_PROJECT) {
			if (resultCode == RESULT_OK) {
				if (mConnectionManager != null)
					onAddProjectActivityGoodFinish();
				else
					mEarlyAddProjectGoodFinish = true;
			}
		}
	}
	
	@Override
	public Object onRetainNonConfigurationInstance() {
		return new SavedState(this);
	}
	
	private void updateActivityState() {
		// if disconnected
		if (mDataDownloadProgressState == ProgressState.IN_PROGRESS && mConnectedClient == null)
			clientDisconnected();
		
		if (mGetFromInstaller) {
			// from installer
			if (mInstaller == null)
				return;
			
			setProgressBarIndeterminateVisibility(mInstaller.isWorking());
			
			if (mConnectedClient != null) {
				if (mProjectDistribs == null) {
					if (mDataDownloadProgressState == ProgressState.IN_PROGRESS) {
						if (Logging.DEBUG) Log.d(TAG, "get List from installer");
						mProjectDistribs = (ArrayList<ProjectDistrib>)
								mInstaller.getPendingOutput(InstallOp.UpdateProjectDistribs);
						
						if (mProjectDistribs != null) {
							mDataDownloadProgressState = ProgressState.FINISHED;
							updateDataSet();
						}
						
					} else if (mDataDownloadProgressState == ProgressState.NOT_RUN) {
						if (Logging.DEBUG) Log.d(TAG, "Download List");
						// ignore if not ran, because we will using previous result 
						mDataDownloadProgressState = ProgressState.IN_PROGRESS;
						mInstaller.updateProjectDistribList();
					}
					// if finished but failed
				} else {
					updateDataSet();
				}
			}
			
			// do not finish when error
			mInstaller.handlePendingErrors(InstallOp.UpdateProjectDistribs, this);
		} else {
			// from boinc client
			if (mConnectionManager == null)
				return;
			
			setProgressBarIndeterminateVisibility(mConnectionManager.isWorking());
			
			if (mConnectionManager.handlePendingClientErrors(null, this))
				return;
			
			if (mConnectedClient == null)
				return;
			
			if (mProjectsList == null) {
				if (mDataDownloadProgressState == ProgressState.IN_PROGRESS) {
					if (Logging.DEBUG) Log.d(TAG, "Fetch List from boinc clmient");
					ArrayList<ProjectListEntry> allProjects = (ArrayList<ProjectListEntry>)
							mConnectionManager.getPendingOutput(BoincOp.GetAllProjectList);
					
					if (allProjects != null) {
						mDataDownloadProgressState = ProgressState.FINISHED;
						setProjectsList(allProjects);
						updateDataSet();
					}
					
				} else if (mDataDownloadProgressState == ProgressState.NOT_RUN) {
					if (Logging.DEBUG) Log.d(TAG, "List from boinc client");
					// ignore if not ran, because we will using previous result
					mDataDownloadProgressState = ProgressState.IN_PROGRESS;
					mConnectionManager.getAllProjectsList();
				}
				// if finished but failed
			} else
				updateDataSet();
		}
	}
	
	@Override
	protected void onConnectionManagerConnected() {
		mConnectedClient = mConnectionManager.getClientId();
		if (mConnectedClient != null) {
			Intent intent = getIntent();
			if (!intent.getBooleanExtra(ARG_FORCE_PROJECT_LIST, false))
				mGetFromInstaller = mConnectionManager.isNativeConnected();
			else // do not get project list from installer (distribs)
				mGetFromInstaller = false;
			
			if (mGetFromInstaller) {
				// show project list help
				TextView projectListHelp = (TextView)findViewById(R.id.projectListHelp);
				projectListHelp.setVisibility(View.VISIBLE);
			}
			
			// attach adapter
			if (!mGetFromInstaller)
				mListAdapter = new ProjectsListAdapter(this);
			else
				mListAdapter = new ProjectDistribsAdapter(this);
			
			mListView.setAdapter(mListAdapter);
		}
		// update activity state: errors and progress
		updateActivityState();
		
		/* if add project activity finish successfully,
		 * but connectionmanager not available at moment */
		if (mEarlyAddProjectGoodFinish) 
			onAddProjectActivityGoodFinish();
	}
	
	@Override
	protected void onConnectionManagerDisconnected() {
		mConnectedClient = null;
		setProgressBarIndeterminateVisibility(false);
	}
	
	@Override
	protected void onInstallerConnected() {
		if (mConnectedClient != null)
			updateActivityState();
	}
	
	@Override
	protected void onInstallerDisconnected() {
		setProgressBarIndeterminateVisibility(false);
	}
	
	private void updateDataSet() {
		boolean isEmpty = false;
		if (mGetFromInstaller)
			isEmpty = mProjectDistribs.isEmpty();
		else
			isEmpty = mProjectsList.isEmpty();
		
		// if empty
		if (isEmpty) {
			mListView.setVisibility(View.GONE);
			mListEmptyText.setVisibility(View.VISIBLE);
		} else {
			mListView.setVisibility(View.VISIBLE);
			mListEmptyText.setVisibility(View.GONE);
		}
		
		mListAdapter.notifyDataSetChanged();
	}
	
	@Override
	protected Dialog onCreateDialog(int dialogId, Bundle args) {
		Dialog dialog = StandardDialogs.onCreateDialog(this, dialogId, args);
		if (dialog != null)
			return dialog;
		
		if (dialogId==DIALOG_ENTER_URL) {
			View view = LayoutInflater.from(this).inflate(R.layout.dialog_edit, null);
			final EditText urlEdit = (EditText)view.findViewById(android.R.id.edit);
			urlEdit.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_NORMAL);
			return new AlertDialog.Builder(this)
				.setIcon(android.R.drawable.ic_input_get)
				.setTitle(R.string.projectUrl)
				.setView(view)
				.setPositiveButton(R.string.ok, new Dialog.OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which) {
						Intent intent = new Intent(ProjectListActivity.this, AddProjectActivity.class);
						intent.putExtra(ProjectItem.TAG, new ProjectItem("", urlEdit.getText().toString()));
						finish();
						startActivity(intent);	// add project activity
					}
				})
				.setNegativeButton(R.string.cancel, null)
				.create();
		}
		return null;
	}
	
	@Override
	public void onPrepareDialog(int dialogId, Dialog dialog, Bundle args) {
		StandardDialogs.onPrepareDialog(this, dialogId, dialog, args);
	}

	@Override
	public void currentProjectDistribList(ArrayList<ProjectDistrib> projectDistribs) {
		if (mGetFromInstaller) {
			mProjectDistribs = projectDistribs;
			mDataDownloadProgressState = ProgressState.FINISHED;
			
			updateDataSet();
		}
	}

	@Override
	public void currentClientDistrib(ClientDistrib clientDistrib) {		
	}

	@Override
	public void onOperation(String distribName, String opDescription) {
	}

	@Override
	public void onOperationProgress(String distribName, String opDescription,
			int progress) {
	}
	
	@Override
	public boolean onOperationError(InstallOp installOp, String distribName, String errorMessage) {
		if (installOp.equals(InstallOp.UpdateProjectDistribs) && mGetFromInstaller &&
				(mDataDownloadProgressState == ProgressState.IN_PROGRESS ||
				 mDataDownloadProgressState == ProgressState.FINISHED)) {
			mDataDownloadProgressState = ProgressState.FAILED;
			StandardDialogs.showInstallErrorDialog(this, distribName, errorMessage);
			return true;
		}
		return false;
	}

	@Override
	public void onOperationCancel(InstallOp installOp, String distribName) {
		// if operation cancelled
		if (mGetFromInstaller && installOp.equals(InstallOp.UpdateProjectDistribs))
			mDataDownloadProgressState = ProgressState.FAILED;
	}

	@Override
	public void onOperationFinish(InstallOp installOp, String distribName) {
	}

	@Override
	public void clientConnectionProgress(BoincOp boincOp, int progress) {
	}

	@Override
	public void clientConnected(VersionInfo clientVersion) {
		mConnectedClient = mConnectionManager.getClientId();
	}

	@Override
	public void clientDisconnected() {
		if (!mGetFromInstaller) {
			if (Logging.WARNING) Log.w(TAG, "Client disconnected");
			mDataDownloadProgressState = ProgressState.FAILED;
			
			StandardDialogs.tryShowDisconnectedErrorDialog(this, mConnectionManager, mRunner,
					mConnectedClient);
		
			mConnectedClient = null;
		}
	}
	
	private static class ProjectItemComparator implements Comparator<ProjectItem> {

		@Override
		public int compare(ProjectItem lhs, ProjectItem rhs) {
			return lhs.getName().compareTo(rhs.getName());
		}
		
	}
	
	private void setProjectsList(ArrayList<ProjectListEntry> allProjects) {
		mProjectsList = new ArrayList<ProjectItem>();
		for (ProjectListEntry entry: allProjects)
			mProjectsList.add(new ProjectItem(entry.name, entry.url));
		
		Collections.sort(mProjectsList, new ProjectItemComparator());
	}

	@Override
	public boolean currentAllProjectsList(ArrayList<ProjectListEntry> allProjects) {
		if (!mGetFromInstaller) {
			if (Logging.DEBUG) Log.d(TAG, "currentAllProjectsList notify:"+allProjects.size());
			
			mDataDownloadProgressState = ProgressState.FINISHED;
			
			setProjectsList(allProjects);
			updateDataSet();
		}
		return true;
	}
	
	@Override
	public boolean clientError(BoincOp boincOp, int errorNum, String message) {
		if (!boincOp.equals(BoincOp.GetAllProjectList))
			return false;
		
		if (!mGetFromInstaller && mDataDownloadProgressState == ProgressState.IN_PROGRESS) {
			mDataDownloadProgressState = ProgressState.FAILED;
			return true;
		}
		StandardDialogs.showClientErrorDialog(this, errorNum, message);
		return false;
	}

	@Override
	public void onChangeInstallerIsWorking(boolean isWorking) {
		setProgressBarIndeterminateVisibility(isWorking);
	}

	@Override
	public void onClientIsWorking(boolean isWorking) {
		// TODO Auto-generated method stub
		setProgressBarIndeterminateVisibility(isWorking);
	}

	@Override
	public void binariesToUpdateOrInstall(UpdateItem[] updateItems) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void binariesToUpdateFromSDCard(String[] projectNames) {
		// TODO Auto-generated method stub
		
	}
}
