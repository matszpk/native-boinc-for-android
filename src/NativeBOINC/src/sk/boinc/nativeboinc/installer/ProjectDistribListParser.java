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

package sk.boinc.nativeboinc.nativeclient;

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
public class ProjectDistribListParser extends BaseParser {
	private static final String TAG = "ProjectDistribParser";
	
	private Vector<ProjectDistrib> mProjectDistribs = null;
	private ProjectDistrib mDistrib = null;
	
	public Vector<ProjectDistrib> getProjectDistribs() {
		return mProjectDistribs;
	}

	public static Vector<ProjectDistrib> parse(InputStream result) {
		try {
			ProjectDistribListParser parser = new ProjectDistribListParser();
			Xml.parse(result, Xml.Encoding.UTF_8, parser);
			return parser.getProjectDistribs();
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
		if (localName.equalsIgnoreCase("app_list")) {
			mProjectDistribs = new Vector<ProjectDistrib>();
		} else if (localName.equalsIgnoreCase("project")) {
			mDistrib = new ProjectDistrib();
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
		if (mDistrib != null) {
			if (localName.equalsIgnoreCase("project")) {
				// Closing tag of <project> - add to vector and be ready for next one
				if (!mDistrib.projectName.equals("")) {
					// master_url is a must
					mProjectDistribs.add(mDistrib);
				}
				mDistrib = null;
			}  else {
				trimEnd();
				if (localName.equalsIgnoreCase("name")) {
					mDistrib.projectName = mCurrentElement.toString();
				} else if (localName.equalsIgnoreCase("url")) {
					mDistrib.projectUrl = mCurrentElement.toString();
				} else if (localName.equalsIgnoreCase("version")) {
					mDistrib.version = mCurrentElement.toString();
				} if (localName.equalsIgnoreCase("file")) {
					mDistrib.filename = mCurrentElement.toString();
				}
			}
		}
		mElementStarted = false;
	}
}
