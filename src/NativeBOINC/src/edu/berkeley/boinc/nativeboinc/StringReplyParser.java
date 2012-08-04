/**
 * 
 */
package edu.berkeley.boinc.nativeboinc;

import org.xml.sax.Attributes;
import org.xml.sax.SAXException;

import sk.boinc.nativeboinc.debug.Logging;
import android.util.Log;
import android.util.Xml;

import edu.berkeley.boinc.lite.BoincBaseParser;
import edu.berkeley.boinc.lite.BoincParserException;

/**
 * @author mat
 *
 */
public class StringReplyParser extends BoincBaseParser {
	private static final String TAG = "StringReplyParser";
	
	private String mString = null;
	
	public String getString() {
		return mString;
	}
	
	public static String parse(String rpcResult) {
		try {
			StringReplyParser parser = new StringReplyParser();
			BoincBaseParser.parse(parser, rpcResult);
			return parser.getString();
		} catch (BoincParserException e) {
			if (Logging.DEBUG) Log.d(TAG, "Malformed XML:\n" + rpcResult);
			else if (Logging.INFO) Log.i(TAG, "Malformed XML");
			return null;
		}
	}
	
	@Override
	public void startElement(String localName) {
		super.startElement(localName);
	}
	
	@Override
	public void endElement(String localName) {
		super.endElement(localName);

		if (localName.equalsIgnoreCase("value")) {
			mString = mCurrentElement;
		}
	}
}
