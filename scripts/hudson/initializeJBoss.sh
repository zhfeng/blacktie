if [ -z "${WORKSPACE}" ]; then
  echo "UNSET WORKSPACE"
  exit -1;
fi

# GET JBOSS AND INITIALIZE IT
if [ -d $WORKSPACE/jboss-as-7.1.0.Final ]; then
  rm -rf $WORKSPACE/jboss-as-7.1.0.Final
fi
if [ -d $WORKSPACE/jbossesb-4.9 ]; then
  rm -rf $WORKSPACE/jbossesb-4.9
fi

cd $WORKSPACE
if [ -e $WORKSPACE/jboss-as-7.1.0.Final.zip ]; then
	echo "JBoss already downloaded"
else
	wget http://albany/userContent/jboss-as-7.1.0.Final.zip
fi
unzip jboss-as-7.1.0.Final.zip
if [ -e $WORKSPACE/jbossesb-4.9.zip ]; then
	echo "JBossESB already downloaded"
else
	wget http://albany/userContent/blacktie/jbossesb-4.9.zip
fi
echo 'A
' | unzip jbossesb-4.9.zip

# INSTALL TRANSACTIONS
# com.arjuna.orbportability.OrbPortabilityEnvironmentBean.bindMechanism
echo 'JAVA_OPTS="$JAVA_OPTS -DOrbPortabilityEnvironmentBean.resolveService=NAME_SERVICE"' >> $WORKSPACE/jboss-as-7.1.0.Final/bin/standalone.conf
sed -i 's?"spec"?"on"?g' $WORKSPACE/jboss-as-7.1.0.Final/standalone/configuration/standalone-full.xml
sed -i 's?<coordinator-environment default-timeout="300"/>?<coordinator-environment default-timeout="300"/>\
	    <jts/>?g' $WORKSPACE/jboss-as-7.1.0.Final/standalone/configuration/standalone-full.xml

sed -n '1h;1!H;${;g;s?<logger category="com.arjuna">\n                <level name="WARN"/>?<logger category="com.arjuna">\n                <level name="ALL"/>?g;p;}' $WORKSPACE/jboss-as-7.1.0.Final/standalone/configuration/standalone-full.xml > foo
mv foo $WORKSPACE/jboss-as-7.1.0.Final/standalone/configuration/standalone-full.xml

sed -i 's?            <root-logger>?            <logger category="org.jboss.narayana.blacktie">\
                <level name="ALL"/>\
            </logger>\
            <root-logger>?g' $WORKSPACE/jboss-as-7.1.0.Final/standalone/configuration/standalone-full.xml

sed -i 's?                </jms-destinations>?                    <jms-queue name="BTR_TestOne">\
                        <entry name="queue/BTR_TestOne"/>\
                        <entry name="java:jboss/exported/jms/queue/BTR_TestOne"/>\
                    </jms-queue>\
                    <jms-queue name="BTR_TestTwo">\
                        <entry name="queue/BTR_TestTwo"/>\
                        <entry name="java:jboss/exported/jms/queue/BTR_TestTwo"/>\
                    </jms-queue>\
                    <jms-queue name="BTC_ConvService">\
                        <entry name="queue/BTC_ConvService"/>\
                        <entry name="java:jboss/exported/jms/queue/BTC_ConvService"/>\
                    </jms-queue>\
                    <jms-queue name="BTR_JAVA_Converse">\
                        <entry name="queue/BTR_JAVA_Converse"/>\
                        <entry name="java:jboss/exported/jms/queue/BTR_JAVA_Converse"/>\
                    </jms-queue>\
                    <jms-topic name="BTR_JAVA_Topic">\
                        <entry name="topic/BTR_JAVA_Topic"/>\
                        <entry name="java:jboss/exported/jms/topic/BTR_JAVA_Topic"/>\
                    </jms-topic>\
                </jms-destinations>?g' $WORKSPACE/jboss-as-7.1.0.Final/standalone/configuration/standalone-full.xml


# CONFIGURE SECURITY FOR THE ADMIN SERVICES
sed -i 's?<security-settings>?<security-settings>\
      <security-setting match="jms.queue.BTR_BTDomainAdmin">\
         <permission type="send" roles="blacktie"/>\
         <permission type="consume" roles="blacktie"/>\
      </security-setting>\
      <security-setting match="jms.queue.BTR_BTStompAdmin">\
         <permission type="send" roles="blacktie"/>\
         <permission type="consume" roles="blacktie"/>\
      </security-setting>?g' $WORKSPACE/jboss-as-7.1.0.Final/standalone/configuration/standalone-full.xml

# CONFIGURE HORNETQ TO NOT USE CONNECTION BUFFERING and TO NOT TIMEOUT INVM CONNECTIONS
sed -i 's?<connection-factory name="InVMConnectionFactory">?<connection-factory name="InVMConnectionFactory">\
      <consumer-window-size>0</consumer-window-size>\
      <connection-ttl>-1</connection-ttl>\
      <client-failure-check-period>86400000</client-failure-check-period>?g' $WORKSPACE/jboss-as-7.1.0.Final/standalone/configuration/standalone-full.xml

#sed -i 's?<resourceadapter-class>org.hornetq.ra.HornetQResourceAdapter</resourceadapter-class>?<resourceadapter-class>org.hornetq.ra.HornetQResourceAdapter</resourceadapter-class>?
#      <config-property>?
#        <description>The connection TTL</description>?
#        <config-property-name>ConnectionTTL</config-property-name>?
#        <config-property-type>java.lang.Long</config-property-type>?
#        <config-property-value>-1</config-property-value>?
#      </config-property>?
#      <config-property>?
#        <description>The client failure check period</description>?
#        <config-property-name>ClientFailureCheckPeriod</config-property-name>?
#        <config-property-type>java.lang.Long</config-property-type>?
#        <config-property-value>86400000</config-property-value>?
#      </config-property>?g' $WORKSPACE/jboss-5.1.0.GA/server/all-with-hornetq/deploy/hornetq-ra.rar/META-INF/ra.xml

#INSTALL JBossESB
#cp $WORKSPACE/scripts/hudson/hornetq/jboss-as-hornetq-int.jar $WORKSPACE/jboss-5.1.0.GA/common/lib
#cp $WORKSPACE/scripts/hudson/hornetq/hornetq-deployers-jboss-beans.xml $WORKSPACE/jboss-5.1.0.GA/server/all-with-hornetq/deployers
#cd $WORKSPACE/jbossesb-4.9/install
#cp deployment.properties-example deployment.properties
#sed -i "s?/jbossesb-server-4.5.GA?$WORKSPACE/jboss-5.1.0.GA?" deployment.properties
#sed -i "s?=default?=all-with-hornetq?" deployment.properties
#sed -i "s?^org.jboss.esb.tomcat.home?#&?" deployment.properties
#sed -i 's?/hornetq?&.sar?' build.xml
#ant deploy

(cd $WORKSPACE/jboss-as-7.1.0.Final/bin/ && JBOSS_HOME= ./add-user.sh guest password -a)

sed -i 's?#guest=guest?guest=guest,blacktie?g' $WORKSPACE/jboss-as-7.1.0.Final/standalone/configuration/application-roles.properties
