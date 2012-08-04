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

package sk.boinc.nativeboinc.installer;

import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.ArrayList;

import sk.boinc.nativeboinc.debug.Logging;
import android.util.Log;

import edu.berkeley.boinc.lite.BoincBaseParser;
import edu.berkeley.boinc.lite.BoincParserException;

/**
 * @author mat
 *
 */
public class ExecFilesAppInfoParser extends BoincBaseParser {

	private static final String TAG = "ExecFilesAppInfoParser";
	
	private ArrayList<String> mExecFiles = new ArrayList<String>();
	
	private boolean mInsideFileInfo = false;
	private boolean mIsExecutable = false;
	private String mFilename = null;
	
	public ArrayList<String> getExecFiles() {
		return mExecFiles;
	}
	
	public static ArrayList<String> parse(InputStream result) {
		try {
			ExecFilesAppInfoParser parser = new ExecFilesAppInfoParser();
			InputStreamReader reader = new InputStreamReader(result, "UTF-8");
			BoincBaseParser.parse(parser, reader);
			return parser.getExecFiles();
		} catch (BoincParserException e) {
			if (Logging.DEBUG) Log.d(TAG, "Malformed XML:\n" + result);
			else if (Logging.INFO) Log.i(TAG, "Malformed XML");
			return null;
		} catch (IOException e2) {
			if (Logging.ERROR) Log.e(TAG, "I/O Error in XML parsing:\n" + result);
			return null;
		}
	}
	
	@Override
	public void startElement(String localName) {
		super.startElement(localName);
		if (localName.equalsIgnoreCase("file_info")) {
			mInsideFileInfo = true;
		}
	}
	
	@Override
	public void endElement(String localName) {
		super.endElement(localName);
		if (mInsideFileInfo) {
			if (localName.equalsIgnoreCase("file_info")) {
				if (mFilename != null && mIsExecutable) {
					mExecFiles.add(mFilename);
				}
				mInsideFileInfo = false;
				mFilename = null;
				mIsExecutable = false;
			} else if (localName.equalsIgnoreCase("executable"))
				mIsExecutable = true;
			else if (localName.equalsIgnoreCase("name"))
				mFilename = mCurrentElement;
		}
	}
}
