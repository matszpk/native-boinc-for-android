/**
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
public class NoticesReplyParser extends BaseParser {
	private static final String TAG = "NoticesReplyParser";
	
	private ArrayList<Notice> mNotices = new ArrayList<Notice>(1);
	private Notice mNotice = null;
	
	public final ArrayList<Notice> getNotices() {
		return mNotices;
	}
	
	public static ArrayList<Notice> parse(String rpcResult) {
		try {
			NoticesReplyParser parser = new NoticesReplyParser();
			Xml.parse(rpcResult, parser);
			return parser.getNotices();
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
		if (localName.equalsIgnoreCase("notice")) {
			mNotice = new Notice();
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
			if (mNotice != null) {
				// We are inside <app>
				if (localName.equalsIgnoreCase("notice")) {
					if (mNotice.seqno!=-1)
						mNotices.add(mNotice);
					else
						mNotices.clear(); // complete
					mNotice = null;
				} else {
					// Not the closing tag - we decode possible inner tags
					trimEnd();
					if (localName.equalsIgnoreCase("seqno")) {
						mNotice.seqno = Integer.parseInt(mCurrentElement.toString());
					} else if(localName.equalsIgnoreCase("title")) {
						mNotice.title = mCurrentElement.toString();
					} else  if (localName.equalsIgnoreCase("description")) {
						mNotice.description = mCurrentElement.toString();
					} else if (localName.equalsIgnoreCase("create_time")) {
						mNotice.create_time = Double.parseDouble(mCurrentElement.toString());
					} else if (localName.equalsIgnoreCase("arrival_time")) {
						mNotice.arrival_time = Double.parseDouble(mCurrentElement.toString());
					} else if (localName.equalsIgnoreCase("is_private")) {
						mNotice.is_private = !mCurrentElement.toString().equals("0");
					} else if (localName.equalsIgnoreCase("category")) {
						mNotice.category = mCurrentElement.toString();
					} else if (localName.equalsIgnoreCase("link")) {
						mNotice.link = mCurrentElement.toString();
					} else if (localName.equalsIgnoreCase("project_name")) {
						mNotice.project_name = mCurrentElement.toString();
					} else if (localName.equalsIgnoreCase("guid")) {
						mNotice.guid = mCurrentElement.toString();
					} else if (localName.equalsIgnoreCase("feed_url")) {
						mNotice.feed_url = mCurrentElement.toString();
					}
				}
			}
		} catch (NumberFormatException e) {
			if (Logging.INFO) Log.i(TAG, "Exception when decoding " + localName);
		}
		mElementStarted = false;
	}
}
