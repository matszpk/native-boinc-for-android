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


public class AppsParser extends BoincBaseParser {
	private static final String TAG = "AppsParser";

	private ArrayList<App> mApps = new ArrayList<App>();
	private App mApp = null;


	public final ArrayList<App> getApps() {
		return mApps;
	}

	/**
	 * Parse the RPC result (app) and generate vector of app
	 * @param rpcResult String returned by RPC call of core client
	 * @return vector of app
	 */
	public static ArrayList<App> parse(String rpcResult) {
		try {
			AppsParser parser = new AppsParser();
			BoincBaseParser.parse(parser, rpcResult);
			return parser.getApps();
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
		if (localName.equalsIgnoreCase("app")) {
			mApp = new App();
		}
	}

	@Override
	public void endElement(String localName) {
		super.endElement(localName);
		if (mApp != null) {
			// We are inside <app>
			if (localName.equalsIgnoreCase("app")) {
				// Closing tag of <app> - add to vector and be ready for next one
				if (!mApp.name.equals("")) {
					// name is a must
					mApps.add(mApp);
				}
				mApp = null;
			}
			else {
				// Not the closing tag - we decode possible inner tags
				if (localName.equalsIgnoreCase("name")) {
					mApp.name = getCurrentElement();
				}
				else if (localName.equalsIgnoreCase("user_friendly_name")) {
					mApp.user_friendly_name = getCurrentElement();
				}
			}
		}
	}
}
