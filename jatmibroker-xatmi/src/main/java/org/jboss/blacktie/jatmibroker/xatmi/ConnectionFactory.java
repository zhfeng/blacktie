package org.jboss.blacktie.jatmibroker.xatmi;

import java.util.Properties;

import org.jboss.blacktie.jatmibroker.core.conf.AtmiBrokerEnvXML;
import org.jboss.blacktie.jatmibroker.core.conf.ConfigurationException;

/**
 * This is a factory that will create connections to remote Blacktie services.
 * 
 * @see Connection
 * @see ConnectionException
 */
public class ConnectionFactory {

	/**
	 * The properties inside the connection factory.
	 */
	private Properties properties = new Properties();

	/**
	 * The connection factory will allocate a connection per thread.
	 */
	private static ThreadLocal<Connection> connections = new ThreadLocal<Connection>();

	/**
	 * Get the default connection factory
	 * 
	 * @param applicationName
	 *            The name of the application (used to ensure new applications
	 *            load their own btconfig.xml)
	 * @return The connection factory
	 * @throws ConfigurationException
	 *             If the configuration cannot be parsed.
	 */
	public static synchronized ConnectionFactory getConnectionFactory(
			String applicationName) throws ConfigurationException {
		return new ConnectionFactory(applicationName);
	}

	/**
	 * Create the connection factory
	 * 
	 * @throws ConfigurationException
	 *             In case the configuration could not be loaded
	 */
	private ConnectionFactory(String applicationName)
			throws ConfigurationException {
		AtmiBrokerEnvXML xml = new AtmiBrokerEnvXML();
		properties.putAll(xml.getProperties(applicationName));
	}

	/**
	 * Get the connection for this thread.
	 * 
	 * @return The connection for this thread.
	 * @throws ConfigurationException
	 */
	public Connection getConnection() throws ConfigurationException {
		Connection connection = connections.get();
		if (connection == null) {
			connection = new Connection(this, properties);
			connections.set(connection);
		}
		return connection;
	}

	/**
	 * Remove the connection from the factory after closure.
	 * 
	 * @param connection
	 *            The connection to remove.
	 */
	void removeConnection(Connection connection) {
		connections.set(null);
	}
}
