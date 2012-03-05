rem CREATE A FILE TO DOWNLOAD JBOSS AND ORACLE DRIVER

rem DOWNLOAD THOSE DEPENDENCIES
cd %WORKSPACE%
call ant -f scripts/hudson/download.xml -Dbasedir=. download
IF %ERRORLEVEL% NEQ 0 exit -1

rem INITIALIZE HORNETQ
cd %WORKSPACE%\hornetq-2.1.2.Final\config\jboss-as-5\
set JBOSS_HOME=%WORKSPACE%\jboss-5.1.0.GA
call build.bat
set JBOSS_HOME=
cd %WORKSPACE%

rem INITIALIZE JBOSS
cd %WORKSPACE%\jboss-5.1.0.GA\docs\examples\transactions
call ant jts -Dtarget.server.dir=../../../server/all-with-hornetq
IF %ERRORLEVEL% NEQ 0 exit -1
cd %WORKSPACE%
call ant replaceJBoss -DJBOSSAS_IP_ADDR=%JBOSSAS_IP_ADDR%
IF %ERRORLEVEL% NEQ 0 exit -1

rem INITIALZE BLACKTIE JBOSS DEPENDENCIES
copy %WORKSPACE%\jatmibroker-xatmi\src\test\resources\hornetq-jms.xml %WORKSPACE%\jboss-5.1.0.GA\server\all-with-hornetq\conf
cd %WORKSPACE%
call ant configureHornetQ
IF %ERRORLEVEL% NEQ 0 exit -1

rem INITIALZE JBOSSESB
copy %WORKSPACE%\scripts\hudson\hornetq\jboss-as-hornetq-int.jar %WORKSPACE%\jboss-5.1.0.GA\common\lib
copy %WORKSPACE%\scripts\hudson\hornetq\hornetq-deployers-jboss-beans.xml %WORKSPACE%\jboss-5.1.0.GA\server\all-with-hornetq\deployers
copy %WORKSPACE%\jbossesb-4.9\install\deployment.properties-example %WORKSPACE%\jbossesb-4.9\install\deployment.properties
call ant configureESB -DWORKSPACE=%WORKSPACE:\=/%
IF %ERRORLEVEL% NEQ 0 exit -1
cd %WORKSPACE%\jbossesb-4.9\install
call ant deploy
IF %ERRORLEVEL% NEQ 0 exit -1
cd %WORKSPACE%
