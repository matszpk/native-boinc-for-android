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

import java.io.BufferedInputStream;
import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.InterruptedIOException;
import java.io.Reader;
import java.net.URI;
import java.net.URL;
import java.net.URLConnection;

import org.apache.http.HttpEntity;
import org.apache.http.HttpResponse;
import org.apache.http.HttpStatus;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.client.utils.URIUtils;
import org.apache.http.impl.client.DefaultHttpClient;
import org.apache.http.params.BasicHttpParams;
import org.apache.http.params.HttpConnectionParams;
import org.bouncycastle.openpgp.PGPObjectFactory;
import org.bouncycastle.openpgp.PGPPublicKey;
import org.bouncycastle.openpgp.PGPPublicKeyRingCollection;
import org.bouncycastle.openpgp.PGPSignature;
import org.bouncycastle.openpgp.PGPSignatureList;
import org.bouncycastle.openpgp.PGPUtil;

import sk.boinc.nativeboinc.R;
import sk.boinc.nativeboinc.debug.Logging;
import android.content.Context;
import android.util.Log;

/**
 * @author mat
 *
 */
public class Downloader {

	private final static String TAG = "InstallDownloader";
	
	private byte[] mPgpKeyContent = null;
	
	private Context mContext = null;
	private InstallerHandler mInstallerHandler = null;
	
	private static final int BUFFER_SIZE = 4096;
	
	private static final int NOTIFY_PERIOD = 400;
	
	public Downloader(Context context, InstallerHandler installerHandler) {
		mContext = context;
		mInstallerHandler = installerHandler;
	}
	
	public void destroy() {
		mInstallerHandler = null;
		mContext = null;
		mPgpKeyContent = null;
	}
	
	public byte[] getPgpKeyContent() {
		return mPgpKeyContent;
	}
	
	public void downloadPGPKey(int channelId, InstallOp installOp, String distribName, String projectUrl)
			throws InstallationException {
		
		Thread currentThread = Thread.currentThread();
		
		if (channelId == InstallerService.DEFAULT_CHANNEL_ID)
			mInstallerHandler.notifyOperation(distribName, projectUrl,
					mContext.getString(R.string.downloadPGPKey));
		
		BasicHttpParams params = new BasicHttpParams();
		HttpConnectionParams.setConnectionTimeout(params, 10000);
		HttpConnectionParams.setSoTimeout(params, 10000);
		
		DefaultHttpClient client = new DefaultHttpClient(params);
		
		Reader reader = null;
		FileOutputStream pgpStream = null;
		
		String[] pgpKeyservers = mContext.getResources().getStringArray(R.array.PGPkeyservers); 
		
		boolean isDownloaded = false;
		
		for (String keyserver: pgpKeyservers) {
			if (Logging.DEBUG) Log.d(TAG, "Use PGP keyserver " + keyserver);
			
			if (currentThread.isInterrupted())
				return;
			
			try {
				URI uri = URIUtils.createURI("http", keyserver, 11371, "/pks/lookup",
						"op=get&search=0x"+mContext.getString(R.string.PGPKey), null);
				HttpResponse response = client.execute(new HttpGet(uri));
				
				StringBuilder keyBlock = new StringBuilder();
				
				if (response.getStatusLine().getStatusCode() == HttpStatus.SC_OK) {
					HttpEntity entity = response.getEntity();
					reader = new InputStreamReader(entity.getContent());
					
					char[] buffer = new char[BUFFER_SIZE];
					while (true) {
						int readed = reader.read(buffer);
						if (readed == -1)	// if end
							break;
						keyBlock.append(buffer, 0, readed);
					}
				} else
					if (Logging.WARNING) Log.w(TAG, "Response: " + response.getStatusLine() +
							" from " + keyserver);
				
				int keyStart = keyBlock.indexOf("-----BEGIN PGP PUBLIC KEY BLOCK-----");
				int keyEnd = keyBlock.indexOf("-----END PGP PUBLIC KEY BLOCK-----");
				
				if (keyStart == -1 || keyEnd == -1) {
					if (Logging.WARNING) Log.w(TAG, "Bad keyBlock: from " + keyserver + ", keyblock: " +
							keyBlock);
					throw new Exception("Error");
				}
	
				pgpStream = mContext.openFileOutput("pgpkey.pgp", Context.MODE_PRIVATE);
				
				mPgpKeyContent = keyBlock.substring(keyStart, keyEnd+35).getBytes();
				pgpStream.write(mPgpKeyContent);
				
				pgpStream.flush();
				pgpStream.close();
				
				isDownloaded = true;
				break;
			} catch(InterruptedIOException ex) {
				// (if time out)
				mContext.deleteFile("pgpkey.pgp");
				continue;
			} catch(Exception ex) {	/* on error */
				mContext.deleteFile("pgpkey.pgp");
				if (Logging.WARNING) Log.w(TAG, "Exception: "+ ex.getMessage() + " for " + keyserver);
			} finally {
				try {
					if (reader != null)
						reader.close();
				} catch(IOException ex) { }
				try {
					if (pgpStream != null)
						pgpStream.close();
				} catch(IOException ex) { }
			}
		}
		
		if (!isDownloaded) {
			mInstallerHandler.notifyError(channelId , installOp, distribName, projectUrl,
					mContext.getString(R.string.downloadPGPKeyError));
			throw new InstallationException();
		}
	}
	
	public static final int VERIFIED_SUCCESFULLY = 1;
	public static final int VERIFICATION_FAILED = 2;
	public static final int VERIFICATION_CANCELLED = 4;
	
	public int verifyFile(File file, String urlString, boolean withProgress, int channelId,
			InstallOp installOp, final String distribName, final String projectUrl)
					throws InstallationException {
		if (Logging.DEBUG) Log.d(TAG, "verifying file "+urlString);
		FileInputStream pgpStream = null;
		
		Thread currentThread = Thread.currentThread();
		
		if (currentThread.isInterrupted())	// if cancelled
			return VERIFICATION_CANCELLED;
		
		try {
			synchronized(this) {
				// should be synchronized, because can be work in multiple threads
				if (mPgpKeyContent == null) {
					if (mContext.getFileStreamPath("pgpkey.pgp").exists())
						pgpStream = mContext.openFileInput("pgpkey.pgp");
					else	// download from keyserver
						downloadPGPKey(channelId, installOp, distribName, projectUrl);
				}
				
				if (mPgpKeyContent == null) {
					if (pgpStream == null) // cancelled
						return VERIFICATION_CANCELLED;
					
					byte[] content = new byte[4096];
					int readed = pgpStream.read(content);
					if (readed >= 4096) {
						throw new IOException("File too big");
					}
					
					mPgpKeyContent = new byte[readed];
					System.arraycopy(content, 0, mPgpKeyContent, 0, readed);
				}
			}
		} catch(InterruptedIOException ex) {
			return VERIFICATION_CANCELLED;
		} catch(IOException ex) {
			mInstallerHandler.notifyError(channelId, installOp, distribName, projectUrl,
					mContext.getString(R.string.loadPGPKeyError));
			throw new InstallationException();
		} finally {
			try {
				if (pgpStream != null)
					pgpStream.close();
			} catch(IOException ex) { }
		}
		
		if (currentThread.isInterrupted())
			return VERIFICATION_CANCELLED;
		
		byte[] signContent = null;
		InputStream signatureStream = null;
		/* download file signature */
		try {
			URL url = new URL(urlString+".asc");
			URLConnection conn = url.openConnection();
			signatureStream = conn.getInputStream();
			
			byte[] content = new byte[4096];
			int readed = signatureStream.read(content);
			if (readed >= 4096) {
				throw new IOException("File too big");
			}
			
			signContent = new byte[readed];
			System.arraycopy(content, 0, signContent, 0, readed);
		} catch(InterruptedIOException ex) {
			return VERIFICATION_CANCELLED;
		} catch(IOException ex) {
			mInstallerHandler.notifyError(channelId, installOp, distribName, projectUrl,
					mContext.getString(R.string.downloadSignatureError));
			throw new InstallationException();
		} finally {
			try {
				if (signatureStream != null)
					signatureStream.close();
			} catch(IOException ex) { }
		}
		
		if (currentThread.isInterrupted())
			return VERIFICATION_CANCELLED;
		
		String opDesc = mContext.getString(R.string.verifySignature);
		
		if (channelId == InstallerService.DEFAULT_CHANNEL_ID)
			mInstallerHandler.notifyOperation(distribName, projectUrl, opDesc);
		
		/* verify file signature */
		InputStream bIStream = new ByteArrayInputStream(signContent);
		InputStream contentStream = null;
		
		try {
			bIStream = PGPUtil.getDecoderStream(bIStream);
			
			PGPObjectFactory pgpFact = new PGPObjectFactory(bIStream);
	        
	        PGPSignatureList pgpSignList = (PGPSignatureList)pgpFact.nextObject();
	        
	        InputStream keyInStream = new ByteArrayInputStream(mPgpKeyContent);
	        
	        PGPPublicKeyRingCollection pgpPubRingCollection = 
	        	new PGPPublicKeyRingCollection(PGPUtil.getDecoderStream(keyInStream));
	        
	        contentStream = new BufferedInputStream(new FileInputStream(file));

	        PGPSignature signature = pgpSignList.get(0);
	        PGPPublicKey key = pgpPubRingCollection.getPublicKey(signature.getKeyID());
	        
	        signature.initVerify(key, "BC");

	        long length = file.length();
	        
	        int ch;
	        long readed = 0;
	        long time = System.currentTimeMillis();
	        
	        while ((ch = contentStream.read()) >= 0) {
	            signature.update((byte)ch);
	            readed++;
	            
	            if(currentThread.isInterrupted())	// do cancel
	            	return VERIFICATION_CANCELLED;
	            
	            if (withProgress && (readed & 8191) == 0) {
	            	long newTime = System.currentTimeMillis(); 
	            	if (newTime-time > NOTIFY_PERIOD) {
	            		if (channelId == InstallerService.DEFAULT_CHANNEL_ID)
	            			mInstallerHandler.notifyProgress(distribName, projectUrl, opDesc,
	            					(int)((double)readed*10000.0/(double)length));
	            		time = newTime;
	            	}
	            	/*try {
						Thread.sleep(40);
					} catch(InterruptedException ex) {
						return VERIFICATION_CANCELLED;
					}*/
	            }
	        }
	        
	        if (withProgress && channelId == InstallerService.DEFAULT_CHANNEL_ID)
	        	mInstallerHandler.notifyProgress(distribName, projectUrl, opDesc,
	        			InstallerProgressListener.FINISH_PROGRESS);
	        
	        if (signature.verify())
	        	return VERIFIED_SUCCESFULLY;
	        else
	        	return VERIFICATION_FAILED;
		} catch(InterruptedIOException ex) {
			if (Logging.DEBUG) Log.d(TAG, "verif cancelled");
			return VERIFICATION_CANCELLED;
		} catch (Exception ex) {
			if (Logging.DEBUG) Log.d(TAG, "verif failed:"+ex.getMessage());
			mInstallerHandler.notifyError(channelId, installOp, distribName, projectUrl,
					mContext.getString(R.string.verifySignatureError));
			throw new InstallationException();
		} finally {
			try {
				if (contentStream != null)
					contentStream.close();
			} catch(IOException ex) { }
		}
	}
	
	public void downloadFile(String urlString, String outFilename, final String opDesc,
			final String messageError, final boolean withProgress, final int channelId,
			final InstallOp installOp, final String distribName, final String projectUrl)
					throws InstallationException {
		
		Thread currentThread = Thread.currentThread();
		
		if (Logging.DEBUG) Log.d(TAG, "downloading file "+urlString);
		
		InputStream inStream = null;
		FileOutputStream outStream = null;
		
		try {
			URL url = new URL(urlString);
			
			if (channelId == InstallerService.DEFAULT_CHANNEL_ID)
				mInstallerHandler.notifyOperation(distribName, projectUrl, opDesc);
			
			if (!url.getProtocol().equals("ftp")) {
				/* if http protocol */
				URLConnection urlConn = url.openConnection();
				
				urlConn.setConnectTimeout(12000);
				urlConn.setReadTimeout(12000);
				
				inStream = urlConn.getInputStream();
				outStream = new FileOutputStream(mContext.getFileStreamPath(outFilename));
				
				byte[] buffer = new byte[BUFFER_SIZE];
				int length = urlConn.getContentLength();
				int totalReaded = 0;
				long currentTime = System.currentTimeMillis();
				
				while(true) {
					int readed = inStream.read(buffer);
					if (readed == -1)
						break;
					
					if (currentThread.isInterrupted())
						break;	// if canceled
					
					outStream.write(buffer, 0, readed);
					totalReaded += readed;
					
					if (length != -1 && withProgress) {
						long newTime = System.currentTimeMillis();
						if (newTime-currentTime > NOTIFY_PERIOD) {
							if (channelId == InstallerService.DEFAULT_CHANNEL_ID)
								mInstallerHandler.notifyProgress(distribName, projectUrl, opDesc,
										(int)((double)totalReaded*10000.0/(double)length));
							currentTime = newTime;
						}
					}
					/*try {
						Thread.sleep(60);
					} catch(InterruptedException ex) {
						currentThread.interrupt();
						return;
					}*/
				}
				
				outStream.flush();
				
				if (withProgress && channelId == InstallerService.DEFAULT_CHANNEL_ID)
					mInstallerHandler.notifyProgress(distribName, projectUrl, opDesc, 10000);
			} else {
				throw new UnsupportedOperationException("Unsupported operation");
			}
		} catch(InterruptedIOException ex) {
			return; // cancelled
		} catch(IOException ex) {
			mContext.deleteFile(outFilename);
			mInstallerHandler.notifyError(channelId, installOp, distribName, projectUrl, messageError);
			throw new InstallationException();
		} finally {
			try {
				if (inStream != null)
					inStream.close();
			} catch (IOException ex) { }
			try {
				if (outStream != null)
					outStream.close();
			} catch (IOException ex) { }
		}
	}
}
