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
package org.jboss.narayana.blacktie.btadmin.commands;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Properties;

import javax.management.InstanceNotFoundException;
import javax.management.MBeanException;
import javax.management.MBeanServerConnection;
import javax.management.ObjectName;
import javax.management.ReflectionException;

import org.apache.log4j.LogManager;
import org.apache.log4j.Logger;
import org.jboss.narayana.blacktie.btadmin.Command;
import org.jboss.narayana.blacktie.btadmin.CommandFailedException;
import org.jboss.narayana.blacktie.btadmin.IncompatibleArgsException;
import org.jboss.narayana.blacktie.jatmibroker.core.conf.Machine;
import org.jboss.narayana.blacktie.jatmibroker.core.conf.Server;

/**
 * The shutdown command will shutdown the server specified. This method is
 * non-blocking, i.e. the server is requested to shutdown, it will still be
 * alive possibly
 */
public class Shutdown implements Command {
	/**
	 * The logger to use for output
	 */
	private static Logger log = LogManager.getLogger(Shutdown.class);

	/**
	 * The name of the server.
	 */
	private String serverName;

	/**
	 * The ID of the server, will be 0 (all) if not provided
	 */
	private int id = 0;

	/**
	 * Does the command require the admin connection.
	 */
	public boolean requiresAdminConnection() {
		return true;
	}

	/**
	 * Show the usage of the command
	 */
	public String getQuickstartUsage() {
		return "[<serverName> [<serverId>]]";
	}

	public void initializeArgs(String[] args) throws IncompatibleArgsException {
		if (args.length > 0) {
			serverName = args[0];
			if (args.length == 2) {
				try {
					id = Integer.parseInt(args[1]);
					log.trace("Successfully parsed: " + args[1]);
				} catch (NumberFormatException nfe) {
					throw new IncompatibleArgsException(
							"The third argument was expected to be the (integer) instance id to shutdown");
				}
			}
		}
	}

	public void invoke(MBeanServerConnection beanServerConnection,
			ObjectName blacktieAdmin, Properties configuration)
			throws InstanceNotFoundException, MBeanException,
			ReflectionException, IOException, CommandFailedException {
		List<ServerToStop> serversToStop = new ArrayList<ServerToStop>();

		List<Server> serverLaunchers = (List<Server>) configuration
				.get("blacktie.domain.serverLaunchers");
		if (serverName == null) {
			Iterator<Server> launchers = serverLaunchers.iterator();
			while (launchers.hasNext()) {
				Server server = launchers.next();
				Iterator<Machine> iterator2 = server.getLocalMachine()
						.iterator();
				while (iterator2.hasNext()) {
					Machine machine = iterator2.next();
					ServerToStop serverToStop = new ServerToStop();
					serverToStop.setName(server.getName());
					serverToStop.setId(machine.getServerId());
					serversToStop.add(serverToStop);
				}
			}
		} else {
			ServerToStop serverToStop = new ServerToStop();
			serverToStop.setName(serverName);
			serverToStop.setId(id);
			serversToStop.add(serverToStop);
		}
		if (serversToStop.size() != 0) {
			Iterator<ServerToStop> iterator = serversToStop.iterator();
			while (iterator.hasNext()) {
				ServerToStop next = iterator.next();
				Boolean result = (Boolean) beanServerConnection.invoke(
						blacktieAdmin, "shutdown",
						new Object[] { next.getName(), next.getId() },
						new String[] { "java.lang.String", "int" });
				if (result) {
					log.info("Server shutdown successfully: " + next.getName()
							+ " with id: " + next.getId());
				} else {
					log.error("Server could not be shutdown (may already be stopped)");
					throw new CommandFailedException(-1);
				}
			}
		} else {
			log.error("No servers were configured for shutdown");
			throw new CommandFailedException(-1);

		}
	}

	private class ServerToStop {
		private String name;
		private int id;

		public String getName() {
			return name;
		}

		public void setName(String name) {
			this.name = name;
		}

		public int getId() {
			return id;
		}

		public void setId(int id) {
			this.id = id;
		}
	}
}
