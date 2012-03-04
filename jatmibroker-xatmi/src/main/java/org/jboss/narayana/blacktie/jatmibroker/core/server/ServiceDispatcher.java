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
package org.jboss.narayana.blacktie.jatmibroker.core.server;

import java.io.IOException;

import org.apache.log4j.LogManager;
import org.apache.log4j.Logger;
import org.jboss.narayana.blacktie.jatmibroker.core.conf.ConfigurationException;
import org.jboss.narayana.blacktie.jatmibroker.core.transport.Message;
import org.jboss.narayana.blacktie.jatmibroker.core.transport.Receiver;
import org.jboss.narayana.blacktie.jatmibroker.xatmi.BlackTieService;
import org.jboss.narayana.blacktie.jatmibroker.xatmi.Connection;
import org.jboss.narayana.blacktie.jatmibroker.xatmi.ConnectionException;
import org.jboss.narayana.blacktie.jatmibroker.xatmi.Response;
import org.jboss.narayana.blacktie.jatmibroker.xatmi.Service;
import org.jboss.narayana.blacktie.jatmibroker.xatmi.TPSVCINFO;

/**
 * This is the compatriot to the MDBBlacktieService found in the xatmi.mdb package. It is a wrapper for user services.
 */
public class ServiceDispatcher extends BlackTieService implements Runnable {
    private static final Logger log = LogManager.getLogger(ServiceDispatcher.class);
    private Service callback;
    private Receiver receiver;
    private Thread thread;
    private volatile boolean closed;
    private String serviceName;
    private int index;
    private Object closeLock = new Object();

    ServiceDispatcher(String serviceName, Service callback, Receiver receiver, int index) throws ConfigurationException {
        super(serviceName);
        this.index = index;
        this.serviceName = serviceName;
        this.callback = callback;
        this.receiver = receiver;
        thread = new Thread(this, serviceName + "-Dispatcher-" + index);
        thread.start();
        log.debug("Created: " + thread.getName());
    }

    public void run() {
        log.debug("Running");

        while (!closed) {
            Message message = null;
            try {
                message = receiver.receive(0);
                log.trace("Received");
            } catch (ConnectionException e) {
                if (closed) {
                    log.trace("Got an exception during close: " + e.getMessage(), e);
                    break;
                }
                if (e.getTperrno() == Connection.TPETIME) {
                    log.debug("Got a timeout");
                } else {
                    log.error("Could not receive the message: " + e.getMessage(), e);
                    break;
                }
            }

            synchronized (closeLock) {
                if (message != null) {
                    if (closed) {
                        log.warn("Message will be ignored as closing");
                    } else {
                        try {

                            this.processMessage(serviceName, message);
                            log.trace("Processed");
                        } catch (Throwable t) {
                            log.error("Can't process the message", t);
                        }

                        try {
                            // Assumes the message was received from stomp -
                            // fair
                            // assumption outside of an MDB
                            message.ack();
                        } catch (IOException t) {
                            log.error("Can't ack the message", t);
                        }
                    }
                }
            }
        }
    }

    public void startClose() {
        synchronized (closeLock) {
            closed = true;
            log.trace("Closed set");
        }
    }

    public void close() throws ConnectionException {
        log.trace("closing");

        log.trace("Closing receiver");
        receiver.close();
        log.trace("Closing receiver");

        try {
            log.trace("Joining");
            thread.join();
            log.trace("Joined");
        } catch (InterruptedException e) {
            log.error("Could not join the dispatcher", e);
        }
        log.trace("closed");
    }

    public Response tpservice(TPSVCINFO svcinfo) throws ConnectionException, ConfigurationException {
        log.trace("Invoking callback");
        return callback.tpservice(svcinfo);
    }
}
