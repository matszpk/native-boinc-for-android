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

package sk.boinc.nativeboinc.installer;

import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;

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
public class ProjectsClientStateParser extends BaseParser {
	private static final String TAG = "ProjectsFromClientRetriever";

	private ProjectDescriptor mProjectDesc = null;
	private ArrayList<ProjectDescriptor> mProjects = new ArrayList<ProjectDescriptor>();
	
	public ProjectDescriptor[] getProjects() {
		return mProjects.toArray(new ProjectDescriptor[0]); // atrape
	}
	
	public static ProjectDescriptor[] parse(InputStream inputStream) {
		try {
			ProjectsClientStateParser parser = new ProjectsClientStateParser();
			Xml.parse(inputStream, Xml.Encoding.UTF_8, parser);
			return parser.getProjects();
		} catch (SAXException e) {
			if (Logging.DEBUG) Log.d(TAG, "Malformed XML:\n" + inputStream);
			else if (Logging.INFO) Log.i(TAG, "Malformed XML");
			return null;
		} catch (IOException e2) {
			if (Logging.ERROR) Log.e(TAG, "I/O Error in XML parsing:\n" + inputStream);
			return null;
		}
	}
	
	@Override
	public void startElement(String uri, String localName, String qName, Attributes attributes) throws SAXException {
		super.startElement(uri, localName, qName, attributes);
		if (localName.equalsIgnoreCase("project")) {
			mProjectDesc = new ProjectDescriptor();
		} else {
			mElementStarted = true;
			mCurrentElement.setLength(0);
		}
	}
	
	@Override
	public void endElement(String uri, String localName, String qName) throws SAXException {
		super.endElement(uri, localName, qName);
		if (localName.equalsIgnoreCase("project")) {
			mProjects.add(mProjectDesc);
			mProjectDesc = null;
		} else if (mProjectDesc != null) {
			trimEnd();
			
			if (localName.equalsIgnoreCase("project_name"))
				mProjectDesc.projectName = mCurrentElement.toString();
			else if (localName.equalsIgnoreCase("master_url"))
				mProjectDesc.masterUrl = mCurrentElement.toString();
		}
		mElementStarted = false;
	}
}
