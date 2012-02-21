/**
 * 
 */
package edu.berkeley.boinc.lite;

import java.util.ArrayList;
import java.util.HashMap;

import org.xml.sax.Attributes;
import org.xml.sax.SAXException;

import sk.boinc.nativeboinc.debug.Logging;
import android.util.Log;
import android.util.Xml;

/**
 * @author mat
 *
 */
public class DiskUsageParser extends BaseParser {

	private static final String TAG = "DiskUsageParser";
	
	private HashMap<String, Double> mDiskUsageMap = new HashMap<String, Double>();
	
	private String mMasterUrl = null;
	private double mDiskUsage = 0.0;
	
	public static boolean parse(String rpcResult, ArrayList<Project> projects) {
		try {
			DiskUsageParser parser = new DiskUsageParser();
			Xml.parse(rpcResult, parser);
			
			parser.setUpDiskUsage(projects);
			return true;
		}
		catch (SAXException e) {
			if (Logging.DEBUG) Log.d(TAG, "Malformed XML:\n" + rpcResult);
			else if (Logging.INFO) Log.i(TAG, "Malformed XML");
			return false;
		}
	}
	
	private void setUpDiskUsage(ArrayList<Project> projects) {
		for (Project project: projects) {
			Double diskUsage = mDiskUsageMap.get(project.master_url);
			if (diskUsage != null)
				project.disk_usage = diskUsage;
			else // unknown disk usage
				project.disk_usage = -1.0;
		}
	}
	
	@Override
	public void startElement(String uri, String localName, String qName, Attributes attributes) throws SAXException {
		super.startElement(uri, localName, qName, attributes);
		if (localName.equalsIgnoreCase("project")) {
			mMasterUrl = null;
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
			if (localName.equalsIgnoreCase("project")) {
				// Closing tag of <project> - add to vector and be ready for next one
				if (mMasterUrl != null) {
					// master_url is a must
					mDiskUsageMap.put(mMasterUrl, mDiskUsage);
					mMasterUrl = null;
				}
			} else {
				trimEnd();
				if (localName.equalsIgnoreCase("master_url")) {
					mMasterUrl = mCurrentElement.toString();
				} else if (localName.equalsIgnoreCase("disk_usage")) {
					mDiskUsage = Double.parseDouble(mCurrentElement.toString());
				}
			}
		} catch (NumberFormatException e) {
			if (Logging.INFO) Log.i(TAG, "Exception when decoding " + localName);
		}
		mElementStarted = false;
	}
}
