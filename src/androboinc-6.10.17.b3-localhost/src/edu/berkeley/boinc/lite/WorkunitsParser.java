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

import java.util.Vector;

import org.xml.sax.Attributes;
import org.xml.sax.SAXException;

import sk.boinc.androboinc.debug.Logging;

import android.util.Log;
import android.util.Xml;

public class WorkunitsParser extends BaseParser {
	private static final String TAG = "WorkunitsParser";

	private Vector<Workunit> mWorkunits = new Vector<Workunit>();
	private Workunit mWorkunit = null;


	public final Vector<Workunit> getWorkunits() {
		return mWorkunits;
	}

	/**
	 * Parse the RPC result (workunit) and generate corresponding vector
	 * @param rpcResult String returned by RPC call of core client
	 * @return vector of workunits
	 */
	public static Vector<Workunit> parse(String rpcResult) {
		try {
			WorkunitsParser parser = new WorkunitsParser();
			Xml.parse(rpcResult, parser);
			return parser.getWorkunits();
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
		if (localName.equalsIgnoreCase("workunit")) {
			mWorkunit = new Workunit();
		}
		else {
			// Another element, hopefully primitive and not constructor
			// (although unknown constructor does not hurt, because there will be primitive start anyway)
			mElementStarted = true;
			mCurrentElement.setLength(0);
		}
	}

	// Method characters(char[] ch, int start, int length) is implemented by BaseParser,
	// filling mCurrentElement (including stripping of leading whitespaces)
	//@Override
	//public void characters(char[] ch, int start, int length) throws SAXException { }

	@Override
	public void endElement(String uri, String localName, String qName) throws SAXException {
		super.endElement(uri, localName, qName);
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
					trimEnd();
					if (localName.equalsIgnoreCase("name")) {
						mWorkunit.name = mCurrentElement.toString();
					}
					else if (localName.equalsIgnoreCase("app_name")) {
						mWorkunit.app_name = mCurrentElement.toString();
					}
					else if (localName.equalsIgnoreCase("version_num")) {
						mWorkunit.version_num = Integer.parseInt(mCurrentElement.toString());
					}
				}
			}
		}
		catch (NumberFormatException e) {
			if (Logging.INFO) Log.i(TAG, "Exception when decoding " + localName);
		}
		mElementStarted = false;
	}
}
