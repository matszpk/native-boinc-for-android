/* 
 * AndroBOINC - BOINC Manager for Android
 * Copyright (C) 2010, Pavol Michalec
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

package sk.boinc.androboinc;

import java.util.Vector;

import sk.boinc.androboinc.clientconnection.ClientOp;
import sk.boinc.androboinc.clientconnection.ClientReplyReceiver;
import sk.boinc.androboinc.clientconnection.HostInfo;
import sk.boinc.androboinc.clientconnection.MessageInfo;
import sk.boinc.androboinc.clientconnection.ModeInfo;
import sk.boinc.androboinc.clientconnection.ProjectInfo;
import sk.boinc.androboinc.clientconnection.TaskInfo;
import sk.boinc.androboinc.clientconnection.TransferInfo;
import sk.boinc.androboinc.clientconnection.VersionInfo;
import sk.boinc.androboinc.debug.Logging;
import sk.boinc.androboinc.service.ConnectionManagerService;
import sk.boinc.androboinc.util.ClientId;
import sk.boinc.androboinc.util.ScreenOrientationHandler;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ListActivity;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.text.Html;
import android.text.TextUtils;
import android.util.Log;
import android.view.ContextMenu;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.ContextMenu.ContextMenuInfo;
import android.widget.BaseAdapter;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.AdapterView.AdapterContextMenuInfo;


public class TransfersActivity extends ListActivity implements ClientReplyReceiver {
	private static final String TAG = "TransfersActivity";

	private static final int DIALOG_DETAILS = 1;
	private static final int DIALOG_WARN_ABORT = 2;

	private static final int RETRY = 1;
	private static final int ABORT = 2;
	private static final int PROPERTIES = 3;

	private ScreenOrientationHandler mScreenOrientation;

	private boolean mRequestUpdates = false;
	private boolean mViewUpdatesAllowed = false;
	private boolean mViewDirty = false;

	private Vector<TransferInfo> mTransfers = new Vector<TransferInfo>();
	private int mPosition = 0;


	private class SavedState {
		private final Vector<TransferInfo> transfers;

		public SavedState() {
			transfers = mTransfers;
			if (Logging.DEBUG) Log.d(TAG, "saved: transfers.size()=" + transfers.size());
		}
		public void restoreState(TransfersActivity activity) {
			activity.mTransfers = transfers;
			if (Logging.DEBUG) Log.d(TAG, "restored: mTransfers.size()=" + activity.mTransfers.size());
		}
	}

	private class TransferListAdapter extends BaseAdapter {
		private Context mContext;

		public TransferListAdapter(Context context) {
			mContext = context;
		}

		@Override
		public int getCount() {
            return mTransfers.size();
        }

		@Override
		public boolean areAllItemsEnabled() {
			return true;
		}

		@Override
		public boolean isEnabled(int position) {
			return true;
		}

		@Override
		public Object getItem(int position) {
			return mTransfers.elementAt(position);
		}

		@Override
		public long getItemId(int position) {
			return position;
		}

		@Override
		public View getView(int position, View convertView, ViewGroup parent) {
			View layout;
			if (convertView == null) {
				layout = LayoutInflater.from(mContext).inflate(
						R.layout.transfers_list_item, parent, false);
			}
			else {
				layout = convertView;
			}
			TransferInfo transfer = mTransfers.elementAt(position);
			TextView tv;
			tv = (TextView)layout.findViewById(R.id.transferFileName);
			tv.setText(transfer.fileName);
			tv = (TextView)layout.findViewById(R.id.transferProjectName);
			tv.setText(transfer.project);
			tv = (TextView)layout.findViewById(R.id.transferSize);
			tv.setText(transfer.size);
			tv = (TextView)layout.findViewById(R.id.transferElapsed);
			tv.setText(transfer.elapsed);
			tv = (TextView)layout.findViewById(R.id.transferSpeed);
			tv.setText(transfer.speed);
			tv = (TextView)layout.findViewById(R.id.transferProgressText);
			tv.setText(transfer.progress);
			ProgressBar progressRunning = (ProgressBar)layout.findViewById(R.id.transferProgressRunning);
			ProgressBar progressWaiting = (ProgressBar)layout.findViewById(R.id.transferProgressWaiting);
			ProgressBar progressSuspended = (ProgressBar)layout.findViewById(R.id.transferProgressSuspended);
			if ((transfer.stateControl & (TransferInfo.SUSPENDED|TransferInfo.ABORTED|TransferInfo.FAILED)) != 0) {
				// Either suspended or aborted task
				progressRunning.setVisibility(View.GONE);
				progressWaiting.setVisibility(View.GONE);
				progressSuspended.setProgress(transfer.progInd);
				progressSuspended.setVisibility(View.VISIBLE);
			}
			else if ((transfer.stateControl & TransferInfo.RUNNING) != 0) {
				// Running task
				progressWaiting.setVisibility(View.GONE);
				progressSuspended.setVisibility(View.GONE);
				progressRunning.setProgress(transfer.progInd);
				progressRunning.setVisibility(View.VISIBLE);
			}
			else {
				// Not running, not suspended, not aborted => waiting for execution
				progressRunning.setVisibility(View.GONE);
				progressSuspended.setVisibility(View.GONE);
				progressWaiting.setProgress(transfer.progInd);
				progressWaiting.setVisibility(View.VISIBLE);
				if ((transfer.stateControl & TransferInfo.STARTED) != 0) {
					// Task already started - set secondary progress to 100
					// The background of progress-bar will be yellow
					progressWaiting.setSecondaryProgress(100);
				}
				else {
					// Task did not started yet - set secondary progress to 0
					// The background of progress-bar will be grey
					progressWaiting.setSecondaryProgress(0);
				}
			}
			return layout;
		}
	}


	private ConnectionManagerService mConnectionManager = null;
	private ClientId mConnectedClient = null;

	private ServiceConnection mServiceConnection = new ServiceConnection() {
		@Override
		public void onServiceConnected(ComponentName name, IBinder service) {
			mConnectionManager = ((ConnectionManagerService.LocalBinder)service).getService();
			if (Logging.DEBUG) { Log.d(TAG, "onServiceConnected()"); }
			mConnectionManager.registerStatusObserver(TransfersActivity.this);
		}

		@Override
		public void onServiceDisconnected(ComponentName name) {
			mConnectionManager = null;
			// This should not happen normally, because it's local service 
			// running in the same process...
			if (Logging.WARNING) Log.w(TAG, "onServiceDisconnected()");
		}
	};

	private void doBindService() {
		if (Logging.DEBUG) { Log.d(TAG, "doBindService()"); }
		getApplicationContext().bindService(new Intent(TransfersActivity.this, ConnectionManagerService.class),
				mServiceConnection, Context.BIND_AUTO_CREATE);
	}

	private void doUnbindService() {
		if (Logging.DEBUG) { Log.d(TAG, "doUnbindService()"); }
		getApplicationContext().unbindService(mServiceConnection);
	}

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setListAdapter(new TransferListAdapter(this));
		registerForContextMenu(getListView());
		mScreenOrientation = new ScreenOrientationHandler(this);
		doBindService();
		// Restore state on configuration change (if applicable)
		final SavedState savedState = (SavedState)getLastNonConfigurationInstance();
		if (savedState != null) {
			// Yes, we have the saved state, this is activity re-creation after configuration change
			savedState.restoreState(this);
			if (!mTransfers.isEmpty()) {
				// We restored transfers - view will be updated on resume (before we will get refresh)
				mViewDirty = true;
			}
		}
	}

	@Override
	protected void onResume() {
		super.onResume();
		mScreenOrientation.setOrientation();
		mRequestUpdates = true;
		if (mConnectedClient != null) {
			// We are connected right now, request fresh data
			if (Logging.DEBUG) Log.d(TAG, "onResume() - Starting refresh of data");
			mConnectionManager.updateTransfers(this);
		}
		mViewUpdatesAllowed = true;
		if (mViewDirty) {
			// There were some updates received while we were not visible
			// The data are stored, but view is not updated yet; Do it now
			sortTransfers();
			((BaseAdapter)getListAdapter()).notifyDataSetChanged();
			mViewDirty = false;
			if (Logging.DEBUG) Log.d(TAG, "Delayed refresh of view was done now");
		}
	}

	@Override
	protected void onPause() {
		super.onPause();
		// We shall not request data updates
		mRequestUpdates = false;
		mViewUpdatesAllowed = false;
		// Also remove possibly scheduled automatic updates
		if (mConnectionManager != null) {
			mConnectionManager.cancelScheduledUpdates(this);
		}
	}

	@Override
	protected void onDestroy() {
		super.onDestroy();
		if (mConnectionManager != null) {
			mConnectionManager.unregisterStatusObserver(this);
			mConnectedClient = null;
		}
		doUnbindService();
		mScreenOrientation = null;
	}

	@Override
	public Object onRetainNonConfigurationInstance() {
		final SavedState savedState = new SavedState();
		return savedState;
	}

	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		Activity parent = getParent();
		if (parent != null) {
			return parent.onKeyDown(keyCode, event);
		}
		return super.onKeyDown(keyCode, event);
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		super.onCreateOptionsMenu(menu);
		MenuInflater inflater = getMenuInflater();
		inflater.inflate(R.menu.refresh_menu, menu);
		return true;
	}

	@Override
	public boolean onPrepareOptionsMenu(Menu menu) {
		super.onPrepareOptionsMenu(menu);
		MenuItem item = menu.findItem(R.id.menuRefresh);
		item.setVisible(mConnectedClient != null);
		return true;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		switch (item.getItemId()) {
		case R.id.menuRefresh:
			mConnectionManager.updateTransfers(this);
			return true;
		}
		return super.onOptionsItemSelected(item);
	}

	@Override
	protected Dialog onCreateDialog(int id) {
		switch (id) {
		case DIALOG_DETAILS:
			return new AlertDialog.Builder(this)
				.setIcon(android.R.drawable.ic_dialog_info)
				.setView(LayoutInflater.from(this).inflate(R.layout.dialog, null))
				.setNegativeButton(R.string.ok, null)
				.create();
		case DIALOG_WARN_ABORT:
	    	return new AlertDialog.Builder(this)
	    		.setIcon(android.R.drawable.ic_dialog_alert)
	    		.setTitle(R.string.warning)
				.setView(LayoutInflater.from(this).inflate(R.layout.dialog, null))
	    		.setPositiveButton(R.string.transferAbort,
	    			new DialogInterface.OnClickListener() {
	    				public void onClick(DialogInterface dialog, int whichButton) {
	    					TransferInfo transfer = (TransferInfo)getListAdapter().getItem(mPosition);
	    					mConnectionManager.transferOperation(TransfersActivity.this, ClientOp.TRANSFER_ABORT, transfer.projectUrl, transfer.fileName);
	    				}
	    			})
	    		.setNegativeButton(R.string.cancel, null)
	    		.create();
		}
		return null;
	}

	@Override
	protected void onPrepareDialog(int id, Dialog dialog) {
		TextView text;
		switch (id) {
		case DIALOG_DETAILS:
			text = (TextView)dialog.findViewById(R.id.dialogText);
			text.setText(Html.fromHtml(prepareTransferDetails(mPosition)));
			break;
		case DIALOG_WARN_ABORT:
			text = (TextView)dialog.findViewById(R.id.dialogText);
			TransferInfo transfer = (TransferInfo)getListAdapter().getItem(mPosition);
			text.setText(getString(R.string.warnAbortTransfer, transfer.fileName));
			break;
		}
	}

	@Override
	protected void onListItemClick(ListView l, View v, int position, long id) {
		mPosition = position;
		showDialog(DIALOG_DETAILS);
	}

	@Override
	public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo) {
		super.onCreateContextMenu(menu, v, menuInfo);
		AdapterContextMenuInfo info = (AdapterContextMenuInfo)menuInfo;
		TransferInfo transfer = (TransferInfo)getListAdapter().getItem(info.position);
		menu.setHeaderTitle(R.string.taskCtxMenuTitle);
		if ((transfer.stateControl & (TransferInfo.ABORTED|TransferInfo.FAILED)) == 0) {
			// Not aborted
			menu.add(0, ABORT, 0, R.string.transferAbort);
			if ((transfer.stateControl & (TransferInfo.RUNNING)) == 0) {
				// Not running now
				menu.add(0, RETRY, 0, R.string.transferRetry);
			}
		}
		menu.add(0, PROPERTIES, 0, R.string.transferProperties);
	}

	@Override
	public boolean onContextItemSelected(MenuItem item) {
		AdapterContextMenuInfo info = (AdapterContextMenuInfo)item.getMenuInfo();
		TransferInfo transfer = (TransferInfo)getListAdapter().getItem(info.position);
		switch(item.getItemId()) {
		case PROPERTIES:
			mPosition = info.position;
			showDialog(DIALOG_DETAILS);
			return true;
		case RETRY:
			mConnectionManager.transferOperation(this, ClientOp.TRANSFER_RETRY, transfer.projectUrl, transfer.fileName);
			return true;
		case ABORT:
			mPosition = info.position;
			showDialog(DIALOG_WARN_ABORT);
			return true;
		}
		return super.onContextItemSelected(item);
	}


	@Override
	public void clientConnectionProgress(int progress) {
		// We don't care about progress indicator in this activity, just ignore this
	}

	@Override
	public void clientConnected(VersionInfo clientVersion) {
		mConnectedClient = mConnectionManager.getClientId();
		if (mConnectedClient != null) {
			// Connected client is retrieved
			if (Logging.DEBUG) Log.d(TAG, "Client is connected");
			if (mRequestUpdates) {
				// Request fresh data
				mConnectionManager.updateTransfers(this);
			}
		}
	}

	@Override
	public void clientDisconnected() {
		if (Logging.DEBUG) Log.d(TAG, "Client is disconnected");
		mConnectedClient = null;
		mTransfers.clear();
		((BaseAdapter)getListAdapter()).notifyDataSetChanged();
		mViewDirty = false;
	}

	@Override
	public boolean updatedClientMode(ModeInfo modeInfo) {
		// Just ignore
		return false;
	}

	@Override
	public boolean updatedHostInfo(HostInfo hostInfo) {
		// Just ignore
		return false;
	}

	@Override
	public boolean updatedProjects(Vector<ProjectInfo> projects) {
		// Just ignore
		return false;
	}

	@Override
	public boolean updatedTasks(Vector<TaskInfo> tasks) {
		// Just ignore
		return false;
	}

	@Override
	public boolean updatedTransfers(Vector<TransferInfo> transfers) {
		mTransfers = transfers;
		if (mViewUpdatesAllowed) {
			// We are visible, update the view with fresh data
			if (Logging.DEBUG) Log.d(TAG, "Transfers are updated, refreshing view");
			sortTransfers();
			((BaseAdapter)getListAdapter()).notifyDataSetChanged();
		}
		else {
			// We are not visible, do not perform costly tasks now
			if (Logging.DEBUG) Log.d(TAG, "Transfers are updated, but view refresh is delayed");
			mViewDirty = true;
		}
		return mRequestUpdates;
	}

	@Override
	public boolean updatedMessages(Vector<MessageInfo> messages) {
		// Just ignore
		return false;
	}

	private void sortTransfers() {
		// TODO: No sort at the moment
	}

	private String prepareTransferDetails(int position) {
		TransferInfo transfer = mTransfers.elementAt(position);
		return getString(R.string.transferDetailedInfo, 
				TextUtils.htmlEncode(transfer.fileName),
				TextUtils.htmlEncode(transfer.project),
				transfer.size,
				transfer.progress,
				transfer.elapsed,
				transfer.speed,
				transfer.state);
	}
}
