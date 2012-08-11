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
public class AccountOutParser extends BoincBaseParser {
	private static final String TAG = "AccountOutParser";

	private AccountOut mAccountOut = null;
	
	public AccountOut getAccountOut() {
		return mAccountOut;
	}
	
	public static AccountOut parse(String rpcResult) {
		try {
			AccountOutParser parser = new AccountOutParser();
			BoincBaseParser.parse(parser, rpcResult, true);
			return parser.getAccountOut();
		} catch (BoincParserException e) {
			if (Logging.DEBUG) Log.d(TAG, "Malformed XML:\n" + rpcResult);
			else if (Logging.INFO) Log.i(TAG, "Malformed XML");
			return null;
		}
	}
	
	@Override
	public void startElement(String localName) {
		super.startElement(localName);
		if (localName.equalsIgnoreCase("error_num") ||
				localName.equalsIgnoreCase("error_msg") ||
				localName.equalsIgnoreCase("authenticator")) {
			if (mAccountOut==null)
				mAccountOut = new AccountOut();
		}
	}
	
	@Override
	public void endElement(String localName) {
		super.endElement(localName);
		try {
			if (mAccountOut != null) {
				if (localName.equalsIgnoreCase("error_num")) {
					mAccountOut.error_num = Integer.parseInt(getCurrentElement());
				} else if (localName.equalsIgnoreCase("error_msg")) {
					mAccountOut.error_msg = getCurrentElement();
				} else if (localName.equalsIgnoreCase("authenticator")) {
					mAccountOut.authenticator = getCurrentElement();
				}
			}
		} catch (NumberFormatException e) {
			if (Logging.INFO) Log.i(TAG, "Exception when decoding " + localName);
		}
	}
}
