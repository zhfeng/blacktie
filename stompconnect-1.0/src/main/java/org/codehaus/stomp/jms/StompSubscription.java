/**
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.codehaus.stomp.jms;

import java.util.Map;

import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.naming.NamingException;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.codehaus.stomp.ProtocolException;
import org.codehaus.stomp.Stomp;
import org.codehaus.stomp.StompFrame;

/**
 * Represents an individual Stomp subscription
 * 
 * @version $Revision: 50 $
 */
public class StompSubscription implements Runnable {
    public static final String AUTO_ACK = Stomp.Headers.Subscribe.AckModeValues.AUTO;
    public static final String CLIENT_ACK = Stomp.Headers.Subscribe.AckModeValues.CLIENT;
    private static final transient Log log = LogFactory.getLog(StompSubscription.class);
    private final StompSession session;
    private final String subscriptionId;
    private Destination destination;
    private MessageConsumer consumer;
    private boolean closed;
    private Thread thread;
    private Map<String, Object> headers;

    public StompSubscription(StompSession session, String subscriptionId, StompFrame frame) throws JMSException,
            ProtocolException, NamingException {
        this.subscriptionId = subscriptionId;
        this.session = session;
        this.headers = frame.getHeaders();
        this.consumer = session.createConsumer(headers);

        thread = new Thread(this, "StompSubscription: " + Thread.currentThread().getName());
        thread.start();
    }

    public void close() throws JMSException {
        closed = true;
        consumer.close();
    }

    public String getSubscriptionId() {
        return subscriptionId;
    }

    public Destination getDestination() {
        return destination;
    }

    public void run() {
        try {
            while (!closed) {
                String destinationName = (String) headers.get(Stomp.Headers.Subscribe.DESTINATION);
                Message message = consumer.receive();
                log.debug("received: " + destinationName + " for: " + message.getObjectProperty("messagereplyto"));
                if (message != null) {
                    try {
                        // Lock the session so that the connection cannot be started before the acknowledge is done
                        synchronized (session) {
                            // Stop the session before sending the message
                            log.debug("Stopping session: " + session);
                            session.stop();
                            // Send the message to the server
                            log.debug("Sending message: " + session);
                            session.sendToStomp(message, this);
                            // Acknowledge the message for this connection as we know the server has received it now
                            log.debug("Acking message: " + session);
                            message.acknowledge();
                        }
                    } catch (Exception e) {
                        log.error("Failed to process message due to: " + e + ". Message: " + message, e);
                    }
                }
            }
        } catch (JMSException e) {
            log.debug("Caught a JMS exception: " + e.getMessage());
        }

    }
}
