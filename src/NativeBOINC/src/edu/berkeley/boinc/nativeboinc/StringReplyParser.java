/**
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
public class StringReplyParser extends BaseParser {
	private static final String TAG = "StringReplyParser";
	
	private String mString = null;
	
	public String getString() {
		return mString;
	}
	
	public static String parse(String rpcResult) {
		try {
			StringReplyParser parser = new StringReplyParser();
			Xml.parse(rpcResult, parser);
			return parser.getString();
		} catch (SAXException e) {
			if (Logging.DEBUG) Log.d(TAG, "Malformed XML:\n" + rpcResult);
			else if (Logging.INFO) Log.i(TAG, "Malformed XML");
			return null;
		}
	}
	
	@Override
	public void startElement(String uri, String localName, String qName, Attributes attributes) throws SAXException {
		super.startElement(uri, localName, qName, attributes);
		mElementStarted = true;
		mCurrentElement.setLength(0);
	}
	
	@Override
	public void endElement(String uri, String localName, String qName) throws SAXException {
		super.endElement(uri, localName, qName);

		if (localName.equalsIgnoreCase("value")) {
			trimEnd();
			mString = mCurrentElement.toString();
		}
		mElementStarted = false;
	}
}
