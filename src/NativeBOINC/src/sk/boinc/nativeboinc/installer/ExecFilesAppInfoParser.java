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
import java.util.Vector;

import org.xml.sax.Attributes;
import org.xml.sax.SAXException;

import sk.boinc.nativeboinc.debug.Logging;
import android.util.Log;
import android.util.Xml;

import edu.berkeley.boinc.lite.BaseParser;

/**
 * @author mat
 *
 */
public class ExecFilesAppInfoParser extends BaseParser {

	private static final String TAG = "ExecFilesAppInfoParser";
	
	private Vector<String> mExecFiles = new Vector<String>();
	
	private boolean mInsideFileInfo = false;
	private boolean mIsExecutable = false;
	private String mFilename = null;
	
	public Vector<String> getExecFiles() {
		return mExecFiles;
	}
	
	public static Vector<String> parse(InputStream result) {
		try {
			ExecFilesAppInfoParser parser = new ExecFilesAppInfoParser();
			Xml.parse(result, Xml.Encoding.UTF_8, parser);
			return parser.getExecFiles();
		} catch (SAXException e) {
			if (Logging.DEBUG) Log.d(TAG, "Malformed XML:\n" + result);
			else if (Logging.INFO) Log.i(TAG, "Malformed XML");
			return null;
		} catch (IOException e2) {
			if (Logging.ERROR) Log.e(TAG, "I/O Error in XML parsing:\n" + result);
			return null;
		}
	}
	
	@Override
	public void startElement(String uri, String localName, String qName, Attributes attributes) throws SAXException {
		super.startElement(uri, localName, qName, attributes);
		if (localName.equalsIgnoreCase("file_info")) {
			mInsideFileInfo = true;
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
				mFilename = mCurrentElement.toString();
		}
		
		mElementStarted = false;
	}
}
