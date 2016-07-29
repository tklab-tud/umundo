# uMundo ReadMe

[![Build Status](https://travis-ci.org/tklab-tud/umundo.png?branch=master)](https://travis-ci.org/tklab-tud/umundo)

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


## FAQ

<dl>
	<dt><b>Why is the distribution so large?</b></dt>
	<dd>That's the price of convenience. The distribution contains most of our
		runtime dependencies prebuilt for every system / compiler combination and
		as debug and release builds. The cross-compiled iOS libraries contain all 
		architectures for all iOS devices and the simulator.</dd>

	<dt><b>How many umundo nodes can I realistically start at once?</b></dt>
	<dd>Using the default ZeroMQ implementation and Bonjour discovery on MacOSX, I
		could start 32 umundo-pingpong instances before getting an <tt>Assertion
		failed: s != retired_fd (tcp_connecter.cpp:278)</tt> within ZeroMQ. I guess
		this is due to the rather low ulimit for open file-handles on MacOSX
		(<tt>ulimit -n</tt> gives 256).</dd>

	<dt><b>Is uMundo Thread-Safe?</b></dt>
	<dd>Well, the individual objects are not, uMundo as a whole is though. That is
		you cannot send via the same publisher object on many threads without locking,
		but you can use a publisher per thread. Sending on a publisher is even lock-free
		as far as uMundo is conceirned, if we were to make the single objects thread-safe,
		even single-threaded applications would pay the performance hit of locking for
		mutual exclusion. Therefore, it is your responsibility to serialize access to the
		individual objects among threads.
	</dd>

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
			<li>Are the nodes in added to compatible Discovery objects?
			<li>Does the channel name of the subscriber match the publisher's?
			<li>Is the publisher already connected to the subscriber? <br />
				<p style="padding-left: 1em;">You can use <tt>int Publisher.waitForSubscribers(int n)</tt> 
				to wait until <tt>n</tt> subscribers connected. The method will return the actual number of subscribers. Therefore,
				calling it with <tt>n = 0</tt> will not block and report the number of subscribers per publisher.</p>
		</ul>
	</dd>

	<dt><b>Why are some idioms with the uMundo codebase .. peculiar?</b></dt>
	<dd>
		There are a couple of issues with the codebase which I, personally, do not consider good style, but for 
		some of them, there is actually a reason:
		<ol>
			<li><b>No nested classes:</b><br/>
				We generate most language bindings of uMundo via SWIG. Some of the potential target languages do not
				support the concept of nested classes, either by design or due to insufficient mappings in swig. To
				keep all options open, we do not use nested classes.
			</li>
			<li><b>Pointers in constructors:</b><br/>
				There are some instances where constructors take pointers to objects (e.g. the various <tt>Config</tt>
				objects). But in the general case, if we need to retain the object pointed to throughout the lifetime
				of the constructed object, we run into garbage collection issues with some language bindings (e.g. Java, 
				C#). This is less of an issue when passing via member functions as SWIG will allow to assign the respective
				object in the target language to a private field via function rename / hide / override, this is virtually
				impossible to do with constructors.
			</li>
			<li><b>No exceptions:</b><br/>
				Up until somewhen in 2012, the C++ compiler in the Android NDK did not support exceptions. Furthermore,
				exceptions are problematic for some potential language bindings.
			</li>
		</ol>
		
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

if (System.Environment.Is64BitProcess) {
    SetDllDirectory("C:\\Program Files (x86)\\uMundo\\share\\umundo\\bindings\\csharp64");
} else {
    SetDllDirectory("C:\\Program Files (x86)\\uMundo\\share\\umundo\\bindings\\csharp");
}</pre></p>
	</dd>

	<dt><b>The C# bindings are leaking memory!</b></dt>
	<dd>No they are not, the bindings implement what Microsoft calls the 
		<a href="http://msdn.microsoft.com/en-us/library/498928w2.aspx">Dispose Pattern</a>.
		It seems like the managed part of the various objects (e.g. a Message) do not build up
		enough pressure for the garbage collector to run. You will have to call their <tt>Dispose()</tt>
		method manually or use the <tt>using</tt> statement.
	</dd>

	<dt><b>No Java binding as JNI cannot be found for Debian / Ubuntu on AMD64!</b></dt>
	<dd>The FindJNI.cmake distributed e.g. with Debian Wheezy is buggy and will sometimes fail to pickup the 
		java libraries required for the language bindings:
		<pre>Could NOT find JNI (missing:  JAVA_AWT_LIBRARY JAVA_JVM_LIBRARY)</pre>
		The solution is help CMake by passing the paths to <tt>libjvm.so</tt> and <tt>libawt.so</tt> when preparing
		the build folder, e.g.:
		<pre>
cmake -DJAVA_AWT_LIBRARY=/usr/lib/jvm/java-7-openjdk-amd64/jre/lib/amd64/libawt.so \
      -DJAVA_JVM_LIBRARY=/usr/lib/jvm/java-7-openjdk-amd64/jre/lib/amd64/server/libjvm.so \
      &lt;UMUNDO_SRC>
		</pre>
		
	</dd>

	<dt><b>When using uMundo from the installers on linux <tt>libpcre.so.3</tt> was not found, what gives?</b></dt>
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
