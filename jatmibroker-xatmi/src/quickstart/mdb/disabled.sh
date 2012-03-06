# ALLOW JOBS TO BE BACKGROUNDED
set -m

echo "Running MDB quickstart"

# RUN THE MDB EXAMPLE
cd $BLACKTIE_HOME/quickstarts/mdb
mvn clean install jboss-as:deploy -DskipTests
if [ "$?" != "0" ]; then
	exit -1
fi
sleep 10
mvn surefire:test
if [ "$?" != "0" ]; then
	exit -1
fi
