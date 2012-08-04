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

package edu.berkeley.boinc.nativeboinc;

import sk.boinc.nativeboinc.debug.Logging;
import android.util.Log;

import edu.berkeley.boinc.lite.BoincBaseParser;
import edu.berkeley.boinc.lite.BoincParserException;

/**
 * @author mat
 *
 */
public class ClientEventParser extends BoincBaseParser {
	private static final String TAG = "ClientEventParser";

	private boolean mWithClientEvent = false;
	private int mClientEventType = -1;
	private String mProjectUrl = null;
	
	public ClientEvent getClientEvent() {
		return new ClientEvent(mClientEventType, mProjectUrl);
	}

	public static ClientEvent parse(String rpcResult) {
		try {
			ClientEventParser parser = new ClientEventParser();
			BoincBaseParser.parse(parser, rpcResult);
			return parser.getClientEvent();
		} catch (BoincParserException e) {
			if (Logging.DEBUG) Log.d(TAG, "Malformed XML:\n" + rpcResult);
			else if (Logging.INFO) Log.i(TAG, "Malformed XML");
			return null;
		}
	}
	
	public void startElement(String localName) {
		super.startElement(localName);
		if (localName.equalsIgnoreCase("reply")) {
			if (mWithClientEvent) {
				if (Logging.DEBUG) Log.d(TAG, "Dropping old reply");
			}
			mWithClientEvent = true;
		}
	}
	
	@Override
	public void endElement(String localName) {
		super.endElement(localName);
		
		try {
			if (mWithClientEvent) {
				if (!localName.equalsIgnoreCase("reply")) {
					if (localName.equalsIgnoreCase("type")) {
						mClientEventType = Integer.parseInt(getCurrentElement());
					} else if (localName.equalsIgnoreCase("project")) {
						mProjectUrl = getCurrentElement();
					}
				}
			}
		} catch(NumberFormatException ex) {
			if (Logging.INFO) Log.i(TAG, "Exception when decoding " + localName);
		}
	}
}
