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

package edu.berkeley.boinc.nativeboinc;

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
public class ClientEventParser extends BaseParser {
	private static final String TAG = "ClientEventParser";

	private boolean mWithClientEvent = false;
	private int mClientEventType = -1;
	private String mProjectUrl = null;
	
	public ClientEvent getClientEvent() {
		return new ClientEvent(mClientEventType, mProjectUrl);
	}

	public static ClientEvent parse(String rpcResult) {
		try {
			ClientEventParser parser = new ClientEventParser();
			Xml.parse(rpcResult, parser);
			return parser.getClientEvent();
		} catch (SAXException e) {
			if (Logging.DEBUG) Log.d(TAG, "Malformed XML:\n" + rpcResult);
			else if (Logging.INFO) Log.i(TAG, "Malformed XML");
			return null;
		}
	}
	
	public void startElement(String uri, String localName, String qName, Attributes attributes) throws SAXException {
		super.startElement(uri, localName, qName, attributes);
		if (localName.equalsIgnoreCase("reply")) {
			if (mWithClientEvent) {
				if (Logging.DEBUG) Log.d(TAG, "Dropping old reply");
			}
			mWithClientEvent = true;
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
			if (mWithClientEvent) {
				if (!localName.equalsIgnoreCase("reply")) {
					trimEnd();
					
					if (localName.equalsIgnoreCase("type")) {
						mClientEventType = Integer.parseInt(mCurrentElement.toString());
					} else if (localName.equalsIgnoreCase("project")) {
						mProjectUrl = mCurrentElement.toString();
					}
				}
			}
		} catch(NumberFormatException ex) {
			if (Logging.INFO) Log.i(TAG, "Exception when decoding " + localName);
		}
		mElementStarted = false;
	}
}
