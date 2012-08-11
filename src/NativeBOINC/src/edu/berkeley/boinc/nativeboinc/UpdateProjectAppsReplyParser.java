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

package edu.berkeley.boinc.nativeboinc;

import edu.berkeley.boinc.lite.BoincBaseParser;
import edu.berkeley.boinc.lite.BoincParserException;

import sk.boinc.nativeboinc.debug.Logging;
import android.util.Log;

/**
 * @author mat
 *
 */
public class UpdateProjectAppsReplyParser extends BoincBaseParser {
	private static final String TAG = "UpdateProjectAppsReplyParser";

	private UpdateProjectAppsReply mUPAR;
	
	public UpdateProjectAppsReply getUpdateProjectAppsReply() {
		return mUPAR;
	}
	
	public static UpdateProjectAppsReply parse(String rpcResult) {
		try {
			UpdateProjectAppsReplyParser parser = new UpdateProjectAppsReplyParser();
			BoincBaseParser.parse(parser, rpcResult, true);
			return parser.getUpdateProjectAppsReply();
		} catch (BoincParserException e) {
			if (Logging.DEBUG) Log.d(TAG, "Malformed XML:\n" + rpcResult);
			else if (Logging.INFO) Log.i(TAG, "Malformed XML");
			return null;
		}
	}
	
	@Override
	public void startElement(String localName) {
		super.startElement(localName);
		if (localName.equalsIgnoreCase("update_project_apps_reply")) {
			if (Logging.INFO) { 
				if (mUPAR != null) {
					// previous <update_project_apps_reply> not closed - dropping it!
					Log.i(TAG, "Dropping unfinished <update_project_apps_reply> data");
				}
			}
			mUPAR = new UpdateProjectAppsReply();
		}
	}
	
	@Override
	public void endElement(String localName) {
		super.endElement(localName);
		try {
			if (mUPAR != null) {
				// we are inside <update_project_apps_reply>
				if (localName.equalsIgnoreCase("update_project_apps_reply")) {
					// Closing tag of <update_project_apps_reply> - nothing to do at the moment
				}
				else {
					// Not the closing tag - we decode possible inner tags
					if (localName.equalsIgnoreCase("error_num")) {
						mUPAR.error_num = Integer.parseInt(getCurrentElement());
					} else if (localName.equalsIgnoreCase("message")) {
						mUPAR.messages.add(getCurrentElement());
					}
				}
			}
		} catch (NumberFormatException e) {
			if (Logging.INFO) Log.i(TAG, "Exception when decoding " + localName);
		}
	}
}
