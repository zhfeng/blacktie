set NOPAUSE=true

rem SHUTDOWN ANY PREVIOUS BUILD REMNANTS
rem if exist jboss-7.1.0.Final call jboss-7.1.0.Final\bin\shutdown.bat -s %JBOSSAS_IP_ADDR%:1099 -S && cd .
rem if exist jboss-7.1.0.Final @ping 127.0.0.1 -n 60 -w 1000 > nul
tasklist
taskkill /F /IM mspdbsrv.exe
taskkill /F /IM testsuite.exe
taskkill /F /IM server.exe
taskkill /F /IM client.exe
taskkill /F /IM cs.exe
tasklist

rem INITIALIZE JBOSS
call %WORKSPACE%\scripts\hudson\initializeJBoss.bat
IF %ERRORLEVEL% NEQ 0 exit -1

rem START JBOSS
cd %WORKSPACE%\jboss-7.1.0.Final\bin
start /B standalone.bat -c standalone-full.xml -Djboss.bind.address=%JBOSSAS_IP_ADDR% -Djboss.bind.address.management=0.0.0.0
echo "Started server"
@ping 127.0.0.1 -n 20 -w 1000 > nul

rem BUILD BLACKTIE
cd %WORKSPACE%
call build.bat clean install "-Dbpa=vc9x32" "-Djbossas.ip.addr=%JBOSSAS_IP_ADDR%"
IF %ERRORLEVEL% NEQ 0 echo "Failing build 2" & exit -1

rem CREATE BLACKTIE DISTRIBUTION
cd %WORKSPACE%\scripts\test
for /f "delims=" %%a in ('hostname') do @set MACHINE_ADDR=%%a
call ant dist -DBT_HOME=%WORKSPACE%\dist\ -DVERSION=blacktie-5.0.0.M2-SNAPSHOT -DJBOSSAS_IP_ADDR=%JBOSSAS_IP_ADDR% -DMACHINE_ADDR=%MACHINE_ADDR% -Dbpa=vc9x32
IF %ERRORLEVEL% NEQ 0 echo "Failing build 3" & exit -1
rem tasklist & call %WORKSPACE%\jboss-7.1.0.Final\bin\shutdown.bat -s %JBOSSAS_IP_ADDR%:1099 -S & echo "Failed build" & 

rem RUN THE SAMPLES
cd %WORKSPACE%
call ant initializeBlackTieQuickstartSecurity
cd %WORKSPACE%\dist\blacktie-5.0.0.M2-SNAPSHOT
IF %ERRORLEVEL% NEQ 0 echo "Failing build 4" & exit -1

set ORACLE_HOME=C:\hudson\workspace\blacktie-windows2003\instantclient_11_2
set TNS_ADMIN=C:\hudson\workspace\blacktie-windows2003\instantclient_11_2\network\admin
set PATH=%PATH%;%ORACLE_HOME%\bin;%ORACLE_HOME%\vc9

set PATH=%PATH%;%WORKSPACE%\tools\maven\bin

echo calling generated setenv - error %ERRORLEVEL%
dir setenv.bat
call setenv.bat
IF %ERRORLEVEL% NEQ 0 echo "Failing build 5 with error %ERRORLEVEL%" & exit -1
copy /Y %WORKSPACE%\dist\blacktie-5.0.0.M2-SNAPSHOT\quickstarts\xatmi\security\hornetq-*.properties %WORKSPACE%\jboss-7.1.0.Final\server\all-with-hornetq\conf\props
IF %ERRORLEVEL% NEQ 0 echo "Failing build 6 with error %ERRORLEVEL%" & exit -1
call run_all_quickstarts.bat tx
IF %ERRORLEVEL% NEQ 0 echo "Failing build 7 with error %ERRORLEVEL%" & exit -1

rem SHUTDOWN ANY PREVIOUS BUILD REMNANTS
tasklist
rem call %WORKSPACE%\jboss-7.1.0.Final\bin\shutdown.bat -s %JBOSSAS_IP_ADDR%:1099 -S && cd .
rem @ping 127.0.0.1 -n 60 -w 1000 > nul
rem taskkill /F /IM mspdbsrv.exe
rem taskkill /F /IM testsuite.exe
rem taskkill /F /IM server.exe
rem taskkill /F /IM client.exe
rem taskkill /F /IM cs.exe
rem tasklist
echo "Finished build"
