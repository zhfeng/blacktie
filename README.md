A video showing this process on Blacktie 2.0 has been uploaded to Vimeo, this
process is very similar for 3.x - feel free to take a look:

[Building BlackTie](http://vimeo.com/13670341) from [Tom
Jenkinson](http://vimeo.com/user4205367) on [Vimeo](http://vimeo.com).

The following instructions indicate how to build the Blacktie components from
source. They assume that you are building on one of the following platforms:

  * Centos 5.4 64 bit (centosx64)
  * Centos 5.5 32 bit (centos55x32)
  * Visual C++ 9 32 bit (vc9x32) [Windows XP and Windows 2003]

If you are not on one of these platforms, or are building from source, you
will need to following the instructions on [Building Thirdparty
Dependencies](https://community.jboss.org/docs/DOC-15457). NOTE: Linux users
should be fine with one of the CentOS varients even if they are using a newer
distribution. [SunOS build
instructions](https://community.jboss.org/docs/DOC-16168) are in a separate
article.

  1. **DOWNLOAD TOOLS**

**ALL PLATFORMS**

1. Download Sun Java SE Development Kit 1.6.0_20 and ensure the bin folder is
in the path

[http://java.sun.com/javase/downloads/index.jsp](http://java.sun.com/javase/do
wnloads/index.jsp)

2. Download Apache Maven 3.0.3 and ensure the bin folder is added to the path

[http://maven.apache.org/download.html](http://maven.apache.org/download.html)

**WINDOWS ONLY**

Install Visual V++ 2008 Express Edition (o0nly used to provide the compiler
and linker on Windows)

The PATH environment variable will need the runtime DLLs in it

C:\Program Files\Microsoft Visual Studio
9.0\VC\redist\Debug_NonRedist\x86\Microsoft.VC90.DebugCRT

The PATH environment variable will also need the VC++ tools in it

C:\Program Files\Microsoft Visual Studio 9.0\VC\vcvarsall.bat

**LINUX ONLY (optional)  
**

Install Valgrind as per the instructions at: [http://community.jboss.org/wiki/
MemoryLeaksCheckwithValgrind](https://community.jboss.org/docs/DOC-13540)

You will need to install the gcc-c++ packages: yum install gcc-c++

  2. **BUILD PRODUCT**

Change directory to a location where you would like to check out BlackTie, the
folder that you check out will then be referred to as <BLACKTIE_HOME>:

**ANON SVN ACCESS**  

svn co [https://anonsvn.jboss.org/repos/blacktie/tags/<LATEST_TAG>](https://an
onsvn.jboss.org/repos/blacktie/tags/) <LATEST_TAG>

**COMMITTER SVN ACCESS**  
We suggest taking trunk for committers as upgrades are rapidly released:

svn co [https://svn.jboss.org/repos/blacktie/trunk](https://svn.jboss.org/repo
s/blacktie/trunk) product

**IMPORTANT NOTE: bpa  
**

The build uses a special user defined flag -Dbpa=<PLAT_ARCH>, the value of
this parameter is intended to be a readily understandable representation of
your platform and architecture. We currently use the following values:
centos54x64 (Linux 64 bit), centos55x32 (Linux 32 bit), vc9x32 (Windows 32
bit). You are free to use any value you like, but it must be used for all
builds and must match that of the available third party dependencies (from
either repository.jboss.org if possible, or
[built](https://community.jboss.org/docs/DOC-15457) if necessary).

**TO BUILD WITHOUT RUNNING TESTS  
**

To build without running tests:

If you are not using a standard platform such as CentOS 5.4 x54, CentOS 5.5
x32 or greater or Windows XP, you will need to define as profile as shown
[here](https://community.jboss.org/docs/DOC-13410).

**TO BUILD AND RUN TEST****S****  
**

To build with running tests:

    1. Change directory into <BLACKTIE_HOME>
    2. Ensure JAVA_HOME is set to the JDK install directory
    3. mvn install -Dbpa=<PLAT_ARCH> -DskipTests
    4. Run the JBoss AS server as can be configured for BlackTie by following the instructions at: [Deploying BlackTie](https://community.jboss.org/docs/DOC-13243) (DO NOT DEPLOY blacktie-admin-services or stompconnectservice)
    5. cp <BLACKTIE_HOME>/jatmibroker-xatmi/src/test/resources/jatmibroker-xatmi-test-service.xml <JBOSS_HOME>/server/all/deploy
    6. Ensure JAVA_HOME is set to the JDK install directory (so we can find the jni header files)
    7. mvn install -Dbpa=<PLAT_ARCH> [-Duse.valgrind=true|false] (If you are on Linux and do not want to run the tests under valgrind please supply the -Duse.valgrind=false command line parameter to maven)
  3. **VERIFY THE BUILD**

To check that the build is successful, you can run the samples which can be
found in the binaries (which come from */src/example and examples).
Instructions on how to get the examples running from a binary can be found at:
[Deploying Blacktie](https://community.jboss.org/docs/DOC-13243)

  4. **Integrating the Build into your Hudson CI environment**

Take a look at the following article to help you: [Integrating the Build into
Hudson](https://community.jboss.org/docs/DOC-13324)

