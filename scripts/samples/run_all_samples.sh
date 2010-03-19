# ALLOW JOBS TO BE BACKGROUNDED
set -m

# RUN THE FOOAPP SERVER
cd $BLACKTIE_HOME/examples/xatmi/fooapp
generate_server.sh -Dservice.names=BAR -Dserver.includes=BarService.c
if [ "$?" != "0" ]; then
	exit -1
fi
export BLACKTIE_CONFIGURATION=linux
btadmin startup
unset BLACKTIE_CONFIGURATION
if [ "$?" != "0" ]; then
	exit -1
fi

# RUN THE JAVA CLIENT
cd $BLACKTIE_HOME/examples/jab
echo hello | mvn test
if [ "$?" != "0" ]; then
	exit -1
fi

# RUN THE C CLIENT
cd $BLACKTIE_HOME/examples/xatmi/fooapp
generate_client.sh -Dclient.includes=client.c
./client
if [ "$?" != "0" ]; then
	killall -9 server
	exit -1
fi

# RUN THE ADMIN JMX CLIENT
cd $BLACKTIE_HOME/examples/admin/jmx
echo '0
0
0
0
1' | mvn test
if [ "$?" != "0" ]; then
	exit -1
fi
# PICK UP THE CLOSING SERVER
sleep 3

# RUN THE FOOAPP SERVER AGAIN
cd $BLACKTIE_HOME/examples/xatmi/fooapp
export BLACKTIE_CONFIGURATION=linux
btadmin startup
unset BLACKTIE_CONFIGURATION
if [ "$?" != "0" ]; then
	exit -1
fi

# SHUTDOWN THE SERVER RUNNING THE XATMI ADMIN CLIENT
cd $BLACKTIE_HOME/examples/admin/xatmi
generate_client.sh -Dclient.includes=client.c
echo '0
0
0
0
1' | ./client
# PICK UP THE CLOSING SERVER
sleep 3

# RUN THE SECURE SERVER
cd $BLACKTIE_HOME/examples/security
generate_server.sh -Dservice.names=SECURE -Dserver.includes=BarService.c
if [ "$?" != "0" ]; then
	exit -1
fi
export BLACKTIE_CONFIGURATION_DIR=serv
export BLACKTIE_CONFIGURATION=linux
btadmin startup secure
unset BLACKTIE_CONFIGURATION
if [ "$?" != "0" ]; then
	exit -1
fi
unset BLACKTIE_CONFIGURATION_DIR

# RUN THE "guest" USER CLIENT
generate_client.sh -Dclient.includes=client.c
export BLACKTIE_CONFIGURATION_DIR=guest
./client
# This test is expected to fail so make sure the exit status was not 0
if [ "$?" == "0" ]; then
	exit -1
fi
unset BLACKTIE_CONFIGURATION_DIR

# RUN THE "dynsub" USER CLIENT
export BLACKTIE_CONFIGURATION_DIR=dynsub
./client
if [ "$?" != "0" ]; then
	exit -1
fi
unset BLACKTIE_CONFIGURATION_DIR

# SHUTDOWN THE SERVER RUNNING THE XATMI ADMIN CLIENT
export BLACKTIE_CONFIGURATION=linux
btadmin shutdown secure
unset BLACKTIE_CONFIGURATION
if [ "$?" != "0" ]; then
	exit -1
fi

# RUN THE MDB EXAMPLE
cd $BLACKTIE_HOME/examples/mdb
mvn install
if [ "$?" != "0" ]; then
	exit -1
fi

# LET THE USER KNOW THE OUTPUT
echo "All samples ran OK"