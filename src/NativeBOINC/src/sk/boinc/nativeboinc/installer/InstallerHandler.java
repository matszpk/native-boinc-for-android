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
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InterruptedIOException;
import java.io.SyncFailedException;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.ArrayList;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

import edu.berkeley.boinc.nativeboinc.ClientEvent;

import sk.boinc.nativeboinc.BoincManagerApplication;
import sk.boinc.nativeboinc.R;
import sk.boinc.nativeboinc.debug.Logging;
import sk.boinc.nativeboinc.nativeclient.MonitorListener;
import sk.boinc.nativeboinc.nativeclient.NativeBoincService;
import sk.boinc.nativeboinc.nativeclient.NativeBoincStateListener;
import sk.boinc.nativeboinc.nativeclient.NativeBoincUpdateListener;
import sk.boinc.nativeboinc.nativeclient.NativeBoincUtils;
import sk.boinc.nativeboinc.nativeclient.WorkerOp;
import sk.boinc.nativeboinc.util.Chmod;
import sk.boinc.nativeboinc.util.ClientId;
import sk.boinc.nativeboinc.util.HostListDbAdapter;
import sk.boinc.nativeboinc.util.PreferenceName;
import sk.boinc.nativeboinc.util.RuntimeUtils;
import sk.boinc.nativeboinc.util.UpdateItem;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Build;
import android.os.Handler;
import android.preference.PreferenceManager;
import android.util.Log;

/**
 * @author mat
 *
 */
public class InstallerHandler extends Handler implements NativeBoincUpdateListener,
	NativeBoincStateListener, MonitorListener {
	
	private final static String TAG = "InstallerHandler";

	private InstallerService mInstallerService = null;
	private BoincManagerApplication mApp = null;
	
	private InstallerService.ListenerHandler mListenerHandler;
	
	private InstalledDistribManager mDistribManager = null;
	private ArrayList<ProjectDistrib> mProjectDistribs = null;
	private ClientDistrib mClientDistrib = null;
	
	private InstallationOps mInstallOps = null; 
	
	private static final int NOTIFY_PERIOD = 200; /* milliseconds */
	
	private NativeBoincService mRunner = null;
	
	private boolean mIsClientBeingInstalled = false;
	private Semaphore mDelayedClientShutdownSem = new Semaphore(1);
	
	private boolean mDoDumpBoincFiles = false;
	private boolean mDoBoincReinstall = false;
	
	private Downloader mDownloader = null;
	
	private ExecutorService mExecutorService = null;
	
	private ProjectsFromClientRetriever mProjectsRetriever = null;
	
	// if handler is working
	private boolean mHandlerIsWorking = false;
	
	// if installer service is working (previous state)
	private boolean mPreviousStateOfIsWorking = false;
	
	private ProjectDescriptor[] mProjectDescs = null;
	
	/* locks resources (such as wifi and CPU) */
	private ResourcesLocker mResourcesLocker = null;
	
	/**
	 * current channelId (default is unset: -1)
	 */
	public int mCurrentChannelId = -1;
	
	/**
	 * abstract class for workers
	 */
	
	private abstract class AbstractWorker implements Runnable {
		protected boolean mIsRan = false;
		
		protected Future<?> mFuture = null;
		
		public void setFuture(Future<?> future) {
			mFuture = future;
		}
		
		public Future<?> getFuture() {
			return mFuture;
		}
		
		public boolean isRan() {
			return mIsRan;
		}
	}
	
	public InstallerHandler(final InstallerService installerService, InstallerService.ListenerHandler listenerHandler) {
		mInstallerService = installerService;
		
		mApp = (BoincManagerApplication)installerService.getApplicationContext();
		
		Log.d(TAG, "Number of processors:"+ RuntimeUtils.getRealCPUCount());
		mExecutorService = Executors.newFixedThreadPool(RuntimeUtils.getRealCPUCount());
		
		mDownloader = new Downloader(installerService, this);
		mProjectsRetriever = new ProjectsFromClientRetriever(installerService);
		mListenerHandler = listenerHandler;
		mDistribManager = new InstalledDistribManager(installerService);
		mDistribManager.load();
		mInstallOps = new InstallationOps(this, installerService, mDownloader, mDistribManager);
		
		mResourcesLocker = new ResourcesLocker(mInstallerService);
		
		// parse project distribs
		mProjectDistribs = mInstallOps.parseProjectDistribs("apps-old.xml");
	}
	
	public void destroy() {
		mExecutorService.shutdown();
		try {
			mExecutorService.awaitTermination(5, TimeUnit.SECONDS);
			mExecutorService.shutdownNow();
			mExecutorService.awaitTermination(5, TimeUnit.SECONDS);
		} catch (InterruptedException ex) {
			mExecutorService.shutdownNow();
		}
		mResourcesLocker.destroy();
		mResourcesLocker = null;
		mProjectsRetriever.destroy();
		mProjectsRetriever = null;
		mDownloader.destroy();
		mDownloader = null;
		mInstallerService = null;
		mRunner = null;
		mDistribManager = null;
		mInstallOps.destroy();
		mInstallOps = null;
	}
	
	public void setRunnerService(NativeBoincService service) {
		mRunner = service;
		mInstallOps.setRunnerService(service);
		mProjectsRetriever.setRunnerService(mRunner);
	}
	
	public String resolveProjectName(String projectUrl) {
		if (mProjectDistribs == null)
			return null;
		
		for (ProjectDistrib distrib: mProjectDistribs)
			if (distrib.projectUrl.equals(projectUrl))
				return distrib.projectName;
		return null;
	}
	
	/**
	 * @return true if success
	 **/
	public boolean updateClientDistrib(int channelId) {
		synchronized (this) {
			mCurrentChannelId = channelId;
			mHandlerIsWorking = true;
			notifyChangeOfIsWorking();
		}
		
		try {
			mClientDistrib = mInstallOps.updateClientDistrib(channelId, InstallOp.UpdateClientDistrib);
	
			if (mClientDistrib == null)
				return false;
			
			notifyClientDistrib(channelId, InstallOp.UpdateClientDistrib, mClientDistrib);
		} finally {
			synchronized (this) {
				mCurrentChannelId = -1; // unset channel id
				mHandlerIsWorking = false;
				notifyChangeOfIsWorking();
			}
		}
		return true;
	}
	
	private static final int BUFFER_SIZE = 4096;
	
	private ClientInstaller mClientInstaller = null;
	
	private boolean mClientShouldBeRun = false;
	
	private class ClientInstaller extends AbstractWorker {
		private boolean mStandalone;
		
		private String mSDCardPath = null;
		
		public ClientInstaller(boolean standalone, String sdCardPath) {
			mStandalone = standalone;
			mSDCardPath = sdCardPath;
		}
		
		@Override
		public void run() {
			Thread currentThread = Thread.currentThread();
			
			boolean clientToUpdate = InstallerService.isClientInstalled(mInstallerService);
			
			if (mStandalone)	{// if standalone called from InstallerService
				if (Logging.DEBUG) Log.d(TAG, "Install client standalone");
			}
			
			boolean previouslyClientIsRan = mRunner.isRun();
			String boincClientFilename = "boinc_client";
			if (previouslyClientIsRan) {
				if (Logging.DEBUG) Log.d(TAG, "Install client when ran.");
				mDelayedClientShutdownSem.tryAcquire();
				mRunner.shutdownClient();
				boincClientFilename = "boinc_client_new";
			}
			
			// should be run during installation
			mClientShouldBeRun = previouslyClientIsRan || !clientToUpdate;
			
			/* acquire resources (wifi, CPU) during installation */
			mResourcesLocker.acquireAllLocks();
			
			try {
				String zipFilename = null;
				
				mIsRan = true;
				
				if (currentThread.isInterrupted() || (mFuture != null && mFuture.isCancelled())) {
					notifyCancel(InstallerService.BOINC_CLIENT_ITEM_NAME, "");
					return;
				}
				
				/* download and unpack */
				if (mSDCardPath == null) {
					if (mClientDistrib == null) {
						notifyError(InstallerService.BOINC_CLIENT_ITEM_NAME, "",
								mInstallerService.getString(R.string.clientDistribNotFound));
						return;
					}
					
					zipFilename = mClientDistrib.filename;
					if (Logging.INFO) Log.i(TAG, "Use zip "+zipFilename);
					
					if (currentThread.isInterrupted()) {
						notifyCancel(InstallerService.BOINC_CLIENT_ITEM_NAME, "");
						return;
					}
					
					// download from server
					try {
						String zipUrlString = mInstallerService.getString(R.string.installClientSourceUrl)+zipFilename; 
						mDownloader.downloadFile(zipUrlString, "boinc_client.zip",
								mInstallerService.getString(R.string.downloadNativeClient),
								mInstallerService.getString(R.string.downloadNativeClientError), true,
								InstallerService.DEFAULT_CHANNEL_ID, InstallOp.ProgressOperation,
								InstallerService.BOINC_CLIENT_ITEM_NAME, "");
						
						if (currentThread.isInterrupted()) {
							mInstallerService.deleteFile("boinc_client.zip");
							notifyCancel(InstallerService.BOINC_CLIENT_ITEM_NAME, "");
							return;
						}
						
						int status = mDownloader.verifyFile(mInstallerService.getFileStreamPath("boinc_client.zip"),
								zipUrlString, true, InstallerService.DEFAULT_CHANNEL_ID,
								InstallOp.ProgressOperation, InstallerService.BOINC_CLIENT_ITEM_NAME, "");
						
						if (status == Downloader.VERIFICATION_CANCELLED) {
							mInstallerService.deleteFile("boinc_client.zip");
							notifyCancel(InstallerService.BOINC_CLIENT_ITEM_NAME, "");
							return;	// cancelled
						}
						if (status == Downloader.VERIFICATION_FAILED) {
							notifyError(InstallerService.BOINC_CLIENT_ITEM_NAME, "",
									mInstallerService.getString(R.string.verifySignatureFailed));
							return;	// cancelled
						}
					} catch(InstallationException ex) {
						/* remove zip file */
						mInstallerService.deleteFile("boinc_client.zip");
						return;
					}
				}
				
				if (currentThread.isInterrupted()) {
					mInstallerService.deleteFile("boinc_client.zip");
					notifyCancel(InstallerService.BOINC_CLIENT_ITEM_NAME, "");
					return;
				}
				
				if (mSDCardPath == null || mSDCardPath.endsWith(".zip")) {
					// unzip if path to zip or zip from server
					FileOutputStream outStream = null;
					InputStream zipStream = null;
					ZipFile zipFile = null;
					/* unpack zip file */
					try {
						if (mSDCardPath == null)
							zipFile = new ZipFile(mInstallerService.getFileStreamPath("boinc_client.zip"));
						else // if from sdcard (zip)
							zipFile = new ZipFile(mSDCardPath);
						
						ZipEntry zipEntry = zipFile.entries().nextElement();
						zipStream = zipFile.getInputStream(zipEntry);
						outStream = mInstallerService.openFileOutput(boincClientFilename, Context.MODE_PRIVATE);
						
						long time = System.currentTimeMillis();
						long length = zipEntry.getSize();
						int totalReaded = 0;
						byte[] buffer = new byte[BUFFER_SIZE];
						/* copying content to file */
						String opDesc = mInstallerService.getString(R.string.unpackNativeClient);
						while (true) {
							int readed = zipStream.read(buffer);
							if (readed == -1)
								break;
							totalReaded += readed;
							
							if (currentThread.isInterrupted()) {
								mInstallerService.deleteFile(boincClientFilename);
								mInstallerService.deleteFile("boinc_client.zip");
								notifyCancel(InstallerService.BOINC_CLIENT_ITEM_NAME, "");
								return;
							}
							
							outStream.write(buffer, 0, readed);
							long newTime = System.currentTimeMillis();
							
							if (newTime-time > NOTIFY_PERIOD) {
								notifyProgress(InstallerService.BOINC_CLIENT_ITEM_NAME, "",
										opDesc, (int)((double)totalReaded*10000.0/(double)length));
								time = newTime;
							}
						}
						
						outStream.flush();
						
						notifyProgress(InstallerService.BOINC_CLIENT_ITEM_NAME, "",
								opDesc, InstallerProgressListener.FINISH_PROGRESS);
					} catch(InterruptedIOException ex) {
						notifyCancel(InstallerService.BOINC_CLIENT_ITEM_NAME, "");
						mInstallerService.deleteFile(boincClientFilename);
						return;
					} catch(IOException ex) {
						notifyError(InstallerService.BOINC_CLIENT_ITEM_NAME, "",
								mInstallerService.getString(R.string.unpackNativeClientError));
						mInstallerService.deleteFile(boincClientFilename);
						return;
					} finally {
						try {
							if (zipStream != null)
								zipStream.close();
						} catch(IOException ex) { }
						try {
							if (zipFile != null)
								zipFile.close();
						} catch(IOException ex) { }
						try {
							if (outStream != null)
								outStream.close();
						} catch(IOException ex) { }
						/* remove zip file */
						if (mSDCardPath == null) // if from server
							mInstallerService.deleteFile("boinc_client.zip");
					}
				} else {
					// copy to destination
					FileInputStream inputStream = null;
					FileOutputStream outputStream = null;
					
					try {
						inputStream = new FileInputStream(mSDCardPath);
						outputStream = mInstallerService.openFileOutput(boincClientFilename, Context.MODE_PRIVATE);
						
						byte[] buffer = new byte[BUFFER_SIZE];
						
						while (true) {
							int readed = inputStream.read(buffer);
							if (readed == -1)
								break;
							outputStream.write(buffer, 0, readed);
						}
						
						outputStream.flush();
					} catch(IOException ex) {
						notifyError(InstallerService.BOINC_CLIENT_ITEM_NAME, "",
								mInstallerService.getString(R.string.copyNativeClientError));
						return;
					} finally {
						try {
							if (inputStream != null)
								inputStream.close();
						} catch(IOException ex) { }
						
						try {
							if (outputStream != null)
								outputStream.close();
						} catch(IOException ex) { }
					}
				}
				
				if (currentThread.isInterrupted()) {
					mInstallerService.deleteFile("boinc_client.zip");
					mInstallerService.deleteFile(boincClientFilename);
					notifyCancel(InstallerService.BOINC_CLIENT_ITEM_NAME, "");
					return;
				}
				
				/* change permissions */
				if (!Chmod.chmod(mInstallerService.getFileStreamPath(boincClientFilename).getAbsolutePath(), 0700)) {
					notifyError(InstallerService.BOINC_CLIENT_ITEM_NAME, "",
							mInstallerService.getString(R.string.changePermissionsError));
			    	mInstallerService.deleteFile(boincClientFilename);
			    	return;
				}
				
			    if (currentThread.isInterrupted()) {
					notifyCancel(InstallerService.BOINC_CLIENT_ITEM_NAME, "");
					return;
				}
			    
			    /* create boinc directory */
			    File boincDir = new File(mInstallerService.getFilesDir()+"/boinc");
			    if (!boincDir.isDirectory()) {
			    	boincDir.delete();
			    	boincDir.mkdir();
			    }
			    
			    if (mSDCardPath == null) {
				    if (mClientDistrib != null)
			    		mDistribManager.setClient(mClientDistrib);
			    } else // from SDCard
		    		mDistribManager.setClientFromSDCard();
			    
			    /* running and killing boinc client (run in this client) */
			    if (!clientToUpdate) {
			    	if (Logging.DEBUG) Log.d(TAG, "First start client");
			    	
			    	notifyOperation(InstallerService.BOINC_CLIENT_ITEM_NAME, "",
			    			mInstallerService.getString(R.string.nativeClientFirstStart));
			    	
				    if (!NativeBoincService.firstStartClient(mInstallerService)) {
				    	if (Logging.DEBUG) Log.d(TAG, "first start client error");
				    	notifyError(InstallerService.BOINC_CLIENT_ITEM_NAME, "",
				    			mInstallerService.getString(R.string.nativeClientFirstStartError));
				    	return;
				    }
				    notifyOperation(InstallerService.BOINC_CLIENT_ITEM_NAME, "",
			    			mInstallerService.getString(R.string.nativeClientFirstKill));	    	
			    }
			    
			    if (previouslyClientIsRan) {
			    	if (Logging.DEBUG) Log.d(TAG, "Rename to real boinc");
			    	try {
			    		notifyOperation(InstallerService.BOINC_CLIENT_ITEM_NAME, "",
			    				mInstallerService.getString(R.string.nativeClientAwaitForShutdown));
				    	mDelayedClientShutdownSem.acquire();
				    	// rename new boinc client file
				    	mInstallerService.getFileStreamPath("boinc_client_new").renameTo(
				    			mInstallerService.getFileStreamPath("boinc_client"));
				    	/* give control over installation to NativeBoincService (second start) */
				    	mIsClientBeingInstalled = false;
				    	
				    	mApp.setRunRestartAfterReinstall(); // inform runner
			    		mRunner.startClient(false);
				    	
				    	mDelayedClientShutdownSem.release();
			    	} catch(InterruptedException ex) {
			    		notifyCancel(InstallerService.BOINC_CLIENT_ITEM_NAME, "");
						return;
			    	}
			    } else {
			    	if (!clientToUpdate) {	// if should be installed first
			    		// add nativeboinc to client list
				    	HostListDbAdapter dbAdapter = null;
				    	
				    	if (Logging.DEBUG) Log.d(TAG, "Adding nativeboinc to hostlist");
					    
				    	// set up waiting for benchmark
						SharedPreferences sharedPrefs = PreferenceManager
								.getDefaultSharedPreferences(mInstallerService);
						sharedPrefs.edit().putBoolean(PreferenceName.WAITING_FOR_BENCHMARK, true)
								.commit();
						
				    	try {
				    		String accessPassword = NativeBoincUtils.getAccessPassword(mInstallerService);
				    		
				    		dbAdapter = new HostListDbAdapter(mInstallerService);
				    		dbAdapter.open();
				    		dbAdapter.addHost(new ClientId(0, "nativeboinc", "127.0.0.1",
				    				31416, accessPassword));
				    		
				    		// change hostname
				    		NativeBoincUtils.setHostname(mInstallerService, Build.PRODUCT);
				    	} catch(IOException ex) {
				    		notifyError(InstallerService.BOINC_CLIENT_ITEM_NAME, "",
				    				mInstallerService.getString(R.string.getAccessPasswordError));
				    	} finally {
				    		if (dbAdapter != null)
				    			dbAdapter.close();
				    	}
			    	}
			    	
			    	mIsClientBeingInstalled = false;
			    	// starting client
			    	if (!clientToUpdate)
			    		mRunner.startClient(true);
			    }
			    
			    notifyFinish(InstallerService.BOINC_CLIENT_ITEM_NAME, "");
			} catch(Exception ex) {
				if (Logging.ERROR) Log.e(TAG, "on client install finishing:"+
							ex.getClass().getCanonicalName()+":"+ex.getMessage());
				notifyError(InstallerService.BOINC_CLIENT_ITEM_NAME, "",
	    				mInstallerService.getString(R.string.unexpectedError)+": "+ex.getMessage());
			} finally {
				if (Logging.DEBUG) Log.d(TAG, "Release semaphore mDelayedClientShutdownSem");
				mDelayedClientShutdownSem.release();
				// notify that client installer is not working
				
				// unlock all locks for project apps installers
				notifyIfClientUpdatedAndRan();
				notifyIfBenchmarkFinished();
				
				/* release resources (wifi, CPU) during installation */
				mResourcesLocker.releaseAllLocks();
				
				synchronized(InstallerHandler.this) {
					mClientShouldBeRun = false;
					mIsClientBeingInstalled = false;
					mClientInstaller = null;
					notifyChangeOfIsWorking();
				}
			}
		}
	}
	
	public void installClientAutomatically(boolean standalone) {
		if (mIsClientBeingInstalled)
			return;	// skip
		
		notifyOperation(InstallerService.BOINC_CLIENT_ITEM_NAME, "",
				mInstallerService.getString(R.string.installClientNotifyBegin));
		synchronized(this) {
			mIsClientBeingInstalled = true;
			mClientInstaller = new ClientInstaller(standalone, null);
			// notify that client installer is working
			notifyChangeOfIsWorking();
			Future<?> future = mExecutorService.submit(mClientInstaller);
			mClientInstaller.setFuture(future);
		}
	}
	
	/* from native boinc lib */
	public static String escapeProjectUrl(String url) {
		StringBuilder out = new StringBuilder();
		
		int i = url.indexOf("://");
		if (i == -1)
			i = 0;
		else
			i += 3;
		
		for (; i < url.length(); i++) {
			char c = url.charAt(i);
			if (Character.isLetterOrDigit(c) || c == '.' || c == '_' || c == '-')
				out.append(c);
			else
				out.append('_');
		}
		/* remove trailing '_' */
		if (out.length() >= 1 && out.charAt(out.length()-1) == '_')
			out.deleteCharAt(out.length()-1);
		return out.toString();
	}
	
	private ArrayList<String> unpackProjectApplications(ProjectDistrib projectDistrib, String zipPath,
			boolean directInstallation) {
		/* */
		Thread currentThread = Thread.currentThread();
		
		if (currentThread.isInterrupted())
			return null;
		
		ArrayList<String> fileList = new ArrayList<String>();
		FileOutputStream outStream = null;
		ZipFile zipFile = null;
		InputStream inStream = null;
		
		long totalLength = 0;
		
		StringBuilder projectAppFilePath = new StringBuilder();
		projectAppFilePath.append(mInstallerService.getFilesDir());
		if (directInstallation)
			projectAppFilePath.append("/boinc/projects/");
		else
			projectAppFilePath.append("/boinc/updates/");
		
		projectAppFilePath.append(escapeProjectUrl(projectDistrib.projectUrl));
		projectAppFilePath.append("/");
		
		File updateDir = new File(projectAppFilePath.toString());
		
		if (!updateDir.isDirectory())
			updateDir.mkdirs();
		
		int projectDirPathLength = projectAppFilePath.length();
		
		try {
			zipFile = new ZipFile(zipPath);
			Enumeration<? extends ZipEntry> zipEntries = zipFile.entries();
			
			/* count total length of uncompressed files */
			while (zipEntries.hasMoreElements()) {
				ZipEntry entry = zipEntries.nextElement();
				if (!entry.isDirectory()) {
					totalLength += entry.getSize();

					String entryName = entry.getName();
					int slashIndex = entryName.lastIndexOf('/');
					if (slashIndex != -1)
						entryName = entryName.substring(slashIndex+1);
					
					fileList.add(entryName);
				}
			}
			
			long totalReaded = 0;
			byte[] buffer = new byte[BUFFER_SIZE];
			/* unpack zip */
			zipEntries = zipFile.entries();
			
			int i = 0;
			long time = System.currentTimeMillis();
			
			String opDesc = mInstallerService.getString(R.string.unpackApplication);
			
			while (zipEntries.hasMoreElements()) {
				ZipEntry entry = zipEntries.nextElement();
				if (entry.isDirectory())
					continue;
				
				inStream = zipFile.getInputStream(entry);
				
				projectAppFilePath.append(fileList.get(i));
				outStream = new FileOutputStream(projectAppFilePath.toString());
				
				/* copying to project file */
				while (true) {
					int readed = inStream.read(buffer);
					if (readed == -1)
						break;
					
					totalReaded += readed;
					
					if (currentThread.isInterrupted()) {
						notifyCancel(projectDistrib.projectName, projectDistrib.projectUrl);
						return null;
					}
					
					outStream.write(buffer, 0, readed);
					
					long newTime = System.currentTimeMillis();
					if (newTime-time > NOTIFY_PERIOD) {
						notifyProgress(projectDistrib.projectName, projectDistrib.projectUrl,
								opDesc, (int)((double)totalReaded*10000.0/(double)totalLength));
						time = newTime;
					}
				}
				
				outStream.flush();
				/* reverts to project dir path */
				projectAppFilePath.delete(projectDirPathLength, projectAppFilePath.length());
				
				try {
					if (outStream != null)
						outStream.close();
				} catch(IOException ex2) { }
				
				outStream = null;
				i++;
			}
			
		} catch(InterruptedIOException ex) {
			projectAppFilePath.delete(projectDirPathLength, projectAppFilePath.length());
			notifyCancel(projectDistrib.projectName, projectDistrib.projectUrl);
			return null;
		} catch(IOException ex) {
			projectAppFilePath.delete(projectDirPathLength, projectAppFilePath.length());
			
			notifyError(projectDistrib.projectName, projectDistrib.projectUrl,
					mInstallerService.getString(R.string.unpackApplicationError));
			/* delete files */
			if (fileList != null) {
				projectAppFilePath.delete(projectDirPathLength, projectAppFilePath.length());
				
				for (String filename: fileList) {
					projectAppFilePath.append(filename);
					new File(projectAppFilePath.toString()).delete();
					projectAppFilePath.delete(projectDirPathLength, projectAppFilePath.length());
				}
			}
			return null;
		} finally {
			try {
				if (zipFile != null)
					zipFile.close();
			} catch(IOException ex2) { }
			try {
				if (inStream != null)
					inStream.close();
			} catch(IOException ex2) { }
			try {
				if (outStream != null)
					outStream.close();
			} catch(IOException ex2) { }
		}
		
		return finalOfPuttingProjectApplications(fileList, projectDistrib,
				projectAppFilePath, projectDirPathLength);
	}
	
	private ArrayList<String> finalOfPuttingProjectApplications(ArrayList<String> fileList,
			ProjectDistrib projectDistrib, StringBuilder projectAppFilePath, int projectDirPathLength) {
		Thread currentThread = Thread.currentThread();
		
		if (currentThread.isInterrupted()) {
			notifyCancel(projectDistrib.projectName, projectDistrib.projectUrl);
			return null;
		}

		FileInputStream inStream = null;
		/* change permissions */
		ArrayList<String> execFilenames = null;
		try {
			projectAppFilePath.append("app_info.xml");
			inStream = new FileInputStream(projectAppFilePath.toString());
			execFilenames = ExecFilesAppInfoParser.parse(inStream);
		} catch(IOException ex) {
		} finally {
			projectAppFilePath.delete(projectDirPathLength, projectAppFilePath.length());
			try {
				if (inStream != null)
					inStream.close();
			} catch(IOException ex) { }
		}
		
		if (currentThread.isInterrupted()) {
			notifyCancel(projectDistrib.projectName, projectDistrib.projectUrl);
			return null;
		}
		
		if (execFilenames != null) {
			for (String filename: execFilenames) {
				projectAppFilePath.append(filename);
				if (!Chmod.chmod(projectAppFilePath.toString(), 0700)) {
					notifyError(projectDistrib.projectName, projectDistrib.projectUrl,
							mInstallerService.getString(R.string.changePermissionsError));
			    	return null;
				}
				projectAppFilePath.delete(projectDirPathLength, projectAppFilePath.length());
			}
		}
		
		if (currentThread.isInterrupted()) {
			notifyCancel(projectDistrib.projectName, projectDistrib.projectUrl);
			return null;
		}
		
		return fileList;
	}
	
	private ArrayList<String> copyProjectApplications(ProjectDistrib projectDistrib, String sdCardPath,
			boolean directInstallation) {
		Thread currentThread = Thread.currentThread();
		
		if (currentThread.isInterrupted())
			return null;
		
		ArrayList<String> fileList = new ArrayList<String>();
		
		StringBuilder projectAppFilePath = new StringBuilder();
		projectAppFilePath.append(mInstallerService.getFilesDir());
		if (directInstallation)
			projectAppFilePath.append("/boinc/projects/");
		else
			projectAppFilePath.append("/boinc/updates/");
		
		projectAppFilePath.append(escapeProjectUrl(projectDistrib.projectUrl));
		projectAppFilePath.append("/");
		int projectDirPathLength = projectAppFilePath.length();
		
		File updateDir = new File(projectAppFilePath.toString());
		
		if (!updateDir.isDirectory())
			updateDir.mkdirs();
		
		File sdCardFile = new File(sdCardPath);
		File[] sdCardFileDirList = sdCardFile.listFiles();
		if (sdCardFileDirList == null) {
			// if error
			notifyError(projectDistrib.projectName, projectDistrib.projectUrl,
					mInstallerService.getString(R.string.copyApplicationError));
			return null;
		}
		
		for (File file: sdCardFileDirList) {
			FileInputStream inStream = null;
			FileOutputStream outStream = null;
			
			projectAppFilePath.append(file.getName());
			fileList.add(file.getName());
			
			try {
				inStream = new FileInputStream(file);
				outStream = new FileOutputStream(projectAppFilePath.toString());
				projectAppFilePath.delete(projectDirPathLength, projectAppFilePath.length());
				
				byte[] buffer = new byte[BUFFER_SIZE];
				
				while(true) {
					int readed = inStream.read(buffer);
					if (readed == -1)
						break;
					
					outStream.write(buffer, 0, readed);
				}
				outStream.flush();
			} catch (IOException ex) {
				projectAppFilePath.delete(projectDirPathLength, projectAppFilePath.length());
				
				notifyError(projectDistrib.projectName, projectDistrib.projectUrl,
						mInstallerService.getString(R.string.unpackApplicationError));
				
				projectAppFilePath.delete(projectDirPathLength, projectAppFilePath.length());
				/* delete files */
				if (fileList != null) {
					for (String filename: fileList) {
						projectAppFilePath.append(filename);
						new File(projectAppFilePath.toString()).delete();
						projectAppFilePath.delete(projectDirPathLength, projectAppFilePath.length());
					}
				}
				return null;
			} finally {
				try {
					if (inStream != null)
						inStream.close();
				} catch(IOException ex) { }
				try {
					if (outStream != null)
						outStream.close();
				} catch(IOException ex) { }
			}
		}
		
		return finalOfPuttingProjectApplications(fileList, projectDistrib,
				projectAppFilePath, projectDirPathLength);
	}
	
	private class ProjectAppsInstaller extends AbstractWorker {
		private String mInput;
		private ProjectDistrib mProjectDistrib;
		private ArrayList<String> mFileList;
		private boolean mFromSDCard = false;
		
		/* if client not installed and ran then acquired */ 
		private Semaphore mClientUpdatedAndRanSem = new Semaphore(1);
		
		/* should be released after benchmark */
		private Semaphore mBenchmarkFinishSem = new Semaphore(1);
		
		/**
		 * 
		 * @param input - path zip or directory or zipFileName
		 * @param projectDistrib
		 * @param fromSDCard
		 */
		public ProjectAppsInstaller(String input, ProjectDistrib projectDistrib,
				boolean fromSDCard) {
			mInput = input;
			mProjectDistrib = projectDistrib;
			mFromSDCard = fromSDCard;
			/* acquiring all semaphores */
			try {
				mClientUpdatedAndRanSem.acquire();
			} catch(InterruptedException ex) { }
			try {
				mBenchmarkFinishSem.acquire();
			} catch(InterruptedException ex) { }
		}
		
		public ProjectDistrib getProjectDistrib() {
			return mProjectDistrib;
		}
		
		@Override
		public void run() {
			boolean removeSelf = true;
			
			/* acquire resources (wifi, CPU) during installation */
			mResourcesLocker.acquireAllLocks();
			
			Thread currentThread = Thread.currentThread();
			try {
				if (Logging.DEBUG) Log.d(TAG, "Runned installer for:"+mProjectDistrib.projectUrl);
				
				mIsRan = true;
				
				if (currentThread.isInterrupted() || (mFuture != null && mFuture.isCancelled())) {
					notifyCancel(mProjectDistrib.projectName, mProjectDistrib.projectUrl);
					return;
				}
				
				/* awaiting for benchmark finish */
				notifyOperation(mProjectDistrib.projectName, mProjectDistrib.projectUrl,
						mInstallerService.getString(R.string.waitingForBenchmarkFinish));
				try {
					mBenchmarkFinishSem.acquire();
				} catch(InterruptedException ex) {
					if (Logging.DEBUG) Log.d(TAG,
							"Cancelled during waiting benchmark finish:"+mProjectDistrib.projectUrl);
					notifyCancel(mProjectDistrib.projectName, mProjectDistrib.projectUrl);
					return;
				} finally {
					mBenchmarkFinishSem.release();
				}
				
				if (Logging.DEBUG) Log.d(TAG,
						"after benchmark finish :"+mProjectDistrib.projectUrl);
				
				String zipUrl = mInstallerService.getString(R.string.installAppsSourceUrl) + mInput;
				/* do download and verify */
				String outZipFilename = null;
				if (!mFromSDCard) {
					outZipFilename = mProjectDistrib.projectName+".zip";
					try {
						mDownloader.downloadFile(zipUrl, outZipFilename,
								mInstallerService.getString(R.string.downloadApplication),
								mInstallerService.getString(R.string.downloadApplicationError), true,
								InstallerService.DEFAULT_CHANNEL_ID, InstallOp.ProgressOperation,
								mProjectDistrib.projectName, mProjectDistrib.projectUrl);
						
						if (currentThread.isInterrupted()) {
							mInstallerService.deleteFile(outZipFilename);
							notifyCancel(mProjectDistrib.projectName, mProjectDistrib.projectUrl);
							return;
						}
						
						int status = mDownloader.verifyFile(mInstallerService.getFileStreamPath(outZipFilename),
								zipUrl, true, InstallerService.DEFAULT_CHANNEL_ID,
								InstallOp.ProgressOperation, mProjectDistrib.projectName,
								mProjectDistrib.projectUrl);
						
						if (status == Downloader.VERIFICATION_CANCELLED) {
							mInstallerService.deleteFile(outZipFilename);
							notifyCancel(mProjectDistrib.projectName, mProjectDistrib.projectUrl);
							return;	// cancelled
						}
						if (status == Downloader.VERIFICATION_FAILED) {
							notifyError(mProjectDistrib.projectName, mProjectDistrib.projectUrl,
									mInstallerService.getString(R.string.verifySignatureFailed));
							return;	// cancelled
						}
					} catch(InstallationException ex) {
						mInstallerService.deleteFile(outZipFilename);
						return;
					}
				}
				
				if (currentThread.isInterrupted()) {
					if (outZipFilename != null)
						mInstallerService.deleteFile(outZipFilename);
					notifyCancel(mProjectDistrib.projectName, mProjectDistrib.projectUrl);
					return;
				}
				
				if (Logging.DEBUG) Log.d(TAG, "After unpacking");
				
				// determine whether direct installation or not
				boolean directInstallation = !mRunner.isRun() && !mClientShouldBeRun;
				
				/* do install in project directory */
				if (!mFromSDCard) {
					mFileList = unpackProjectApplications(mProjectDistrib,
							mInstallerService.getFileStreamPath(outZipFilename).getAbsolutePath(), directInstallation);
					
					if (outZipFilename != null)
						mInstallerService.deleteFile(outZipFilename);
				} else if (mInput.endsWith(".zip")) {
					// if zip from sdcard
					mFileList = unpackProjectApplications(mProjectDistrib, mInput, directInstallation);
					
					if (mFileList == null)
						return;
				
					if (outZipFilename != null)
						mInstallerService.deleteFile(outZipFilename);
				} else {
					// copy to update apps directory
					mFileList = copyProjectApplications(mProjectDistrib, mInput, directInstallation);
					
					if (mFileList == null)
						return;
				}
					
				if (currentThread.isInterrupted()) {
					notifyCancel(mProjectDistrib.projectName, mProjectDistrib.projectUrl);
					return;
				}
				
				if (!directInstallation) {
					if (Logging.DEBUG) Log.d(TAG, "On indirect project installation");
					
					notifyOperation(mProjectDistrib.projectName, mProjectDistrib.projectUrl,
							mInstallerService.getString(R.string.waitingForClientUpdateAndRun));
					
					try {
						mClientUpdatedAndRanSem.acquire();
					} catch(InterruptedException ex) {
						Log.d(TAG, "Cancelled during waiting for client update");
						notifyCancel(mProjectDistrib.projectName, mProjectDistrib.projectUrl);
						return;
					} finally {
						mClientUpdatedAndRanSem.release();
					}
					
					if (mRunner.isRun()) {
						// if really run
						notifyOperation(mProjectDistrib.projectName, mProjectDistrib.projectUrl,
								mInstallerService.getString(R.string.finalizeProjectAppInstall));
						
						/* update in client side */
						if (Logging.DEBUG) Log.d(TAG, "Run update_apps on boinc_client for "+
									mProjectDistrib.projectName);
						
						removeSelf = false; // do not self remove from list
						mRunner.updateProjectApps(mProjectDistrib.projectUrl);
					} else {
						// otherwise move to proper place
						StringBuilder projectAppFilePath = new StringBuilder();
						StringBuilder updateAppFilePath = new StringBuilder();
						
						if (Logging.DEBUG) Log.d(TAG, "During installation client run is failed!!!");
						
						String escapedUrl = escapeProjectUrl(mProjectDistrib.projectUrl);
						
						updateAppFilePath.append(mInstallerService.getFilesDir());
						updateAppFilePath.append("/boinc/updates/");
						updateAppFilePath.append(escapedUrl);
						updateAppFilePath.append("/");
						
						projectAppFilePath.append(mInstallerService.getFilesDir());
						projectAppFilePath.append("/boinc/projects/");
						projectAppFilePath.append(escapedUrl);
						projectAppFilePath.append("/");
						int projectDirPathLength = projectAppFilePath.length();
						
						if (Logging.DEBUG) Log.d(TAG, "Copy from "+updateAppFilePath+" to "+
									projectAppFilePath);
						
						File updateDirFile = new File(updateAppFilePath.toString());
						// rename files
						File[] filesToRename = updateDirFile.listFiles();
						if (filesToRename == null) {
							notifyError(mProjectDistrib.projectName, mProjectDistrib.projectUrl,
									mInstallerService.getString(R.string.unexpectedError));
							return;
						}
						
						for (File file: filesToRename) {
							projectAppFilePath.append(file.getName());
							file.renameTo(new File(projectAppFilePath.toString()));
							projectAppFilePath.delete(projectDirPathLength, projectAppFilePath.length());
						}
						removeSelf = false;
						updatedProjectApps(mProjectDistrib.projectUrl);
					}
				} else {
					// direct installation (finalize)
					removeSelf = false;
					updatedProjectApps(mProjectDistrib.projectUrl);
				}
			} catch(Exception ex) {
				if (Logging.ERROR) Log.e(TAG, "on project install finishing:"+
						ex.getClass().getCanonicalName()+":"+ex.getMessage());
				notifyError(mProjectDistrib.projectName, mProjectDistrib.projectUrl,
	    				mInstallerService.getString(R.string.unexpectedError)+": "+ex.getMessage());
			} finally {
				if (removeSelf) {
					/* release resources (wifi, CPU) during installation */
					mResourcesLocker.releaseAllLocks();
					
					mProjectAppsInstallers.remove(mProjectDistrib.projectUrl);
				}
				
				// notify change of is working
				notifyChangeOfIsWorking();
			}
		}
		
		public synchronized void notifyIfClientUpdatedAndRan() {
			mClientUpdatedAndRanSem.release();
		}
		
		public synchronized void notifyIfBenchmarkFinished() {
			mBenchmarkFinishSem.release();
		}
	}
	
	/* this map contains  app installer threads, key - projectUrl, value - app installer */
	private Map<String, ProjectAppsInstaller> mProjectAppsInstallers =
			new HashMap<String, ProjectAppsInstaller>();
	
	private synchronized void notifyIfClientUpdatedAndRan() {
		for (ProjectAppsInstaller appInstaller: mProjectAppsInstallers.values())
			appInstaller.notifyIfClientUpdatedAndRan();
	}
	
	private synchronized void notifyIfBenchmarkFinished() {
		for (ProjectAppsInstaller appInstaller: mProjectAppsInstallers.values())
			appInstaller.notifyIfBenchmarkFinished();
	}
	
	private synchronized boolean isProjectAppBeingInstalled(String projectUrl) {
		return mProjectAppsInstallers.containsKey(projectUrl);
	}
	
	private boolean mBenchmarkIsRun = false;
	
	private static final int MAX_PERIOD_BEFORE_BENCHMARK = 3000;
	
	public void installProjectApplicationsAutomatically(String projectUrl) {
		if (Logging.DEBUG) Log.d(TAG, "install App "+projectUrl);
		
		ProjectDistrib projectDistrib = null;
		
		String zipFilename = null;
		/* search project, and retrieve project url */
		for (ProjectDistrib distrib: mProjectDistribs)
			if (distrib.projectUrl.equals(projectUrl)) {
				zipFilename = distrib.filename;
				projectDistrib = distrib;
				break;
			}
		
		if (zipFilename == null) {
			notifyError(projectDistrib.projectName, projectUrl,
					mInstallerService.getString(R.string.projectAppNotFound));
			return;
		}
		
		if (isProjectAppBeingInstalled(projectUrl)) {	// do nothing if installed
			notifyFinish(projectDistrib.projectName, projectUrl);
			return;
		}
		
		// installBoincApplicationAutomatically(zipFilename, projectDistrib);
		// run in separate thread
		final ProjectAppsInstaller appInstaller = new ProjectAppsInstaller(zipFilename,
				projectDistrib, false);
		// unlock lock for client updating
		if (!mIsClientBeingInstalled)
			appInstaller.notifyIfClientUpdatedAndRan();
		
		SharedPreferences sharedPrefs = PreferenceManager
				.getDefaultSharedPreferences(mInstallerService);
		
		boolean awaitForBenchmark = sharedPrefs.getBoolean(PreferenceName.WAITING_FOR_BENCHMARK, false);
		if (Logging.DEBUG) Log.d(TAG, "Await for benchmark:"+awaitForBenchmark);
		
		if (awaitForBenchmark) {
			postDelayed(new Runnable() {
				@Override
				public void run() {
					if (!mBenchmarkIsRun) // if again benchmark is not ran
						// unlock benchmark lock
						appInstaller.notifyIfBenchmarkFinished();
				}
			}, MAX_PERIOD_BEFORE_BENCHMARK);
			
			// unset waiting for benchmark
			sharedPrefs.edit().putBoolean(PreferenceName.WAITING_FOR_BENCHMARK, false)
					.commit();
			
		} else // no wait
			appInstaller.notifyIfBenchmarkFinished();
		
		notifyOperation(projectDistrib.projectName, projectUrl,
				mInstallerService.getString(R.string.installProjectBegin));
		
		synchronized(this) {
			mProjectAppsInstallers.put(projectDistrib.projectUrl, appInstaller);
			notifyChangeOfIsWorking();
			Future<?> future = mExecutorService.submit(appInstaller);
			appInstaller.setFuture(future);
		}
	}
	
	
	public void reinstallUpdateItems(ArrayList<UpdateItem> updateItems) {
		if (updateItems.isEmpty())
			return;
		
		/* first, install client */
		for (UpdateItem updateItem: updateItems)
			if (updateItem.name.equals(InstallerService.BOINC_CLIENT_ITEM_NAME))
				/* update boinc client */
				if (!mIsClientBeingInstalled) {
					notifyOperation(InstallerService.BOINC_CLIENT_ITEM_NAME, "",
							mInstallerService.getString(R.string.installClientNotifyBegin));
					synchronized(this) {
						mIsClientBeingInstalled = true;
						notifyChangeOfIsWorking();
						mClientInstaller = new ClientInstaller(false, null);
						Future<?> future = mExecutorService.submit(mClientInstaller);
						mClientInstaller.setFuture(future);
					}
				}
		
		/* next install project applications */
		for (UpdateItem updateItem: updateItems) {
			if (!updateItem.name.equals(InstallerService.BOINC_CLIENT_ITEM_NAME)) {
				/* install */
				ProjectDistrib foundDistrib = null;
				for (ProjectDistrib distrib: mProjectDistribs)
					if (distrib.projectName.equals(updateItem.name)) {
						foundDistrib = distrib;
						break;
					}
				
				if (foundDistrib == null)
					continue;	// is not official distrib
				
				if (isProjectAppBeingInstalled(foundDistrib.projectUrl))
					continue;	// if being installed
				// run in separate thread
				notifyOperation(foundDistrib.projectName, foundDistrib.projectUrl,
						mInstallerService.getString(R.string.installProjectBegin));
				
				final ProjectAppsInstaller appInstaller =
						new ProjectAppsInstaller(updateItem.filename, foundDistrib, false);

				// no await for benchmark
				appInstaller.notifyIfBenchmarkFinished();
				
				synchronized(this) {
					mProjectAppsInstallers.put(foundDistrib.projectUrl, appInstaller);
					notifyChangeOfIsWorking();
					Future<?> future = mExecutorService.submit(appInstaller);
					appInstaller.setFuture(future);
				}
			}
		}
		
		// unlocks project apps installer if client not installed
		if (!mIsClientBeingInstalled) {
			if (Logging.DEBUG) Log.d(TAG, "if only project apps will being installed");
			notifyIfClientUpdatedAndRan();
		}
	}
	
	public void updateDistribsFromSDCard(String dirPath, String[] distribNames) {
		if (distribNames.length == 0)
			return;
		
		/* first, install client */
		for (String distribName: distribNames)
			if (distribName.equals(InstallerService.BOINC_CLIENT_ITEM_NAME)) {
				File clientFile = new File(dirPath+"boinc_client");
				
				if (!clientFile.exists()) // if zip
					clientFile = new File(dirPath+"boinc_client.zip");
				
				/* update boinc client */
				if (!mIsClientBeingInstalled) {
					notifyOperation(InstallerService.BOINC_CLIENT_ITEM_NAME, "",
							mInstallerService.getString(R.string.installClientNotifyBegin));
					synchronized(this) {
						mIsClientBeingInstalled = true;
						notifyChangeOfIsWorking();
						mClientInstaller = new ClientInstaller(false, clientFile.getAbsolutePath());
						Future<?> future = mExecutorService.submit(mClientInstaller);
						mClientInstaller.setFuture(future);
					}
				}
			}
		
		if (mProjectDescs == null)
			mProjectDescs = mProjectsRetriever.getProjectDescriptors();
		
		/* next install project applications */
		for (String distribName: distribNames) {
			if (!distribName.equals(InstallerService.BOINC_CLIENT_ITEM_NAME)) {
				/* install */
				ProjectDistrib foundDistrib = null;
				for (ProjectDistrib distrib: mProjectDistribs)
					if (distrib.projectName.equals(distribName)) {
						foundDistrib = distrib;
						break;
					}
				
				// if not official
				if (foundDistrib == null) {
					if (mProjectDescs == null)
						continue;
					
					for (ProjectDescriptor desc: mProjectDescs)
						// handling desc.projectName is null
						if (distribName.equals(desc.projectName)) {
							foundDistrib = new ProjectDistrib();
							foundDistrib.projectName = desc.projectName;
							foundDistrib.projectUrl = desc.masterUrl;
							break;
						}
				}
				
				if (foundDistrib == null)
					continue;
				
				if (isProjectAppBeingInstalled(foundDistrib.projectUrl))
					continue;	// if being installed
				
				File distribFile = new File(dirPath+distribName);
				
				if (!distribFile.exists()) // if zip
					distribFile = new File(dirPath+distribName+".zip");
				
				// run in separate thread
				notifyOperation(foundDistrib.projectName, foundDistrib.projectUrl,
						mInstallerService.getString(R.string.installProjectBegin));
				
				final ProjectAppsInstaller appInstaller =
						new ProjectAppsInstaller(distribFile.getAbsolutePath(), foundDistrib, true);

				// no await for benchmark
				appInstaller.notifyIfBenchmarkFinished();
				
				synchronized(this) {
					mProjectAppsInstallers.put(foundDistrib.projectUrl, appInstaller);
					notifyChangeOfIsWorking();
					Future<?> future = mExecutorService.submit(appInstaller);
					appInstaller.setFuture(future);
				}
			}
		}
		
		// unlocks project apps installer if client not installed
		if (!mIsClientBeingInstalled) {
			if (Logging.DEBUG) Log.d(TAG, "if only project apps will being installed");
			notifyIfClientUpdatedAndRan();
		}
	}
	
	/**
	 * @return true if success
	 */
	public boolean updateProjectDistribList(int channelId, boolean excludeAttachedProjects) {
		synchronized(this) {
			mCurrentChannelId = channelId;
			mHandlerIsWorking = true;
			notifyChangeOfIsWorking();
		}
		try {
			ArrayList<ProjectDistrib> projectDistribs = mInstallOps.updateProjectDistribList(channelId,
					InstallOp.UpdateProjectDistribs, mProjectDistribs);
			
			if (projectDistribs != null) {
				mProjectDistribs = projectDistribs;
				
				if (excludeAttachedProjects) {
					mProjectDescs = mProjectsRetriever.getProjectDescriptors();
					if (mProjectDescs != null) {
						HashSet<String> projectUrls = new HashSet<String>();
						
						for (ProjectDescriptor projDesc: mProjectDescs)
							projectUrls.add(projDesc.masterUrl);
						
						ArrayList<ProjectDistrib> filteredProjectDistribs =
								new ArrayList<ProjectDistrib>();
						
						// filtering project distribs if required
						for (ProjectDistrib distrib: projectDistribs)
							if (!projectUrls.contains(distrib.projectUrl))
								filteredProjectDistribs.add(distrib);
						
						notifyProjectDistribs(channelId, InstallOp.UpdateProjectDistribs,
								filteredProjectDistribs);
					} else
						notifyProjectDistribs(channelId, InstallOp.UpdateProjectDistribs,
								new ArrayList<ProjectDistrib>(mProjectDistribs));
				} else
					notifyProjectDistribs(channelId, InstallOp.UpdateProjectDistribs,
							new ArrayList<ProjectDistrib>(mProjectDistribs));
			}
		} finally {
			// hanbdler doesnt working currently
			synchronized(this) {
				mCurrentChannelId = -1; // unset channel id
				mHandlerIsWorking = false;
				notifyChangeOfIsWorking();
			}
		}
		return true;
	}
	
	/* remove detached projects from installed projects */
	public void synchronizeInstalledProjects() {
		mDistribManager.synchronizeWithProjectList(mInstallerService);
	}
	
	public synchronized void cancelAllProgressOperations() {
		cancelClientInstallation();
		cancelReinstallation();
		cancelDumpFiles();
		for (ProjectAppsInstaller appInstaller: mProjectAppsInstallers.values()) {
			Future<?> future = appInstaller.getFuture();
			if (future != null)
				future.cancel(true);
			
			if (!appInstaller.isRan()) { // notify cancel if not ran yet
				ProjectDistrib distrib = appInstaller.getProjectDistrib();
				notifyCancel(distrib.projectName, distrib.projectUrl);
			}
		}
		mProjectAppsInstallers.clear();
	}
	
	public synchronized void cancelClientInstallation() {
		if (mClientInstaller != null) {
			Future<?> future = mClientInstaller.getFuture();
			if (future != null)
				future.cancel(true);
			
			if (mClientInstaller != null && !mClientInstaller.isRan()) // notify cancel if not ran yet
				notifyCancel(InstallerService.BOINC_CLIENT_ITEM_NAME, "");
		}
		mClientInstaller = null;
	}
	
	public synchronized void cancelDumpFiles() {
		if (mBoincFilesDumper != null) {
			Future<?> future = mBoincFilesDumper.getFuture();
			if (future != null)
				future.cancel(true);
			
			if (mBoincFilesDumper != null && !mBoincFilesDumper.isRan()) // notify cancel if not ran yet
				notifyCancel(InstallerService.BOINC_DUMP_ITEM_NAME, ""); 
		}
		mBoincFilesDumper = null;
	}
	
	public synchronized void cancelReinstallation() {
		if (mBoincReinstaller != null) {
			Future<?> future = mBoincReinstaller.getFuture();
			if (future != null)
				future.cancel(true);
			
			if (mBoincReinstaller != null && !mBoincReinstaller.isRan()) // notify cancel if not ran yet
				notifyCancel(InstallerService.BOINC_REINSTALL_ITEM_NAME, ""); 
		}
		mBoincReinstaller = null;
	}
	
	public synchronized void cancelSimpleOperation(int channelId, Thread workingThread) {
		if (mCurrentChannelId != channelId)
			return; // do nothing if not same channel
		if (isWorking()) {
			workingThread.interrupt();
		}
	}
	
	public synchronized void cancelSimpleOperationAlways(Thread workingThread) {
		if (isWorking()) {
			workingThread.interrupt();
		}
	}
	
	public synchronized boolean isWorking() {
		return  mIsClientBeingInstalled || !mProjectAppsInstallers.isEmpty() ||
				mHandlerIsWorking || mDoDumpBoincFiles || mDoBoincReinstall;
	}
	
	public synchronized void cancelProjectAppsInstallation(String projectUrl) {
		ProjectAppsInstaller appsInstaller = mProjectAppsInstallers.remove(projectUrl);
		if (appsInstaller != null) {
			Future<?> future = appsInstaller.getFuture();
			if (future != null)
				future.cancel(true);
			
			if (!appsInstaller.isRan()) { // notify cancel if not ran yet
				ProjectDistrib distrib = appsInstaller.getProjectDistrib();
				notifyCancel(distrib.projectName, distrib.projectUrl);
			}
		}
	}
	
	public InstalledBinary[] getInstalledBinaries() {
		return mInstallOps.getInstalledBinaries();
	}
	
	/**
	 * get list of binaries to update or install (from project url)
	 * @return update list (sorted)
	 */
	public void getBinariesToUpdateOrInstall(int channelId) {
		synchronized(this) {
			mCurrentChannelId = channelId;
			mHandlerIsWorking = true;
			notifyChangeOfIsWorking();
		}
		synchronizeInstalledProjects();
		
		mProjectDescs = mProjectsRetriever.getProjectDescriptors();
		
		String[] attachedProjectUrls = new String[0];
		if (mProjectDescs != null) {
			attachedProjectUrls = new String[mProjectDescs.length];
			for (int i = 0; i < mProjectDescs.length; i++)
				attachedProjectUrls[i] = mProjectDescs[i].masterUrl;
		}
		
		try {
			/* update client and project list */
			ClientDistrib clientDistrib = mInstallOps.updateClientDistrib(channelId,
					InstallOp.GetBinariesToInstall);
			if (clientDistrib != null)
				mClientDistrib = clientDistrib;
			else // fail
				return;
			
			ArrayList<ProjectDistrib> projectDistribs = mInstallOps.updateProjectDistribList(channelId,
							InstallOp.GetBinariesToInstall, mProjectDistribs);
			if (projectDistribs != null)
				mProjectDistribs = projectDistribs;
			else // fail
				return;
			
			/* client binary */
			UpdateItem[] output = mInstallOps.getBinariesToUpdateOrInstall(mClientDistrib, mProjectDistribs,
					attachedProjectUrls);
			
			// notify final result
			if (output != null)
				notifyBinariesToInstallOrUpdate(channelId, InstallOp.GetBinariesToInstall, output);
		} catch(Exception ex) {
			notifyError("", "", mInstallerService.getString(R.string.unexpectedError));
		} finally {
			synchronized(this) {
				mCurrentChannelId = -1;
				mHandlerIsWorking = false;
				notifyChangeOfIsWorking();
			}
		}
	}
	
	/**
	 * get list of distrib to update from sdcard
	 */
	public void getBinariesToUpdateFromSDCard(int channelId, String path) {
		synchronized (this) {
			mCurrentChannelId = channelId;
			mHandlerIsWorking = true;
			notifyChangeOfIsWorking();
		}
		
		InstallOp installOp = InstallOp.GetBinariesFromSDCard(path);
		
		try {
			// mProjectDescs used by updateDistribsFromSDCard to retrieve projectUrl
			mProjectDescs = mProjectsRetriever.getProjectDescriptors();
			if (mProjectDescs == null) { // returns empty list
				notifyError("", "", mInstallerService.getString(R.string.getProjectsFromClientError));
				return;
			}
			
			String[] output = mInstallOps.getBinariesToUpdateFromSDCard(channelId, installOp,
					path, mProjectDescs);
			if (output != null)
				notifyBinariesToUpdateFromSDCard(channelId,
						InstallOp.GetBinariesFromSDCard(path), output);
		} finally {
			synchronized(this) {
				mCurrentChannelId = -1;
				mHandlerIsWorking = false;
				notifyChangeOfIsWorking();
			}
		}
	}
	
	/**
	 * dump boinc files (installation) to directory
	 */
	public class BoincFilesDumper extends AbstractWorker {

		private String mDirectory = null;
		
		public BoincFilesDumper(String directory) {
			mDirectory = directory;
		}
		
		@Override
		public void run() {
			mIsRan = true;
			synchronized(InstallerHandler.this) {
				mDoDumpBoincFiles = true;
				notifyChangeOfIsWorking();
			}
			
			try {
				mInstallOps.dumpBoincFiles(mDirectory);
			} catch(Exception ex) {
				notifyError(InstallerService.BOINC_DUMP_ITEM_NAME, "", 
						mInstallerService.getString(R.string.unexpectedError));
			} finally {
				synchronized(InstallerHandler.this) {
					mBoincFilesDumper = null;
					mDoDumpBoincFiles = false;
					notifyChangeOfIsWorking();
				}
			}
		}
	}
	
	private BoincFilesDumper mBoincFilesDumper = null;
	
	public void dumpBoincFiles(String directory) {
		if (mDoDumpBoincFiles)
			return;
		
		notifyOperation(InstallerService.BOINC_DUMP_ITEM_NAME, "",
				mInstallerService.getString(R.string.dumpBoincBegin));
		synchronized(this) {
			mBoincFilesDumper = new BoincFilesDumper(directory);
			// notify that boinc dumper is working
			notifyChangeOfIsWorking();
			Future<?> future = mExecutorService.submit(mBoincFilesDumper);
			mBoincFilesDumper.setFuture(future);
		}
	}
	
	public boolean isBeingDumpedFiles() {
		return mDoDumpBoincFiles;
	}
	
	/**
	 * reinstall boinc (delete directory, and make first start)
	 */
	public class BoincReinstaller extends AbstractWorker {

		@Override
		public void run() {
			mIsRan = true;
			synchronized(InstallerHandler.this) {
				mDoBoincReinstall = true;
				notifyChangeOfIsWorking();
			}
			
			try {
				mInstallOps.reinstallBoinc();
			} catch(Exception ex) {
				notifyError(InstallerService.BOINC_REINSTALL_ITEM_NAME, "", 
						mInstallerService.getString(R.string.unexpectedError));
			} finally {
				synchronized(InstallerHandler.this) {
					mBoincReinstaller = null;
					mDoBoincReinstall = false;
					notifyChangeOfIsWorking();
				}
			}
		}
	}
	
	private BoincReinstaller mBoincReinstaller = null;
	
	public void reinstallBoinc() {
		if (mDoBoincReinstall)
			return;
		
		notifyOperation(InstallerService.BOINC_REINSTALL_ITEM_NAME, "",
				mInstallerService.getString(R.string.reinstallBegin));
		synchronized(this) {
			mDoBoincReinstall = true;
			mBoincReinstaller = new BoincReinstaller();
			// notify that client reinstaller is working
			notifyChangeOfIsWorking();
			Future<?> future = mExecutorService.submit(mBoincReinstaller);
			mBoincReinstaller.setFuture(future);
		}
	}
	
	public boolean isBeingReinstalled() {
		return mDoBoincReinstall;
	}
	
	/*
	 *
	 */
	public synchronized void notifyOperation(final String distribName, final String projectUrl,
			final String opDesc) {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.onOperation(distribName, projectUrl, opDesc);
			}
		});
	}
	
	public synchronized void notifyProgress(final String distribName, final String projectUrl,
			final String opDesc, final int progress) {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.onOperationProgress(distribName, projectUrl, opDesc, progress);
			}
		});
	}
	
	public synchronized void notifyError(final String distribName, final String projectUrl,
			final String errorMessage) {
		notifyError(InstallerService.DEFAULT_CHANNEL_ID, InstallOp.ProgressOperation, distribName, projectUrl,
				errorMessage);
	}
	
	public synchronized void notifyError(final int channelId, final InstallOp installOp,
			final String distribName, final String projectUrl, final String errorMessage) {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.onOperationError(channelId, installOp, distribName, projectUrl, errorMessage);
			}
		});
	}
	
	public void notifyCancel(final String distribName, final String projectUrl) {
		notifyCancel(InstallerService.DEFAULT_CHANNEL_ID, InstallOp.ProgressOperation,
				distribName, projectUrl);
	}
	
	public synchronized void notifyCancel(final int channelId, final InstallOp installOp,
			final String distribName, final String projectUrl) {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.onOperationCancel(channelId, installOp, distribName, projectUrl);
			}
		});
	}
	
	public void notifyFinish(final String distribName, final String projectUrl) {
		notifyFinish(InstallOp.ProgressOperation, distribName, projectUrl);
	}
	
	public synchronized void notifyFinish(final InstallOp installOp,
			final String distribName, final String projectUrl) {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.onOperationFinish(installOp, distribName, projectUrl);
			}
		});
	}
	
	public synchronized void notifyProjectDistribs(final int channelId, final InstallOp installOp,
			final ArrayList<ProjectDistrib> projectDistribs) {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.currentProjectDistribList(channelId, installOp, projectDistribs);
			}
		});
	}
	
	public synchronized void notifyClientDistrib(final int channelId, final InstallOp installOp,
			final ClientDistrib clientDistrib) {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.currentClientDistrib(channelId, installOp, clientDistrib);
			}
		});
	}
	
	public synchronized void notifyBinariesToInstallOrUpdate(final int channelId,
			final InstallOp installOp, final UpdateItem[] updateItems) {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.notifyBinariesToInstallOrUpdate(channelId, installOp, updateItems);
			}
		});
	}
	
	public synchronized void notifyBinariesToUpdateFromSDCard(final int channelId,
			final InstallOp installOp, final String[] projectNames) {
		mListenerHandler.post(new Runnable() {
			@Override
			public void run() {
				mListenerHandler.notifyBinariesToUpdateFromSDCard(channelId, installOp, projectNames);
			}
		});
	}
	
	public synchronized void notifyChangeOfIsWorking() {
		final boolean currentIsWorking = isWorking();
		if (mPreviousStateOfIsWorking != currentIsWorking) {
			mPreviousStateOfIsWorking = currentIsWorking;
			mListenerHandler.post(new Runnable() {
				@Override
				public void run() {
					mListenerHandler.onChangeIsWorking(currentIsWorking);
				}
			});
		}
		if (!currentIsWorking && mInstallerService.doStopWhenNotWorking()) {
			// stop service
			Log.d(TAG, "Stop when not working");
			mInstallerService.stopSelf();
		}
	}

	@Override
	public boolean onNativeBoincClientError(String message) {
		return false;
	}

	@Override
	public synchronized void updatedProjectApps(String projectUrl) {
		ProjectAppsInstaller appsInstaller = mProjectAppsInstallers.remove(projectUrl);
		if (Logging.DEBUG) Log.d(TAG, "After update on client side: "+
				appsInstaller.mProjectDistrib.projectName);
		
		if (!appsInstaller.mFromSDCard)
			mDistribManager.addOrUpdateDistrib(appsInstaller.mProjectDistrib, appsInstaller.mFileList);
		else // if from sdcard
			mDistribManager.addOrUpdateDistribFromSDCard(appsInstaller.mProjectDistrib.projectName,
					appsInstaller.mProjectDistrib.projectUrl, appsInstaller.mFileList);
		
		notifyFinish(appsInstaller.mProjectDistrib.projectName, projectUrl);
		
		/* release resources (wifi, CPU) during installation */
		mResourcesLocker.releaseAllLocks();
		
		notifyChangeOfIsWorking();
	}
	
	@Override
	public synchronized void updateProjectAppsError(String projectUrl) {
		ProjectAppsInstaller appsInstaller = mProjectAppsInstallers.remove(projectUrl);
		if (Logging.DEBUG) Log.d(TAG, "After update on client side error: "+
				appsInstaller.mProjectDistrib.projectName);
		
		notifyError(appsInstaller.mProjectDistrib.projectName, projectUrl,
				mInstallerService.getString(R.string.updateProjectAppError));
		
		/* release resources (wifi, CPU) during installation */
		mResourcesLocker.releaseAllLocks();
		
		notifyChangeOfIsWorking();
	}

	@Override
	public void onMonitorEvent(final ClientEvent event) {
		switch(event.type) {
		case ClientEvent.EVENT_ATTACHED_PROJECT:
			// do nothing
			break;
		case ClientEvent.EVENT_DETACHED_PROJECT:
			// synchronize with installed projects
			post(new Runnable() {
				@Override
				public void run() {
					synchronizeInstalledProjects();
				}
			});
			break;
		case ClientEvent.EVENT_RUN_BENCHMARK:
			mBenchmarkIsRun = true;
			break;
		case ClientEvent.EVENT_FINISH_BENCHMARK:
			mBenchmarkIsRun = false;
			// unlock
			notifyIfBenchmarkFinished();
			break;
		}
	}

	@Override
	public void onClientStart() {
		if (mIsClientBeingInstalled) {
			mDelayedClientShutdownSem.tryAcquire();
		} else
			notifyIfClientUpdatedAndRan();
	}
	
	@Override
	public void onClientStop(int exitCode, boolean stoppedByManager) {
		if (mIsClientBeingInstalled)
			mDelayedClientShutdownSem.release();
		if (mDoBoincReinstall)
			mInstallOps.notifyShutdownClient();
	}

	@Override
	public boolean onNativeBoincServiceError(WorkerOp workerOp, String message) {
		// also notify
		notifyIfClientUpdatedAndRan();
		return false; // do not consume error
	}

	@Override
	public void onChangeRunnerIsWorking(boolean isWorking) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void onMonitorDoesntWork() {
		
	}
}
