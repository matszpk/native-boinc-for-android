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

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Bundle;

/**
 * @author mat
 *
 */
public class BugCatchErrorActivity extends Activity {
	
	private static final int DIALOG_BUG_CATCHER_ERROR = 1;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		showDialog(DIALOG_BUG_CATCHER_ERROR);
	}
	
	@Override
	protected Dialog onCreateDialog(int id) {
		if (id == DIALOG_BUG_CATCHER_ERROR)
			return new AlertDialog.Builder(this)
	    		.setIcon(android.R.drawable.ic_dialog_alert)
	    		.setTitle(R.string.warning)
	    		.setMessage(R.string.bugCatchAttachFailMessage)
	    		.setPositiveButton(R.string.dismiss,  new DialogInterface.OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which) {
						BugCatchErrorActivity.this.finish();
					}
				})
				.setOnCancelListener(new DialogInterface.OnCancelListener() {
					@Override
					public void onCancel(DialogInterface dialog) {
						BugCatchErrorActivity.this.finish();
					}
				})
	    		.create();
		return null;
	}
}
