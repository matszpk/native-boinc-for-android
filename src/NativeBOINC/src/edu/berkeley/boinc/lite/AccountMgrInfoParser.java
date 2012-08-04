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
public class AccountMgrInfoParser extends BoincBaseParser {
	private static final String TAG = "AccountMgrInfoParser";

	private AccountMgrInfo mAccountMgrInfo = null;
	
	public AccountMgrInfo getAccountMgrInfo() {
		return mAccountMgrInfo;
	}

	public static AccountMgrInfo parse(String rpcResult) {
		try {
			AccountMgrInfoParser parser = new AccountMgrInfoParser();
			BoincBaseParser.parse(parser, rpcResult);
			return parser.getAccountMgrInfo();
		} catch (BoincParserException e) {
			if (Logging.DEBUG) Log.d(TAG, "Malformed XML:\n" + rpcResult);
			else if (Logging.INFO) Log.i(TAG, "Malformed XML");
			return null;
		}
	}
	
	@Override
	public void startElement(String localName) {
		super.startElement(localName);
		if (localName.equalsIgnoreCase("acct_mgr_info")) {
			if (Logging.INFO) { 
				if (mAccountMgrInfo != null) {
					// previous <acct_mgr_info> not closed - dropping it!
					Log.i(TAG, "Dropping unfinished <acct_mgr_info> data");
				}
			}
			mAccountMgrInfo = new AccountMgrInfo();
		}
	}
	
	@Override
	public void endElement(String localName) {
		super.endElement(localName);
		try {
			if (mAccountMgrInfo != null) {
				// we are inside <acct_mgr_info>
				if (localName.equalsIgnoreCase("acct_mgr_info")) {
					// Closing tag of <acct_mgr_info> - nothing to do at the moment
				}
				else {
					// Not the closing tag - we decode possible inner tags
					if (localName.equalsIgnoreCase("acct_mgr_name")) {
						mAccountMgrInfo.acct_mgr_name = mCurrentElement;
					} else if (localName.equalsIgnoreCase("acct_mgr_url")) {
						mAccountMgrInfo.acct_mgr_url = mCurrentElement;
					} else if (localName.equalsIgnoreCase("have_credentials")) {
						mAccountMgrInfo.have_credentials = !mCurrentElement.equals("0");
					} else if (localName.equalsIgnoreCase("cookie_required")) {
						mAccountMgrInfo.cookie_required = !mCurrentElement.equals("0");
					} else if (localName.equalsIgnoreCase("cookie_failure_url"))
						mAccountMgrInfo.cookie_failure_url = mCurrentElement;
				}
			}
		} catch (NumberFormatException e) {
			if (Logging.INFO) Log.i(TAG, "Exception when decoding " + localName);
		}
	}
}
