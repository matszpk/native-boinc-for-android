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

import org.xml.sax.Attributes;
import org.xml.sax.SAXException;

import sk.boinc.nativeboinc.debug.Logging;
import android.util.Log;
import android.util.Xml;

/**
 * @author mat
 *
 */
public class AccountOutParser extends BaseParser {
	private static final String TAG = "AccountOutParser";

	private AccountOut mAccountOut = null;
	
	public AccountOut getAccountOut() {
		return mAccountOut;
	}
	
	public static AccountOut parse(String rpcResult) {
		try {
			String outResult;
			int xmlHeaderStart = rpcResult.indexOf("<?xml");
			if (xmlHeaderStart!=-1) { // remove xml header inside body
				int xmlHeaderEnd = rpcResult.indexOf("?>");
				outResult = rpcResult.substring(0, xmlHeaderStart);
				outResult += rpcResult.substring(xmlHeaderEnd+2);
			} else
				outResult = rpcResult;
			
			AccountOutParser parser = new AccountOutParser();
			Xml.parse(outResult, parser);
			return parser.getAccountOut();
		} catch (SAXException e) {
			if (Logging.DEBUG) Log.d(TAG, "Malformed XML:\n" + rpcResult);
			else if (Logging.INFO) Log.i(TAG, "Malformed XML");
			return null;
		}
	}
	
	@Override
	public void startElement(String uri, String localName, String qName, Attributes attributes) throws SAXException {
		super.startElement(uri, localName, qName, attributes);
		if (localName.equalsIgnoreCase("error_num") ||
				localName.equalsIgnoreCase("error_msg") ||
				localName.equalsIgnoreCase("authenticator")) {
			if (mAccountOut==null)
				mAccountOut = new AccountOut();
		} else {
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
			if (mAccountOut != null) {
				trimEnd();
				if (localName.equalsIgnoreCase("error_num")) {
					mAccountOut.error_num = Integer.parseInt(mCurrentElement.toString());
				} else if (localName.equalsIgnoreCase("error_msg")) {
					mAccountOut.error_msg = mCurrentElement.toString();
				} else if (localName.equalsIgnoreCase("authenticator")) {
					mAccountOut.authenticator = mCurrentElement.toString();
				}
			}
		} catch (NumberFormatException e) {
			if (Logging.INFO) Log.i(TAG, "Exception when decoding " + localName);
		}
		mElementStarted = false;
	}
}
