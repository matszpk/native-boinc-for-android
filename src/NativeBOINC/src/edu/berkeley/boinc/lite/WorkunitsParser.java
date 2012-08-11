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

public class WorkunitsParser extends BoincBaseParser {
	private static final String TAG = "WorkunitsParser";

	private ArrayList<Workunit> mWorkunits = new ArrayList<Workunit>();
	private Workunit mWorkunit = null;


	public final ArrayList<Workunit> getWorkunits() {
		return mWorkunits;
	}

	/**
	 * Parse the RPC result (workunit) and generate corresponding vector
	 * @param rpcResult String returned by RPC call of core client
	 * @return vector of workunits
	 */
	public static ArrayList<Workunit> parse(String rpcResult) {
		try {
			WorkunitsParser parser = new WorkunitsParser();
			BoincBaseParser.parse(parser, rpcResult, true);
			return parser.getWorkunits();
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
		if (localName.equalsIgnoreCase("workunit")) {
			mWorkunit = new Workunit();
		}
	}

	@Override
	public void endElement(String localName) {
		super.endElement(localName);
		try {
			if (mWorkunit != null) {
				// We are inside <workunit>
				if (localName.equalsIgnoreCase("workunit")) {
					// Closing tag of <workunit> - add to vector and be ready for next one
					if (!mWorkunit.name.equals("")) {
						// name is a must
						mWorkunits.add(mWorkunit);
					}
					mWorkunit = null;
				}
				else {
					// Not the closing tag - we decode possible inner tags
					if (localName.equalsIgnoreCase("name")) {
						mWorkunit.name = getCurrentElement();
					}
					else if (localName.equalsIgnoreCase("app_name")) {
						mWorkunit.app_name = getCurrentElement();
					}
					else if (localName.equalsIgnoreCase("version_num")) {
						mWorkunit.version_num = Integer.parseInt(getCurrentElement());
					}
					else if (localName.equalsIgnoreCase("rsc_fpops_est")) {
						mWorkunit.rsc_fpops_est = Double.parseDouble(getCurrentElement());
					}
					else if (localName.equalsIgnoreCase("rsc_fpops_bound")) {
						mWorkunit.rsc_fpops_bound = Double.parseDouble(getCurrentElement());
					}
					else if (localName.equalsIgnoreCase("rsc_memory_bound")) {
						mWorkunit.rsc_memory_bound = Double.parseDouble(getCurrentElement());
					}
					else if (localName.equalsIgnoreCase("rsc_disk_bound")) {
						mWorkunit.rsc_disk_bound = Double.parseDouble(getCurrentElement());
					}
				}
			}
		}
		catch (NumberFormatException e) {
			if (Logging.INFO) Log.i(TAG, "Exception when decoding " + localName);
		}
	}
}
