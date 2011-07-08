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

import org.xml.sax.Attributes;
import org.xml.sax.SAXException;
import org.xml.sax.helpers.DefaultHandler;

import sk.boinc.androboinc.debug.Logging;

import android.util.Log;
import android.util.Xml;

public class SimpleReplyParser extends DefaultHandler {
	private static final String TAG = "SimpleReplyParser";

	private boolean mParsed = false;
	private boolean mInReply = false;
	private boolean mSuccess = false;

	// Disable direct instantiation of this class
	private SimpleReplyParser() {}

	public final boolean result() {
		return mSuccess;
	}

	public static boolean isSuccess(String reply) {
		try {
			SimpleReplyParser parser = new SimpleReplyParser();
			Xml.parse(reply, parser);
			return parser.result();
		}
		catch (SAXException e) {
			if (Logging.DEBUG) Log.d(TAG, "Malformed XML:\n" + reply);
			else if (Logging.INFO) Log.i(TAG, "Malformed XML");
			return false;
		}		

	}

	@Override
	public void startElement(String uri, String localName, String qName, Attributes attributes) throws SAXException {
		super.startElement(uri, localName, qName, attributes);
		if (localName.equalsIgnoreCase("boinc_gui_rpc_reply")) {
			mInReply = true;
		}
	}

	@Override
	public void endElement(String uri, String localName, String qName) throws SAXException {
		super.endElement(uri, localName, qName);

		if (localName.equalsIgnoreCase("boinc_gui_rpc_reply")) {
			mInReply = false;
		}
		else if (mInReply && !mParsed) {
			if (localName.equalsIgnoreCase("success")) {
				mSuccess = true;
				mParsed = true;
			}
			else if (localName.equalsIgnoreCase("failure")) {
				mSuccess = false;
				mParsed = true;
			}
		}
	}
}
