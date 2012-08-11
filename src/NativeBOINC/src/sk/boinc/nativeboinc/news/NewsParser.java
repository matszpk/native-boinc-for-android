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
import java.text.ParseException;
import java.util.ArrayList;

import org.xml.sax.Attributes;
import org.xml.sax.SAXException;

import android.util.Log;
import android.util.Xml;

import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.util.BaseParser;

/**
 * @author mat
 *
 */
public class NewsParser extends BaseParser {
	private static final String TAG = "NewsParser";
	
	private ArrayList<NewsMessage> mNewsMessages = null;
	private NewsMessage mMessage = null;
	
	public ArrayList<NewsMessage> getNewsMessages() {
		return mNewsMessages;
	}
	
	public static ArrayList<NewsMessage> parse(InputStream result) {
		try {
			NewsParser newsParser = new NewsParser();
			Xml.parse(result, Xml.Encoding.UTF_8, newsParser);
			return newsParser.getNewsMessages();
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
		if (localName.equalsIgnoreCase("news")) {
			mNewsMessages = new ArrayList<NewsMessage>();
		} else if (localName.equalsIgnoreCase("message")) {
			mMessage = new NewsMessage();
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
			if (mMessage != null) {
				if (localName.equalsIgnoreCase("message")) {
					// Closing tag of <project> - add to vector and be ready for next one
					if (mMessage.getTitle() != null && mMessage.getContent() != null) {
						// master_url is a must
						mNewsMessages.add(mMessage);
					}
					mMessage = null;
				}  else {
					trimEnd();
					if (localName.equalsIgnoreCase("title")) {
						mMessage.setTitle(mCurrentElement.toString());
					} else if (localName.equalsIgnoreCase("time")) {
						/* TODO: handle timezones */
						mMessage.setTimestamp(NewsUtil.sDateFormat.parse(mCurrentElement.toString()).getTime());
					} else if (localName.equalsIgnoreCase("content")) {
						mMessage.setContent(mCurrentElement.toString());
					}
				}
			}
		} catch(ParseException ex) {
			if (Logging.WARNING) Log.w(TAG, "Bad format of the date");
		}
	}
}
