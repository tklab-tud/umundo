# Building from Source

The source code is build using CMake, see the dependencies below and install all the required packages, then see the platform
notes to build uMundo on your platform.

## Build Dependencies

<table>
    <tr><th>Platform</th><th>Dependency</th><th>Version</th><th>Comment</th></tr>
	<tr>
		<td rowspan="6">*Everyone*</td>
			<td><a href="http://www.cmake.org/cmake/resources/software.html">CMake</a><br />required</td>
			<td>>=&nbsp;2.8.6</td>
			<td>The build-system used to build umundo from sources.</td></tr>
		<tr>
			<td><a href="http://www.swig.org/">SWIG</a><br />optional</td>
			<td>>=&nbsp;2.0.5</td>
			<td>Wraps the umundo C/C++ code for Java and other languages. Make sure to get version 2.0.5, older ones won't do.</td></tr>
		<tr>
			<td><a href="http://www.oracle.com/technetwork/java/javase/downloads/index.html">Java Developer Kit</a><br />optional</td>
			<td>>=&nbsp;>= 5</td>
			<td>The JDK is required for the JNI wrappers and the Java bindings.</td></tr>
		<tr>
			<td><a href="http://ant.apache.org/bindownload.cgi">Ant</a><br />optional</td>
			<td>>=&nbsp;>= 1.8.x</td>
			<td>Build system used for the Java bindings.</td></tr>
		<tr>
			<td><a href="http://code.google.com/p/protobuf/">Protocol&nbsp;Buffers</a><br />required s11n</td>
			<td>2.4.1 works</td>
			<td>Object serializer currently used.</td></tr>
		<tr>
			<td><a href="http://www.pcre.org/">PCRE</a><br />required rpc</td>
			<td>7.0 works</td>
			<td>Regular expressions implementation for service queries. There are <a href="http://sourceforge.net/projects/gnuwin32/files/pcre/7.0/pcre-7.0.exe/download">pre-built binaries for windows</a> available.</td></tr>
		<tr>
			<td><a href="http://git-scm.com/">Git</a><br />required</td>
			<td></td>
			<td>Versioning control system.</td></tr>
		<tr>
			<td><a href="http://www.stack.nl/~dimitri/doxygen/">Doxygen</a><br />optional</td>
			<td></td>
			<td>Used by <tt>make docs</tt> to generate documentation from source comments.</td></tr>
	</tr>
	<tr>
		<td rowspan="2">Mac OSX</td>
			<td><a href="http://developer.apple.com/xcode/">XCode</a><br />required</td>
			<td>4.2.1 works</td>
			<td>Apples SDK with all the toolchains.</td></tr>
		<tr>
			<td><a href="http://www.macports.org/">MacPorts</a><br />recommended</td>
			<td>>= 2.0.3</td>
			<td>Build system for a wide selection of open-source packages.</td></tr>

	</tr>
	<tr>
	<td rowspan="2">Windows</td>
		<td><a href="http://www.microsoft.com/visualstudio/en-us">Visual&nbsp;Studio&nbsp;10</a><br />required</td>
		<td>v10 pro works</td>
		<td>As a student, you can get your version through MSAA.</td></tr>
	<tr>
		<td>
			<a href="http://support.apple.com/kb/DL999">Bonjour Print Wizard</a> or 
			<a href="http://www.apple.com/itunes/download/">iTunes</a><br />optional</td>
		<td></td>
		<td>If you plan to use the system wide Bonjour service, you will need a mDNSResponder daemon contained in both distributions.
			The uMundo libraries from the installers contain an embedded mDNS implementation.</td></tr>
	</tr>
	<tr>
	<td rowspan="3">Linux</td>
		<td>Build system<br />required</td>
		<td>g++&nbsp;>=&nbsp;4.4<br />make&nbsp;>=3.81</td>
		<td>For Debian:<br /><tt>$ sudo apt-get install cmake cmake-curses-gui make g++</tt></td></tr>
	<tr>
		<td>JDK<br />optional</td>
		<td>>= 6.x</td>
		<td>For Debian with <emph>non-free<emph>:<br /><tt>$ sudo apt-get install sun-java6-jdk</tt></td></tr>
	<tr>
		<td>Avahi<br />required</td>
		<td>3.x works</td>
		<td>For Debian:<br /><tt>$ sudo apt-get install avahi-daemon libavahi-client-dev</tt></td></tr>
	</tr>
</table>


The process of building umundo is essentially the same on every platform:

1. Resolve build dependencies for your platform given in table above.
2. Checkout the umundo sources into a convenient directory:

	<tt>git clone git@github.com:tklab-tud/umundo.git</tt>

3. Create a new directory for an *out-of-source* build.
4. Run cmake to create the artifacts for your preferred build-system or development environment.
5. Use your actual build-environment or development environment to build umundo.

If you want to build for another IDE or build-environment, just empty the *out-of-source* folder and start over with cmake. To
get an idea of supported IDEs and build-environments on your platform, type <tt>cmake --help</tt> and look for the *Generators*
section at the end of the output.

## Platform Notes

### Mac OSX

#### Console / Make

	$ cd /somewhere/convenient/
	$ git clone git@github.com:tklab-tud/umundo.git
	$ mkdir umundo/build && cd umundo/build
	$ cmake ..
	[...]
	-- Build files have been written to: /somewhere/convenient/umundo/build
	$ make

You can test whether everything works by starting one of the sample programs:

	$ ./bin/umundo-pingpong &
	$ ./bin/umundo-pingpong &
	oioioioioi
	[...]
	$ killall umundo-pingpong
	
#### Xcode
	
	$ cd /somewhere/even/more/convenient/
	$ git clone git@github.com:tklab-tud/umundo.git
	$ mkdir umundo/build && cd umundo/build
	$ cmake -G Xcode ..
	[...]
	-- Build files have been written to: /somewhere/convenient/umundo/build
	$ open umundo.xcodeproj

You can of course reuse the same source directory for many build directories.

### Linux

#### Console / Make

Instructions are literally a verbatim copy of building umundo for MacOSX on the console.

#### Eclipse CDT

	$ cd /somewhere/convenient/
	$ git clone git@github.com:tklab-tud/umundo.git
	$ mkdir umundo/build && cd umundo/build
	$ cmake -G "Eclipse CDT4 - Unix Makefiles" ..
	[...]
	-- Build files have been written to: /somewhere/convenient/umundo/build

Now open Eclipse CDT and import the out-of-source directory as an existing project into workspace, leaving the "Copy projects 
into workspace" checkbox unchecked. There are some more [detailed instruction](http://www.cmake.org/Wiki/Eclipse_CDT4_Generator) available 
in the cmake wiki as well.

#### Debian Stable - Complete Walkthrough

I downloaded and installed a *fresh* installation of Debian GNU/Linux 6.0.4 stable for i386 from the netinst.iso, here are all the 
steps required to arrive at a static <tt>libumundocore.a</tt> (<tt>sudo</tt> is no actually installed by default, install it and 
add yourself to the <tt>sudo</tt> group or use a root console):

    $ sudo apt-get install cmake g++ avahi-daemon libavahi-client-dev

Then build as described above - this is everything you need to compile a static <tt>libumundocore.a</tt>. If you want to generate 
the Java bindings, you also need:
- a JDK 
- >= SWIG 2.0.5 (prior versions lacked some typemaps for directors in Java)
- >= CMake 2.8.6 (Java support was only added in 2.8.6)

I had some mixed experiences with GCJ in the past, so I went ahead, enabled *non-free* in <tt>/etc/apt/sources.list</tt> 
and installed SUNs Java implementation:

    $ sudo apt-get install sun-java6-jdk 

To generate the wrappers for JNI, checkout and build the current SWIG distribution:

    $ sudo apt-get install subversion autoconf libpcre3-dev bison wget
    $ wget http://prdownloads.sourceforge.net/swig/swig-2.0.6.tar.gz
    $ tar xvzf swig-2.0.6.tar.gz
    $ cd swig-2.0.6/
    $ ./configure
    $ make
    $ sudo make install
    $ swig -version

This ought to yield version 2.0.5 or higher. Now all we need is a current CMake version:

    $ sudo apt-get remove cmake cmake-data
    $ wget http://www.cmake.org/files/v2.8/cmake-2.8.8.tar.gz
    $ tar xvzf cmake-2.8.8.tar.gz
    $ cd cmake-2.8.8/
    $ ./configure
    $ make
    $ sudo make install
    $ cmake --version

This should say <tt>cmake version 2.8.8</tt>. If you get the bash complaining about not finding cmake, logout and login again. Now
you got everything needed to compile Java wrappers. 

### Windows

Building from source on windows is somewhat more involved and instructions are necessarily in prose form.

1. Use git to checkout the source from <tt>git@github.com:tklab-tud/umundo.git</tt> into any convenient directory.
2. Start the CMake-GUI and enter the checkout directory in the "Where is the source code" text field.
3. Choose any convenient directory to build the binaries in.
4. Hit "Configure" and choose your toolchain and compiler - I only tested with Visual Studio 10.
I can say for sure that MinGW won't work for now, as we do not have prebuilt libraries for the the MinGW compiler yet.
	1. CMake will complain about missing SWIG executable (see above for building a current SWIG on windows)
	2. CMake will also complain about protobuf, but the source distribution from Google contains a solution file for Visual Studio
	so you could build it easily. Just point CMake to the directory where you built protobuf by setting the
	<tt>PROTOBUF_SRC_ROOT_FOLDER</tt> field and be done.
5. Get Swig 2.0.6 or later and make sure it swig.exe is in the %PATH.
6. Press "Configure" some more until no fields are marked red, then press "Generate".
7. Navigate to the directory where you told CMake-GUI to build the libraries and find a umundo solution file for MS Visual Studio
there.
8. Open the solution with MS Visual Studio. Only <tt>Debug</tt> and <tt>Release</tt> builds are supported for now.

### Cross Compiling

Cross compiling for Android and iOS is best done with the <tt>build-umundo-*</tt> scripts in <tt>contrib</tt>. You have to make 
sure that CMake can find <tt>protoc-umundo-cpp-rpc</tt> and <tt>protoc-umundo-java-rpc</tt> on your system, both can be build
and installed with a host-native (non cross-compiled) installation first.

Cross Compiling for Android on Windows is possible but the process is not wrapped in a script yet. Have a look at the unix shell 
scripts to see what's needed.

## Build process

We are using CMake to build uMundo for every platform. When <tt>cmake</tt> is invoked, it will look for a <tt>CMakeLists.txt</tt>
file in the given directory and prepare a build in the current directory. CMake itself can be considered as a meta build-system as
it will only generate the artifacts required for an actual build-system. The default is to generate files for <tt>make</tt> on 
unices and files for <tt>nmake</tt> on windows. If you invoke <tt>ccmake</tt> instead of <tt>cmake</tt>, you get an user interfaces
to set some variables related to the build:

#### What to build

<dt><b>CMAKE_BUILD_TYPE</b></dt>
<dd>Only <tt>Debug</tt> and <tt>Release</tt> are actually supported. In debug builds, all asserts are stripped and the default
	log-level is decreased.</dd>

<dt><b>BUILD_PREFER_STATIC_LIBRARIES</b></dt>
<dd>Prefer static libraries in <tt>contrib/prebuilt/</tt> or system supplied libraries as found by CMake.</dd>

<dt><b>BUILD_STATIC_LIBRARIES</b></dt>
<dd>Create the uMundo libraries as static libraries. This does not apply to the JNI library which needs to be a shared library for
	Java.</dd>

<dt><b>BUILD_TESTING</b></dt>
<dd>Build the test executables.</dd>

<dt><b>BUILD_UMUNDO_APPS</b></dt>
<dd>Include the <tt>apps/</tt> directory when building.</dd>

<dt><b>BUILD_UMUNDO_RPC</b></dt>
<dd>Build the <tt>umundorpc</tt> library for remote procedure calls via uMundo.</dd>

<dt><b>BUILD_UMUNDO_S11N</b></dt>
<dd>Build the <tt>umundos11n</tt> library for object serialization. Only Googles ProtoBuf is supported as o now.</dd>

<dt><b>BUILD_UMUNDO_UTIL</b></dt>
<dd>Build <tt>umundoutil</tt> with some growing set of convenience services.</dd>

<dt><b>DIST_PREPARE</b></dt>
<dd>Put all libraries and binaries into SOURCE_DIR/package/ to prepare a release. We need access to all artifacts from other 
	platforms to create the installers with platform independent JARs and cross-compiled mobile platforms.</dd>

#### Implementations

<dt><b>DISC_AVAHI</b></dt>
<dd>Use the Avahi ZeroConf implementation with umundocore found on modern Linux distributions.</dd>

<dt><b>DISC_BONJOUR</b></dt>
<dd>Use the Bonjour ZeroConf implementation found on every MacOSX installation and every iOS device.</dd>

<dt><b>DISC_BONJOUR_EMBED</b></dt>
<dd>Embed the Bonjour ZeroConf implementation into umundocore. This is the default for Android and Windows.</dd>

<dt><b>NET_ZEROMQ</b></dt>
<dd>Use ZeroMQ to connect nodes to each other and publishers to subscribers.</dd>

<dt><b>NET_ZEROMQ_RCV_HWM, NET_ZEROMQ_SND_HWM</b></dt>
<dd>High water mark for ZeroMQ queues in messages. One uMundo message represents multiple ZeroMQ messages, one per meta field and 
	one for the actual data.</dd>

<dt><b>S11N_PROTOBUF</b></dt>
<dd>Use Google's ProtoBuf to serialize objects.</dd>

<dt><b>RPC_PROTOBUF</b></dt>
<dd>Use Google's ProtoBuf to call remote methods.</dd>

### CMake files

Throughout the source, there are <tt>CMakeLists.txt</tt> build files for CMake. The topmost build file will call the build files
from the directories directly contained via <tt>add_directory</tt>, which in turn call build files further down the directory 
structure.

    uMundo
     |-CMakeLists.txt 
     |          Uppermost CMakeLists.txt to setup the project with all variables listed above.
     |          Includes contrib/cmake/ as the module path for CMake modules.
     |          Defines where built files will end up.
     |          Configures additional search paths for libraries and executables.
     |          Sets global compiler flags.
     |          Uses ant to build the JAR for Java.
     |
     |-apps/CMakeLists.txt
     |          Invokes CMakeLists.txt in the sub-directories to build all apps if their dependencies are met. 
     |
     |-core/CMakeLists.txt
     |-s11n/CMakeLists.txt
     |-rpc/CMakeLists.txt
     |          Gather all source files for the respective component into a library.
     |          Find and link all required libraries either as prebuilts or system supplied libraries.
     |          Call INSTALL* CMake macros to register files for install and package targets.
     |
     |-core/bindings/CMakeLists.txt
     |          Find SWIG to build the bindings.
     |
     |-core/bindings/java/CMakeLists.txt
     |          Find the JNI libraries and use SWIG to build the Java wrappers.
