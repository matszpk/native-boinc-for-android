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
public class NoticesReplyParser {
	private static final String TAG = "NoticesReplyParser";
	
	private Notices mNotices = new Notices();
	
	public final Notices getNotices() {
		return mNotices;
	}
	
	public static Notices parse(String rpcResult) {
		try {
			NoticesReplyParser parser = new NoticesReplyParser();
			parser.parseNotices(rpcResult);
			return parser.getNotices();
		}
		catch (Exception e) {
			if (Logging.DEBUG) Log.d(TAG, "Malformed XML:\n" + rpcResult);
			else if (Logging.INFO) Log.i(TAG, "Malformed XML");
			return null;
		}
	}
	
	public void parseNotices(String xml) {
		int pos = 0;
		int end = xml.length();
		boolean inNotices = false;
		
		int newPos;
		Notice notice = null;
		
		try {
			while (pos < end) {
				/* skip spaces */
				while (pos < end) {
					if (!Character.isSpace(xml.charAt(pos)))
						break;
					pos++;
				}
				if (!inNotices) {
					newPos = xml.indexOf("<notices>");
					if (newPos == -1)
						throw new RuntimeException("Cant parse notices");
					pos = newPos + 9;
					inNotices = true;
				} else if (inNotices && notice == null && xml.startsWith("<notice>", pos)) {
					pos += 8;
					notice = new Notice();
				} else if (notice != null && xml.startsWith("</notice>", pos)) {
					if (notice.seqno != -1)
						mNotices.notices.add(notice);
					else {
						mNotices.notices.clear();
						mNotices.complete = true;
					}
					notice = null;
					pos += 9;
				} else if (inNotices && xml.startsWith("</notices>", pos)) {
					break; // end
				} else if (notice != null) {
					if (xml.startsWith("<seqno>", pos)) {
						pos += 7;
						newPos = xml.indexOf("</seqno>", pos);
						if (newPos == -1)
							throw new RuntimeException("Cant parse notices");
						notice.seqno = Integer.parseInt(xml.substring(pos, newPos).trim());
						pos = newPos+8;
					} else if (xml.startsWith("<title>", pos)) {
						pos += 7;
						newPos = xml.indexOf("</title>", pos);
						if (newPos == -1)
							throw new RuntimeException("Cant parse notices");
						notice.title = xml.substring(pos, newPos).trim();
						pos = newPos+8;
					} else if (xml.startsWith("<description><![CDATA[", pos)) {
						pos += 13 + 9;
						newPos = xml.indexOf("]]></description>", pos);
						if (newPos == -1)
							throw new RuntimeException("Cant parse notices");
						notice.description = xml.substring(pos, newPos).trim();
						pos = newPos+14+3;
					} else if (xml.startsWith("<create_time>", pos)) {
						pos += 13;
						newPos = xml.indexOf("</create_time>", pos);
						if (newPos == -1)
							throw new RuntimeException("Cant parse notices");
						notice.create_time = Double.parseDouble(xml.substring(pos, newPos).trim());
						pos = newPos+14;
					} else if (xml.startsWith("<arrival_time>", pos)) {
						pos += 14;
						newPos = xml.indexOf("</arrival_time>", pos);
						if (newPos == -1)
							throw new RuntimeException("Cant parse notices");
						notice.arrival_time = Double.parseDouble(xml.substring(pos, newPos).trim());
						pos = newPos+15;
					} else if (xml.startsWith("<category>", pos)) {
						pos += 10;
						newPos = xml.indexOf("</category>", pos);
						if (newPos == -1)
							throw new RuntimeException("Cant parse notices");
						notice.category = xml.substring(pos, newPos).trim();
						pos = newPos+11;
					} else if (xml.startsWith("<link>", pos)) {
						pos += 6;
						newPos = xml.indexOf("</link>", pos);
						if (newPos == -1)
							throw new RuntimeException("Cant parse notices");
						notice.link = xml.substring(pos, newPos).trim();
						pos = newPos+7;
					} else if (xml.startsWith("<project_name>", pos)) {
						pos += 14;
						newPos = xml.indexOf("</project_name>", pos);
						if (newPos == -1)
							throw new RuntimeException("Cant parse notices");
						notice.project_name = xml.substring(pos, newPos).trim();
						pos = newPos+15;
					} else if (xml.startsWith("<guid>", pos)) {
						pos += 6;
						newPos = xml.indexOf("</guid>", pos);
						if (newPos == -1)
							throw new RuntimeException("Cant parse notices");
						notice.guid = xml.substring(pos, newPos).trim();
						pos = newPos+7;
					} else if (xml.startsWith("<feed_url>", pos)) {
						pos += 10;
						newPos = xml.indexOf("</feed_url>", pos);
						if (newPos == -1)
							throw new RuntimeException("Cant parse notices");
						notice.feed_url = xml.substring(pos, newPos).trim();
						pos = newPos+11;
					} else if (xml.startsWith("<is_private>", pos)) {
						pos += 12;
						newPos = xml.indexOf("</is_private>", pos);
						if (newPos == -1)
							throw new RuntimeException("Cant parse notices");
						notice.is_private = !xml.substring(pos, newPos).trim().equals("0");
						pos = newPos+13;
					}
				}
			}
		} catch (NumberFormatException e) {
			if (Logging.INFO) Log.i(TAG, "Exception when decoding");
		}
	}
}
