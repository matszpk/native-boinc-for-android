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

package sk.boinc.nativeboinc.util;

import java.util.HashMap;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.Map;

import sk.boinc.nativeboinc.debug.Logging;

import android.util.Log;

/**
 * @author mat
 * Pending outputs controller for queued tasks - controls operation flow and manages pending output 
 */
public class PendingController<Operation> {
	
	private class OpEntry {
		// default is true
		public boolean isRan = true;
		public long finihTimestamp = -1L;	// if run not setted
		public Object output = null;
		public Object error = null;
		
		public OpEntry() { }
	}
	
	private class ErrorQueueEntry {
		public Operation pendingOp;
		public long finishTimestamp;
		public Object error;
		
		public ErrorQueueEntry(Operation pendingOp, OpEntry opEntry) {
			this.pendingOp = pendingOp;
			this.finishTimestamp = opEntry.finihTimestamp;
			this.error = opEntry.error;
		}
	}
	
	// for debug
	private final String mTag;
	
	private HashMap<Operation, OpEntry> mPendingOutputsMap = new HashMap<Operation, OpEntry>();
	private LinkedList<ErrorQueueEntry> mPendingErrorQueue = new LinkedList<ErrorQueueEntry>();
	
	public PendingController(String tag) {
		mTag = tag;
	}
	
	private void removeFromErrorQueue(Operation pendingOp) {
		// remove proper error from global queue
		Iterator<ErrorQueueEntry> it = mPendingErrorQueue.iterator();
		while (it.hasNext()) {
			ErrorQueueEntry entry = it.next();
			if (entry.pendingOp.equals(pendingOp)) {
				it.remove();	// remove it
				break;
			}
		}
	}
	
	public synchronized boolean begin(Operation pendingOp) {
		OpEntry opEntry = mPendingOutputsMap.get(pendingOp);
		
		if (Logging.DEBUG) Log.d(mTag, "Do begin:"+pendingOp);
		
		if (opEntry != null && opEntry.isRan)
			return false;	// currently is working
		
		if (Logging.DEBUG) Log.d(mTag, "Began:"+pendingOp);
		// remove from queue pending errors
		removeFromErrorQueue(pendingOp);
		
		if (mPendingOutputsMap.put(pendingOp, new OpEntry()) == null) {
			// if add new entry, check whether saving memory is needed
			saveMemoryIfNeeded();
		}
		return true;
	}
	
	public synchronized void finish(Operation pendingOp) {
		OpEntry opEntry = mPendingOutputsMap.get(pendingOp);
		
		if (opEntry == null) {
			if (Logging.WARNING) Log.w(mTag, "Failed finish for "+pendingOp);
			return;
		}
		
		if (Logging.DEBUG) Log.d(mTag, "Do finish:"+pendingOp);
		
		opEntry.isRan = false;
		if (opEntry.finihTimestamp == -1)
			opEntry.finihTimestamp = System.currentTimeMillis();
	}
	
	public synchronized void finishSelected(PendingOpSelector<Operation> selector) {
		Iterator<Map.Entry<Operation, OpEntry>> mapIt = mPendingOutputsMap.entrySet().iterator();
		while(mapIt.hasNext()) {
			Map.Entry<Operation, OpEntry> entry = mapIt.next();
			if (selector.select(entry.getKey())) {
				if (Logging.DEBUG) Log.d(mTag, "Do finish in selection:"+entry.getKey());
				OpEntry opEntry = entry.getValue();
				opEntry.isRan = false;
				if (opEntry.finihTimestamp == -1)
					opEntry.finihTimestamp = System.currentTimeMillis();
			}
		}
	}
	
	public synchronized void finishWithOutput(Operation pendingOp, Object output) {
		OpEntry opEntry = mPendingOutputsMap.get(pendingOp);
		
		if (opEntry == null) {
			if (Logging.WARNING) Log.w(mTag, "Failed finish for "+pendingOp);
			return;
		}
		
		Log.d(mTag, "Do finish:"+pendingOp+",out.");
		
		opEntry.isRan = false;
		opEntry.output = output;
		if (opEntry.finihTimestamp == -1)
			opEntry.finihTimestamp = System.currentTimeMillis();
	}
	
	public synchronized void finishWithError(Operation pendingOp, Object error) {
		OpEntry opEntry = mPendingOutputsMap.get(pendingOp);
		
		if (opEntry == null) {
			if (Logging.WARNING) Log.w(mTag, "Failed finish for "+pendingOp);
			return;
		}
		
		if (Logging.DEBUG) Log.d(mTag, "Do finish:"+pendingOp+",err.");
		
		opEntry.isRan = false;
		opEntry.error = error;
		if (opEntry.finihTimestamp == -1)
			opEntry.finihTimestamp = System.currentTimeMillis();
		if (error != null)
			mPendingErrorQueue.addLast(new ErrorQueueEntry(pendingOp, opEntry));
	}
	
	public synchronized Object takePendingOutput(Operation pendingOp) {
		if (Logging.DEBUG) Log.d(mTag, "Take out:"+pendingOp);
		OpEntry opEntry = mPendingOutputsMap.get(pendingOp);
		if (opEntry == null)
			return null;
		Object output = opEntry.output;
		// error should be taken only once
		opEntry.output = null;
		return output;
	}
	
	public synchronized Object takePendingError(Operation pendingOp) {
		if (pendingOp != null) { // if select op
			OpEntry opEntry = mPendingOutputsMap.get(pendingOp);
			if (opEntry == null)
				return null;
			Object pendingError = opEntry.error;
			// error should be taken only once
			opEntry.error = null;
			
			removeFromErrorQueue(pendingOp);
			if (Logging.DEBUG) Log.d(mTag, "Take error "+pendingOp+":"+pendingError);
			return pendingError;
		} else { // take from global queue
			ErrorQueueEntry entry = mPendingErrorQueue.pollFirst();
			if (entry != null) {
				OpEntry opEntry = mPendingOutputsMap.get(entry.pendingOp);
				opEntry.error = null;
				if (Logging.DEBUG) Log.d(mTag, "Take error from queue:"+opEntry.error);
				return entry.error; 
			}
			return null;
		}
	}
	
	public synchronized boolean handlePendingErrors(final Operation pendingOp,
			PendingErrorHandler<Operation> callback) {
		
		if (pendingOp != null) {
			OpEntry opEntry = mPendingOutputsMap.get(pendingOp);
			if (opEntry == null || opEntry.error == null)
				return false;	// no error set
			// do handle errors
			if (Logging.DEBUG) Log.d(mTag, "try handle error for "+pendingOp+":"+opEntry.error);
			if (callback.handleError(pendingOp, opEntry.error)) {
				if (Logging.DEBUG) Log.d(mTag, "handle error for "+pendingOp+":"+opEntry.error);
				// removing error
				opEntry.error = null;
				
				removeFromErrorQueue(pendingOp);
				// if handled
				return true;
			}
		} else {
			// handle all pending errors
			boolean handled = false;
			LinkedList<ErrorQueueEntry> copyOfQueue = new LinkedList<ErrorQueueEntry>(mPendingErrorQueue);
			
			Iterator<ErrorQueueEntry> queueIt = copyOfQueue.iterator();
			while (queueIt.hasNext()) {
				ErrorQueueEntry queueEntry = queueIt.next();
				if (Logging.DEBUG) Log.d(mTag, "try handle error from queue:"+queueEntry.error);
				if (callback.handleError(queueEntry.pendingOp, queueEntry.error)) {
					if (Logging.DEBUG) Log.d(mTag, "handle error from queue:"+queueEntry.error);
					OpEntry opEntry = mPendingOutputsMap.get(queueEntry.pendingOp);
					opEntry.error = null;
					handled = true;
					// remove from queue
					removeFromErrorQueue(queueEntry.pendingOp);
				}
			}
			return handled;
		}
		return false;
	}
	
	/*
	 * cancel all pendings
	 */
	public synchronized void cancelAll() {
		mPendingErrorQueue.clear();
		mPendingOutputsMap.clear();
	}
	
	/**
	 * saving memory
	 */
	private static final int MAX_ENTRIES_SIZE = 1000;
	private static final int EXPIRE_PERIOD = 500000; // 1000 seconds
	
	private void saveMemoryIfNeeded() {
		if (mPendingOutputsMap.size() < MAX_ENTRIES_SIZE)
			return;	// to small 
		
		long currentTime = System.currentTimeMillis();
		
		if (Logging.DEBUG) Log.d(mTag, "save memory if needed");
		
		// remove expired entries in the map
		Iterator<Map.Entry<Operation, OpEntry>> mapIt = mPendingOutputsMap.entrySet().iterator();
		while(mapIt.hasNext()) {
			Map.Entry<Operation, OpEntry> entry = mapIt.next();
			OpEntry opEntry = entry.getValue();
			if (opEntry.finihTimestamp != -1L && opEntry.finihTimestamp-currentTime > EXPIRE_PERIOD)
				mapIt.remove();
		}
		
		// also we remove expired errors in queue
		Iterator<ErrorQueueEntry> it = mPendingErrorQueue.iterator();
		while(it.hasNext()) {
			ErrorQueueEntry entry = it.next();
			if (entry.finishTimestamp != -1L && entry.finishTimestamp-currentTime > EXPIRE_PERIOD)
				it.remove();
			else // no further task to remove
				break;
		}
	}
	
	public synchronized boolean isRan(Operation pendingOp) {
		OpEntry opEntry = mPendingOutputsMap.get(pendingOp);
		return (opEntry != null && opEntry.isRan);
	}
}
