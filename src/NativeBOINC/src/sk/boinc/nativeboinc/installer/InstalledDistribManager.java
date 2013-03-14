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

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStreamWriter;
import java.util.ArrayList;

import sk.boinc.nativeboinc.BoincManagerApplication;
import sk.boinc.nativeboinc.debug.Logging;

import android.content.Context;
import android.util.Log;

/**
 * @author mat
 *
 */
public class InstalledDistribManager {
	private final static String TAG = "InstalledDistribManager";
	
	private Context mContext = null;
	
	private InstalledClient mInstalledClient = null;
	private ArrayList<InstalledDistrib> mInstalledDistribs = null;
	
	public InstalledDistribManager(Context context) {
		mContext = context;
	}
	
	public synchronized void setClient(ClientDistrib clientDistrib) {
		mInstalledClient.version = clientDistrib.version;
		mInstalledClient.description = clientDistrib.description;
		mInstalledClient.changes = clientDistrib.changes;
		mInstalledClient.fromSDCard = false;
		save();
	}
	
	public synchronized void setClientFromSDCard() {
		mInstalledClient.version = "";
		mInstalledClient.description = "";
		mInstalledClient.changes = "";
		mInstalledClient.fromSDCard = true;
		save();
	}
	
	public synchronized void addOrUpdateDistrib(ProjectDistrib distrib, ArrayList<String> files) {
		int index = -1;
		for (int i = 0; i < mInstalledDistribs.size(); i++)
			if (mInstalledDistribs.get(i).projectUrl.equals(distrib.projectUrl)) {
				index = i;	// if found
				break;
			}
		
		if (index != -1) {	// update
			InstalledDistrib oldDistrib = mInstalledDistribs.get(index);
			oldDistrib.projectName = distrib.projectName;
			oldDistrib.projectUrl = distrib.projectUrl;
			oldDistrib.version = distrib.version;
			oldDistrib.description = distrib.description;
			oldDistrib.changes = distrib.changes;
			oldDistrib.fromSDCard = false;
			oldDistrib.files = files;
		} else {	// add
			InstalledDistrib newDistrib = new InstalledDistrib();
			newDistrib.projectName = distrib.projectName;
			newDistrib.projectUrl = distrib.projectUrl;
			newDistrib.version = distrib.version;
			newDistrib.description = distrib.description;
			newDistrib.files = files;
			newDistrib.changes = distrib.changes;
			newDistrib.fromSDCard = false;
			mInstalledDistribs.add(newDistrib);
		}
		
		save();
	}
	
	public synchronized void addOrUpdateDistribFromSDCard(String projectName, String projectUrl,
			ArrayList<String> files) {
		int index = -1;
		for (int i = 0; i < mInstalledDistribs.size(); i++)
			if (mInstalledDistribs.get(i).projectUrl.equals(projectUrl)) {
				index = i;	// if found
				break;
			}
		
		if (index != -1) {	// update
			InstalledDistrib oldDistrib = mInstalledDistribs.get(index);
			oldDistrib.projectName = projectName;
			oldDistrib.projectUrl = projectUrl;
			oldDistrib.version = "";
			oldDistrib.description = "";
			oldDistrib.changes = "";
			oldDistrib.fromSDCard = true;
			oldDistrib.files = files;
		} else {
			InstalledDistrib newDistrib = new InstalledDistrib();
			newDistrib.projectName = projectName;
			newDistrib.projectUrl = projectUrl;
			newDistrib.version = "";
			newDistrib.description = "";
			newDistrib.files = files;
			newDistrib.changes = "";
			newDistrib.fromSDCard = true;
			mInstalledDistribs.add(newDistrib);
		}
		
		save();
	}
	
	public synchronized void removeDistrib(String projectUrl) {
		for (int i = 0; i < mInstalledDistribs.size(); i++)
			if (mInstalledDistribs.get(i).projectUrl.equals(projectUrl)) {
				mInstalledDistribs.remove(i);
				save();
				break;
			}
	}
	
	public synchronized void removeDistribsByProjectName(ArrayList<String> projectNames) {
		for (String projectName: projectNames) {
			for (int i = 0; i < mInstalledDistribs.size(); i++)
				if (mInstalledDistribs.get(i).projectName.equals(projectName)) {
					mInstalledDistribs.remove(i);
					break;
				}
		}
		save();
	}
	
	
	public synchronized void synchronizeWithProjectList(Context context) {
		ArrayList<InstalledDistrib> newDistribs = new ArrayList<InstalledDistrib>();
		
		String projectsPath = BoincManagerApplication.getBoincDirectory(context)+"/projects/";
		
		for (InstalledDistrib distrib: mInstalledDistribs) {
			String escapedUrl = InstallerHandler.escapeProjectUrl(distrib.projectUrl);
			/* check whether projects's directory exists and app_info.xml */
			if (new File(projectsPath+escapedUrl).isDirectory() && 
					new File(projectsPath+escapedUrl+"/app_info.xml").exists())
				newDistribs.add(distrib);
		}
		
		mInstalledDistribs = newDistribs;
		save();
	}

	public synchronized boolean load() {
		InputStream inStream = null;
		boolean failed = false;
		try {
			inStream = mContext.openFileInput("installed_distribs.xml");
			/* parse file */
			mInstalledDistribs = InstalledDistribListParser.parse(inStream);
			if (mInstalledDistribs == null) {
				mInstalledDistribs = new ArrayList<InstalledDistrib>();
				if (Logging.ERROR) Log.e(TAG, "Cant load installedDistribs");
				failed = true;
			}
		} catch(FileNotFoundException ex) { 
			/* set as empty installed Distrib */
			mInstalledDistribs = new ArrayList<InstalledDistrib>();
		} finally {
			try {
				if (inStream != null)
					inStream.close();
			} catch(IOException ex2) { }
		}
		
		try {
			inStream = mContext.openFileInput("installed_client.xml");
			/* parse file */
			mInstalledClient = InstalledClientParser.parse(inStream);
			if (mInstalledClient == null) {
				mInstalledClient = new InstalledClient();
				if (Logging.ERROR) Log.e(TAG, "Cant load installedClient");
				return false;
			}
		} catch(FileNotFoundException ex) { 
			mInstalledClient = new InstalledClient();
		} finally {
			try {
				if (inStream != null)
					inStream.close();
			} catch(IOException ex2) { }
		}
		return !failed;
	}
	
	public synchronized boolean save() {
		if (Logging.DEBUG) Log.d(TAG, "Saving installed binaries list");
		
		OutputStreamWriter writer = null;
		StringBuilder sB = new StringBuilder();
		try {
			writer = new OutputStreamWriter(mContext.openFileOutput("installed_distribs.xml",
					Context.MODE_PRIVATE), "UTF-8");
			
			writer.write("<distribs>\n");
			
			for (InstalledDistrib distrib: mInstalledDistribs) {
				if (distrib.files == null)
					continue;
				sB.delete(0, sB.length());
				sB.append("  <project>\n    <name>");
				sB.append(distrib.projectName);
				sB.append("</name>\n    <url>");
				sB.append(distrib.projectUrl);
				sB.append("</url>\n    <version>");
				sB.append(distrib.version);
				sB.append("</version>\n");
				for (String file: distrib.files) {
					sB.append("    <file>");
					sB.append(file);
					sB.append("</file>\n");
				}
				sB.append("    <description><![CDATA[");
				sB.append(distrib.description);
				sB.append("]]></description>\n    <changes><![CDATA[");
				sB.append(distrib.changes);
				if (distrib.fromSDCard)
					sB.append("]]></changes>\n    <fromSDCard/>\n  </project>\n");
				else
					sB.append("]]></changes>\n  </project>\n");
				writer.write(sB.toString());
			}
			
			writer.write("</distribs>\n");
			
			writer.flush();
		} catch(IOException ex) {
			return false;
		} finally {
			try {
				if (writer != null)
					writer.close();
			} catch ( IOException ex) { }
		}
		
		sB.setLength(0);
		try {
			writer = new OutputStreamWriter(mContext.openFileOutput("installed_client.xml",
					Context.MODE_PRIVATE), "UTF-8");
			
			sB.append("<client>\n  <version>");
			sB.append(mInstalledClient.version);
			sB.append("</version>\n  <description><![CDATA[");
			sB.append(mInstalledClient.description);
			sB.append("]]></description>\n  <changes><![CDATA[");
			sB.append(mInstalledClient.changes);
			if (mInstalledClient.fromSDCard)
				sB.append("]]></changes>\n  <fromSDCard/>\n</client>\n");
			else
				sB.append("]]></changes>\n</client>\n");
			
			writer.write(sB.toString());
		} catch(IOException ex) {
			return false;
		} finally {
			try {
				if (writer != null)
					writer.close();
			} catch ( IOException ex) { }
		}
		
		return true;
	}
	
	public InstalledClient getInstalledClient() {
		return mInstalledClient;
	}
	
	public ArrayList<InstalledDistrib> getInstalledDistribs() {
		return mInstalledDistribs;
	}
	
	public InstalledDistrib getInstalledDistribByName(String name) {
		for (InstalledDistrib distrib: mInstalledDistribs)
			if (distrib.projectName.equals(name))
				return distrib;
		return null;
	}
}
