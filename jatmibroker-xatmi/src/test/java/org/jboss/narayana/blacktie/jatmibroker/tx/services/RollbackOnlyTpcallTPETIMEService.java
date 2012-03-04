package org.jboss.narayana.blacktie.jatmibroker.tx.services;

import org.apache.log4j.LogManager;
import org.apache.log4j.Logger;
import org.jboss.narayana.blacktie.jatmibroker.core.conf.ConfigurationException;
import org.jboss.narayana.blacktie.jatmibroker.xatmi.Connection;
import org.jboss.narayana.blacktie.jatmibroker.xatmi.ConnectionException;
import org.jboss.narayana.blacktie.jatmibroker.xatmi.Response;
import org.jboss.narayana.blacktie.jatmibroker.xatmi.Service;
import org.jboss.narayana.blacktie.jatmibroker.xatmi.TPSVCINFO;
import org.jboss.narayana.blacktie.jatmibroker.xatmi.X_OCTET;

public class RollbackOnlyTpcallTPETIMEService implements Service {
    private static final Logger log = LogManager.getLogger(RollbackOnlyTpcallTPETIMEService.class);

    public Response tpservice(TPSVCINFO svcinfo) throws ConnectionException, ConfigurationException {
        try {
            int timeout = 21;
            log.info("test_tpcall_TPETIME_service, sleeping for " + timeout + " seconds");
            Thread.sleep(timeout * 1000);
            log.info("test_tpcall_TPETIME_service, slept for " + timeout + " seconds");

            int len = 60;
            X_OCTET toReturn = (X_OCTET) svcinfo.getConnection().tpalloc("X_OCTET", null, len);
            toReturn.setByteArray("test_tpcall_TPETIME_service".getBytes());
            return new Response(Connection.TPSUCCESS, 0, toReturn, 0);
        } catch (InterruptedException e) {
            return new Response(Connection.TPFAIL, 0, null, 0);
        }
    }
}
