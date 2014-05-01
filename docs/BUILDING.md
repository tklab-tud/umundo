# Building from Source

The source code is built using CMake, the process of building umundo is
essentially the same on every platform:

1. Read the <b>[Platform Notes](#platform-notes)</b> below to prepare your system.
2. Checkout umundo into a convenient directory:

	<tt>git clone git://github.com/tklab-tud/umundo.git</tt>

3. Create a new directory for an *out-of-source* build. I usually create sub-directories
in <tt>&lt;UMUNDO_SRC&gt;/build/</tt>.
4. Run cmake (or ccmake / CMake-GUI) to create the files required by your actual build-system.
5. Use your actual build-system or development environment to build umundo.
6. Have a look at the <b>[Getting Started](https://github.com/tklab-tud/umundo/blob/master/docs/GETTING_STARTED.md)</b>
document to start using umundo.

If you want to build for another IDE or build-system, just create a new
*out-of-source* build directory and start over with cmake. To get an idea of
supported IDEs and build-environments on your platform, type <tt>cmake --help</tt>
or run the CMake-GUI and look for the *Generators* section at the end of the
output. Default on Unices is Makefiles.

# Build Dependencies

Overview of the umundo dependencies. See the [Platform Notes](#platform-notes) for details.

<table>
    <tr><th>Platform</th><th>Dependency</th><th>Version</th><th>Comment</th></tr>
	<tr>
		<td rowspan="9"><b>Everyone</b></td>
			<td bgcolor="#ffffdd"><a href="http://www.cmake.org/cmake/resources/software.html">CMake</a><br />required</td>
			<td>>=&nbsp;2.8.6</td>
			<td>The build-system used for umundo.</td></tr>
		<tr>
			<td bgcolor="#ffffdd"><a href="http://git-scm.com/">Git</a><br />required</td>
			<td></td>
			<td>Versioning control system.</td></tr>
		<tr>
			<td bgcolor="#ffffdd"><a href="http://code.google.com/p/protobuf/">Protocol&nbsp;Buffers</a><br />included</td>
			<td>2.4.1 works</td>
			<td>Object serializer currently used. You will have to build them from source in MS Windows (see platform notes).</td></tr>
		<tr>
			<td bgcolor="#ffffdd"><a href="http://www.pcre.org/">PCRE</a><br />included for non unices</td>
			<td>7.0 works</td>
			<td>Regular expressions implementation for service queries. At the moment in core as Regex.cpp. Prebuilt binaries for Windows and the mobile platforms are included.</tr>
		<tr>
			<td bgcolor="#ffffdd"><a href="http://code.google.com/p/protobuf/">ZeroMQ</a><br />included</td>
			<td>3.2</td>
			<td>Network socket abstractions, just use the prebuilt binaries that come with the umundo distribution.</td></tr>
		<tr>
			<td bgcolor="#ddffdd"><a href="http://www.swig.org/">SWIG</a><br />optional</td>
			<td>>=&nbsp;2.0.5</td>
			<td>Wraps the C/C++ code from umundo.core for Java, CSharp and potentially  other languages. Make sure to
				get version 2.0.5, older ones won't do.</tr>
		<tr>
			<td bgcolor="#ddffdd"><a href="http://www.oracle.com/technetwork/java/javase/downloads/index.html">Java Developer Kit</a><br />optional</td>
			<td>>=&nbsp;5</td>
			<td>The JDK is required for the umundo.core JNI wrappers and the Java bindings. Only useful if you plan to install SWIG as well.</td></tr>
		<tr>
			<td bgcolor="#ddffdd"><a href="http://ant.apache.org/bindownload.cgi">Ant</a><br />optional</td>
			<td>>=&nbsp;1.8.x</td>
			<td>Build system used for the Java bindings.</td></tr>
		<tr>
			<td bgcolor="#ddffff"><a href="http://www.stack.nl/~dimitri/doxygen/">Doxygen</a><br />recommended</td>
			<td></td>
			<td>Used by <tt>make docs</tt> to generate documentation from source comments.</td></tr>
	</tr>
	<tr bgcolor="grey"><td bgcolor="#dddddd" colspan="4"></td></tr>

	<tr>
		<td rowspan="2"><b>Mac OSX</b></td>
			<td bgcolor="#ffffdd"><a href="http://developer.apple.com/xcode/">XCode</a><br />required</td>
			<td>4.2.1 works</td>
			<td>Apples SDK with all the toolchains.</td></tr>
		<tr>
			<td bgcolor="#ddffff"><a href="http://www.macports.org/">MacPorts</a><br />recommended</td>
			<td>>= 2.0.3</td>
			<td>Build system for a wide selection of open-source packages.</td></tr>
	</tr>

	<tr>
		<td rowspan="1"><b>Linux</b></td>
		<td bgcolor="#ffffdd">Avahi<br />optional</td>
		<td>3.x works</td>
		<td>For Debian:<br /><tt>$ sudo apt-get install avahi-daemon libavahi-client-dev</tt>. As of 0.3.2 we use the Bonjour mDNSResponder even on Linux.</td></tr>
	</tr>

	<tr>
	<td rowspan="3"><b>Windows</b></td>
		<td bgcolor="#ffffdd"><a href="http://www.microsoft.com/visualstudio/en-us">Visual&nbsp;Studio&nbsp;10</a><br />required</td>
		<td>v10 pro works</td>
		<td>As a student, you can get your version through MSAA.</td></tr>
	<tr>
		<td bgcolor="#ddffdd">
			<a href="http://www.microsoft.com/net/download">.NET Framework</a><br />optional</td>
		<td>3.5 or 4.0 are fine</td>
		<td>If SWIG is installed and a C# compiler found, the build-process will generate C# bindings.</td></tr>
	</tr>
	<tr>
		<td bgcolor="#ddffdd">
			<a href="http://support.apple.com/kb/DL999">Bonjour Print Wizard</a> or
			<a href="http://www.apple.com/itunes/download/">iTunes</a><br />optional</td>
		<td></td>
		<td>If you plan to use the system wide Bonjour service, you will need a mDNSResponder daemon contained in both these
			distributions. The uMundo libraries from the installers contain an embedded mDNS implementation.</td></tr>
	</tr>
</table>

# Platform Notes

This section will detail the preparation of the respective platforms to ultimately compile umundo.

## Mac OSX

* [<b>Build Reports</b>](http://umundo.tk.informatik.tu-darmstadt.de/cdash/index.php?project=umundo)
* [<b>Precompiled dependencies</b>](https://github.com/tklab-tud/umundo/tree/master/contrib/prebuilt/darwin-i386/gnu/lib)

All the build-dependencies are straight forward to install using <tt>port</tt>
from [MacPorts](http://www.macports.org/). The rest is most likely distributed
as precompiled binaries with umundo. 

	$ sudo port install cmake swig swig-java protobuf-cpp protobuf-java

If you like the Mono/CSharp bindings, do install the respective package

	$ sudo port install swig mono swig-csharp

Once you have installed all the dependencies you need, you can invoke CMake to
create Makefiles or a Xcode project.

### Console / Make

	$ mkdir -p build/umundo/cli && cd build/umundo/cli
	$ cmake <UMUNDO_SRCDIR>
	[...]
	#
	# TODO: Show how to resolve build-dependencies - For now rely on MacPorts and the respective installers.
	#
	-- Build files have been written to: .../build/umundo/cli
	$ make

You can test whether everything works by starting one of the sample programs:

	$ ./bin/umundo-pingpong &
	$ ./bin/umundo-pingpong &
	oioioioioi
	[...]
	$ killall umundo-pingpong

### Xcode

	$ mkdir -p build/umundo/xcode && cd build/umundo/xcode
	$ cmake -G Xcode <UMUNDO_SRCDIR>
	[...]
	-- Build files have been written to: .../build/umundo/xcode
	$ open umundo.xcodeproj

You can of course reuse the same source directory for many build directories.

## Linux

* [<b>Build Reports</b>](http://umundo.tk.informatik.tu-darmstadt.de/cdash/index.php?project=umundo)
* [<b>Precompiled dependencies (32Bit)</b>](https://github.com/tklab-tud/umundo/tree/master/contrib/prebuilt/linux-i686/gnu/lib)
/ [<b>Precompiled dependencies (64Bit)</b>](https://github.com/tklab-tud/umundo/tree/master/contrib/prebuilt/linux-x86_64/gnu/lib)

You will have to install all required build-dependencies via your package-manager.
If your system does not support the minimum version (especially with CMake, SWIG
and ProtoBuf), you will have to compile the dependency from source.

### Preparing *apt-get based* distributions

For the following instructions, I downloaded and installed a fresh
[Debian Testing](http://cdimage.debian.org/cdimage/wheezy_di_alpha1/i386/iso-cd/debian-wheezy-DI-a1-i386-netinst.iso)
wheezy Alpha1 release. You can go for Debian Stable as well, but at least
the ProtoBuf and CMake versions in stable are too old and would need to be pulled
from testing or compiled from source. The whole process should be pretty similar
with other *apt-get* based distributions.

	# build system and compiler
	$ sudo apt-get install git cmake cmake-curses-gui make g++

	# umundo required dependencies
	$ sudo apt-get install avahi-daemon libavahi-client-dev libprotoc-dev protobuf-compiler libpcre3-dev

There may still be packages missing due to the set of dependencies among packages
in other distributons but these are all the packages needed on Debian Testing to
compile and run the core functionality of umundo. If you want to build the Java
language bindings, you will need the following packages in addition:

	# umundo optional dependencies - SWIG and the Java Developer Kit
	$ sudo apt-get install swig openjdk-7-jdk ant

If CMake does not find your JDK location, make sure JAVA_HOME is set:

	$ echo $JAVA_HOME
	/usr/lib/jvm/java-7-openjdk-i386

If it is not, you can use the <tt>/etc/environment</tt> file to export the variable system wide:

	$ sudo nano /etc/environment
	> JAVA_HOME="/usr/lib/jvm/java-7-openjdk-i386" 

<b>Note:</b> If you are still not able to generate the Java bindings, double check that your JAVA_HOME directory actually contains an include directory with the JNI header files.

For the Mono language bindings, just install <tt>mono-devel</tt>

	$ sudo apt-get install swig mono-devel

### Preparing *yum based* distributions

The following instructions work as they are for
[Fedora 17 Desktop](http://download.fedoraproject.org/pub/fedora/linux/releases/17/Live/i686/Fedora-17-i686-Live-Desktop.iso).
As with the *apt-get* based distributions, there might be additional packages
needed for other distributions.

	# build system and compiler
	$ sudo yum install git cmake cmake-gui gcc-c++

	# umundo required dependencies
	$ sudo yum install avahi-devel dbus-devel protobuf-c-devel protobuf-devel pcre-devel

	# umundo optional dependencies - SWIG and the Java Developer Kit
	$ sudo yum install swig java-openjdk ant

### Console / Make

Instructions are a literal copy of building umundo for MacOSX on the console from above:

	$ mkdir -p build/umundo/cli && cd build/umundo/cli
	$ cmake <UMUNDO_SRCDIR>
	[...]
	-- Build files have been written to: .../build/umundo/cli
	$ make

### Eclipse CDT

	$ mkdir -p build/umundo/eclipse && cd build/umundo/eclipse
	$ cmake -G "Eclipse CDT4 - Unix Makefiles" ..
	[...]
	-- Build files have been written to: .../build/umundo/eclipse

Now open Eclipse CDT and import the out-of-source directory as an existing project into workspace, leaving the "Copy projects
into workspace" checkbox unchecked. There are some more [detailed instruction](http://www.cmake.org/Wiki/Eclipse_CDT4_Generator) available
in the cmake wiki as well.

<b>Note:</b> Eclipse does not like the project to be a subdirectory in the source.
You may need to choose your build directory with the generated project accordingly.

### Compiling Dependencies

If the packages in your distribution are too old, you will have to compile current
binaries. This applies especially for SWIG and CMake as they *need* to be rather
current. Have a look at the build dependencies above for minimum versions.

#### SWIG

    $ sudo apt-get install subversion autoconf libpcre3-dev bison wget
    $ wget http://prdownloads.sourceforge.net/swig/swig-2.0.6.tar.gz
    $ tar xvzf swig-2.0.6.tar.gz
    $ cd swig-2.0.6/
    $ ./configure
    $ make
    $ sudo make install
    $ swig -version

This ought to yield version 2.0.5 or higher.

#### CMake

    $ sudo apt-get remove cmake cmake-data
    $ wget http://www.cmake.org/files/v2.8/cmake-2.8.8.tar.gz
    $ tar xvzf cmake-2.8.8.tar.gz
    $ cd cmake-2.8.8/
    $ ./configure
    $ make
    $ sudo make install
    $ cmake --version

This should say <tt>cmake version 2.8.8</tt>. If you get the bash complaining
about not finding cmake, logout and login again.

## Windows

* [<b>Build Reports</b>](http://umundo.tk.informatik.tu-darmstadt.de/cdash/index.php?project=umundo)
* [<b>Precompiled dependencies (32Bit)</b>](https://github.com/tklab-tud/umundo/tree/master/contrib/prebuilt/windows-x86/msvc/lib)
/ [<b>Precompiled dependencies (64Bit)</b>](https://github.com/tklab-tud/umundo/tree/master/contrib/prebuilt/windows-x86_64/msvc/lib)

Building from source on windows is somewhat more involved and instructions are necessarily in prose form. These instructions were
created using Windows 7 and MS Visual Studio 2010.

<b>64 bit Note:</b> There has been some changes with regard to accessing registry
Keys, e.g. <tt>RegCreateKey(HKEY_LOCAL_MACHINE ...)</tt> will return a permission
denied error code 5. The embedded Bonjour does so and will have to be patched.
Until I get around to do it, just turn-off UAC.

### Details on required build-time dependencies

<table>
    <tr><th>Dependency</th><th>Search Path</th><th>CMake Variables</th><th>Comment</th></tr>
	<tr><td bgcolor="#ffffdd">Protocol Buffers<br>[<a href="http://protobuf.googlecode.com/files/protobuf-2.4.1.zip">download</a>]</td>
		<td>
			<tt>&lt;UMUNDO_BUILD_DIR>/..</tt><br>
			<tt>&lt;UMUNDO_BUILD_DIR>/../..</tt><br>
		</td><td>
			<tt>PROTOBUF_SRC_ROOT_FOLDER</tt> path to compiled protocol buffers<br/>
		</td><td>
			It is <emph>not</emph> sufficient to download just the precompiled
			<tt>protoc.exe</tt>. You will need to compile protocol buffers
			for the libraries anyway (see below).
		</td>
	</tr>
</table>

### Prepare compilation

1. Use git to **checkout** the source from <tt>git://github.com/tklab-tud/umundo.git</tt>
	into any convenient directory. Try not to run CMake with a build directory you
	plan to use until you compiled protobuf. If you did and are having trouble with
	finding protobuf, just delete the CMake cache and empty the directory to start over.

2. For **ProtoBuf**, we *need* to build from source for the libraries. Just
	download and unpack the sources linked from the table above into the same
	folder as the build directory or one directory above. You can still place them
	anywhere but then you will have to set <tt>PROTOBUF_SRC_ROOT_FOLDER</tt> for
	every new build directory you create.
<pre>
build> dir
07/26/2012 06:41 PM    &lt;DIR> umundo
07/26/2012 05:16 PM    &lt;DIR> protobuf–2.4.1 # first option
build> dir ..
07/26/2012 05:16 PM    &lt;DIR> protobuf-2.4.1 # second option
</pre>
	1. Within the ProtoBuf source folder open <tt>vsprojects/protobuf.sln</tt> with
		MS Visual Studio and build everything in Debug and Release configuration via
		<tt>Build->Build Solution</tt>. Just ignore all the errors with regard to
		<tt>gtest</tt>, the libraries are built nevertheless.

	2. You most likely **have to build ProtoBuf twice** for both configurations until you
		get 5 projects to succeed for both Release and Debug builds.

3. Start the **CMake-GUI** and enter the checkout directory in the "Where is the source
	code" text field. Choose any convenient directory to build the binaries in, but
	try to put it next to the directory with the ProtoBuf build or one level deeper.

4. Hit "**Configure**" and choose your toolchain and compiler - I only tested with
	Visual Studio 10. Hit "Configure" again until there are no more red items in
	the list. If these instructions are still correct and you did as described
	above, you should be able to "Generate" the Visual Project Solution.

	1. If you have the compiled ProtoBuf libraries installed somewhere else, provide
		the path to the directory in <tt>PROTOBUF_SRC_ROOT_FOLDER</tt> and hit
		"Configure" again, hoping that he will find all the ProtoBuf libraries.

	2. CMake will still complain about missing the <tt>SWIG_EXECUTABLE</tt> but that
		is just a friendly reminder and no error at this point.

Now you can generate the MS Visual Studio project file <tt><UMUNDO_SRCDIR>/umundo.sln</tt>.
Just open it up to continue in your IDE.

<b>Note:</b> We only tested with the MSVC compiler. You can try to compile
with MinGW but you would have to build all the dependent libraries as well.
I did not manage to compile ZeroMQ the last time I tried.

### Language Bindings

The process described above will let you compile the umundo libraries. If you
want some language bindings into Java or C#, you will have to install some more
packages:

<table>
  <tr><th>Dependency</th><th>Search Path</th><th>CMake Variables</th><th>Comment</th></tr>
	<tr><td bgcolor="#ddffdd">SWIG<br>[<a href="http://prdownloads.sourceforge.net/swig/swigwin-2.0.7.zip">download</a>]</td>
		<td>
			<tt>C:/Program Files/swig/</tt><br>
			<tt>C:/Program Files (x86)/swig/</tt><br>
			Every entry in <tt>%ENV{PATH}</tt>
		</td><td>
			<tt>SWIG_EXECUTABLE</tt> with a full path to swig.exe<br/>
		</td>
		<td>
			Just download the archive and extract it into the search path (make
			sure to remove an eventual version suffix from the directory name)
			If CMake cannot find <tt>swig.exe</tt>, you can always specify a path in
			<tt>SWIG_EXECUTABLE</tt> per hand.
		</td>
	</tr>
	<tr><td bgcolor="#ddffdd">Java SDK<br>[<a href="http://www.oracle.com/technetwork/java/javase/downloads/index.html">download</a>]</td>
		<td>
			<tt>%ENV{JAVA_HOME}</tt><br>
			<a href="http://cmake.org/gitweb?p=cmake.git;a=blob_plain;f=Modules/FindJNI.cmake;hb=HEAD">FindJNI.cmake</a>

		</td><td>
			<tt>JAVA_*</tt>
		</td><td>
			The linked FindJNI.cmake is the actual module used by CMake to find a
			Java installation with for JNI headers on the system. If you run into
			any troubles with your Java installation, make sure to set the <tt>%JAVA_HOME</tt>
			environment variable to the root of your JDK.
		</td>
	</tr>
	<tr><td bgcolor="#ddffdd">Ant<br>[<a href="http://ant.apache.org/bindownload.cgi">download</a>]</td>
		<td>
			<tt>%ENV{ANT_HOME}</tt><br>
			Every entry in <tt>%ENV{PATH}</tt>
		</td><td>
			<tt>ANT_EXECUTABLE</tt>
		</td><td>
			Just download a binary release, copy it somewhere and export <tt>%ANT_HOME</tt>
			as the full path to your ant root directory. Alternatively, set the CMake
			variable every time.
		</td>
	</tr>
	<tr><td bgcolor="#ddffdd">MS .NET Framework<br>[<a href="http://www.microsoft.com/net/download">download</a>]</td>
		<td>
			<tt>C:/Windows/Microsoft.NET/Framework/v3.5</tt><br>
			<tt>C:/Windows/Microsoft.NET/Framework/v4.0</tt><br>
			Every entry in <tt>%PATH</tt>
		</td><td>
			<tt>CSC_EXECUTABLE</tt> with a full path to csc.exe<br/>
		</td>
		<td>
			Just use the Microsoft installer. I am not even sure whether you actually
			change the installation directory. If you have problems, just provide the
			full path to the C# compiler in <tt>CSC_EXECUTABLE</tt>.
		</td>
	</tr>
</table>

You will need SWIG in any case. If you only want Java, you would not need to
install the .NET Framework and the other way around. If you installed the
packages as outlined in the table above, CMake will offer two more targets
called <tt>java</tt> and <tt>csharp</tt> - they are not build per default.

#### Java
There are only two files of interest built for Java:
<pre>
build\umundo>ls lib
umundo.jar             # The Java archive
umundoNativeJava.dll   # The JNI library for System.load()
</pre>

The Java archive contains generated wrappers for the umundo.core C++ code in
umundoNativeJava.dll and hand-written implementations of the layers on top. See the
[Eclipse sample projects](https://github.com/tklab-tud/umundo/tree/master/examples/java)
to get an idea on how to use the API.

#### CSharp
There are again, only two files of interest for C# development
<pre>
build\umundo>ls lib
umundoCSharp.dll       # The managed code part
umundoNativeCSharp.dll # The native C++ code used via DLLInvoke
</pre>

Here again, the umundoCSharp.dll contains all generated wrappers for the umundo.core
C++ code in umundocoreCSharp.dll. Other C# functionality for the umundo layers
on top will also eventually find its way into this dll. There is also a sample
[Visual Studio solution](https://github.com/tklab-tud/umundo/tree/master/examples/csharp),
illustrating how to use umundo.core with C#.

## Cross Compiling

See the dedicated [cross-compiling guide](https://github.com/tklab-tud/umundo/tree/master/docs/CROSS_COMPILING.md).

# Build process

We are using CMake to build uMundo for every platform. When <tt>cmake</tt> is invoked, it will look for a <tt>CMakeLists.txt</tt>
file in the given directory and prepare a build in the current directory. CMake itself can be considered as a meta build-system as
it will only generate the artifacts required for an actual build-system. The default is to generate files for <tt>make</tt> on
unices and files for <tt>nmake</tt> on windows. If you invoke <tt>ccmake</tt> instead of <tt>cmake</tt>, you get an user interfaces
to set some variables related to the build:

## Influential CMake Variables

<table>
	<tr><th>Name</th><th>Description</th><th style="width: 17%;">Default Value</th></tr>
	<tr><td colspan=3><small><b>How to build</b>:</small></td></tr>
	<tr>
		<td><tt>CMAKE_BUILD_TYPE</tt></td>
		<td>The build type as in <tt>Debug</tt> or <tt>Release</tt>. CMake also
		offers <tt>MinSizeRel</tt> and <tt>RelWithDebInfo</tt>, but I have
		never used them. In debug builds, all asserts are stripped and the default
		log-level is decreased.

		When you choose the Visual Studio generator, you can change the build-type
		within the IDE as well.</td>
		<td>Release</td>
	</tr>
	<tr>
		<td><tt>BUILD_PREFER_STATIC_LIBRARIES</tt></td>
		<td>When searching for libraries, prefer a statically compiled variant.</td>
		<td>On</td>
	</tr>
	<tr>
		<td><tt>BUILD_STATIC_LIBRARIES</tt></td>
		<td>Whether to build uMundo as shared or static libraries.<br/>
			<b>Note: </b>This does not influence the libraries for language bindings.</td>
		<td>On for Unices<br />Off for Windows<br />Off when cross-compiling</td>
	</tr>
	<tr>
		<td><tt>DIST_PREPARE</tt></td>
		<td>Put all libraries and binaries into SOURCE_DIR/package/ to prepare a
		release. We need access to all artifacts from other platforms to create
		the installers with platform independent JARs and cross-compiled mobile
		platforms.</td>
		<td>Off</td>
	</tr>
	<tr><td colspan=3><small><b>What to build</b> (umundo.core is always built):</small></td></tr>
	<tr>
		<td><tt>BUILD_UMUNDO_S11N</tt></td>
		<td>Build the <tt>umundoserial</tt> library for object serialization.<br />
		<b>Note:</b> This only applies to the umundo.s11n C++ component. Individual target
		languages are expected to bring their own implementation anyway.</td>
		<td>Off for Android<br /> On otherwise</td>
	</tr>
	<tr>
		<td><tt>BUILD_UMUNDO_RPC</tt></td>
		<td>Build the <tt>umundorpc</tt> library for remote procedure calls via uMundo.</td>
		<td>Off for Android<br /> On otherwise</td>
	</tr>
	<tr>
		<td><tt>BUILD_TESTS</tt></td>
		<td>Build the test executables.</td>
		<td>Off</td>
	</tr>
	<tr>
		<td><tt>BUILD_UMUNDO_APPS</tt></td>
		<td>Include the <tt>apps/</tt> directory when building.</td>
		<td>Off</td>
	</tr>
	<tr><td colspan=3><small><b>Implementations</b>:</small></td></tr>
	<tr>
		<td><tt>DISC_AVAHI</tt></td>
		<td>Use the Avahi ZeroConf implementation for discovery with umundo.core.</td>
		<td>On for Linux<br />Off otherwise</td>
	</tr>
	<tr>
		<td><tt>DISC_BONJOUR</tt></td>
		<td>Use the Bonjour ZeroConf implementation found on every MacOSX installation and every iOS device.</td>
		<td>On for Mac OSX<br />On for iOS<br />Off otherwise</td>
	</tr>
	<tr>
		<td><tt>DISC_BONJOUR_EMBED</tt></td>
		<td>Embed the Bonjour ZeroConf implementation into umundo.core, much like with printers or routers.</td>
		<td>On for Windows<br />On for Android<br />Off otherwise</td>
	</tr>
	<tr>
		<td><tt>NET_ZEROMQ</tt></td>
		<td>Use ZeroMQ to connect nodes to each other and publishers to subscribers.</td>
		<td>On</td>
	</tr>
	<tr>
		<td><tt>NET_ZEROMQ_SND_HWM</tt><br /><tt>NET_ZEROMQ_RCV_HWM</tt></td>
		<td>High water mark for ZeroMQ queues in messages. One uMundo message represents multiple ZeroMQ messages, one per meta field and
		one for the actual data.</td>
		<td>10000<br /> 10000</td>
	</tr>
	<tr>
		<td><tt>S11N_PROTOBUF</tt></td>
		<td>Use Google's ProtoBuf to serialize objects.</td>
		<td>On</td>
	</tr>
	<tr>
		<td><tt>RPC_PROTOBUF</tt></td>
		<td>Use Google's ProtoBuf to call remote procedure calls.</td>
		<td>On</td>
	</tr>

</table>

## CMake files

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

