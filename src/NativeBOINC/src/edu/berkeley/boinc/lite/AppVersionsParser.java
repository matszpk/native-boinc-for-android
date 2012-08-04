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

import java.util.ArrayList;

import sk.boinc.nativeboinc.debug.Logging;

import android.util.Log;


public class AppVersionsParser extends BoincBaseParser {
	private static final String TAG = "AppVersionsParser";

	private ArrayList<AppVersion> mAppVersions = new ArrayList<AppVersion>();
	private AppVersion mAppVersion = null;

	public final ArrayList<AppVersion> getAppVersions() {
		return mAppVersions;
	}

	/**
	 * Parse the RPC result (app_version) and generate corresponding vector
	 * @param rpcResult String returned by RPC call of core client
	 * @return vector of application version
	 */
	public static ArrayList<AppVersion> parse(String rpcResult) {
		try {
			AppVersionsParser parser = new AppVersionsParser();
			BoincBaseParser.parse(parser, rpcResult);
			return parser.getAppVersions();
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
		if (localName.equalsIgnoreCase("app_version")) {
			mAppVersion = new AppVersion();
		}
	}

	@Override
	public void endElement(String localName) {
		super.endElement(localName);
		try {
			if (mAppVersion != null) {
				// We are inside <app_version>
				if (localName.equalsIgnoreCase("app_version")) {
					// Closing tag of <app_version> - add to vector and be ready for next one
					if (!mAppVersion.app_name.equals("")) {
						// app_name is a must
						mAppVersions.add(mAppVersion);
					}
					mAppVersion = null;
				}
				else {
					// Not the closing tag - we decode possible inner tags
					if (localName.equalsIgnoreCase("app_name")) {
						mAppVersion.app_name = mCurrentElement;
					}
					else if (localName.equalsIgnoreCase("version_num")) {
						mAppVersion.version_num = Integer.parseInt(mCurrentElement);
					}
				}
			}
		}
		catch (NumberFormatException e) {
			if (Logging.INFO) Log.i(TAG, "Exception when decoding " + localName);
		}
	}
}
