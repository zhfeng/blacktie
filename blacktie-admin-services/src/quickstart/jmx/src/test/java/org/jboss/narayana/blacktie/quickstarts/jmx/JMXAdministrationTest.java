package org.jboss.narayana.blacktie.quickstarts.jmx;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.Hashtable;
import java.util.Iterator;
import java.util.List;

import javax.management.InstanceNotFoundException;
import javax.management.MBeanException;
import javax.management.MBeanServerConnection;
import javax.management.MalformedObjectNameException;
import javax.management.ObjectName;
import javax.management.ReflectionException;
import javax.management.remote.JMXConnector;
import javax.management.remote.JMXConnectorFactory;
import javax.management.remote.JMXServiceURL;

import junit.framework.TestCase;

import org.jboss.narayana.blacktie.jatmibroker.core.conf.ConfigurationException;

/**
 * As this is an interactive test, the forkMode must be set to none is vital for the input to be received by maven
 */
public class JMXAdministrationTest extends TestCase {

    private InputStreamReader isr = new InputStreamReader(System.in);
    private BufferedReader br = new BufferedReader(isr);

    public void test() throws IOException, ConfigurationException, MalformedObjectNameException, NullPointerException,
            InstanceNotFoundException, MBeanException, ReflectionException {
        String url = "service:jmx:remoting-jmx://localhost:9999";
        System.out.println("usage: mvn test");
        System.out.println("warning: forkMode must be set to none, please see README");

        prompt("Start JBoss Application Server, the following url must be available \"" + url + "\"");

        JMXServiceURL u = new JMXServiceURL(url);
        Hashtable h = new Hashtable();
        String[] creds = new String[2];
        creds[0] = "admin";
        creds[1] = "password";
        h.put(JMXConnector.CREDENTIALS, creds);
        JMXConnector c = JMXConnectorFactory.connect(u, h);
        MBeanServerConnection beanServerConnection = c.getMBeanServerConnection();

        ObjectName blacktieAdmin = new ObjectName("jboss.blacktie:service=Admin");

        prompt("Start an XATMI server");

        List<String> listRunningServers = (ArrayList<String>) beanServerConnection.invoke(blacktieAdmin, "listRunningServers",
                null, null);
        output("listRunningServers", listRunningServers);

        if (!listRunningServers.isEmpty()) {

            String response = prompt("Enter the id of a server to get the instance numbers of");
            int index = Integer.parseInt(response);

            List<Integer> ids = (List<Integer>) beanServerConnection.invoke(blacktieAdmin, "listRunningInstanceIds",
                    new Object[] { listRunningServers.get(index) }, new String[] { "java.lang.String" });
            output("listRunningInstanceIds", ids);

            prompt("Start a second instance of the same server");

            ids = (List<Integer>) beanServerConnection.invoke(blacktieAdmin, "listRunningInstanceIds",
                    new Object[] { listRunningServers.get(index) }, new String[] { "java.lang.String" });
            output("listRunningInstanceIds", ids);

            response = prompt("Enter the instance id of the server you wish to shutdown");
            int id = Integer.parseInt(response);
            beanServerConnection.invoke(blacktieAdmin, "shutdown", new Object[] { listRunningServers.get(index), id },
                    new String[] { "java.lang.String", "int" });
        } else {
            System.err.println("ERROR: There were no running servers detected");
            throw new RuntimeException("ERROR: There were no running servers detected");
        }
    }

    private String prompt(String prompt) throws IOException {
        System.out.println("Please press return after you: " + prompt + "...");
        return br.readLine().trim();
    }

    private void output(String operationName, List list) {
        System.out.println("Output from: " + operationName);
        int i = 0;
        Iterator iterator = list.iterator();
        while (iterator.hasNext()) {
            System.out.println("Element: " + i + " Value: " + iterator.next());
            i++;
        }
    }
}
