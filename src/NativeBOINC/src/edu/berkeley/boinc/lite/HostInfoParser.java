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


public class HostInfoParser extends BoincBaseParser {
	private static final String TAG = "HostInfoParser";

	private HostInfo mHostInfo = null;


	public final HostInfo getHostInfo() {
		return mHostInfo;
	}

	/**
	 * Parse the RPC result (host_info) and generate vector of projects info
	 * @param rpcResult String returned by RPC call of core client
	 * @return HostInfo
	 */
	public static HostInfo parse(String rpcResult) {
		try {
			HostInfoParser parser = new HostInfoParser();
			BoincBaseParser.parse(parser, rpcResult, true);
			return parser.getHostInfo();
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
		if (localName.equalsIgnoreCase("host_info")) {
			if (Logging.INFO) { 
				if (mHostInfo != null) {
					// previous <host_info> not closed - dropping it!
					Log.i(TAG, "Dropping unfinished <host_info> data");
				}
			}
			mHostInfo = new HostInfo();
		}
	}
	
	@Override
	public void endElement(String localName) {
		super.endElement(localName);
		try {
			if (mHostInfo != null) {
				// we are inside <host_info>
				if (localName.equalsIgnoreCase("host_info")) {
					// Closing tag of <host_info> - nothing to do at the moment
				}
				else {
					// Not the closing tag - we decode possible inner tags
					if (localName.equalsIgnoreCase("timezone")) {
						mHostInfo.timezone = Integer.parseInt(getCurrentElement());
					}
					else if (localName.equalsIgnoreCase("domain_name")) {
						mHostInfo.domain_name = getCurrentElement();
					}
					else if (localName.equalsIgnoreCase("ip_addr")) {
						mHostInfo.ip_addr = getCurrentElement();
					}
					else if (localName.equalsIgnoreCase("host_cpid")) {
						mHostInfo.host_cpid = getCurrentElement();
					}
					else if (localName.equalsIgnoreCase("p_ncpus")) {
						mHostInfo.p_ncpus = Integer.parseInt(getCurrentElement());
					}
					else if (localName.equalsIgnoreCase("p_vendor")) {
						mHostInfo.p_vendor = getCurrentElement();
					}
					else if (localName.equalsIgnoreCase("p_model")) {
						mHostInfo.p_model = getCurrentElement();
					}
					else if (localName.equalsIgnoreCase("p_features")) {
						mHostInfo.p_features = getCurrentElement();
					}
					else if (localName.equalsIgnoreCase("p_fpops")) {
						mHostInfo.p_fpops = Double.parseDouble(getCurrentElement());
					}
					else if (localName.equalsIgnoreCase("p_iops")) {
						mHostInfo.p_iops = Double.parseDouble(getCurrentElement());
					}
					else if (localName.equalsIgnoreCase("p_membw")) {
						mHostInfo.p_membw = Double.parseDouble(getCurrentElement());
					}
					else if (localName.equalsIgnoreCase("p_calculated")) {
						mHostInfo.p_calculated = (long)Double.parseDouble(getCurrentElement());
					}
					else if (localName.equalsIgnoreCase("m_nbytes")) {
						mHostInfo.m_nbytes = Double.parseDouble(getCurrentElement());
					}
					else if (localName.equalsIgnoreCase("m_cache")) {
						mHostInfo.m_cache = Double.parseDouble(getCurrentElement());
					}
					else if (localName.equalsIgnoreCase("m_swap")) {
						mHostInfo.m_swap = Double.parseDouble(getCurrentElement());
					}
					else if (localName.equalsIgnoreCase("d_total")) {
						mHostInfo.d_total = Double.parseDouble(getCurrentElement());
					}
					else if (localName.equalsIgnoreCase("d_free")) {
						mHostInfo.d_free = Double.parseDouble(getCurrentElement());
					}
					else if (localName.equalsIgnoreCase("os_name")) {
						mHostInfo.os_name = getCurrentElement();
					}
					else if (localName.equalsIgnoreCase("os_version")) {
						mHostInfo.os_version = getCurrentElement();
					}
					else if (localName.equalsIgnoreCase("virtualbox_version")) {
						mHostInfo.virtualbox_version = getCurrentElement();
					}
				}
			}
		}
		catch (NumberFormatException e) {
			if (Logging.INFO) Log.i(TAG, "Exception when decoding " + localName);
		}
	}
}
