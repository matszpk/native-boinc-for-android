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

import org.xml.sax.Attributes;
import org.xml.sax.SAXException;

import edu.berkeley.boinc.lite.BaseParser;

import sk.boinc.nativeboinc.debug.Logging;
import android.util.Log;
import android.util.Xml;

/**
 * @author mat
 *
 */
public class UpdateProjectAppsReplyParser extends BaseParser {
	private static final String TAG = "UpdateProjectAppsReplyParser";

	private UpdateProjectAppsReply mUPAR;
	
	public UpdateProjectAppsReply getUpdateProjectAppsReply() {
		return mUPAR;
	}
	
	public static UpdateProjectAppsReply parse(String rpcResult) {
		try {
			UpdateProjectAppsReplyParser parser = new UpdateProjectAppsReplyParser();
			Xml.parse(rpcResult, parser);
			return parser.getUpdateProjectAppsReply();
		} catch (SAXException e) {
			if (Logging.DEBUG) Log.d(TAG, "Malformed XML:\n" + rpcResult);
			else if (Logging.INFO) Log.i(TAG, "Malformed XML");
			return null;
		}
	}
	
	@Override
	public void startElement(String uri, String localName, String qName, Attributes attributes) throws SAXException {
		super.startElement(uri, localName, qName, attributes);
		if (localName.equalsIgnoreCase("update_project_apps_reply")) {
			if (Logging.INFO) { 
				if (mUPAR != null) {
					// previous <update_project_apps_reply> not closed - dropping it!
					Log.i(TAG, "Dropping unfinished <update_project_apps_reply> data");
				}
			}
			mUPAR = new UpdateProjectAppsReply();
		}
		else {
			// Another element, hopefully primitive and not constructor
			// (although unknown constructor does not hurt, because there will be primitive start anyway)
			mElementStarted = true;
			mCurrentElement.setLength(0);
		}
	}
	
	@Override
	public void endElement(String uri, String localName, String qName) throws SAXException {
		super.endElement(uri, localName, qName);
		try {
			if (mUPAR != null) {
				// we are inside <update_project_apps_reply>
				if (localName.equalsIgnoreCase("update_project_apps_reply")) {
					// Closing tag of <update_project_apps_reply> - nothing to do at the moment
				}
				else {
					// Not the closing tag - we decode possible inner tags
					trimEnd();
					if (localName.equalsIgnoreCase("error_num")) {
						mUPAR.error_num = Integer.parseInt(mCurrentElement.toString());
					} else if (localName.equalsIgnoreCase("message")) {
						mUPAR.messages.add(mCurrentElement.toString());
					}
				}
			}
		} catch (NumberFormatException e) {
			if (Logging.INFO) Log.i(TAG, "Exception when decoding " + localName);
		}
		mElementStarted = false;
	}
}
