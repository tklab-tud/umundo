# Cross-Compiling uMundo

<b>Note:</b> Before you attempt to cross-compile, make sure you can
[build uMundo](https://github.com/tklab-tud/umundo/blob/master/docs/BUILDING.md)
for your host system.

To cross-compile with CMake is to provide a [toolchain file](http://www.cmake.org/Wiki/CMake_Cross_Compiling)
to CMake at configure-time. It will set the compiler, the linker, required flags,
include directories and so on.

There are three such toolchain files to cross-compile umundo in <tt>contrib/cmake/</tt>,
namely:

 * <b>iOS Devices</b>: [CrossCompile-iOS.cmake](https://github.com/tklab-tud/umundo/blob/master/contrib/cmake/CrossCompile-iOS.cmake)
 * <b>iOS Simulator</b>: [CrossCompile-iOS-Sim.cmake](https://github.com/tklab-tud/umundo/blob/master/contrib/cmake/CrossCompile-iOS-Sim.cmake)
 * <b>Android</b>: [CrossCompile-Android.cmake](https://github.com/tklab-tud/umundo/blob/master/contrib/cmake/CrossCompile-Android.cmake)

While the iOS toolchain files for the device and the simulator are still quite
approachable, the Android one is rather intimidating. So intimidating in fact,
that we know longer write our own, but take the one maintained by the
[android-cmake](http://code.google.com/p/android-cmake/source/browse/toolchain/android.toolchain.cmake)
project.

To invoke CMake with a toolchain file, you will have to pass
<tt>-DCMAKE_TOOLCHAIN_FILE=/path/to/toolchain.cmake</tt> as a command-line parameter
or select the file from the CMake-GUI when setting up a new build directory.

## iOS (iPhone, iPad, iPod)

<b>Note:</b> You can only cross-compile for iOS on a Mac OSX host.

The iOS platform prohibits the loading of 3rd party shared libraries - everything
needs to be statically compiled. CMake will realize that you are building for iOS
if you use the toolchain files from above and sets all build parameters accordingly.

To build umundo for the iOS simulator:

    $ mkdir -p build/ios
    $ cd build/ios
    $ cmake -DCMAKE_TOOLCHAIN_FILE=../../contrib/cmake/CrossCompile-iOS-Sim.cmake ../..
    $ make -j2

If you have a recent Xcode installation, cmake will automatically detect the latest
installed iOS SDK to build umundo. You can explicitly choose a version by exporting e.g.
<tt>IOS_SDK_VERSION=5.1</tt>, but if it is not found with your Xcode from <tt>xcode-select</tt>
the latest version is chosen silently. The same is true, when choosing the toolchain file
for the devices.

<b>Note:</b> We are using the SDK version as part of the path to the precompiled libraries for
iOS. If there is a new version of iOS, make sure the prebuilt path exists by symlinking
e.g. the <tt>contrib/prebuilt/ios/5.0</tt> directory to the new version.

### Universal Libraries

When you want to deploy applications using uMundo, you might want to build universal
libraries that will run in the simulator and on the device. Binaries for the simulator
are built for _i386_ and the devices need _arm_ compiled code. You can cross-compile
both versions individually using their toolchain files and create universal libraries
using <tt>lipo</tt> by hand, or you can run the script in <tt>contrib/build-umundo-ios.sh</tt>
to do it for you.

The script invokes CMake with the <tt>-DDIST_PREPARE</tt> flag, which will cause
the built libraries to end up in <tt>package/cross-compiled/</tt> in the source
directory. The universal libraries will be created there as well:

	epikur:umundo sradomski$ ls -l package/cross-compiled/ios-5.1/
	total 60112
	drwxr-xr-x  3 sradomski  staff       102 Aug  1 19:31 arm
	drwxr-xr-x  3 sradomski  staff       102 Aug  1 19:30 i386
	-rw-r--r--  1 sradomski  staff   2027132 Aug  1 19:32 libumundocore.ios.a
	-rw-r--r--  1 sradomski  staff  15426868 Aug  1 19:31 libumundocore_d.ios.a
	-rw-r--r--  1 sradomski  staff    768116 Aug  1 19:32 libumundorpc.ios.a
	-rw-r--r--  1 sradomski  staff   8929820 Aug  1 19:31 libumundorpc_d.ios.a
	-rw-r--r--  1 sradomski  staff    311140 Aug  1 19:32 libumundoserial.ios.a
	-rw-r--r--  1 sradomski  staff   2987500 Aug  1 19:31 libumundoserial_d.ios.a
	-rw-r--r--  1 sradomski  staff     33116 Aug  1 19:32 libumundoutil.ios.a
	-rw-r--r--  1 sradomski  staff    277780 Aug  1 19:31 libumundoutil_d.ios.a

The <tt>arm</tt> and <tt>i386</tt> directories contain the libraries for the devices
and simulator respectively.

<b>Note:</b> There is no _convinience_ library including all of umundo and the
dependent libraries. You will have to link them individually with your application
in Xcode.
<b>Update:</b> Yes there is. For iOS we link al static libraries together into a
big <tt>libumundo.ios.a</tt>.


## Android

For Android, we rely on the toolchain file from the
[android-cmake](http://code.google.com/p/android-cmake/source/browse/toolchain/android.toolchain.cmake)
project to setup CMake. As a consequence, we are limited to the Android NDK versions
supported by them. At the time of this writing that includes all NDKs up to version <tt>r8</tt>
(version <tt>r8b</tt> will already complain about missing STL headers). Our build-slave builds uMundo with
NDK versions <tt>r7</tt>, <tt>r7b</tt>, <tt>r7c</tt> and <tt>r8</tt>. Pick your
NDK version and just export <tt>ANDROID_NDK=path/to/ndk</tt> accordingly.

With Android, the devices and the simulator are using the same architecture for
their binaries. The simulator is assumed to be run on a host that can effectively
emulate the CPU from the device. Apart from a slow simulator, that means, that
we can build everything in one swoop:

    $ export ANDROID_NDK=/Developer/Applications/android-ndk-r7c
    $ mkdir -p build/android
    $ cd build/android
    $ cmake -DCMAKE_TOOLCHAIN_FILE=../../contrib/cmake/CrossCompile-Android.cmake ../..
    $ make
    $ make java

The final <tt>make java</tt> is needed as the language bindings are no longer built
with the default target. After building, the libraries are available in <tt>lib/</tt>

	epikur:android sradomski$ ls -l lib/
	total 6600
	-rwxr-xr-x  1 sradomski  staff  1569816 Aug  2 19:53 libumundoNativeJava.so
	-rw-r--r--  1 sradomski  staff  1029070 Aug  2 19:53 libumundocore.a
	-rw-r--r--  1 sradomski  staff   772254 Aug  2 19:57 umundo.jar

The static library <tt>libumundocore.a</tt> is included into the <tt>libumundoNativeJava.so</tt>
library along with some JNI code and not needed when deploying. Have a look at the sample
[Eclipse project for Android](https://github.com/tklab-tud/umundo/tree/master/contrib/samples/android) to continue.

<b>Note:</b> Make sure your <tt>JAVA_HOME</tt> points to a JVM installation that
will run on the device.

## Cross Compiling on Windows

To cross-compile the Android binaries on a Windows system, you cannot use MS
Visual Studio, instead, you need to create Makefiles and use the <tt>make</tt>
binary shipped as part of the <tt>android-ndk-7</tt> and above. 

<b>Note:</b> You need to have installed the protoc compiler plugins, either from 
the installer or by installing the umundo binaries you built from source.

First, download the [Android NDK](http://dl.google.com/android/ndk/android-ndk-r8-windows.zip)
and extract it somewhere, without spaces or special characters in the path. Then
open up the <emph>Advanced system settings</emph> and set the <tt>%ANDROID_NDK%</tt>
environment variable to the path of the ndk installation.

Start the CMake-GUI, select the folder for the umundo source and any folder for
the binaries. Again, try to avoid special characters as part of the path - that
includes spaces. Click <tt>Configure</tt> and select <tt>MinGW Makefiles</tt>
and specify the toolchain file as <tt>&lt;UMUNDO_SRC&gt;/contrib/cmake/CrossCompile-Android.cmake</tt>.
CMake will most likely complain about not finding <tt>CMAKE_MAKE_PROGRAM</tt>, just
select the <tt>&lt;ANDROID_NDK&gt;/prebuilt/windows/bin/make.exe</tt> distributed
with the NDK and hit <tt>Configure</tt> until there are no more red items. Then
hit <tt>Generate</tt>.

If everything worked out, CMake generated a <tt>Makefile</tt> in the build folder
you specified. Navigate there in <tt>cmd.exe</tt> and invoke make:

	$ "%ANDROID_NDK%\prebuilt\windows\bin\make.exe"
	
