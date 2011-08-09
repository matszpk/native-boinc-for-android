/*
 * ftp4j - A pure Java FTP client library
 * 
 * Copyright (C) 2008-2010 Carlo Pelliccia (www.sauronsoftware.it)
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version
 * 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License 2.1 for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License version 2.1 along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 */
package it.sauronsoftware.ftp4j;

import java.io.IOException;
import java.net.Socket;

/**
 * This interface describes a connector. Connectors are used by the client to
 * establish connections with remote servers.
 * 
 * @author Carlo Pelliccia
 */
public interface FTPConnector {

	/**
	 * This methods returns an established connection to a remote host, suitable
	 * for a FTP communication channel.
	 * 
	 * @param host
	 *            The remote host name or address.
	 * @param port
	 *            The remote port.
	 * @return The connection with the remote host.
	 * @throws IOException
	 *             If the connection cannot be established.
	 */
	public Socket connectForCommunicationChannel(String host, int port)
			throws IOException;

	/**
	 * This methods returns an established connection to a remote host, suitable
	 * for a FTP data transfer channel.
	 * 
	 * @param host
	 *            The remote host name or address.
	 * @param port
	 *            The remote port.
	 * @return The connection with the remote host.
	 * @throws IOException
	 *             If the connection cannot be established.
	 */
	public Socket connectForDataTransferChannel(String host, int port)
			throws IOException;
	
}
