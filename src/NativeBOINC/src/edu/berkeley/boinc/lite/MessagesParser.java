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


public class MessagesParser extends BoincBaseParser {
	private static final String TAG = "MessagesParser";

	private ArrayList<Message> mMessages = new ArrayList<Message>();
	private Message mMessage = null;

	public static ArrayList<Message> parse(String rpcResult) {
		try {
			MessagesParser parser = new MessagesParser();
			BoincBaseParser.parse(parser, rpcResult, true);
			return parser.getMessages();
		}
		catch (BoincParserException e) {
			if (Logging.DEBUG) Log.d(TAG, "Malformed XML:\n" + rpcResult);
			else if (Logging.INFO) Log.i(TAG, "Malformed XML");
			return null;
		}
	}

	public final ArrayList<Message> getMessages() {
		return mMessages;
	}

	@Override
	public void startElement(String localName) {
		super.startElement(localName);
		if (localName.equalsIgnoreCase("msg")) {
			mMessage = new Message();
		}
	}
	
	@Override
	public void endElement(String localName) {
		super.endElement(localName);
		try {
			if (mMessage != null) {
				// We are inside <msg>
				if (localName.equalsIgnoreCase("msg")) {
					// Closing tag of <msg> - add to vector and be ready for next one
					if (mMessage.seqno != 0) {
						// seqno is a must
						mMessages.add(mMessage);
					}
					mMessage = null;
				}
				else {
					// Not the closing tag - we decode possible inner tags
					if (localName.equalsIgnoreCase("project")) {
						mMessage.project = getCurrentElement();
					}
					else if (localName.equalsIgnoreCase("seqno")) {
						mMessage.seqno = Integer.parseInt(getCurrentElement());
					}
					else if (localName.equalsIgnoreCase("pri")) {
						mMessage.priority = Integer.parseInt(getCurrentElement());
					}
					else if (localName.equalsIgnoreCase("time")) {
						mMessage.timestamp = (long)Double.parseDouble(getCurrentElement());
					}
					else if (localName.equalsIgnoreCase("body")) {
						mMessage.body = getCurrentElement();
					}
				}
			}
		}
		catch (NumberFormatException e) {
			if (Logging.INFO) Log.i(TAG, "Exception when decoding " + localName);
		}
	}
}
