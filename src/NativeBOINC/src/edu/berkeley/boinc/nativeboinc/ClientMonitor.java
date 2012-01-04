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

package edu.berkeley.boinc.nativeboinc;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStreamWriter;
import java.io.Writer;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.UnknownHostException;

import android.util.Log;

import sk.boinc.nativeboinc.debug.Logging;

/**
 * @author mat
 *
 */
public class ClientMonitor {
	private static final String TAG = "ClientMonitor";
	
	private static final int CONNECT_TIMEOUT = 1000;
	
	private Socket mSocket;
	private InputStream mInput;
	private Writer mOutput;
	private boolean mConnected = false;
	
	private static final int MAX_BUFFER_SIZE = 1024;
	
	private byte[] mReadBuffer = null;
	private StringBuilder mReply;
	
	public ClientMonitor() { }
	
	public boolean open(String clientUrl, int port, String authCode) {
		if (mConnected) {
			if (Logging.ERROR) Log.e(TAG, "Client monitor already connected");
			close();
			return false;
		}
		if (Logging.DEBUG) Log.d(TAG, "Connecting with client monitor");
		
		try {
			mSocket = new Socket();
			mSocket.connect(new InetSocketAddress(clientUrl, port), CONNECT_TIMEOUT);
			mInput = mSocket.getInputStream();
			mOutput = new OutputStreamWriter(mSocket.getOutputStream(), "ISO-8859-1");
		} catch(UnknownHostException ex) {
			if (Logging.ERROR)
				Log.e(TAG, "Cant connect with client monitor - " + clientUrl + " Unknown host");
			close();
			return false;
		} catch(IOException ex) {
			if (Logging.ERROR) Log.e(TAG, "Cant connect with client monitor:"+ex.getMessage());
			close();
			return false;
		}
		
		if (Logging.DEBUG) Log.d(TAG, "Connected with client monitor. Now authorizing.");
		
		mReadBuffer = new byte[MAX_BUFFER_SIZE];
		mReply = new StringBuilder();
		mConnected = true;
		
		if (Logging.INFO) Log.i(TAG, "Authorizing access to monitor");
		boolean success;
		try {
			mOutput.write("<authorize>"+authCode+"</authorize>\n");
			mOutput.flush();
			
			int readed = mInput.read(mReadBuffer);
			mReply.append(new String(mReadBuffer, 0, readed));
			if (mReadBuffer[readed-1] == '\003') {
				mReply.setLength(mReply.length()-1);
			}
			
			/* if return success */
			success = mReply.toString().equals("<success/>");
		} catch(IOException ex) {
			if (Logging.ERROR) Log.e(TAG, "Cant authorize access to monitor: "+ ex.getMessage());
			close();
			return false;
		}
		
		if (!success) { // if failed
			if (Logging.ERROR) Log.e(TAG, "Cant authorize access to monitor");
			close();	// close connections
		}
		return success;
	}
	
	public boolean isConnected() {
		return mConnected;
	}
	
	public void close() {
		try {
			if (mInput != null)
				mInput.close();
		} catch(IOException ex) {
			if (Logging.WARNING) Log.w(TAG, "Cant close client monitor socket - input");
		}
		
		try {
			if (mOutput != null)
				mOutput.close();
		} catch(IOException ex) {
			if (Logging.WARNING) Log.w(TAG, "Cant close client monitor socket - output");
		}
		
		try {
			if (mSocket != null)
				mSocket.close();
		} catch(IOException ex) {
			if (Logging.WARNING) Log.w(TAG, "Cant close client monitor socket");
		}
		mSocket = null;
		mConnected = false;
		mReadBuffer = null;
		mReply = null;
	}
	
	public ClientEvent poll() throws IOException {
		mReply.setLength(0);
		/* reading reply */
		while (true) {
			int readed = mInput.read(mReadBuffer);
			if (readed == -1) {
				break;
			}
			/* if last chunk */
			mReply.append(new String(mReadBuffer, 0, readed));
			if (mReadBuffer[readed-1] == '\003') {
				mReply.setLength(mReply.length()-1);
				break;
			}
		}
		
		/* parse reply */
		return ClientEventParser.parse(mReply.toString());
	}
}
