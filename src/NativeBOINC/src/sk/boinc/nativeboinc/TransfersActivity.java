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

package sk.boinc.nativeboinc;

import java.util.ArrayList;
import java.util.HashSet;

import sk.boinc.nativeboinc.bridge.AutoRefresh;
import sk.boinc.nativeboinc.clientconnection.AutoRefreshListener;
import sk.boinc.nativeboinc.clientconnection.BoincOp;
import sk.boinc.nativeboinc.clientconnection.ClientOp;
import sk.boinc.nativeboinc.clientconnection.ClientUpdateTransfersReceiver;
import sk.boinc.nativeboinc.clientconnection.TransferDescriptor;
import sk.boinc.nativeboinc.clientconnection.TransferInfo;
import sk.boinc.nativeboinc.clientconnection.VersionInfo;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.service.ConnectionManagerService;
import sk.boinc.nativeboinc.util.ClientId;
import sk.boinc.nativeboinc.util.ScreenOrientationHandler;
import sk.boinc.nativeboinc.util.StandardDialogs;
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
import android.os.SystemClock;
import android.text.Html;
import android.text.TextUtils;
import android.util.Log;
import android.view.ContextMenu;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.ContextMenu.ContextMenuInfo;
import android.widget.BaseAdapter;
import android.widget.CheckBox;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.AdapterView.AdapterContextMenuInfo;


public class TransfersActivity extends ListActivity implements ClientUpdateTransfersReceiver,
		AutoRefreshListener {
	private static final String TAG = "TransfersActivity";

	private static final int DIALOG_DETAILS = 1;
	private static final int DIALOG_WARN_ABORT = 2;

	private static final int RETRY = 1;
	private static final int ABORT = 2;
	private static final int PROPERTIES = 3;
	private static final int SELECT_ALL = 4;
	private static final int UNSELECT_ALL = 5;

	private ScreenOrientationHandler mScreenOrientation;

	private boolean mRequestUpdates = false;
	private boolean mViewUpdatesAllowed = false;
	private boolean mViewDirty = false;

	private ArrayList<TransferInfo> mTransfers = new ArrayList<TransferInfo>();
	private ArrayList<TransferInfo> mPendingTransfers = new ArrayList<TransferInfo>();
	private HashSet<TransferDescriptor> mSelectedTransfers = new HashSet<TransferDescriptor>();
	//private int mPosition = 0;
	private TransferInfo mChoosenTransfer = null;
	private boolean mShowDetailsDialog = false;
	private boolean mShowWarnAbortDialog = false;
	
	private boolean mUpdateTransfersInProgress = false;
	private long mLastUpdateTime = -1;
	private boolean mAfterRecreating = false;
	
	private ClientId mPrevClientId = null;
	private ClientId mOperationClientId = null;
	private boolean mContextMenuOpened = false;
	
	private static class SavedState {
		private final ArrayList<TransferInfo> transfers;
		private final ArrayList<TransferInfo> pendingTransfers;
		private final HashSet<TransferDescriptor> selectedTransfers;
		private TransferInfo choosenTransfer;
		private final boolean showDetailsDialog;
		private final boolean showWarnAbortDialog;
		private final boolean updateTransfersInProgress;
		private final long lastUpdateTime;
		private final ClientId prevClientId;
		private final boolean contextMenuOpened;
		private final ClientId operationClientId;

		public SavedState(TransfersActivity activity) {
			transfers = activity.mTransfers;
			if (Logging.DEBUG) Log.d(TAG, "saved: transfers.size()=" + transfers.size());
			pendingTransfers = activity.mPendingTransfers;
			selectedTransfers = activity.mSelectedTransfers;
			choosenTransfer = activity.mChoosenTransfer;
			showDetailsDialog = activity.mShowDetailsDialog;
			showWarnAbortDialog = activity.mShowWarnAbortDialog;
			updateTransfersInProgress = activity.mUpdateTransfersInProgress;
			lastUpdateTime = activity.mLastUpdateTime;
			prevClientId = activity.mPrevClientId;
			contextMenuOpened = activity.mContextMenuOpened;
			operationClientId = activity.mOperationClientId;
		}
		public void restoreState(TransfersActivity activity) {
			activity.mTransfers = transfers;
			if (Logging.DEBUG) Log.d(TAG, "restored: mTransfers.size()=" + activity.mTransfers.size());
			activity.mPendingTransfers = pendingTransfers;
			activity.mSelectedTransfers = selectedTransfers;
			activity.mChoosenTransfer = choosenTransfer;
			activity.mShowDetailsDialog = showDetailsDialog;
			activity.mUpdateTransfersInProgress = updateTransfersInProgress;
			activity.mShowWarnAbortDialog = showWarnAbortDialog;
			activity.mLastUpdateTime = lastUpdateTime;
			activity.mPrevClientId = prevClientId;
			activity.mContextMenuOpened = contextMenuOpened;
			activity.mOperationClientId = operationClientId;
		}
	}
	
	private class TransferOnClickListener implements View.OnClickListener {
		private int mItemPosition;
		
		public TransferOnClickListener(int position) {
			mItemPosition = position;
		}
		
		public void setItemPosition(int position) {
			mItemPosition = position;
		}
		
		@Override
		public void onClick(View view) {
			if (mTransfers.size() <= mItemPosition)
				return; // do nothing
			TransferInfo transfer = mTransfers.get(mItemPosition);
			if (Logging.DEBUG) Log.d(TAG, "Transfer "+transfer.fileName+" change checked:"+mItemPosition);
			
			TransferDescriptor txDesc = new TransferDescriptor(transfer.projectUrl, transfer.fileName);
			if (mSelectedTransfers.contains(txDesc)) // remove
				mSelectedTransfers.remove(txDesc);
			else // add to selected
				mSelectedTransfers.add(txDesc);
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
			return mTransfers.get(position);
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
			TransferInfo transfer = mTransfers.get(position);
			
			CheckBox checkBox = (CheckBox)layout.findViewById(R.id.check);
			checkBox.setChecked(mSelectedTransfers.contains(
					new TransferDescriptor(transfer.projectUrl, transfer.fileName)));
			
			TransferOnClickListener clickListener = (TransferOnClickListener)layout.getTag();
			if (clickListener == null) {
				clickListener = new TransferOnClickListener(position);
				layout.setTag(clickListener);
			} else {
				clickListener.setItemPosition(position);
			}
			checkBox.setOnClickListener(clickListener);
			
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
					progressWaiting.setSecondaryProgress(1000);
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
			
			if (mConnectionManager.getClientId() == null) {
				if (Logging.DEBUG) Log.d(TAG, "when is now disconnected");
				// if after disconnection
				clientDisconnected(false);
			}
		}

		@Override
		public void onServiceDisconnected(ComponentName name) {
			if (mConnectionManager != null)
				mConnectionManager.unregisterStatusObserver(TransfersActivity.this);
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
		setContentView(R.layout.tab_layout);
		
		setListAdapter(new TransferListAdapter(this));
		registerForContextMenu(getListView());
		View emptyView = findViewById(R.id.emptyContent);
		registerForContextMenu(emptyView);
		getListView().setFastScrollEnabled(true);
		
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
			mAfterRecreating = true;
		}
	}

	@Override
	protected void onResume() {
		super.onResume();
		mScreenOrientation.setOrientation();
		mRequestUpdates = true;
		
		Log.d(TAG, "onUpdaTransfersprogress:"+mUpdateTransfersInProgress);
		if (mConnectedClient != null) {
			if (mUpdateTransfersInProgress) {
				ArrayList<TransferInfo> transfers = (ArrayList<TransferInfo>)mConnectionManager
						.getPendingOutput(BoincOp.UpdateTransfers);
				if (transfers != null) // if already updated
					updatedTransfers(transfers);
				
				mConnectionManager.addToScheduledUpdates(this, AutoRefresh.TRANSFERS, -1);
			} else { // if after update
				int autoRefresh = mConnectionManager.getAutoRefresh();
				
				if (autoRefresh != -1) {
					long period = SystemClock.elapsedRealtime()-mLastUpdateTime;
					if (period >= autoRefresh*1000) {
						// if later than 4 seconds
						if (Logging.DEBUG) Log.d(TAG, "do update transfers");
						mConnectionManager.updateTransfers();
					} else { // only add auto updates
						if (Logging.DEBUG) Log.d(TAG, "do add to schedule update transfers");
						mConnectionManager.addToScheduledUpdates(this, AutoRefresh.TRANSFERS,
								(int)(autoRefresh*1000-period));
					}
				}
			}
		}
		mViewUpdatesAllowed = true;
		if (mViewDirty) {
			// There were some updates received while we were not visible
			// The data are stored, but view is not updated yet; Do it now
			//sortTransfers();
			mTransfers = mPendingTransfers; 
			((BaseAdapter)getListAdapter()).notifyDataSetChanged();
			mViewDirty = false;
			if (Logging.DEBUG) Log.d(TAG, "Delayed refresh of view was done now");
		}
		
		if (mShowDetailsDialog) {
			if (mChoosenTransfer != null) {
				showDialog(DIALOG_DETAILS);
			} else {
				if (Logging.DEBUG) Log.d(TAG, "If failed to recreate details");
				mShowDetailsDialog = false;
			}
		}
		if (mShowWarnAbortDialog) {
			if (mChoosenTransfer != null || !mSelectedTransfers.isEmpty()) {
				showDialog(DIALOG_WARN_ABORT);
			} else {
				if (Logging.DEBUG) Log.d(TAG, "If failed to recreate warn abort");
				mShowWarnAbortDialog = false;
			}
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
			mConnectionManager.cancelScheduledUpdates(AutoRefresh.TRANSFERS);
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
		return new SavedState(this);
	}

	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		Activity parent = getParent();
		if (parent != null) {
			return parent.onKeyDown(keyCode, event);
		}
		return super.onKeyDown(keyCode, event);
	}
	
	private void onDismissDetailsDialog() {
		if (Logging.DEBUG) Log.d(TAG, "On dismiss details");
		mShowDetailsDialog = false;
	}

	private void onDismissWarnAbortDialog() {
		if (Logging.DEBUG) Log.d(TAG, "On dismiss warn abort");
		mShowWarnAbortDialog = false;
	}
	
	@Override
	protected Dialog onCreateDialog(int id) {
		switch (id) {
		case DIALOG_DETAILS:
			return new AlertDialog.Builder(this)
				.setIcon(android.R.drawable.ic_dialog_info)
				.setView(LayoutInflater.from(this).inflate(R.layout.dialog, null))
				.setNegativeButton(R.string.ok, new DialogInterface.OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which) {
						onDismissDetailsDialog();
					}
				})
				.setOnCancelListener(new DialogInterface.OnCancelListener() {
					@Override
					public void onCancel(DialogInterface dialog) {
						onDismissDetailsDialog();
					}
				})
				.create();
		case DIALOG_WARN_ABORT:
	    	return new AlertDialog.Builder(this)
	    		.setIcon(android.R.drawable.ic_dialog_alert)
	    		.setTitle(R.string.warning)
				.setView(LayoutInflater.from(this).inflate(R.layout.dialog, null))
	    		.setPositiveButton(R.string.transferAbort,
	    			new DialogInterface.OnClickListener() {
	    				public void onClick(DialogInterface dialog, int whichButton) {
	    					onDismissWarnAbortDialog();
	    					
	    					if (!mOperationClientId.equals(mConnectedClient)) // do nothing when other client
	    						return;
	    					
	    					if (mSelectedTransfers.isEmpty()) {
	    						mConnectionManager.transferOperation(ClientOp.TRANSFER_ABORT, mChoosenTransfer.projectUrl,
	    								mChoosenTransfer.fileName);
	    					} else
	    						mConnectionManager.transfersOperation(ClientOp.TRANSFER_ABORT,
	    								mSelectedTransfers.toArray(new TransferDescriptor[0]));
	    				}
	    			})
	    		.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which) {
						onDismissWarnAbortDialog();
					}
				})
				.setOnCancelListener(new DialogInterface.OnCancelListener() {
					@Override
					public void onCancel(DialogInterface dialog) {
						onDismissWarnAbortDialog();
					}
				})
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
			text.setText(Html.fromHtml(prepareTransferDetails(mChoosenTransfer)));
			break;
		case DIALOG_WARN_ABORT:
			text = (TextView)dialog.findViewById(R.id.dialogText);
			if (mSelectedTransfers.isEmpty())
				text.setText(getString(R.string.warnAbortTransfer, mChoosenTransfer.fileName));
			else
				text.setText(getString(R.string.warnAbortTransfers));
			break;
		}
	}

	@Override
	protected void onListItemClick(ListView l, View v, int position, long id) {
		mChoosenTransfer = (TransferInfo)getListAdapter().getItem(position);
		mShowDetailsDialog = true;
		showDialog(DIALOG_DETAILS);
	}

	@Override
	public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo) {
		super.onCreateContextMenu(menu, v, menuInfo);
		AdapterContextMenuInfo info = (AdapterContextMenuInfo)menuInfo;
		
		mOperationClientId = mConnectedClient;
		mContextMenuOpened = true;
		
		TransferInfo transfer = null;
		if (info != null)
			transfer = (TransferInfo)getListAdapter().getItem(info.position);
		menu.setHeaderTitle(R.string.taskCtxMenuTitle);
		
		if (mSelectedTransfers.size() != mTransfers.size())
			menu.add(0, SELECT_ALL, 0, R.string.selectAll);
		
		if (!mSelectedTransfers.isEmpty())
			menu.add(0, UNSELECT_ALL, 0, R.string.unselectAll);
		
		if (mSelectedTransfers.isEmpty()) {
			if (transfer != null) {
				if ((transfer.stateControl & (TransferInfo.ABORTED|TransferInfo.FAILED)) == 0) {
					// Not aborted
					menu.add(0, ABORT, 0, R.string.transferAbort);
					if ((transfer.stateControl & (TransferInfo.RUNNING)) == 0) {
						// Not running now
						menu.add(0, RETRY, 0, R.string.transferRetry);
					}
				}
			}
		} else {
			menu.add(0, ABORT, 0, R.string.transferAbort);
			menu.add(0, RETRY, 0, R.string.transferRetry);
		}
		if (transfer != null)
			menu.add(0, PROPERTIES, 0, R.string.transferProperties);
	}

	@Override
	public boolean onContextItemSelected(MenuItem item) {
		AdapterContextMenuInfo info = (AdapterContextMenuInfo)item.getMenuInfo();
		
		if (!mOperationClientId.equals(mConnectedClient)) // do nothing when other client
			return super.onContextItemSelected(item);
		
		TransferInfo transfer = null;
		if (info != null)
			transfer = (TransferInfo)getListAdapter().getItem(info.position);
		
		switch(item.getItemId()) {
		case PROPERTIES:
			mChoosenTransfer = transfer;
			showDialog(DIALOG_DETAILS);
			return true;
		case RETRY:
			if (mSelectedTransfers.isEmpty())
				mConnectionManager.transferOperation(ClientOp.TRANSFER_RETRY, transfer.projectUrl, transfer.fileName);
			else
				mConnectionManager.transfersOperation(ClientOp.TRANSFER_RETRY,
						mSelectedTransfers.toArray(new TransferDescriptor[0]));
			return true;
		case ABORT:
			mChoosenTransfer = transfer;
			showDialog(DIALOG_WARN_ABORT);
			return true;
		case SELECT_ALL:
			for (TransferInfo tx: mTransfers)
				mSelectedTransfers.add(new TransferDescriptor(tx.projectUrl, tx.fileName));
			((BaseAdapter)getListAdapter()).notifyDataSetChanged();
			return true;
		case UNSELECT_ALL:
			mSelectedTransfers.clear();
			((BaseAdapter)getListAdapter()).notifyDataSetChanged();
			return true;
		}
		return super.onContextItemSelected(item);
	}

	@Override
	public void onContextMenuClosed(Menu menu) {
		if (Logging.DEBUG) Log.d(TAG, "Context menu closed");
		mContextMenuOpened = false;
	}

	@Override
	public void clientConnectionProgress(BoincOp boincOp, int progress) {
		// We don't care about progress indicator in this activity, just ignore this
	}

	@Override
	public void clientConnected(VersionInfo clientVersion) {
		mConnectedClient = mConnectionManager.getClientId();
		if (mConnectedClient != null) {
			if (mPrevClientId != null && !mPrevClientId.equals(mConnectedClient)) {
				if (Logging.DEBUG) Log.d(TAG, "Other client is connected");
				if (mContextMenuOpened) {
					closeContextMenu();
				}
				mShowDetailsDialog = false;
				mShowWarnAbortDialog = false;
				StandardDialogs.dismissDialog(this, DIALOG_DETAILS);
				StandardDialogs.dismissDialog(this, DIALOG_WARN_ABORT);
				mSelectedTransfers.clear();
				mPrevClientId = mConnectedClient;
			}
			
			// Connected client is retrieved
			if (Logging.DEBUG) Log.d(TAG, "Client is connected");
			if (mRequestUpdates) {
				// Request fresh data
				if (!mUpdateTransfersInProgress && !mAfterRecreating) {
					if (Logging.DEBUG) Log.d(TAG, "do update transfers");
					mUpdateTransfersInProgress = true;
					mConnectionManager.updateTransfers();
				} else {
					if (Logging.DEBUG) Log.d(TAG, "do add to scheduled updates");
					int autoRefresh = mConnectionManager.getAutoRefresh();
					
					if (autoRefresh != -1) {
						long period = SystemClock.elapsedRealtime()-mLastUpdateTime;
						Log.d(TAG, "period at update:"+period+","+(autoRefresh*1000-period));
						mConnectionManager.addToScheduledUpdates(this, AutoRefresh.TRANSFERS,
								(int)(autoRefresh*1000-period));
					} else
						mConnectionManager.addToScheduledUpdates(this, AutoRefresh.TRANSFERS, -1);
					mAfterRecreating = false;
				}
			}
		}
	}

	@Override
	public void clientDisconnected(boolean disconnectedByManager) {
		if (Logging.DEBUG) Log.d(TAG, "Client is disconnected");
		mPrevClientId = mConnectedClient;
		mConnectedClient = null;
		mUpdateTransfersInProgress = false;
		mTransfers.clear();
		updateSelectedTransfers();
		((BaseAdapter)getListAdapter()).notifyDataSetChanged();
		mViewDirty = false;
		mAfterRecreating = false;
	}
	
	@Override
	public boolean clientError(BoincOp boincOp, int err_num, String message) {
		if (!boincOp.equals(BoincOp.UpdateTransfers))
			return false;
		// do not consume
		mUpdateTransfersInProgress = false;
		mAfterRecreating = false;
		return false;
	}

	@Override
	public boolean updatedTransfers(ArrayList<TransferInfo> transfers) {
		mPendingTransfers = transfers;
		mUpdateTransfersInProgress = false;
		mLastUpdateTime = SystemClock.elapsedRealtime();
		
		sortTransfers();
		updateSelectedTransfers();
		
		if (mViewUpdatesAllowed) {
			// We are visible, update the view with fresh data
			if (Logging.DEBUG) Log.d(TAG, "Transfers are updated, refreshing view");
			mTransfers = mPendingTransfers;
			((BaseAdapter)getListAdapter()).notifyDataSetChanged();
		}
		else {
			// We are not visible, do not perform costly tasks now
			if (Logging.DEBUG) Log.d(TAG, "Transfers are updated, but view refresh is delayed");
			mViewDirty = true;
		}
		return mRequestUpdates;
	}

	private void sortTransfers() {
		// TODO: No sort at the moment
	}
	
	private void updateSelectedTransfers() {
		ArrayList<TransferDescriptor> toRetain = new ArrayList<TransferDescriptor>();
		for (TransferInfo txInfo: mPendingTransfers)
			toRetain.add(new TransferDescriptor(txInfo.projectUrl, txInfo.fileName));
		mSelectedTransfers.retainAll(toRetain);
		//if (Logging.DEBUG) Log.d(TAG, "update selected transfers:"+mSelectedTransfers);
	}

	private String prepareTransferDetails(TransferInfo transfer) {
		return getString(R.string.transferDetailedInfo, 
				TextUtils.htmlEncode(transfer.fileName),
				TextUtils.htmlEncode(transfer.project),
				transfer.size,
				transfer.progress,
				transfer.elapsed,
				transfer.speed,
				transfer.state);
	}

	@Override
	public void onClientIsWorking(boolean isWorking) {
		// TODO Auto-generated method stub
		
	}
	
	@Override
	public void onStartAutoRefresh(int requestType) {
		if (requestType == AutoRefresh.TRANSFERS) // in progress
			mUpdateTransfersInProgress = true;
	}
}
