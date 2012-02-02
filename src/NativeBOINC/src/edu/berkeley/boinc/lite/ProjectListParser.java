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

import org.xml.sax.Attributes;
import org.xml.sax.SAXException;

import sk.boinc.nativeboinc.debug.Logging;
import android.util.Log;
import android.util.Xml;

/**
 * @author mat
 *
 */
public class ProjectListParser extends BaseParser {
	private static final String TAG = "ProjectListParser";
	
	private ArrayList<ProjectListEntry> mProjectList = new ArrayList<ProjectListEntry>();
	private ProjectListEntry mProjectEntry = null;
	private boolean mInsidePlatforms = false;
	
	public ArrayList<ProjectListEntry> getProjectList() {
		return mProjectList;
	}
	
	public static ArrayList<ProjectListEntry> parse(String rpcResult) {
		try {
			String outResult;
			int xmlHeaderStart = rpcResult.indexOf("<?xml");
			if (xmlHeaderStart!=-1) { // remove xml header inside body
				int xmlHeaderEnd = rpcResult.indexOf("?>");
				outResult = rpcResult.substring(0, xmlHeaderStart);
				outResult += rpcResult.substring(xmlHeaderEnd+2);
			} else
				outResult = rpcResult;
				
			ProjectListParser parser = new ProjectListParser();
			Xml.parse(outResult, parser);
			return parser.getProjectList();
		}
		catch (SAXException e) {
			if (Logging.DEBUG) Log.d(TAG, "Malformed XML:\n" + rpcResult);
			else if (Logging.INFO) Log.i(TAG, "Malformed XML");
			return null;
		}
	}
	
	@Override
	public void startElement(String uri, String localName, String qName, Attributes attributes) throws SAXException {
		super.startElement(uri, localName, qName, attributes);
		if (localName.equalsIgnoreCase("project")) {
			if (Logging.INFO) {
				if (mProjectEntry != null) {
					// previous <project> not closed - dropping it!
					Log.i(TAG, "Dropping unfinished <project> data");
				}
			}
			mProjectEntry = new ProjectListEntry();
		}
		else if (localName.equalsIgnoreCase("platforms")) {
			mInsidePlatforms = true;
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
			if (mProjectEntry != null) {
				// We are inside <project>
				if (localName.equalsIgnoreCase("project")) {
					// Closing tag of <project> - add to vector and be ready for next one
					if (!mProjectEntry.name.equals("")) {
						// master_url is a must
						mProjectList.add(mProjectEntry);
					}
					mProjectEntry = null;
				} else if (localName.equalsIgnoreCase("platforms")) {
					mInsidePlatforms = false;
				} else {
					trimEnd();
					if (localName.equalsIgnoreCase("name")) {
						if (mInsidePlatforms) {
							/* platform name */
							mProjectEntry.platforms.add(mCurrentElement.toString());
						} else {
							mProjectEntry.name = mCurrentElement.toString();
						}
					} else if (localName.equalsIgnoreCase("url")) {
						mProjectEntry.url = mCurrentElement.toString();
					} else if (localName.equalsIgnoreCase("general_area")) {
						mProjectEntry.general_area = mCurrentElement.toString();
					} else if (localName.equalsIgnoreCase("specific_area")) {
						mProjectEntry.specific_area = mCurrentElement.toString();
					} else if (localName.equalsIgnoreCase("description")) {
						mProjectEntry.description = mCurrentElement.toString();
					} else if (localName.equalsIgnoreCase("home")) {
						mProjectEntry.home = mCurrentElement.toString();
					} else if (localName.equalsIgnoreCase("image")) {
						mProjectEntry.image = mCurrentElement.toString();
					}
				}
			}
		} catch (NumberFormatException e) {
			if (Logging.INFO) Log.i(TAG, "Exception when decoding " + localName);
		}
		mElementStarted = false;
	}
}
