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

import java.io.FileInputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

import android.content.Context;
import android.util.Log;

import edu.berkeley.boinc.lite.Project;
import sk.boinc.nativeboinc.BoincManagerApplication;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.nativeclient.NativeBoincProjectsListener;
import sk.boinc.nativeboinc.nativeclient.NativeBoincService;
import sk.boinc.nativeboinc.nativeclient.NativeBoincStateListener;
import sk.boinc.nativeboinc.nativeclient.WorkerOp;

/**
 * @author mat
 *
 */
public class ProjectsFromClientRetriever implements NativeBoincProjectsListener, NativeBoincStateListener {

	private static final String TAG = "ProjectsFromClient";
	
	private static final int PROJECTS_FROM_CLIENT_ID = 1; // channelId
	
	private Context mContext = null;
	private NativeBoincService mRunner = null;
	
	private ProjectDescriptor[] mProjectDescs = null;
	private boolean mIsTimeout = false; 
	
	@Override
	public int getRunnerServiceChannelId() {
		return PROJECTS_FROM_CLIENT_ID;
	}
	
	public ProjectsFromClientRetriever(Context context) {
		mContext = context;
	}
	
	public void setRunnerService(NativeBoincService runner) {
		mRunner = runner;
	}
	
	private Semaphore mProjectDescsSem = new Semaphore(1);
	
	public ProjectDescriptor[] getProjectDescriptors() {
		ProjectDescriptor[] projDescs = null;
		mProjectDescs = null;
		mIsTimeout = false;
		
		if (mRunner != null && mRunner.isRun()) {
			// if run
			mRunner.addNativeBoincListener(this);
			
			// lock waiter
			try {
				mProjectDescsSem.acquire();
			} catch(InterruptedException ex) {
			}
			
			mRunner.getProjects(getRunnerServiceChannelId());
			
			// awaiting for callback 
			try {
				if (!mProjectDescsSem.tryAcquire(5000, TimeUnit.MILLISECONDS))
					mIsTimeout = true;
			}  catch(InterruptedException ex) {
				mIsTimeout = true;
			} finally {
				mProjectDescsSem.release();
			}
			
			if (Logging.DEBUG) Log.d(TAG, "Timeout: "+mIsTimeout);
			
			if (mProjectDescs != null)
				projDescs = mProjectDescs;
			
			mRunner.removeNativeBoincListener(this);
		}
		
		if (mProjectDescs == null) {
			// if client is not run or not works
			FileInputStream inputStream = null;
			
			try {
				inputStream = new FileInputStream(
						BoincManagerApplication.getBoincDirectory(mContext)+"/client_state.xml");
				projDescs = ProjectsClientStateParser.parse(inputStream);
			} catch(IOException ex) {
				// do nothing
			} finally {
				try {
					if (inputStream != null)
						inputStream.close();
				} catch(IOException ex2) { }
			}
		}
		
		return projDescs;
	}
	
	public void destroy() {
		mRunner = null;
		mContext = null;
	}
	
	@Override
	public boolean onNativeBoincClientError(String message) {
		mProjectDescsSem.release();
		return true; // consume because is separate channel
	}

	@Override
	public void onChangeRunnerIsWorking(boolean isWorking) {
	}

	@Override
	public boolean onNativeBoincServiceError(WorkerOp workerOp, String message) {
		if (workerOp.equals(WorkerOp.GetProjects))
			mProjectDescsSem.release();
		return false; // do not consume
	}

	@Override
	public void getProjects(ArrayList<Project> projects) {
		if (!mIsTimeout) {
			if (Logging.DEBUG) Log.d(TAG, "Get projects");
			
			mProjectDescs = new ProjectDescriptor[projects.size()];
			
			for (int i = 0; i < projects.size(); i++) {
				Project project = projects.get(i);
				mProjectDescs[i] = new ProjectDescriptor(project.project_name, project.master_url);
			}
		}
		
		mProjectDescsSem.release();
	}

	@Override
	public void onClientStart() {
	}

	@Override
	public void onClientStop(int exitCode, boolean stoppedByManager) {
		// release wait lock
		mProjectDescsSem.release();
	}
}
