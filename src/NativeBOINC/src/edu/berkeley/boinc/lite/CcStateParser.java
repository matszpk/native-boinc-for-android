/* 
 * AndroBOINC - BOINC Manager for Android
 * Copyright (C) 2010, Pavol Michalec
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

package edu.berkeley.boinc.lite;

import sk.boinc.nativeboinc.debug.Logging;
import android.util.Log;

public class CcStateParser extends BoincBaseParser {
	private static final String TAG = "CcStateParser";

	private CcState mCcState = new CcState();
	private VersionInfo mVersionInfo = new VersionInfo();
	private HostInfoParser mHostInfoParser = new HostInfoParser();
	private boolean mInHostInfo = false;
	private ProjectsParser mProjectsParser = new ProjectsParser();
	private boolean mInProject = false;
	private AppsParser mAppsParser = new AppsParser();
	private boolean mInApp = false;
	private AppVersionsParser mAppVersionsParser = new AppVersionsParser();
	private boolean mInAppVersion = false;
	private WorkunitsParser mWorkunitsParser = new WorkunitsParser();
	private boolean mInWorkunit = false;
	private ResultsParser mResultsParser = new ResultsParser();
	private boolean mInResult = false;
	
	public final CcState getCcState() {
		return mCcState;
	}

	/**
	 * Parse the RPC result (state) and generate vector of projects info
	 * @param rpcResult String returned by RPC call of core client
	 * @return connected client state
	 */
	public static CcState parse(String rpcResult) {
		try {
			CcStateParser parser = new CcStateParser();
			BoincBaseParser.parse(parser, rpcResult);
			return parser.getCcState();
		}
		catch (BoincParserException e) {
			if (Logging.DEBUG) Log.d(TAG, "Malformed XML:\n" + rpcResult);
			else if (Logging.INFO) Log.i(TAG, "Malformed XML");
			return null;
		}		

	}

	@Override
	public void endDocument() {
		// Commit sub-parsers data to resulting CcState
		mCcState.version_info = mVersionInfo;
		mCcState.host_info = mHostInfoParser.getHostInfo();
		mCcState.projects = mProjectsParser.getProjects();
		mCcState.apps = mAppsParser.getApps();
		mCcState.app_versions = mAppVersionsParser.getAppVersions();
		mCcState.workunits = mWorkunitsParser.getWorkunits();
		mCcState.results = mResultsParser.getResults();
	}

	@Override
	public void startElement(String localName) {
		super.startElement(localName);
		if (localName.equalsIgnoreCase("host_info")) {
			// Just stepped inside <host_info>
			mInHostInfo = true;
		}
		if (mInHostInfo) {
			mHostInfoParser.startElement(localName);
		}
		if (localName.equalsIgnoreCase("project")) {
			// Just stepped inside <project>
			mInProject = true;
		}
		if (mInProject) {
			mProjectsParser.startElement(localName);
		}
		if (localName.equalsIgnoreCase("app")) {
			// Just stepped inside <app>
			mInApp = true;
		}
		if (mInApp) {
			mAppsParser.startElement(localName);
		}
		if (localName.equalsIgnoreCase("app_version")) {
			// Just stepped inside <app_version>
			mInAppVersion = true;
		}
		if (mInAppVersion) {
			mAppVersionsParser.startElement(localName);
		}
		if (localName.equalsIgnoreCase("workunit")) {
			// Just stepped inside <workunit>
			mInWorkunit = true;
		}
		if (mInWorkunit) {
			mWorkunitsParser.startElement(localName);
		}
		if (localName.equalsIgnoreCase("result")) {
			// Just stepped inside <result>
			mInResult = true;
		}
		if (mInResult) {
			mResultsParser.startElement(localName);
		}
	}

	@Override
	public void characters(StringBuilder chars, int startPos, int endPos) {
		super.characters(chars, startPos, endPos);
		if (mInHostInfo) {
			// We are inside <host_info>
			mHostInfoParser.characters(chars, startPos, endPos);
		}
		if (mInProject) {
			// We are inside <project>
			mProjectsParser.characters(chars, startPos, endPos);
		}
		if (mInApp) {
			// We are inside <project>
			mAppsParser.characters(chars, startPos, endPos);
		}
		if (mInAppVersion) {
			// We are inside <project>
			mAppVersionsParser.characters(chars, startPos, endPos);
		}
		if (mInWorkunit) {
			// We are inside <workunit>
			mWorkunitsParser.characters(chars, startPos, endPos);
		}
		if (mInResult) {
			// We are inside <result>
			mResultsParser.characters(chars, startPos, endPos);
		}
		// VersionInfo elements are handled in super.characters()
	}

	@Override
	public void endElement(String localName) {
		super.endElement(localName);
		try {
			if (mInHostInfo) {
				// We are inside <host_info>
				// parse it by sub-parser in any case (to parse also closing element)
				mHostInfoParser.endElement(localName);
				if (localName.equalsIgnoreCase("host_info")) {
					mInHostInfo = false;
				}
			}
			if (mInProject) {
				// We are inside <project>
				// parse it by sub-parser in any case (must parse also closing element!)
				mProjectsParser.endElement(localName);
				if (localName.equalsIgnoreCase("project")) {
					// Closing tag of <project>
					mInProject = false;
				}
			}
			if (mInApp) {
				// We are inside <app>
				// parse it by sub-parser in any case (must parse also closing element!)
				mAppsParser.endElement(localName);
				if (localName.equalsIgnoreCase("app")) {
					// Closing tag of <app>
					mInApp = false;
				}
			}
			if (mInAppVersion) {
				// We are inside <app_version>
				// parse it by sub-parser in any case (must parse also closing element!)
				mAppsParser.endElement(localName);
				if (localName.equalsIgnoreCase("app_version")) {
					// Closing tag of <app_version>
					mInAppVersion = false;
				}
			}
			if (mInWorkunit) {
				// We are inside <workunit>
				// parse it by sub-parser in any case (must parse also closing element!)
				mWorkunitsParser.endElement(localName);
				if (localName.equalsIgnoreCase("workunit")) {
					// Closing tag of <workunit>
					mInWorkunit = false;
				}
			}
			if (mInResult) {
				// We are inside <result>
				// parse it by sub-parser in any case (must parse also closing element!)
				mResultsParser.endElement(localName);
				if (localName.equalsIgnoreCase("result")) {
					// Closing tag of <result>
					mInResult = false;
				}
			}
			// VersionInfo?
			if (localName.equalsIgnoreCase("core_client_major_version")) {
				mVersionInfo.major = Integer.parseInt(getCurrentElement());
			}
			else if (localName.equalsIgnoreCase("core_client_minor_version")) {
				mVersionInfo.minor = Integer.parseInt(getCurrentElement());
			}
			else if (localName.equalsIgnoreCase("core_client_release")) {
				mVersionInfo.release = Integer.parseInt(getCurrentElement());
			}
			else if (localName.equalsIgnoreCase("have_ati")) {
				mCcState.have_ati = !getCurrentElement().equals("0");
			}
			else if (localName.equalsIgnoreCase("have_cuda")) {
				mCcState.have_cuda = !getCurrentElement().equals("0");
			}
		}
		catch (NumberFormatException e) {
			if (Logging.INFO) Log.i(TAG, "Exception when decoding " + localName);
		}
	}
}
