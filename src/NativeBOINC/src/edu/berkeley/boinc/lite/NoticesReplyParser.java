/**
 * 
 */
package edu.berkeley.boinc.lite;

import sk.boinc.nativeboinc.debug.Logging;
import android.util.Log;

/**
 * @author mat
 *
 */
public class NoticesReplyParser extends BoincBaseParser {
	private static final String TAG = "NoticesReplyParser";
	
	private Notices mNotices = new Notices();
	private Notice mNotice = null;
	
	public final Notices getNotices() {
		return mNotices;
	}
	
	public static Notices parse(String rpcResult) {
		try {
			NoticesReplyParser parser = new NoticesReplyParser();
			//parser.parseNotices(rpcResult);
			BoincBaseParser.parse(parser, rpcResult);
			return parser.getNotices();
		}
		catch (BoincParserException e) {
			if (Logging.DEBUG) Log.d(TAG, "Malformed XML:\n" + rpcResult);
			else if (Logging.INFO) Log.i(TAG, "Malformed XML");
			return null;
		}
	}
	
	@Override
	public void startElement(String localName) {
		super.startElement(localName);
		if (localName.equalsIgnoreCase("notice")) {
			mNotice = new Notice();
		}
	}
	
	@Override
	public void endElement(String localName) {
		super.endElement(localName);
		try {
			if (mNotice != null) {
				// We are inside <msg>
				if (localName.equalsIgnoreCase("notice")) {
					// Closing tag of <msg> - add to vector and be ready for next one
					if (mNotice.seqno != -1) {
						// seqno is a must
						mNotices.notices.add(mNotice);
					} else {
						mNotices.notices.clear();
						mNotices.complete = true;
					}
					mNotice = null;
				}
				else {
					// Not the closing tag - we decode possible inner tags
					if (localName.equalsIgnoreCase("seqno")) {
						mNotice.seqno = Integer.parseInt(getCurrentElement());
					} else if (localName.equalsIgnoreCase("title")) {
						mNotice.title = getCurrentElement();
					} else if (localName.equalsIgnoreCase("description")) {
						String current = getCurrentElement();
						if (current.startsWith("<![CDATA["))
							mNotice.description = current.substring(8, current.length()-3);
						else
							mNotice.description = current;
					} else if (localName.equalsIgnoreCase("create_time")) {
						mNotice.create_time = Double.parseDouble(getCurrentElement());
					} else if (localName.equalsIgnoreCase("arrival_time")) {
						mNotice.arrival_time = Double.parseDouble(getCurrentElement());
					} else if (localName.equalsIgnoreCase("category")) {
						mNotice.category = getCurrentElement();
					} else if (localName.equalsIgnoreCase("link")) {
						mNotice.link = getCurrentElement();
					} else if (localName.equalsIgnoreCase("project_name")) {
						mNotice.project_name = getCurrentElement();
					} else if (localName.equalsIgnoreCase("guid")) {
						mNotice.guid = getCurrentElement();
					} else if (localName.equalsIgnoreCase("feed_url")) {
						mNotice.feed_url = getCurrentElement();
					} else if (localName.equalsIgnoreCase("is_private")) {
						mNotice.is_private = !getCurrentElement().equals("0");
					}
				}
			}
		}
		catch (NumberFormatException e) {
			if (Logging.INFO) Log.i(TAG, "Exception when decoding " + localName);
		}
	}
}
