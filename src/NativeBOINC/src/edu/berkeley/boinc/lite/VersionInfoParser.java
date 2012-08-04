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

package edu.berkeley.boinc.lite;

import sk.boinc.nativeboinc.debug.Logging;
import android.util.Log;

public class VersionInfoParser extends BoincBaseParser {
	private static final String TAG = "VersionInfoParser";
	private VersionInfo mVersionInfo = null;

	public final VersionInfo getVersionInfo() {
		return mVersionInfo;
	}

	/**
	 * Parse the RPC result (host_info) and generate vector of projects info
	 * @param rpcResult String returned by RPC call of core client
	 * @return VersionInfo (of core client)
	 */
	public static VersionInfo parse(String rpcResult) {
		try {
			VersionInfoParser parser = new VersionInfoParser();
			BoincBaseParser.parse(parser, rpcResult);
			return parser.getVersionInfo();
		}
		catch (BoincParserException e) {
			if (Logging.DEBUG) Log.d(TAG, "Malformed XML:\n" + rpcResult);
			else if (Logging.INFO) Log.i(TAG, "Malformed XML");
			return null;
		}		
	}

	@Override
	public void startElement(String localName) {
		super.startElement(localName);
		if (localName.equalsIgnoreCase("server_version")) {
			if (Logging.INFO) { 
				if (mVersionInfo != null) {
					// previous <server_version> not closed - dropping it!
					Log.i(TAG, "Dropping unfinished <server_version> data");
				}
			}
			mVersionInfo = new VersionInfo();
		}
	}
	
	@Override
	public void endElement(String localName) {
		super.endElement(localName);
		try {
			if (mVersionInfo != null) {
				// we are inside <server_version>
				if (localName.equalsIgnoreCase("server_version")) {
					// Closing tag of <server_version> - nothing to do at the moment
				}
				else {
					// Not the closing tag - we decode possible inner tags
					if (localName.equalsIgnoreCase("major")) {
						mVersionInfo.major = Integer.parseInt(getCurrentElement());
					}
					else if (localName.equalsIgnoreCase("minor")) {
						mVersionInfo.minor = Integer.parseInt(getCurrentElement());
					}
					else if (localName.equalsIgnoreCase("release")) {
						mVersionInfo.release = Integer.parseInt(getCurrentElement());
					}
				}
			}
		}
		catch (NumberFormatException e) {
			if (Logging.INFO) Log.i(TAG, "Exception when decoding " + localName);
		}
	}
}
