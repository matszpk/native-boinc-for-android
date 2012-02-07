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

import sk.boinc.nativeboinc.R;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.widget.TextView;

/**
 * @author mat
 *
 */
public class StandardDialogs {
	public final static int DIALOG_ERROR = 12345;
	
	private final static String ARG_ERROR = "Error";
	
	public final static Dialog onCreateDialog(Activity activity, int dialogId, Bundle args) {
		if (dialogId == DIALOG_ERROR)
			return new AlertDialog.Builder(activity)
				.setIcon(android.R.drawable.ic_dialog_alert)
				.setTitle(R.string.installError)
				.setView(LayoutInflater.from(activity).inflate(R.layout.dialog, null))
				.setNegativeButton(R.string.ok, null)
				.create();
		return null;
	}
	
	public static boolean onPrepareDialog(Activity activity, int dialogId, Dialog
			dialog, Bundle args) {
		if (dialogId == DIALOG_ERROR) {
			TextView textView = (TextView)dialog.findViewById(R.id.dialogText);
			textView.setText(args.getString(ARG_ERROR));
			return true;
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
		
		activity.showDialog(DIALOG_ERROR, args);
	}
	
	public static void showClientErrorDialog(Activity activity, int errorNum, String errorMessage) {
		Bundle args = new Bundle();
		
		if (errorNum != 0)
			args.putString(ARG_ERROR, errorMessage + " " +
					String.format(activity.getString(R.string.errorNumFormat), errorNum));
		else
			args.putString(ARG_ERROR, errorMessage);
		
		activity.showDialog(DIALOG_ERROR, args);
	}
	
	public static void showInstallErrorDialog(Activity activity, String distribName,
			String errorMessage) {
		Bundle args = new Bundle();
		
		args.putString(ARG_ERROR, distribName + ": " + errorMessage);
		
		activity.showDialog(DIALOG_ERROR, args);
	}
	
	public static void showErrorDialog(Activity activity,String errorMessage) {
		Bundle args = new Bundle();
		args.putString(ARG_ERROR, errorMessage);
		activity.showDialog(DIALOG_ERROR, args);
	}
}
