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

import edu.berkeley.boinc.lite.ProjectListEntry;

import sk.boinc.nativeboinc.clientconnection.ClientAllProjectsListReceiver;
import sk.boinc.nativeboinc.clientconnection.ClientReplyReceiver;
import sk.boinc.nativeboinc.clientconnection.VersionInfo;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.installer.ClientDistrib;
import sk.boinc.nativeboinc.installer.InstallerProgressListener;
import sk.boinc.nativeboinc.installer.InstallerUpdateListener;
import sk.boinc.nativeboinc.installer.ProjectDistrib;
import sk.boinc.nativeboinc.util.ProjectItem;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
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
	
	private static final int DIALOG_ENTER_URL = 1;
	private static final int DIALOG_PROJECT_INFO = 2;
	private final static int DIALOG_ERROR = 3;
	
	public static final String TAG_OTHER_PROJECT_OPTION = "OtherProjectOption";
	public static final String TAG_FROM_INSTALLER = "FromInstaller";
	
	private static final String STATE_PROJECTS_LIST = "ProjectsList";
	private static final String STATE_PROJECT_DISTRIBS = "ProjectDistribs";
	
	private static final String ARG_DISTRIB_NAME = "DistribName";
	private static final String ARG_DISTRIB_DESC = "DistribDesc";
	private static final String ARG_DISTRIB_CHANGES = "DistribChanges";
	
	private final static String ARG_ERROR = "Error";
	
	private ArrayList<ProjectItem> mProjectsList = null;
	private ArrayList<ProjectDistrib> mProjectDistribs = null;
	private boolean mOtherProjectOption = true;
	private boolean mGetFromInstaller = false;
	
	private class ProjectsListAdapter extends BaseAdapter {
		private Context mContext;
		
		public ProjectsListAdapter(Context context) {
			mContext = context;
		}
		
		@Override
		public int getCount() {
			return (mOtherProjectOption) ? mProjectsList.size()+1 : mProjectsList.size();
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
			if (pos < mProjectsList.size()) {
				ProjectItem projectEntry = mProjectsList.get(pos);
				text1.setText(projectEntry.getName());
				text2.setText(projectEntry.getUrl());
			} else if (pos == mProjectsList.size()) {
				text1.setText(getString(R.string.otherProject));
				text2.setText(getString(R.string.otherProjectDetails));
			}
			return view;
		}
	}
	
	private class InfoClickListener implements View.OnClickListener {

		private int mDistribIndex = 0;
		
		public InfoClickListener(int distribIndex) {
			mDistribIndex = distribIndex;
		}
		
		@Override
		public void onClick(View v) {
			Bundle args = new Bundle();
			
			if (mProjectDistribs == null)
				return;
			
			ProjectDistrib distrib = mProjectDistribs.get(mDistribIndex);
			
			args.putString(ARG_DISTRIB_NAME, distrib.projectName + " " + distrib.version);
			args.putString(ARG_DISTRIB_DESC, distrib.description);
			args.putString(ARG_DISTRIB_CHANGES, distrib.changes);
			showDialog(DIALOG_PROJECT_INFO, args);
		}
	}
	
	private InfoClickListener[] mInfoClickListeners = null;
	
	/* initializes click listeners */
	private void initInfoClickListeners() {
		int count = mProjectDistribs.size();
		mInfoClickListeners = new InfoClickListener[count];
		for (int i = 0; i < count; i++)
			mInfoClickListeners[i] = new InfoClickListener(i);
	}
	
	private class ProjectDistribsAdapter extends BaseAdapter {
		private Context mContext;
		
		public ProjectDistribsAdapter(Context context) {
			mContext = context;
		}
		
		@Override
		public int getCount() {
			if (mProjectDistribs == null)
				return (mOtherProjectOption) ? 1 : 0;
			return (mOtherProjectOption) ? mProjectDistribs.size()+1 : mProjectDistribs.size();
		}

		@Override
		public Object getItem(int index) {
			if (mProjectDistribs == null || mProjectDistribs.size() <= index)
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
				view = inflater.inflate(R.layout.distrib_list_item, null);
			}
			
			TextView text1 = (TextView)view.findViewById(android.R.id.text1);
			TextView text2 = (TextView)view.findViewById(android.R.id.text2);
			Button infoButton = (Button)view.findViewById(R.id.info);
			
			if (pos < mProjectDistribs.size()) {
				ProjectDistrib projectDistrib = mProjectDistribs.get(pos);
				text1.setText(projectDistrib.projectName + " " + projectDistrib.version);
				text2.setText(projectDistrib.projectUrl);
			} else if (pos == mProjectDistribs.size()) {
				text1.setText(getString(R.string.otherProject));
				text2.setText(getString(R.string.otherProjectDetails));
			}
			
			if (mInfoClickListeners != null)
				infoButton.setOnClickListener(mInfoClickListeners[pos]);
			
			return view;
		}
	}
	
	
	private TextView mListEmptyText = null;
	private ListView mListView = null;
	private BaseAdapter mListAdapter = null;
	
	@Override
	public void onCreate(Bundle savedInstanceState) {
		requestWindowFeature(Window.FEATURE_INDETERMINATE_PROGRESS);
		setUpService(true, true, false, false, true, true);
		
		super.onCreate(savedInstanceState);
		setContentView(R.layout.project_list);
		
		Intent intent = getIntent();
		mOtherProjectOption = intent.getBooleanExtra(TAG_OTHER_PROJECT_OPTION, true);
		mGetFromInstaller = intent.getBooleanExtra(TAG_FROM_INSTALLER, false);
		
		mListView = (ListView)findViewById(android.R.id.list);
		mListEmptyText = (TextView)findViewById(android.R.id.empty);
		
		
		if (!mGetFromInstaller)
			mListAdapter = new ProjectsListAdapter(this);
		else
			mListAdapter = new ProjectDistribsAdapter(this);
		
		mListView.setAdapter(mListAdapter);
		
		if (savedInstanceState != null) {
			mProjectDistribs = savedInstanceState.getParcelableArrayList(STATE_PROJECT_DISTRIBS);
			mProjectsList = savedInstanceState.getParcelableArrayList(STATE_PROJECTS_LIST);
			if (mProjectDistribs != null || mProjectsList != null) {
				if (Logging.DEBUG) Log.d(TAG, "From save instance");
				updateDataSet();
			}
		}
		
		mListView.setOnItemClickListener(new AdapterView.OnItemClickListener() {
			@Override
			public void onItemClick(AdapterView<?> parent, View view, int position,
					long id) {
				if (!mGetFromInstaller) { // if from gett
					if (position < mProjectsList.size()) {
						Intent intent = new Intent(ProjectListActivity.this, AddProjectActivity.class);
						ProjectItem entry = mProjectsList.get(position);
						intent.putExtra(ProjectItem.TAG, entry);
						
						finish();
						startActivity(intent);	// add project activity
					} else if (position == mProjectsList.size()) {
						showDialog(DIALOG_ENTER_URL);
					}
				} else { // if from installer
					if (position < mProjectDistribs.size()) {
						Intent intent = new Intent(ProjectListActivity.this, AddProjectActivity.class);
						ProjectDistrib distrib = mProjectDistribs.get(position);
						ProjectItem projectItem = new ProjectItem(distrib.projectName,
								distrib.projectUrl, distrib.version);
						intent.putExtra(ProjectItem.TAG, projectItem);
						finish();
						
						// reseting all pendings
						startActivity(intent);	// add project activity
					} else if (position == mProjectDistribs.size()) {
						showDialog(DIALOG_ENTER_URL);
					}
				}
			}
		});		
	}
	
	@Override
	public void onSaveInstanceState(Bundle outState) {
		/*if (mProjectsList != null)
			outState.putBoolean(STATE_PROJECT_LIST, mPro);*/
		if (mProjectsList != null)
			outState.putParcelableArrayList(STATE_PROJECTS_LIST, mProjectsList);
		if (mProjectDistribs != null)
			outState.putParcelableArrayList(STATE_PROJECT_DISTRIBS, mProjectDistribs);
		super.onSaveInstanceState(outState);
	}
	
	@Override
	protected void onConnectionManagerConnected() {
		if (!mGetFromInstaller) {
			if (mProjectsList == null) {
				if (Logging.DEBUG) Log.d(TAG, "List from boinc client");
				mConnectionManager.getAllProjectsList(this);
			}
		}
	}
	
	@Override
	protected void onInstallerConnected() {
		if (mGetFromInstaller) {
			if (mProjectDistribs == null) {
				if (Logging.DEBUG) Log.d(TAG, "List from installer");
				mInstaller.updateProjectDistribList();
			}
		}
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
		
		if (mGetFromInstaller) // initializes info click listeners
			initInfoClickListeners();
		
		mListAdapter.notifyDataSetChanged();
	}
	
	@Override
	protected Dialog onCreateDialog(int dialogId, Bundle args) {
		switch(dialogId) {
		case DIALOG_ENTER_URL:
		{
			View view = LayoutInflater.from(this).inflate(R.layout.project_url, null);
			final EditText urlEdit = (EditText)view.findViewById(R.id.projectUrlUrl);
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
				.setNegativeButton(R.string.cancel, new Dialog.OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which) {
					}
				})
				.create();
		}
		case DIALOG_PROJECT_INFO:
		{
			LayoutInflater inflater = LayoutInflater.from(this);
			View view = inflater.inflate(R.layout.distrib_info, null);
			
			return new AlertDialog.Builder(this)
				.setIcon(android.R.drawable.ic_dialog_info)
				.setNegativeButton(R.string.dismiss, null)
				.setTitle(R.string.clientInfo)
				.setView(view)
				.create();
		}
		case DIALOG_ERROR:
		{
			if (dialogId==DIALOG_ERROR)
				return new AlertDialog.Builder(this)
					.setIcon(android.R.drawable.ic_dialog_alert)
					.setTitle(R.string.installError)
					.setView(LayoutInflater.from(this).inflate(R.layout.dialog, null))
					.setNegativeButton(R.string.ok, null)
					.create();
		}
		}
		return null;
	}
	
	@Override
	public void onPrepareDialog(int dialogId, Dialog dialog, Bundle args) {
		if (dialogId == DIALOG_ERROR) {
			TextView textView = (TextView)dialog.findViewById(R.id.dialogText);
			textView.setText(args.getString(ARG_ERROR));
		} else if (dialogId == DIALOG_PROJECT_INFO) {
			TextView distribVersion = (TextView)dialog.findViewById(R.id.distribVersion);
			TextView distribDesc = (TextView)dialog.findViewById(R.id.distribDesc);
			TextView distribChanges = (TextView)dialog.findViewById(R.id.distribChanges);
			
			/* setup client info texts */
			distribVersion.setText(args.getString(ARG_DISTRIB_NAME));
			distribDesc.setText(getString(R.string.description)+":\n"+
					args.getString(ARG_DISTRIB_DESC));
			distribChanges.setText(getString(R.string.changes)+":\n"+
					args.getString(ARG_DISTRIB_CHANGES));
		}
	}

	@Override
	public void currentProjectDistribList(ArrayList<ProjectDistrib> projectDistribs) {
		mProjectDistribs = projectDistribs;
		updateDataSet();
		setProgressBarIndeterminateVisibility(false);
	}

	@Override
	public void currentClientDistrib(ClientDistrib clientDistrib) {		
	}

	@Override
	public void onOperation(String distribName, String opDescription) {
		setProgressBarIndeterminateVisibility(true);
	}

	@Override
	public void onOperationProgress(String distribName, String opDescription,
			int progress) {
		setProgressBarIndeterminateVisibility(true);
	}

	@Override
	public void onOperationError(String distribName, String errorMessage) {
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
	public void clientConnectionProgress(int progress) {
		if (progress == ClientReplyReceiver.PROGRESS_XFER_FINISHED)
			setProgressBarIndeterminateVisibility(false);
		else
			setProgressBarIndeterminateVisibility(true);
	}

	@Override
	public void clientConnected(VersionInfo clientVersion) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void clientDisconnected() {
		if (Logging.WARNING) Log.w(TAG, "Client disconnected");
		setProgressBarIndeterminateVisibility(false);
		
		Bundle args = new Bundle();
		args.putString(ARG_ERROR, getString(R.string.clientDisconnected));
		showDialog(DIALOG_ERROR, args);
	}

	@Override
	public boolean currentAllProjectsList(ArrayList<ProjectListEntry> allProjects) {
		if (Logging.DEBUG) Log.d(TAG, "currentAllProjectsList notify:"+allProjects.size());
		setProgressBarIndeterminateVisibility(false);
		mProjectsList = new ArrayList<ProjectItem>();
		for (ProjectListEntry entry: allProjects)
			mProjectsList.add(new ProjectItem(entry.name, entry.url));
		updateDataSet();
		return true;
	}	
}
