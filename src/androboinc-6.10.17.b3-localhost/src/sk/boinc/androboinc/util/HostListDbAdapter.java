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

package sk.boinc.androboinc.util;

import sk.boinc.androboinc.debug.Logging;
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.SQLException;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.util.Log;


public class HostListDbAdapter {
	private static final String TAG = "HostListDbAdapter";

	private static final String DATABASE_NAME = "data";
	private static final int    DATABASE_VERSION = 1;
	private static final String TABLE_HOSTS = "hosts";

	public static final String KEY_ROWID           = "_id";
	public static final String FIELD_HOST_NICKNAME = "nickname";
	public static final String FIELD_HOST_ADDRESS  = "address";
	public static final String FIELD_HOST_PORT     = "port";
	public static final String FIELD_HOST_PASSWORD = "password";

	/* Database creation SQL statement */
	private static final String DATABASE_CREATE = "CREATE TABLE IF NOT EXISTS " + TABLE_HOSTS + " ("
				+ KEY_ROWID           + " INTEGER PRIMARY KEY AUTOINCREMENT, "
				+ FIELD_HOST_NICKNAME + " TEXT UNIQUE, "
				+ FIELD_HOST_ADDRESS  + " TEXT NOT NULL, " 
				+ FIELD_HOST_PORT     + " INTEGER NOT NULL, "
				+ FIELD_HOST_PASSWORD + " TEXT NOT NULL);";
	/* Delete table */
	private static final String DATABASE_CLEAN = "DROP TABLE IF EXISTS " + TABLE_HOSTS + ";";

	private static class DatabaseHelper extends SQLiteOpenHelper {
		DatabaseHelper(Context context) {
			super(context, DATABASE_NAME, null, DATABASE_VERSION);
		}
		@Override
		public void onCreate(SQLiteDatabase db) {
			if (Logging.INFO) Log.i(TAG, "Creating database, version " + DATABASE_VERSION);
			db.execSQL(DATABASE_CREATE);
		}
		@Override
		public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
			// No upgrade needed yet, default stuff used here:
			if (Logging.WARNING) Log.w(TAG, "Upgrading database from version " + oldVersion + " to " + newVersion + ", destroying all old data");
			db.execSQL(DATABASE_CLEAN);
			onCreate(db);
		}
	}

	private final Context mCtx;
	private DatabaseHelper mDbHelper;
	private SQLiteDatabase mDb;

	/**
	 * Constructor - takes the context to allow the database to be
	 * opened/created
	 * 
	 * @param ctx the Context within which to work
	 */
	public HostListDbAdapter(Context ctx) {
		this.mCtx = ctx;
	}

	/**
	 * Open the database.
	 * If it cannot be opened (not existing yet), try to create a new
	 * instance of the database.
	 * If it cannot be created, throw an exception to signal the failure
	 * 
	 * @return this (self reference, allowing this to be chained in an
	 *         initialization call)
	 * @throws SQLException if the database could be neither opened or created
	 */
	public HostListDbAdapter open() throws SQLException {
		if (Logging.DEBUG) Log.d(String.valueOf(this), "open()");
		mDbHelper = new DatabaseHelper(mCtx);
		mDb = mDbHelper.getWritableDatabase();
		return this;
	}

	/**
	 * Close the database
	 */
	public void close() {
		if (Logging.DEBUG) Log.d(String.valueOf(this), "close()");
		if (mDbHelper != null) {
			mDbHelper.close();
			mDbHelper = null;
		}
		mDb = null;
	}

	/**
	 * Create a new entry in the table.
	 * 
	 * @param clientId data of a host record
	 * @return the new rowId for that entry in case of success, otherwise -1 to indicate failure
	 */
	public long addHost(ClientId clientId) {
		ContentValues initialValues = new ContentValues();
		initialValues.put(FIELD_HOST_NICKNAME, clientId.getNickname());
		initialValues.put(FIELD_HOST_ADDRESS, clientId.getAddress());
		initialValues.put(FIELD_HOST_PORT, clientId.getPort());
		initialValues.put(FIELD_HOST_PASSWORD, clientId.getPassword());
		return mDb.insert(TABLE_HOSTS, null, initialValues);
	}

	/**
	 * Delete the entry specified by rowId
	 * 
	 * @param rowId id of note to delete
	 * @return true if deleted, false otherwise
	 */
	public boolean deleteHost(long rowId) {
		return mDb.delete(TABLE_HOSTS, KEY_ROWID + "=" + rowId, null) > 0;
	}

	/**
	 * Modify host record
	 * 
	 * @param clientId data of a host record
	 * @return true if the note was successfully updated, false otherwise
	 */
	public boolean updateHost(ClientId clientId) {
		long rowId = clientId.getId();
		ContentValues newValues = new ContentValues();
		newValues.put(FIELD_HOST_NICKNAME, clientId.getNickname());
		newValues.put(FIELD_HOST_ADDRESS, clientId.getAddress());
		newValues.put(FIELD_HOST_PORT, clientId.getPort());
		newValues.put(FIELD_HOST_PASSWORD, clientId.getPassword());
		return (mDb.update(TABLE_HOSTS, newValues, KEY_ROWID + "=" + rowId, null) > 0);
	}

	/**
	 * Fetch list of all the hosts
	 * 
	 * @return Cursor over all data
	 */
	public Cursor fetchAllHosts() {
		return mDb.query(TABLE_HOSTS,
				new String[] {KEY_ROWID, FIELD_HOST_NICKNAME, FIELD_HOST_ADDRESS, FIELD_HOST_PORT, FIELD_HOST_PASSWORD},
				null, null, null, null, null);
	}

	/**
	 * Fetch single host by nickname
	 * 
	 * @param nickname of the host to retrieve
	 * @return ClientId class if host is successfully retrieved or null if host is not found
	 */
	public ClientId fetchHost(String nickname) {
		ClientId clientId = null;
		Cursor cur = mDb.query(true, TABLE_HOSTS,
				new String[] {KEY_ROWID, FIELD_HOST_NICKNAME, FIELD_HOST_ADDRESS, FIELD_HOST_PORT, FIELD_HOST_PASSWORD},
				FIELD_HOST_NICKNAME + "=\"" + nickname + "\"", null, null, null, null, null);
		if (cur != null) {
			if (cur.moveToFirst()) {
				clientId = new ClientId(cur);
			}
			cur.close();
		}
		return clientId;
	}

	/**
	 * Check, whether host's nickname is unique in database
	 * 
	 * @param rowId the row ID, which is excluded from check
	 * @param nickname the host's nickname to be checked
	 * @return true if nickname is unique, false otherwise
	 */
	public boolean hostUnique(long rowId, String nickname) {
		Cursor cur = mDb.query(true, TABLE_HOSTS,
				new String[] { KEY_ROWID, FIELD_HOST_NICKNAME },
				KEY_ROWID + "!=" + rowId + " AND " + FIELD_HOST_NICKNAME + "='" + nickname + "'",
				null, null, null, null, null);
		if (cur == null) return true;
		boolean found = cur.moveToFirst();
		cur.close();
		cur = null;
		return !found;
	}
}
