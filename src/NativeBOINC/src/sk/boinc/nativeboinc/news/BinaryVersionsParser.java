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
package sk.boinc.nativeboinc.news;

import java.io.IOException;
import java.io.InputStream;
import java.util.HashMap;
import java.util.Map;

import org.xml.sax.Attributes;
import org.xml.sax.SAXException;

import edu.berkeley.boinc.lite.SecureXmlParser;

import android.util.Log;
import android.util.Xml;

import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.util.BaseParser;

/**
 * @author mat
 *
 */
public class BinaryVersionsParser extends BaseParser {
	private static final String TAG = "NewsParser";
	
	private HashMap<String, String> mBinaryVersions = null;
	private String mBinaryName = null;
	private String mBinaryVersion = null;
	
	public Map<String, String> getBinaryVersions() {
		return mBinaryVersions;
	}
	
	public static Map<String, String> parse(InputStream result) {
		try {
			BinaryVersionsParser parser = new BinaryVersionsParser();
			SecureXmlParser.parse(result, Xml.Encoding.UTF_8, parser);
			return parser.getBinaryVersions();
		} catch(SAXException ex) {
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
		if (localName.equalsIgnoreCase("versions")) {
			mBinaryVersions = new HashMap<String, String>();
		} else if (localName.equalsIgnoreCase("binary")) {
			mBinaryName = null;
			mBinaryVersion = null;
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
		if (localName.equalsIgnoreCase("binary")) {
			// Closing tag of <project> - add to vector and be ready for next one
			if (mBinaryName != null && mBinaryVersion != null) {
				// master_url is a must
				mBinaryVersions.put(mBinaryName, mBinaryVersion);
			}
			mBinaryName = null;
			mBinaryVersion = null;
		}  else {
			trimEnd();
			if (localName.equalsIgnoreCase("name")) {
				mBinaryName = mCurrentElement.toString();
			} else if (localName.equalsIgnoreCase("version")) {
				mBinaryVersion = mCurrentElement.toString();
			}
		}
	}
}
