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

package sk.boinc.nativeboinc.nativeclient;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStreamWriter;
import java.util.Vector;

import edu.berkeley.boinc.lite.Md5;

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
	
	private Vector<InstalledDistrib> mInstalledDistribs = null;
	
	public InstalledDistribManager(Context context) {
		mContext = context;
	}
	
	public synchronized void addOrUpdateDistrib(ProjectDistrib distrib, int cpuType, Vector<String> files) {
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
			oldDistrib.cpuType = cpuType;
			oldDistrib.files = files;
		} else {	// add
			InstalledDistrib newDistrib = new InstalledDistrib();
			newDistrib.projectName = distrib.projectName;
			newDistrib.projectUrl = distrib.projectUrl;
			newDistrib.version = distrib.version;
			newDistrib.cpuType = cpuType;
			newDistrib.files = files;
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
	
	public synchronized void synchronizeWithProjectList(Context context) {
		Vector<InstalledDistrib> newDistribs = new Vector<InstalledDistrib>();
		
		String projectsPath = context.getFilesDir().getAbsolutePath()+"/boinc/projects/";
		
		for (InstalledDistrib distrib: mInstalledDistribs) {
			String escapedUrl = InstallerHandler.escapeProjectUrl(distrib.projectUrl);
			/* check whether projects's directory exists */
			if (new File(projectsPath+escapedUrl).isDirectory())
				newDistribs.add(distrib);
		}
		
		mInstalledDistribs = newDistribs;
		save();
	}

	public synchronized boolean load() {
		InputStream inStream = null;
		try {
			inStream = mContext.openFileInput("installed_distribs.xml");
			/* parse file */
			mInstalledDistribs = InstalledDistribListParser.parse(inStream);
			if (mInstalledDistribs == null) {
				if (Logging.ERROR) Log.e(TAG, "Cant load installedDistribs");
				return false;
			}
		} catch(FileNotFoundException ex) { 
			/* set as empty installed Distrib */
			mInstalledDistribs = new Vector<InstalledDistrib>();
		} finally {
			try {
				if (inStream != null)
					inStream.close();
			} catch(IOException ex2) { }
		}
		return true;
	}
	
	public synchronized boolean save() {
		OutputStreamWriter writer = null;
		StringBuilder sB = new StringBuilder();
		try {
			writer = new FileWriter(mContext.getFileStreamPath("installed_distribs.xml"));
			
			writer.write("<distribs>\n");
			
			for (InstalledDistrib distrib: mInstalledDistribs) {
				sB.delete(0, sB.length());
				sB.append("  <project>\n    <name>");
				sB.append(distrib.projectName);
				sB.append("</name>\n    <url>");
				sB.append(distrib.projectUrl);
				sB.append("</url>\n    <version>");
				sB.append(distrib.version);
				sB.append("</version>\n    <cpu>");
				sB.append(CpuType.getCpuName(distrib.cpuType));
				sB.append("</cpu>\n");
				for (String file: distrib.files) {
					sB.append("    <file>");
					sB.append(file);
					sB.append("</file>\n");
				}
				sB.append("  </project>\n");
				writer.write(sB.toString());
			}
			
			writer.write("</distribs>\n");
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
	
	public Vector<InstalledDistrib> getInstalledDistribs() {
		return mInstalledDistribs;
	}
	
	public InstalledDistrib getInstalledDistribByName(String name) {
		for (InstalledDistrib distrib: mInstalledDistribs)
			if (distrib.projectName.equals(name))
				return distrib;
		return null;
	}
}
