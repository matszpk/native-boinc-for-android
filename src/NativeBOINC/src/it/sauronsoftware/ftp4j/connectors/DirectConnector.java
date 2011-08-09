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
package it.sauronsoftware.ftp4j.connectors;

import it.sauronsoftware.ftp4j.FTPConnector;

import java.io.IOException;
import java.net.InetSocketAddress;
import java.net.Socket;

/**
 * The DirectConnector connects the remote host with a straight socket
 * connection, using no proxy.
 * 
 * @author Carlo Pelliccia
 */
public class DirectConnector implements FTPConnector {

	public Socket connectForCommunicationChannel(String host, int port)
			throws IOException {
		return new Socket(host, port);
	}

	public Socket connectForDataTransferChannel(String host, int port)
			throws IOException {
		Socket socket = new Socket();
		socket.setReceiveBufferSize(512 * 1024);
		socket.setSendBufferSize(512 * 1024);
		socket.connect(new InetSocketAddress(host, port));
		return socket;
	}

}
