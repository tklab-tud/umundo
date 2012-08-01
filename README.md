# uMundo ReadMe

uMundo is a lightweight refactoring of the Mundo publish/subscribe middleware to deliver byte
arrays and objects on channels from publishers to subscribers. With lightweight, we do not necessarily 
refer to runtime behavior or memory footprint (though we work hard to keep it sane) but to the amount 
of code we have to maintain in order to arrive at a working system. It runs on Windows, Linux, Mac OSX, 
iOS devices and Android.

<dt><b>Other Documents</b></dt>
<dd>- <a href="https://github.com/tklab-tud/umundo/tree/master/docs/GETTING_STARTED.md">Getting started</a></dd>
<dd>- <a href="https://github.com/tklab-tud/umundo/tree/master/docs/BUILDING.md">Building from source</a></dd>
<dd>- <a href="https://github.com/tklab-tud/umundo/tree/master/contrib/ctest/README.md">Setting up a test-slave</a></dd>

<dt><b>External Resources</b></dt>
<dd>- <a href="http://umundo.tk.informatik.tu-darmstadt.de/cdash/index.php?project=umundo">Build Reports</a></dd>
<dd>- <a href="http://umundo.tk.informatik.tu-darmstadt.de/docs">Doxygen Documentation</a></dd>

## License
uMundo is distributed under the Simplified BSD license.

## Download
The fastest way to get started is to grab the [uMundo SDK](http://umundo.tk.informatik.tu-darmstadt.de/installer)
for your platform. It contains

* uMundo libraries and headers
* Available language bindings
* Android cross-compiled libraries
* iOS cross-compiled libraries with the darwin installer

## Components

uMundo is divided into four components with different responsibilities in 
various stages of maturity:

<dl>
	<dt><b>umundo.core</b></dt>
	<dd>The responsibility of umundo.core is to deliver messages from publishers 
	to subscribers. A message contains foremost a raw byte array but can also be 
	annotated with meta information as key/value pairs. It employs
	<a href="http://www.zeromq.org">ZeroMQ</a> to deliver byte arrays between nodes 
	discovered via <a href="http://developer.apple.com/opensource/">Bonjour</a> or 
	<a href="http://avahi.org/">Avahi</a>. The core is written in C++ and has 
	bindings to CSharp, Java and Objective-C. <a href="/tklab-tud/umundo/tree/master/core">[more]</a>
	<br/><br/><table>
		<tr>
			<td style="background-color: #dfd; border-right: solid #bbb 1px;">C++</td>
			<td style="background-color: #dfd; border-right: solid #bbb 1px;">Java</td>
			<td style="background-color: #dfd; border-right: solid #bbb 1px;">Objective-C</td>
			<td style="background-color: #dfd; border-right: solid #bbb 1px;">C#</td>
		</tr>
	</table>
	</dd>

	<dt><b>umundo.s11n</b></dt>
	<dd>On top of the core, umundo.s11n offers object (de-)serialization using 
	<a href="http://code.google.com/p/protobuf/">Protocol Buffers</a>. As it is 
	unpractical to wrap the whole typing system via native access from other languages, 
	this component exists once for every target language. At the moment, 
	implementations for C++, Java and Objective-C exist.<a href="/tklab-tud/umundo/tree/master/s11n">[more]</a>
	<br/><br/><b>Note:</b> You can always access all of uMundos functionality from 
	Objective-C as you can just use .mm extensions and write mixed code. Green with 
	Objective-C therefore means "Has an actual, Objective-C facade". The biggest 
	stumbling block for umundo.s11n to have a pure Objective-C facade is the absence 
	of a suited protobuf compiler.
	<br/><br/><table>
		<tr>
			<td style="background-color: #dfd; border-right: solid #bbb 1px;">C++</td>
			<td style="background-color: #dfd; border-right: solid #bbb 1px;">Java</td>
			<td style="background-color: #ffd; border-right: solid #bbb 1px;">Objective-C</td>
			<td style="background-color: #fdd; border-right: solid #bbb 1px;">C#</td>
		</tr>
	</table>
	</dd>

	<dt><b>umundo.rpc</b></dt>
	<dd>This component provides a service concept for remote procedure calls on top 
	of umundo.s11n. The new implementation features continuous service discovery with
	pattern matching, but is only available in C++ at the moment.
		<a href="/tklab-tud/umundo/tree/master/rpc">[more]</a>
	<br/><br/><table>
		<tr>
			<td style="background-color: #dfd; border-right: solid #bbb 1px;">C++</td>
			<td style="background-color: #fdd; border-right: solid #bbb 1px;">Java</td>
			<td style="background-color: #ffd; border-right: solid #bbb 1px;">Objective-C</td>
			<td style="background-color: #fdd; border-right: solid #bbb 1px;">C#</td>
		</tr>
	</table>
	</dd>

	<dt><b>umundo.util</b></dt>
	<dd>Implementation of various C++ components for uMundo and some utility classes.
		<a href="/tklab-tud/umundo/tree/master/util">[more]</a>
	<br/><br/><table>
		<tr>
			<td style="background-color: #dfd; border-right: solid #bbb 1px;">C++</td>
			<td style="background-color: #fdd; border-right: solid #bbb 1px;">Java</td>
			<td style="background-color: #ffd; border-right: solid #bbb 1px;">Objective-C</td>
			<td style="background-color: #fdd; border-right: solid #bbb 1px;">C#</td>
		</tr>
	</table>
	</dd>
</dl>
	
## Contributors

- Stefan Radomski <radomski@tk.informatik.tu-darmstadt.de>
- Daniel Schreiber <schreiber@tk.informatik.tu-darmstadt.de>
- Felix Heinrichs <felix.heinrichs@cs.tu-darmstadt.de>

## Status

The authority on the state of uMundo is 
<a href="http://umundo.tk.informatik.tu-darmstadt.de/cdash/index.php?project=umundo">
our build-server</a>. The tests, while not numerous, are rather strict as there 
are plenty of <tt>asserts</tt> in the source code. We plan to add more test-slaves
as they become available.

<table>
    </tr>
    <tr><th>Platform</th><th>Issues</th></tr>
	<tr><td>Mac OSX 10.7</td><td>
		<ul>
			<li>umundo.s11n employs the C++ generator for ProtoBuf as the various Objective-C generators are out of date (just use <tt>.mm</tt> file extensions).
		</ul>
	</td></tr>

	<tr><td>iOS 5.x</td><td>
		<ul>
			<li>umundo.s11n uses the C++ generator as with Mac OSX.
			<li>The automated test are not executed on the build-server.
		</ul>
	</td></tr>
	<tr><td>Windows 7</td><td>
		<ul>
			<li>Everything builds and runs just fine with MS Visual Compiler and Visual Studio 10 and NMake.
			<li>We do not support MinGW as we would need precompiled libraries for our build-time dependencies. Feel free to put them as static libraries into contrib/prebuilt/ if you manage to compile them.
		</ul>
	</td></tr>
	<tr><td>Debian Linux 6.0.4</td><td>
		<ul>
			<li>Everything builds and runs just fine with GCC.
		</ul>
	</td></tr>
	<tr><td>Android 2.3</td><td>
		<ul>
			<li>Worked whenever we tried, but we cannot automatically test on the simulator, as <a href="http://developer.android.com/guide/developing/devices/emulator.html#emulatornetworking">google does not deem multicast to be important</a>.
			<li>The automated test are not executed on the build-server.
		</ul>
	</td></tr>
    </tr>
</table>

## Directory Structure (excerpt)

	contrib         # Contributed scripts and components not part of uMundo as such.
	|-archives      # Archives and patches for the libraries we rely upon.
	|-samples
	| |-android     # Eclipse project illustrating usage of umundo on Android devices.
	| |-java        # Sample java applications illustrating usage of umundo from Java.
	|-cmake         # Finding libraries, cross-compilation and packaging support for build-system.
	|-prebuilt      # Prebuilt libraries of our dependencies for all supported platforms.
	core            # umundo.core implementation, see README.md within.
	docs            # General documentation, API docs are in respective components docs/ directory.
	installer       # Installers of uMundo for the supported platforms.
	lib             # Prebuilt libraries of uMundo itself for all supported platforms.
	rpc             # umundo.rpc implementation.
	s11n            # umundo.s11n implementation, see README.md within.
	util            # umundo.util with some umundo components and utility classes.

## FAQ

<dl>
	<dt><b>Why is the source distribution so large?</b></dt>
	<dd>That's the price of convenience. The distribution contains most of our 
		runtime dependencies prebuilt for every system / compiler combination and 
		as debug and release builds.</dd>

	<dt><b>How many umundo nodes can I realistically start at once?</b></dt>
	<dd>Using the default ZeroMQ implementation and Bonjour discovery on MacOSX, I 
		could start 32 umundo-pingpong instances before getting an <tt>Assertion 
		failed: s != retired_fd (tcp_connecter.cpp:278)</tt> within ZeroMQ. I guess 
		this is due to the rather low ulimit for open file-handles on MacOSX 
		(<tt>ulimit -n</tt> gives 256).</dd>

	<dt><b>Does uMundo support IPv6?</b></dt>
	<dd>No, but only because I couldn't get ZeroMQ to compile with IPv6 on Android 
		devices. Both ZeroConf implementations (Avahi and Bonjour) support IPv6 and 
		we already gather these addresses (BonjourNodeStub and AvahiNodeStub). All 
		that is needed is for both of these stubs to return an IPv6 address in 
		<tt>getIP()</tt> and for ZeroMQ to be compiled with IPv6. The plan is to wait
		for another release of ZeroMQ 3.x and have another look.</dd>

	<dt><b>Does uMundo build for 64bit architectures?</b></dt>
	<dd>Yes. But it is your responsibility to provide some libraries on your system. 
		Have a look at the top-level CMakeLists.txt at the 64bit section to add 
		library directories. If you have some unorthodox paths to your 64bit libraries, 
		consider using the environment variables for the various <tt>contrib/cmake/Find*.cmake</tt> 
		modules or edit these files themselves.<br />
		On Debian amd64 stable, there are some linking issues when enabling 
		<tt>BUILD_PREFER_STATIC_LIBRARIES</tt> via cmake.</dd> This is not a problem 
		as you need to provide libzmq and all the libproto* libraries installed on 
		your system anyway.

	<dt><b>What's with all the "narrowing conversion" warnings when building with a current gcc?</b></dt>
	<dd>It seems that new versions of the gcc use C++11 as the default C++ standard 
		and warn about implicit conversions with loss of precision. There is a 
		<tt>-Wno-narrowing</tt> flag for gcc, but older versions will croak on it.</dd>

	<dt><b>Are these actually questions that are asked frequently?</b></dt>
	<dd>No, it's more like a set of questions I can imagine other people might have. It will eventually grow into a real FAQ.</dd>
</dl>