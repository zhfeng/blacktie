ulimit -c unlimited

# CHECK IF WORKSPACE IS SET
if [ -n "${WORKSPACE+x}" ]; then
  echo WORKSPACE is set
else
  echo WORKSPACE not set
  exit
fi

set NOPAUSE=true

# KILL ANY PREVIOUS BUILD REMNANTS
ps -f
for i in `ps -eaf | grep java | grep "standalone-full.xml" | grep -v grep | cut -c10-15`; do kill -9 $i; done
killall -9 testsuite
killall -9 server
killall -9 client
killall -9 cs
ps -f

# GET THE TNS NAMES
TNS_ADMIN=$WORKSPACE/instantclient_11_2/network/admin
mkdir -p $TNS_ADMIN
if [ -e $TNS_ADMIN/tnsnames.ora ]; then
	echo "tnsnames.ora already downloaded"
else
	(cd $TNS_ADMIN; wget http://albany/userContent/blacktie/tnsnames.ora)
fi

# INITIALIZE JBOSS
. $WORKSPACE/scripts/hudson/initializeJBoss.sh
if [ "$?" != "0" ]; then
	exit -1
fi

# START JBOSS
$WORKSPACE/jboss-as-7.1.0.Final/bin/standalone.sh -c standalone-full.xml -Djboss.bind.address=$JBOSSAS_IP_ADDR -Djboss.bind.address.management=0.0.0.0&
sleep 15

# BUILD BLACKTIE
cd $WORKSPACE
JBOSS_HOME=$WORKSPACE/jboss-as-7.1.0.Final ./build.sh clean install -Duse.valgrind=false -Djbossas.ip.addr=$JBOSSAS_IP_ADDR
if [ "$?" != "0" ]; then
	ps -f
	for i in `ps -eaf | grep java | grep "standalone-full.xml" | grep -v grep | cut -c10-15`; do kill -9 $i; done
	killall -9 testsuite
	killall -9 server
	killall -9 client
	killall -9 cs
  ps -f
	exit -1
fi

# INITIALIZE THE BLACKTIE DISTRIBUTION
cd $WORKSPACE/scripts/test
ant dist -DBT_HOME=$WORKSPACE/dist/ -DVERSION=blacktie-5.0.0.M2-SNAPSHOT -DMACHINE_ADDR=`hostname` -DJBOSSAS_IP_ADDR=localhost -Dbpa=centos54x64
if [ "$?" != "0" ]; then
	ps -f
	for i in `ps -eaf | grep java | grep "standalone-full.xml" | grep -v grep | cut -c10-15`; do kill -9 $i; done
	killall -9 testsuite
	killall -9 server
	killall -9 client
	killall -9 cs
  ps -f
	exit -1
fi

# RUN ALL THE SAMPLES
cd $WORKSPACE/dist/blacktie-5.0.0.M2-SNAPSHOT/
chmod 775 setenv.sh
. setenv.sh
if [ "$?" != "0" ]; then
	ps -f
	for i in `ps -eaf | grep java | grep "standalone-full.xml" | grep -v grep | cut -c10-15`; do kill -9 $i; done
	killall -9 testsuite
	killall -9 server
	killall -9 client
	killall -9 cs
  ps -f
	exit -1
fi
export ORACLE_HOME=/usr/lib/oracle/11.2/client64
export ORACLE_LIB_DIR=/usr/lib/oracle/11.2/client64/lib
export ORACLE_INC_DIR=/usr/include/oracle/11.2/client64
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$ORACLE_LIB_DIR
export TNS_ADMIN=$WORKSPACE/instantclient_11_2/network/admin

export DB2DIR=/opt/ibm/db2/V9.7
export DB2_LIB=$DB2DIR/lib64
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$DB2_LIB

export PATH=$PATH:$WORKSPACE/tools/maven/bin

#cp $WORKSPACE/dist/blacktie-5.0.0.M2-SNAPSHOT/quickstarts/xatmi/security/hornetq-*.properties $WORKSPACE/jboss-5.1.0.GA/server/all-with-hornetq/conf/props
#sed -i 's?</security-settings>?      <security-setting match="jms.queue.BTR_SECURE">\
#         <permission type="send" roles="blacktie"/>\
#         <permission type="consume" roles="blacktie"/>\
#      </security-setting>\
#</security-settings>?g' $WORKSPACE/jboss-5.1.0.GA/server/all-with-hornetq/deploy/hornetq.sar/hornetq-configuration.xml

./run_all_quickstarts.sh tx
if [ "$?" != "0" ]; then
	ps -f
	for i in `ps -eaf | grep java | grep "standalone-full.xml" | grep -v grep | cut -c10-15`; do kill -9 $i; done
	killall -9 testsuite
	killall -9 server
	killall -9 client
	killall -9 cs
  ps -f
	exit -1
fi

# KILL ANY BUILD REMNANTS
ps -f
for i in `ps -eaf | grep java | grep "standalone-full.xml" | grep -v grep | cut -c10-15`; do kill -9 $i; done
killall -9 testsuite
killall -9 server
killall -9 client
killall -9 cs
ps -f
