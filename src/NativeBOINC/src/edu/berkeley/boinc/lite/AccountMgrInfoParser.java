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
public class AccountMgrInfoParser extends BaseParser {
	private static final String TAG = "AccountMgrInfoParser";

	private AccountMgrInfo mAccountMgrInfo = null;
	
	public AccountMgrInfo getAccountMgrInfo() {
		return mAccountMgrInfo;
	}

	public static AccountMgrInfo parse(String rpcResult) {
		try {
			AccountMgrInfoParser parser = new AccountMgrInfoParser();
			Xml.parse(rpcResult, parser);
			return parser.getAccountMgrInfo();
		} catch (SAXException e) {
			if (Logging.DEBUG) Log.d(TAG, "Malformed XML:\n" + rpcResult);
			else if (Logging.INFO) Log.i(TAG, "Malformed XML");
			return null;
		}
	}
	
	@Override
	public void startElement(String uri, String localName, String qName, Attributes attributes) throws SAXException {
		super.startElement(uri, localName, qName, attributes);
		if (localName.equalsIgnoreCase("acct_mgr_info")) {
			if (Logging.INFO) { 
				if (mAccountMgrInfo != null) {
					// previous <acct_mgr_info> not closed - dropping it!
					Log.i(TAG, "Dropping unfinished <acct_mgr_info> data");
				}
			}
			mAccountMgrInfo = new AccountMgrInfo();
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
			if (mAccountMgrInfo != null) {
				// we are inside <acct_mgr_info>
				if (localName.equalsIgnoreCase("acct_mgr_info")) {
					// Closing tag of <acct_mgr_info> - nothing to do at the moment
				}
				else {
					// Not the closing tag - we decode possible inner tags
					trimEnd();
					if (localName.equalsIgnoreCase("acct_mgr_name")) {
						mAccountMgrInfo.acct_mgr_name = mCurrentElement.toString();
					} else if (localName.equalsIgnoreCase("acct_mgr_url")) {
						mAccountMgrInfo.acct_mgr_url = mCurrentElement.toString();
					} else if (localName.equalsIgnoreCase("have_credentials")) {
						mAccountMgrInfo.have_credentials = !mCurrentElement.toString().equals("0");
					} else if (localName.equalsIgnoreCase("cookie_required")) {
						mAccountMgrInfo.cookie_required = !mCurrentElement.toString().equals("0");
					} else if (localName.equalsIgnoreCase("cookie_failure_url"))
						mAccountMgrInfo.cookie_failure_url = mCurrentElement.toString();
				}
			}
		} catch (NumberFormatException e) {
			if (Logging.INFO) Log.i(TAG, "Exception when decoding " + localName);
		}
		mElementStarted = false;
	}
}
