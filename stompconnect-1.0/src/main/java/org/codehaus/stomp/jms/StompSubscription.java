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
            ProtocolException {
        this.subscriptionId = subscriptionId;
        this.session = session;
        this.headers = frame.getHeaders();

        thread = new Thread(this);
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
            consumer = session.createConsumer(headers);
            while (!closed) {
                String destinationName = (String) headers.get(Stomp.Headers.Subscribe.DESTINATION);
                Message message = consumer.receive();
                log.debug("received:" + destinationName);
                if (message != null) {
                    try {
                        session.getProtocolConverter().stopStompSession(message, session);
                        session.sendToStomp(message, this);
                    } catch (Exception e) {
                        log.error("Failed to process message due to: " + e + ". Message: " + message, e);
                    }
                }
            }
        } catch (JMSException e) {
            log.debug("Caught a JMS exception: " + e.getMessage());
        } catch (ProtocolException e) {
            log.debug("Caught a Protocol exception: " + e.getMessage());
        }

    }
}
