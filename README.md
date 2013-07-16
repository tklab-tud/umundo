# uMundo ReadMe

uMundo is a lightweight refactoring of the distributed Mundo publish/subscribe middleware to deliver byte
arrays and objects on channels from publishers to subscribers. With lightweight, we do not necessarily
refer to runtime behavior or memory footprint (though we work hard to keep it sane) but to the amount
of code we have to maintain in order to arrive at a working system. It runs on Windows, Linux, Mac OSX,
iOS devices and Android.

<dt><b>Other Documents</b></dt>
<dd>- <a href="https://github.com/tklab-tud/umundo/tree/master/docs/GETTING_STARTED.md">Getting started</a></dd>
<dd>- <a href="https://github.com/tklab-tud/umundo/tree/master/docs/BUILDING.md">Building from source</a></dd>
<dd>- <a href="https://github.com/tklab-tud/umundo/tree/master/docs/CROSS_COMPILING.md">Cross-Compiling for iOS and Android</a></dd>
<dd>- <a href="https://github.com/tklab-tud/umundo/tree/master/contrib/ctest/README.md">Setting up a test-slave</a></dd>

<dt><b>External Resources</b></dt>
<dd>- <a href="http://umundo.tk.informatik.tu-darmstadt.de/cdash/index.php?project=umundo">Build Reports</a></dd>
<dd>- <a href="http://umundo.tk.informatik.tu-darmstadt.de/docs">Doxygen Documentation</a></dd>

## License
uMundo itself is distributed under the Simplified BSD license.

Please have a look at the licenses of the [libraries we depend upon](https://github.com/tklab-tud/umundo/blob/master/docs/BUILDING.md#build-dependencies) as well.

## Download
The fastest way to get started is to grab the [uMundo SDK](http://umundo.tk.informatik.tu-darmstadt.de/installer)
for your platform. It contains

* uMundo libraries and headers
* Available language bindings (build from source for PHP, Python or Perl bindings)
* Android cross-compiled libraries
* iOS cross-compiled libraries with the darwin installer
* Templates for Eclipse, XCode and Visual Studio

## Components

uMundo is divided into three components with different responsibilities in
various stages of maturity:

<dl>
	<dt><b>umundo.core</b></dt>
	<dd>The responsibility of umundo.core is to deliver messages from publishers
	to subscribers. A message contains foremost a raw byte array but can also be
	annotated with meta information as key/value pairs. It employs
	<a href="http://www.zeromq.org">ZeroMQ</a> to deliver byte arrays between nodes
	discovered via <a href="http://developer.apple.com/opensource/">Bonjour</a> or
	<a href="http://avahi.org/">Avahi</a>. The core is written in C++ and has
	bindings to CSharp, Java, Python, Perl, PHP and Objective-C. <a href="/tklab-tud/umundo/tree/master/core">[more]</a>
	<br/><br/><table>
		<tr>
			<td style="border-right: solid #bbb 1px;">C++</td>
			<td style="border-right: solid #bbb 1px;">Java</td>
			<td style="border-right: solid #bbb 1px;">Objective-C</td>
			<td style="border-right: solid #bbb 1px;">C#</td>
			<td style="border-right: solid #bbb 1px;">Python</td>
			<td style="border-right: solid #bbb 1px;">Perl</td>
			<td style="border-right: solid #bbb 1px;">PHP5</td>
		</tr>
	</table>
	</dd>

	<dt><b>umundo.s11n</b></dt>
	<dd>On top of the core, umundo.s11n offers object (de-)serialization using
	<a href="http://code.google.com/p/protobuf/">Protocol Buffers</a>. As it is
	unpractical to wrap the whole typing system via native access from other languages,
	this component exists once for every target language. At the moment,
	implementations for C++, Java, C# and Objective-C exist.<a href="/tklab-tud/umundo/tree/master/s11n">[more]</a>
	<br/><br/><b>Note:</b> You can always access all of uMundos functionality from
	Objective-C as you can just use .mm extensions and write mixed code. The biggest
	stumbling block for umundo.s11n to have a pure Objective-C facade is the absence
	of a suited protobuf compiler.
	<br/><br/><table>
		<tr>
			<td style="border-right: solid #bbb 1px;">C++</td>
			<td style="border-right: solid #bbb 1px;">Java</td>
			<td style="border-right: solid #bbb 1px;">Objective-C</td>
			<td style="border-right: solid #bbb 1px;">C#</td>
		</tr>
	</table>
	</dd>

	<dt><b>umundo.rpc</b></dt>
	<dd>This component provides a service concept for remote procedure calls on top
	of umundo.s11n. The new implementation features continuous service discovery with
	pattern matching, but is only available in C++ and Java at the moment.
		<a href="/tklab-tud/umundo/tree/master/rpc">[more]</a>
	<br/><br/><table>
		<tr>
			<td style="border-right: solid #bbb 1px;">C++</td>
			<td style="border-right: solid #bbb 1px;">Java</td>
			<td style="border-right: solid #bbb 1px;">Objective-C</td>
		</tr>
	</table>
	</dd>

</dl>

## Status

The authority on the state of uMundo is
<a href="http://umundo.tk.informatik.tu-darmstadt.de/cdash/index.php?project=umundo">
our build-server</a>. The tests, while not numerous, are rather strict as there
are plenty of <tt>asserts</tt> in the source code. We plan to add more test-slaves
as they become available.

<table>
	<tr><th>Platform</th><th>Issues</th></tr>
	<tr><td>Mac OSX 10.7</td><td>
		<ul>
			<li>umundo.s11n employs the C++ generator for ProtoBuf as the various Objective-C generators are out of date (just use <tt>.mm</tt> file extensions).
		</ul>
	</td></tr>

	<tr><td>iOS 5.x / 6.x</td><td>
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
	<tr><td>Raspberry Pi</td><td>
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
</table>

<table>
	<tr><th>Language Bindings</th><th>Issues</th></tr>

	<tr><td>Java</td><td>
		<ul>
			<li>No known issues.
		</ul>
	</td></tr>

	<tr><td>CSharp</td><td>
		<ul>
			<li>Make sure to use 64Bit on 64Bit systems.
			<li>No umundo.rpc implementation yet.
		</ul>
	</td></tr>

	<tr><td>Objective C</td><td>
		<ul>
			<li>No umundo.rpc implementation yet, just use <tt>.mm</tt> extensions and the C++ implementation.
		</ul>
	</td></tr>
	
	<tr><td>Python</td><td>
		<ul>
			<li>Only umundo.core is available.
			<li>Object instances get garbage collected if they leave scope.
			<li>Not part of the SDK yet.
			<li>Messages are not copied into runtime but destroyed when <tt>received</tt> returns.
		</ul>
	</td></tr>

	<tr><td>Perl</td><td>
		<ul>
			<li>Only umundo.core is available.
			<li>Object instances get garbage collected if they leave scope.
			<li>Not part of the SDK yet.
			<li>Messages are not copied into runtime but destroyed when <tt>received</tt> returns.
		</ul>
	</td></tr>

	<tr><td>PHP5</td><td>
		<ul>
			<li>Only umundo.core is available.
			<li>Object instances get garbage collected if they leave scope.
			<li>Not part of the SDK yet.
			<li>Messages are not copied into runtime but destroyed when <tt>received</tt> returns.
		</ul>
	</td></tr>

</table>

<b>Note:</b> The scripting languages still have the problem of <b>premature garbage collection</b>.
If you assign e.g. a <tt>Publisher</tt> to a <tt>Node</tt>, the node will only take the pointer to
the underlying C++ object and not keep a reference to the language specific instance. This means
that the respective garbage collectors will potentially remove these objects while they are still
being used. This applies to all umundo.core objects.

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

	<dt><b>Why am I not receiving messages?</b></dt>
	<dd>First make sure that your network is setup correctly by running two instances of <tt>umundo-pingpong</tt>.
		If you do not receive <tt>i</tt>'s to your <tt>o</tt>'s make sure that:
		<ul>
			<li>Multicast messages can be delivered.
			<li>The firewall does not block tcp/4242 and upwards.
			<li>You actually ran more than one instance of <tt>umundo-pingpong</tt>.
		</ul>
		If you do receive messages with <tt>umundo-pingpong</tt>, there are a few things
		you should check:
		<ul>
			<li>Are the subscriber and publisher added to different nodes?
			<li>Are the nodes in the same (or default) domain?
			<li>Does the channel name of the subscriber match the publisher's?
			<li>Is the publisher already connected to the subscriber? <br />
				<p style="padding-left: 1em;">You can use <tt>int Publisher.waitForSubscribers(int n)</tt> 
				to wait until <tt>n</tt> subscribers connected. The method will return the actual number of subscribers. Therefore,
				calling it with <tt>n = 0</tt> will not block and report the number of subscribers per publisher.</p>
		</ul>
	</dd>

	<dt><b>How do I use the language bindings?</b></dt>
	<dd>All language bindings constitute of two components: 
		<ul>
		 	<li>a native extension named <tt>umundoNative&lt;LANGUAGE>&lt;LIB_SUFFIX></tt>.
			<li>a language specific wrapper that (loads and) provides access to this library.
		</ul>
		<p>It depends on the actual language how to register/load extensions. There are a few 
		<a href="/tklab-tud/umundo/tree/master/examples">examples</a>. The exceptions are the 
		Java language bindings built for desktop systems, here the JAR contains all required files.</p>
	
		<p>If you built a recent version, the CSharp/Mono managed code dll will always try to load 
		its unmanaged code from <tt>umundoNative[CSharp|Mono].dll</tt>, without the 64Bit or debug suffix.
		Just copy the various <tt>umundoNativeCSharp[64[_d]].dll</tt> into seperate directories and
		use a 64Bit switch programmatically:
		<pre>
[DllImport("kernel32.dll", CharSet = CharSet.Auto)]
private static extern void SetDllDirectory(string lpPathName);

if (System.Environment.Is64BitOperatingSystem) {
	SetDllDirectory("..\\..\\..\\lib\\win64");
} else {
	SetDllDirectory("..\\..\\..\\lib\\win32");
}
		</pre></p>
	</dd>

	<dt><b>When using umundo from the installers on linux <tt>libpcre.so.3</tt> was not found, what gives?</b></dt>
	<dd>Some distributions, such as Fedora or Gentoo will not create <tt>libpcre.so.3</tt> but only <tt>libpcre.so.1.2.0</tt>.
		Just symlink the actual library to libpcre.so.3 and rerun <tt>ldconfig</tt>.</dd>
		
	<dt><b>Does uMundo support IPv6?</b></dt>
	<dd>No, but only because I couldn't get ZeroMQ to compile with IPv6 on Android
		devices. Both ZeroConf implementations (Avahi and Bonjour) support IPv6 and
		we already gather these addresses (BonjourNodeStub and AvahiNodeStub). All
		that is needed is for both of these stubs to return an IPv6 address in
		<tt>getIP()</tt> and for ZeroMQ to be compiled with IPv6. The plan is to wait
		for another release of ZeroMQ 3.x and have another look.</dd>

	<dt><b>Why do I get <tt>NullPointerException</tt>s when using custom ClassLoaders in a umundo callback?</b></dt>
	<dd>Java has some strange issues with regard to custom class-loaders and JNI. This blog entry details the issue and 
		<a href="http://schnelle-walka.blogspot.de/2013/04/jni-causes-xml-parsing-problems-when.html">provides a workaround</a>.</dd>

	<dt><b>Are these actually questions that are asked frequently?</b></dt>
	<dd>No, it's more like a set of questions I can imagine other people might have. It will eventually grow into a real FAQ.</dd>	
</dl>
