/*
 * JBoss, Home of Professional Open Source
 * Copyright 2008, Red Hat, Inc., and others contributors as indicated
 * by the @authors tag. All rights reserved.
 * See the copyright.txt in the distribution for a
 * full listing of individual contributors.
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License, v. 2.1.
 * This program is distributed in the hope that it will be useful, but WITHOUT A
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
 * You should have received a copy of the GNU Lesser General Public License,
 * v.2.1 along with this distribution; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 */
package org.jboss.blacktie.jatmibroker.xatmi;

import junit.framework.TestCase;

import org.jboss.blacktie.jatmibroker.JAtmiBrokerException;
import org.jboss.blacktie.jatmibroker.server.AtmiBrokerServer;

public class BlacktieServiceTestCase extends TestCase {
	private AtmiBrokerServer server;
	private Connection connection;

	public void setUp() throws ConnectionException, JAtmiBrokerException {
		this.server = new AtmiBrokerServer("standalone-server", null);
		this.server.tpadvertise("EchoService", EchoServiceTestService.class);

		ConnectionFactory connectionFactory = ConnectionFactory
				.getConnectionFactory();
		connection = connectionFactory.getConnection("", "");
	}

	public void tearDown() throws ConnectionException {
		connection.close();
		server.tpunadvertise("EchoService");
	}

	public void test() throws ConnectionException {
		byte[] echo = "echo".getBytes();
		Response response = connection.tpcall("EchoService", echo, 4, 0);
		byte[] responseData = response.getData();
		String receivedMessage = new String(responseData);
		assertEquals("echo", receivedMessage);
	}
}
