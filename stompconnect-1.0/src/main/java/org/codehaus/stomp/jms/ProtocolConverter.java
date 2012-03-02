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

import java.io.ByteArrayOutputStream;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.Session;
import javax.jms.XAConnection;
import javax.jms.XAConnectionFactory;
import javax.jms.XASession;
import javax.naming.InitialContext;
import javax.naming.NamingException;
import javax.transaction.Transaction;
import javax.transaction.TransactionManager;
import javax.transaction.xa.XAResource;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.codehaus.stomp.ProtocolException;
import org.codehaus.stomp.Stomp;
import org.codehaus.stomp.StompFrame;
import org.codehaus.stomp.StompFrameError;
import org.codehaus.stomp.StompHandler;
import org.omg.CosTransactions.Control;

import com.arjuna.ats.internal.jta.transaction.jts.AtomicTransaction;
import com.arjuna.ats.internal.jta.transaction.jts.TransactionImple;
import com.arjuna.ats.internal.jts.ControlWrapper;
import com.arjuna.ats.internal.jts.ORBManager;

/**
 * A protocol switch between JMS and Stomp
 * 
 * @author <a href="http://people.apache.org/~jstrachan/">James Strachan</a>
 * @author <a href="http://hiramchirino.com">chirino</a>
 */
public class ProtocolConverter implements StompHandler {
    private static final transient Log log = LogFactory.getLog(ProtocolConverter.class);
    private final StompHandler outputHandler;
    private ConnectionFactory connectionFactory;
    private XAConnectionFactory xaConnectionFactory;
    private Connection connection;
    private XAConnection xaConnection;
    private StompSession session;
    private StompSession xaSession;
    private final Map<String, StompSubscription> subscriptions = new ConcurrentHashMap<String, StompSubscription>();
    private final Map<String, Connection> stoppedConnections = new ConcurrentHashMap<String, Connection>();

    private static TransactionManager tm;

    public ProtocolConverter(ConnectionFactory connectionFactory, XAConnectionFactory xaConnectionFactory,
            StompHandler outputHandler) throws NamingException {
        this.connectionFactory = connectionFactory;
        this.xaConnectionFactory = xaConnectionFactory;
        this.outputHandler = outputHandler;
        tm = (TransactionManager) new InitialContext().lookup("java:/TransactionManager");
    }

    public StompHandler getOutputHandler() {
        return outputHandler;
    }

    /**
     * Convert an IOR representing an OTS transaction into a JTA transaction
     * 
     * @param orb
     * 
     * @param ior the CORBA reference for the OTS transaction
     * @return a JTA transaction that wraps the OTS transaction
     */
    private static Transaction controlToTx(String ior) {
        log.debug("controlToTx: ior: " + ior);

        ControlWrapper cw = createControlWrapper(ior);
        TransactionImple tx = (TransactionImple) TransactionImple.getTransactions().get(cw.get_uid());

        if (tx == null) {
            log.debug("controlToTx: creating a new tx - wrapper: " + cw);
            tx = new JtsTransactionImple(cw);
        }

        return tx;
    }

    private static ControlWrapper createControlWrapper(String ior) {
        org.omg.CORBA.Object obj = ORBManager.getORB().orb().string_to_object(ior);

        Control control = org.omg.CosTransactions.ControlHelper.narrow(obj);
        if (control == null)
            log.warn("createProxy: ior not a control");

        return new ControlWrapper(control);
    }

    public synchronized void close() throws JMSException {
        try {
            // now the connetions
            if (xaConnection != null) {
                xaConnection.close();
            }
            if (connection != null) {
                connection.close();
            }
        } finally {
            xaConnection = null;
            connection = null;
            session = null;
            xaSession = null;
            subscriptions.clear();
            stoppedConnections.clear();
        }
    }

    /**
     * Process a Stomp Frame
     */
    public void onStompFrame(StompFrame command) throws Exception {
        try {
            if (log.isDebugEnabled()) {
                log.debug(">>>> " + command.getAction() + " headers: " + command.getHeaders());
            }

            if (command.getClass() == StompFrameError.class) {
                throw ((StompFrameError) command).getException();
            }

            String action = command.getAction();
            if (action.startsWith(Stomp.Commands.SEND)) {
                onStompSend(command);
            } else if (action.startsWith(Stomp.Commands.RECEIVE)) {
                onStompReceive(command);
            } else if (action.startsWith(Stomp.Commands.ACK)) {
                onStompAck(command);
            } else if (action.startsWith(Stomp.Commands.SUBSCRIBE)) {
                onStompSubscribe(command);
            } else if (action.startsWith(Stomp.Commands.UNSUBSCRIBE)) {
                onStompUnsubscribe(command);
            } else if (action.startsWith(Stomp.Commands.CONNECT)) {
                onStompConnect(command);
            } else if (action.startsWith(Stomp.Commands.DISCONNECT)) {
                onStompDisconnect(command);
            } else {
                throw new ProtocolException("Unknown STOMP action: " + action);
            }
        } catch (Exception e) {
            e.printStackTrace();

            // Let the stomp client know about any protocol errors.
            ByteArrayOutputStream baos = new ByteArrayOutputStream();
            PrintWriter stream = new PrintWriter(new OutputStreamWriter(baos, "UTF-8"));
            e.printStackTrace(stream);
            stream.close();

            Map<String, Object> headers = new HashMap<String, Object>();
            headers.put(Stomp.Headers.Error.MESSAGE, e.getMessage());

            final String receiptId = (String) command.getHeaders().get(Stomp.Headers.RECEIPT_REQUESTED);
            if (receiptId != null) {
                headers.put(Stomp.Headers.Response.RECEIPT_ID, receiptId);
            }

            StompFrame errorMessage = new StompFrame(Stomp.Responses.ERROR, headers, baos.toByteArray());
            sendToStomp(errorMessage);

            // TODO need to do anything else? Should we close the connection?
        }
    }

    public void onException(Exception e) {
        log.error("Caught: " + e, e);
    }

    public void stopConnection(Message message, Connection connection) throws JMSException {
        connection.stop();
        stoppedConnections.put(message.getJMSMessageID(), connection);
        log.debug("Stopped connection: " + connection);
    }

    // Implemenation methods
    // -------------------------------------------------------------------------
    protected void onStompConnect(StompFrame command) throws Exception {
        if (xaConnection != null) {
            throw new ProtocolException("Already connected.");
        }
        if (connection != null) {
            throw new ProtocolException("Already connected.");
        }

        Map<String, Object> headers = command.getHeaders();
        String login = (String) headers.get(Stomp.Headers.Connect.LOGIN);
        String passcode = (String) headers.get(Stomp.Headers.Connect.PASSCODE);
        String clientId = (String) headers.get(Stomp.Headers.Connect.CLIENT_ID);

        if (login != null) {
            xaConnection = xaConnectionFactory.createXAConnection(login, passcode);
            connection = connectionFactory.createConnection(login, passcode);
        } else {
            xaConnection = xaConnectionFactory.createXAConnection();
            connection = connectionFactory.createConnection();
        }
        if (clientId != null) {
            connection.setClientID(clientId);
            xaConnection.setClientID(clientId);
        }

        connection.start();
        xaConnection.start();

        Map<String, Object> responseHeaders = new HashMap<String, Object>();

        responseHeaders.put(Stomp.Headers.Connected.SESSION, clientId);
        String requestId = (String) headers.get(Stomp.Headers.Connect.REQUEST_ID);
        if (requestId == null) {
            // TODO legacy
            requestId = (String) headers.get(Stomp.Headers.RECEIPT_REQUESTED);
        }
        if (requestId != null) {
            // TODO legacy
            responseHeaders.put(Stomp.Headers.Connected.RESPONSE_ID, requestId);
            responseHeaders.put(Stomp.Headers.Response.RECEIPT_ID, requestId);
        }

        StompFrame sc = new StompFrame();
        sc.setAction(Stomp.Responses.CONNECTED);
        sc.setHeaders(responseHeaders);
        sendToStomp(sc);
    }

    protected void onStompDisconnect(StompFrame command) throws Exception {
        checkConnected();
        close();
    }

    protected void onStompSend(StompFrame command) throws Exception {
        checkConnected();

        Map<String, Object> headers = command.getHeaders();

        String xid = (String) headers.get("messagexid");

        if (xid != null) {
            log.trace("Transaction was propagated: " + xid);
            Transaction tx = controlToTx(xid);
            tm.resume(tx);
            log.trace("Resumed transaction");

            StompSession session = getXASession();

            Transaction transaction = tm.getTransaction();
            log.trace("Got transaction: " + transaction);

            // BLACKTIE-308 we no longer need to enlist the JMS resource as JCA does this for us
            session.sendToJms(command);

            tm.suspend();
            log.trace("Suspended transaction");
        } else {
            log.trace("WAS NULL XID");

            StompSession session = getSession();

            session.sendToJms(command);
            log.trace("Sent to JMS");
        }
        sendResponse(command);
        log.trace("Sent Response");
    }

    protected void onStompReceive(StompFrame command) throws Exception {
        checkConnected();

        Map<String, Object> headers = command.getHeaders();
        String destinationName = (String) headers.remove(Stomp.Headers.Send.DESTINATION);
        String xid = (String) headers.get("messagexid");
        StompSession session = null;
        if (xid != null) {
            session = getXASession();
        } else {
            session = getSession();
        }
        long ttl = session.getTimeToLive(headers);
        Destination destination = session.convertDestination(destinationName, true);
        Message msg;
        MessageConsumer consumer;

        log.trace("Consuming message - ttl=" + ttl + " IOR=" + xid);

        if (xid != null) {
            log.trace("Transaction was propagated: " + xid);
            Transaction tx = controlToTx(xid);
            tm.resume(tx);
            log.trace("Resumed transaction");

            Transaction transaction = tm.getTransaction();
            log.trace("Got transaction: " + transaction);

            // create an XA consumer
            XASession xaSession = (XASession) session.getSession();
            consumer = xaSession.createConsumer(destination);
            
            msg = (ttl > 0 ? consumer.receive(ttl) : consumer.receive());

            tm.suspend();
            log.trace("Suspended transaction");
        } else {
            javax.jms.Session ss = session.getSession();
            consumer = ss.createConsumer(destination);
            msg = (ttl > 0 ? consumer.receive(ttl) : consumer.receive());
        }

        log.trace("Consumed message: " + msg);
        consumer.close();

        StompFrame sf;

        if (msg == null) {
            ByteArrayOutputStream baos = new ByteArrayOutputStream();
            PrintWriter stream = new PrintWriter(new OutputStreamWriter(baos, "UTF-8"));
            stream.print("No messages available");
            stream.close();

            Map<String, Object> eheaders = new HashMap<String, Object>();
            eheaders.put(Stomp.Headers.Error.MESSAGE, "timeout");

            sf = new StompFrame(Stomp.Responses.ERROR, eheaders, baos.toByteArray());
        } else {
            // Don't use sendResponse since it uses Stomp.Responses.RECEIPT as the action
            // which only allows zero length message bodies, Stomp.Responses.MESSAGE is correct:
            sf = session.convertMessage(msg);
        }

        if (headers.containsKey(Stomp.Headers.RECEIPT_REQUESTED))
            sf.getHeaders().put(Stomp.Headers.Response.RECEIPT_ID, headers.get(Stomp.Headers.RECEIPT_REQUESTED));

        sendToStomp(sf);
    }

    protected void onStompSubscribe(StompFrame command) throws Exception {
        checkConnected();

        if (subscriptions.size() > 0) {
            throw new ProtocolException("This connection already has a subscription");
        }

        Map<String, Object> headers = command.getHeaders();
        // We know this is going to be none-XA as the XA receive is handled in onStompReceive
        StompSession session = getSession();

        String subscriptionId = (String) headers.get(Stomp.Headers.Subscribe.ID);
        if (subscriptionId == null) {
            subscriptionId = createSubscriptionId(headers);
        }

        StompSubscription subscription = (StompSubscription) subscriptions.get(subscriptionId);
        if (subscription != null) {
            throw new ProtocolException("There already is a subscription for: " + subscriptionId
                    + ". Either use unique subscription IDs or do not create multiple subscriptions for the same destination");
        }
        subscription = new StompSubscription(session, subscriptionId, command);
        subscriptions.put(subscriptionId, subscription);

        sendResponse(command);
    }

    protected void onStompUnsubscribe(StompFrame command) throws Exception {
        checkConnected();
        Map<String, Object> headers = command.getHeaders();

        String destinationName = (String) headers.get(Stomp.Headers.Unsubscribe.DESTINATION);
        String subscriptionId = (String) headers.get(Stomp.Headers.Unsubscribe.ID);

        if (subscriptionId == null) {
            if (destinationName == null) {
                throw new ProtocolException("Must specify the subscriptionId or the destination you are unsubscribing from");
            }
            subscriptionId = createSubscriptionId(headers);
        }

        StompSubscription subscription = (StompSubscription) subscriptions.remove(subscriptionId);
        if (subscription == null) {
            throw new ProtocolException("Cannot unsubscribe as mo subscription exists for id: " + subscriptionId);
        }
        subscription.close();
        sendResponse(command);
    }

    protected void onStompAck(StompFrame command) throws Exception {
        checkConnected();

        // TODO: acking with just a message id is very bogus
        // since the same message id could have been sent to 2 different subscriptions
        // on the same stomp connection. For example, when 2 subs are created on the same topic.

        Map<String, Object> headers = command.getHeaders();
        String messageId = (String) headers.get(Stomp.Headers.Ack.MESSAGE_ID);
        if (messageId == null) {
            throw new ProtocolException("ACK received without a message-id to acknowledge!");
        }

        Connection connection = stoppedConnections.remove(messageId);
        if (connection == null) {
            log.warn("Could not start connection for message: " + messageId);
            throw new ProtocolException("No such message for message-id: " + messageId);
        }

        // PATCHED BY TOM FOR SINGLE MESSAGE DELIVERY
        log.debug("Started connection: " + connection);
        connection.start();
        sendResponse(command);
    }

    protected void checkConnected() throws ProtocolException {
        if (xaConnection == null) {
            throw new ProtocolException("Not connected.");
        }
        if (connection == null) {
            throw new ProtocolException("Not connected.");
        }
    }

    /**
     * Auto-create a subscription ID using the destination
     */
    protected String createSubscriptionId(Map<String, Object> headers) {
        return "/subscription-to/" + headers.get(Stomp.Headers.Subscribe.DESTINATION);
    }

    protected StompSession getSession() throws JMSException {
        if (this.session == null) {
            Session session = connection.createSession(false, Session.AUTO_ACKNOWLEDGE);
            if (log.isDebugEnabled()) {
                log.debug("Created session with ack mode: " + session.getAcknowledgeMode());
            }
            this.session = new StompSession(this, session, connection);
        }
        return session;
    }

    protected StompSession getXASession() throws JMSException {
        if (xaSession == null) {
            Session session = xaConnection.createXASession();
            if (log.isDebugEnabled()) {
                log.debug("Created XA session");
            }
            xaSession = new StompSession(this, session, xaConnection);
            log.trace("Created XA Session");
        } else {
            log.trace("Returned existing XA session");
        }
        return xaSession;
    }

    protected void sendResponse(StompFrame command) throws Exception {
        final String receiptId = (String) command.getHeaders().get(Stomp.Headers.RECEIPT_REQUESTED);
        // A response may not be needed.
        if (receiptId != null) {
            StompFrame sc = new StompFrame();
            sc.setAction(Stomp.Responses.RECEIPT);
            sc.setHeaders(new HashMap<String, Object>(1));
            sc.getHeaders().put(Stomp.Headers.Response.RECEIPT_ID, receiptId);
            sendToStomp(sc);
        }
    }

    protected void sendToStomp(StompFrame frame) throws Exception {
        if (log.isDebugEnabled()) {
            log.debug("<<<< " + frame.getAction() + " headers: " + frame.getHeaders());
        }
        outputHandler.onStompFrame(frame);
    }

    private static class JtsTransactionImple extends TransactionImple {

        /**
         * Construct a transaction based on an OTS control
         * 
         * @param wrapper the wrapped OTS control
         */
        public JtsTransactionImple(ControlWrapper wrapper) {
            super(new AtomicTransaction(wrapper));
            putTransaction(this);
        }
    }
}
