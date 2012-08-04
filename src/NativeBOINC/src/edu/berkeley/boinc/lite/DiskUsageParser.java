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

import java.util.ArrayList;
import java.util.HashMap;

import sk.boinc.nativeboinc.debug.Logging;
import android.util.Log;

/**
 * @author mat
 *
 */
public class DiskUsageParser extends BoincBaseParser {

	private static final String TAG = "DiskUsageParser";
	
	private HashMap<String, Double> mDiskUsageMap = new HashMap<String, Double>();
	
	private String mMasterUrl = null;
	private double mDiskUsage = 0.0;
	
	public static boolean parse(String rpcResult, ArrayList<Project> projects) {
		try {
			DiskUsageParser parser = new DiskUsageParser();
			BoincBaseParser.parse(parser, rpcResult);
			
			parser.setUpDiskUsage(projects);
			return true;
		}
		catch (BoincParserException e) {
			if (Logging.DEBUG) Log.d(TAG, "Malformed XML:\n" + rpcResult);
			else if (Logging.INFO) Log.i(TAG, "Malformed XML");
			return false;
		}
	}
	
	private void setUpDiskUsage(ArrayList<Project> projects) {
		for (Project project: projects) {
			Double diskUsage = mDiskUsageMap.get(project.master_url);
			if (diskUsage != null)
				project.disk_usage = diskUsage;
			else // unknown disk usage
				project.disk_usage = -1.0;
		}
	}
	
	@Override
	public void startElement(String localName) {
		super.startElement(localName);
		if (localName.equalsIgnoreCase("project")) {
			mMasterUrl = null;
		}
	}
	
	@Override
	public void endElement(String localName) {
		super.endElement(localName);
		try {
			if (localName.equalsIgnoreCase("project")) {
				// Closing tag of <project> - add to vector and be ready for next one
				if (mMasterUrl != null) {
					// master_url is a must
					mDiskUsageMap.put(mMasterUrl, mDiskUsage);
					mMasterUrl = null;
				}
			} else {
				if (localName.equalsIgnoreCase("master_url")) {
					mMasterUrl = mCurrentElement;
				} else if (localName.equalsIgnoreCase("disk_usage")) {
					mDiskUsage = Double.parseDouble(mCurrentElement);
				}
			}
		} catch (NumberFormatException e) {
			if (Logging.INFO) Log.i(TAG, "Exception when decoding " + localName);
		}
	}
}
