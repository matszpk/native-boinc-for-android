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

import sk.boinc.nativeboinc.debug.Logging;
import android.util.Log;

import edu.berkeley.boinc.lite.BoincBaseParser;
import edu.berkeley.boinc.lite.BoincParserException;

/**
 * @author mat
 *
 */
public class StringReplyParser extends BoincBaseParser {
	private static final String TAG = "StringReplyParser";
	
	private String mString = null;
	
	public String getString() {
		return mString;
	}
	
	public static String parse(String rpcResult) {
		try {
			StringReplyParser parser = new StringReplyParser();
			BoincBaseParser.parse(parser, rpcResult, true);
			return parser.getString();
		} catch (BoincParserException e) {
			if (Logging.DEBUG) Log.d(TAG, "Malformed XML:\n" + rpcResult);
			else if (Logging.INFO) Log.i(TAG, "Malformed XML");
			return null;
		}
	}
	
	@Override
	public void startElement(String localName) {
		super.startElement(localName);
	}
	
	@Override
	public void endElement(String localName) {
		super.endElement(localName);

		if (localName.equalsIgnoreCase("value")) {
			mString = getCurrentElement();
		}
	}
}
