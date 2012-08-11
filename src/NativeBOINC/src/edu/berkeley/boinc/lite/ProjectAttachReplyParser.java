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

package edu.berkeley.boinc.lite;

import sk.boinc.nativeboinc.debug.Logging;
import android.util.Log;

/**
 * @author mat
 *
 */
public class ProjectAttachReplyParser extends BoincBaseParser {
	private static final String TAG = "ProjectAttachReplyParser";

	private ProjectAttachReply mPAR;
	
	public ProjectAttachReply getProjectAttachReply() {
		return mPAR;
	}
	
	public static ProjectAttachReply parse(String rpcResult) {
		try {
			ProjectAttachReplyParser parser = new ProjectAttachReplyParser();
			BoincBaseParser.parse(parser, rpcResult, true);
			return parser.getProjectAttachReply();
		} catch (BoincParserException  e) {
			if (Logging.DEBUG) Log.d(TAG, "Malformed XML:\n" + rpcResult);
			else if (Logging.INFO) Log.i(TAG, "Malformed XML");
			return null;
		}
	}
	
	@Override
	public void startElement(String localName) {
		super.startElement(localName);
		if (localName.equalsIgnoreCase("project_attach_reply")) {
			if (Logging.INFO) { 
				if (mPAR != null) {
					// previous <project_attach_reply> not closed - dropping it!
					Log.i(TAG, "Dropping unfinished <project_attach_reply> data");
				}
			}
			mPAR = new ProjectAttachReply();
		}
	}
	
	@Override
	public void endElement(String localName) {
		super.endElement(localName);
		try {
			if (mPAR != null) {
				// we are inside <project_attach_reply>
				if (localName.equalsIgnoreCase("project_attach_reply")) {
					// Closing tag of <project_attach_reply> - nothing to do at the moment
				}
				else {
					// Not the closing tag - we decode possible inner tags
					if (localName.equalsIgnoreCase("error_num")) {
						mPAR.error_num = Integer.parseInt(getCurrentElement());
					} else if (localName.equalsIgnoreCase("message")) {
						mPAR.messages.add(getCurrentElement());
					}
				}
			}
		} catch (NumberFormatException e) {
			if (Logging.INFO) Log.i(TAG, "Exception when decoding " + localName);
		}
	}
}
