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

package sk.boinc.nativeboinc.util;

import sk.boinc.nativeboinc.BoincManagerApplication;
import sk.boinc.nativeboinc.R;
import sk.boinc.nativeboinc.clientconnection.PollOp;
import sk.boinc.nativeboinc.installer.InstallerService;
import sk.boinc.nativeboinc.nativeclient.NativeBoincService;
import sk.boinc.nativeboinc.service.ConnectionManagerService;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.TextView;

/**
 * @author mat
 *
 */
public class StandardDialogs {
	public final static int DIALOG_ERROR = 12345;
	public final static int DIALOG_CLIENT_ERROR = 12346;
	public final static int DIALOG_POLL_ERROR = 12347;
	public final static int DIALOG_POLL2_ERROR = 12348;
	public final static int DIALOG_INSTALL_ERROR = 12349;
	private static final int DIALOG_CLIENT_INFO = 12350;
	private static final int DIALOG_DISTRIB_INFO = 12351;
	
	private static final String ARG_DISTRIB_NAME = "DistribName";
	private static final String ARG_DISTRIB_VERSION = "DistribVersion";
	private static final String ARG_DISTRIB_DESC = "DistribDesc";
	private static final String ARG_DISTRIB_CHANGES = "DistribChanges";
	
	private final static String ARG_ERROR = "Error";
	
	public final static Dialog onCreateDialog(Activity activity, int dialogId, Bundle args) {
		switch (dialogId) {
		case DIALOG_ERROR:
			return new AlertDialog.Builder(activity)
				.setIcon(android.R.drawable.ic_dialog_alert)
				.setTitle(R.string.error)
				.setView(LayoutInflater.from(activity).inflate(R.layout.dialog, null))
				.setNegativeButton(R.string.ok, null)
				.create();
		case DIALOG_INSTALL_ERROR:
			return new AlertDialog.Builder(activity)
				.setIcon(android.R.drawable.ic_dialog_alert)
				.setTitle(R.string.installError)
				.setView(LayoutInflater.from(activity).inflate(R.layout.dialog, null))
				.setNegativeButton(R.string.ok, null)
				.create();
		case DIALOG_CLIENT_ERROR:
		case DIALOG_POLL_ERROR:
		case DIALOG_POLL2_ERROR:
			return new AlertDialog.Builder(activity)
				.setIcon(android.R.drawable.ic_dialog_alert)
				.setTitle(R.string.clientError)
				.setView(LayoutInflater.from(activity).inflate(R.layout.dialog, null))
				.setNegativeButton(R.string.ok, null)
				.create();
		case DIALOG_CLIENT_INFO:
		case DIALOG_DISTRIB_INFO: {
			LayoutInflater inflater = LayoutInflater.from(activity);
			View view = inflater.inflate(R.layout.distrib_info, null);

			int titleId = -1;
			if (dialogId == DIALOG_CLIENT_INFO)
				titleId = R.string.clientInfo;
			else
				titleId = R.string.appInfo;
			
			return new AlertDialog.Builder(activity)
				.setIcon(android.R.drawable.ic_dialog_info)
				.setTitle(titleId)
				.setView(view)
				.setNegativeButton(R.string.dismiss, null)
				.create();
		}
		}
		return null;
	}
	
	public static boolean onPrepareDialog(Activity activity, int dialogId, Dialog dialog,
			Bundle args) {
		switch (dialogId) {
		case DIALOG_CLIENT_ERROR:
		case DIALOG_POLL_ERROR:
		case DIALOG_POLL2_ERROR:
		case DIALOG_INSTALL_ERROR:
		case DIALOG_ERROR: {
			TextView textView = (TextView)dialog.findViewById(R.id.dialogText);
			textView.setText(args.getString(ARG_ERROR));
			return true;
		}
		case DIALOG_CLIENT_INFO:
		case DIALOG_DISTRIB_INFO: {
			TextView distribVersion = (TextView)dialog.findViewById(R.id.distribVersion);
			TextView distribDesc = (TextView)dialog.findViewById(R.id.distribDesc);
			TextView distribChanges = (TextView)dialog.findViewById(R.id.distribChanges);
			
			String name = args.getString(ARG_DISTRIB_NAME);
			String version = args.getString(ARG_DISTRIB_VERSION);
			
			if (dialogId == DIALOG_CLIENT_INFO)
				distribVersion.setText(activity.getString(R.string.clientVersion) +
						": " + version);
			else // if project info
				distribVersion.setText(name + " " + version);
			
			BoincManagerApplication app = (BoincManagerApplication)activity.getApplication();
			app.setHtmlText(distribDesc, activity.getString(R.string.description), 
					args.getString(ARG_DISTRIB_DESC));
			app.setHtmlText(distribChanges, activity.getString(R.string.changes), 
					args.getString(ARG_DISTRIB_CHANGES));
			return true;
		}
		}
		return false;
	}
	
	public static void showPollErrorDialog(Activity activity, int errorNum, int operation,
			String errorMessage, String param) {
		Bundle args = new Bundle();
		
		String opName = activity.getResources().getStringArray(R.array.pollOps)[operation];
		
		String mainText = null;
		if (param != null && param.length() != 0)
			mainText = opName + ": " + param + ": " + errorMessage;
		else
			mainText = opName + ": " + errorMessage;
		
		if (errorNum != 0)
			args.putString(ARG_ERROR, mainText + " " +
					String.format(activity.getString(R.string.errorNumFormat), errorNum));
		else
			args.putString(ARG_ERROR, mainText);
		
		if (operation != PollOp.POLL_PROJECT_CONFIG)
			activity.showDialog(DIALOG_POLL_ERROR, args);
		else // if project config
			activity.showDialog(DIALOG_POLL2_ERROR, args);
	}
	
	public static void showClientErrorDialog(Activity activity, int errorNum, String errorMessage) {
		Bundle args = new Bundle();
		
		if (errorNum != 0)
			args.putString(ARG_ERROR, errorMessage + " " +
					String.format(activity.getString(R.string.errorNumFormat), errorNum));
		else
			args.putString(ARG_ERROR, errorMessage);
		
		activity.showDialog(DIALOG_CLIENT_ERROR, args);
	}
	
	public static void showInstallErrorDialog(Activity activity, String distribName,
			String errorMessage) {
		Bundle args = new Bundle();
		
		if (distribName.length() != 0)
			args.putString(ARG_ERROR, distribName + ": " + errorMessage);
		else
			args.putString(ARG_ERROR, errorMessage);
		
		activity.showDialog(DIALOG_INSTALL_ERROR, args);
	}
	
	public static void showErrorDialog(Activity activity,String errorMessage) {
		Bundle args = new Bundle();
		args.putString(ARG_ERROR, errorMessage);
		activity.showDialog(DIALOG_ERROR, args);
	}
	
	public static void tryShowDisconnectedErrorDialog(Activity activity, 
			ConnectionManagerService connectionManager, NativeBoincService runner,
			ClientId clientId, boolean disconnectedByManager) {
		
		if (clientId != null) {
			boolean isNativeClient = clientId.isNativeClient();
			 // if not disconnected by manager and
			if ((connectionManager == null || !disconnectedByManager) &&
					// if not native client or
				(!isNativeClient ||
						// if not stopped by manager
						(runner == null || !runner.ifStoppedByManager()))) { // show dial
				Bundle args = new Bundle();
				args.putString(ARG_ERROR, activity.getString(R.string.clientDisconnected));
				activity.showDialog(DIALOG_ERROR, args);
			}
		}
	}
	
	/* info dialog */
	public static void showDistribInfoDialog(Activity activity, String name, String version,
			String description, String changes) {
		Bundle args = new Bundle();
		args.putString(ARG_DISTRIB_NAME, name);
		args.putString(ARG_DISTRIB_VERSION, version);
		args.putString(ARG_DISTRIB_DESC, description);
		args.putString(ARG_DISTRIB_CHANGES, changes);
		
		if (name.equals(InstallerService.BOINC_CLIENT_ITEM_NAME))
			activity.showDialog(DIALOG_CLIENT_INFO, args);
		else // if distrib
			activity.showDialog(DIALOG_DISTRIB_INFO, args);
	}
	
	/* fault tolerant dismiss dialog */
	public static void dismissDialog(Activity activity, int dialogId) {
		try {
			activity.dismissDialog(dialogId);
		} catch(IllegalArgumentException ex) { }
	}
}
