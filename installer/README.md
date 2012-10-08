# uMundo installers

Installer packages are available at <a href="http://umundo.tk.informatik.tu-darmstadt.de/installer/">the umundo
server at the TUD</a>.

The installers contain prebuilt uMundo libraries in both debug and release configuration. Debug libraries are suffixed with <tt>*_d</tt>
and contain unstripped symbols and debugging information. All installers contain libraries for Android development and the MacOSX
installer contains support for iOS devices as well.

All the different file formats per platform contain the same file, it's just a matter of taste whether you prefer archives or installation
packages.

## Contents of Windows Installer (as of v0.1.1)

The contents of the other installers are pretty similar, except that the MacOSX
installer contains iOS libraries and samples and every installer but the Windows
installer is missing the C# related files.

I just realized that there is some cruft in the installers. I will fix it for 
0.1.2 - sorry for the confusion.

<table>
<tr>
<td valign="top">
<pre>
Executables
  RPC protobuf plugin for C++
  RPC protobuf plugin for Java
  Diagnosis Tool (somewhat unmaintained)
  Test Messages (i=incoming, o=outgoing)
C++ Header Files (excerpt)

  Core Layer
  Remote Procedure Calls
  Serialization
  Utilities
Pure C++ Libraries
  SWIG C# backend
  
  SWIG C# backend (debug)

  SWIG Java backend
  
  SWIG Java backend (debug)

  Core layer library
  Core layer library (debug)
  Core layer debug DB
  SWIG C# backend debug DB
  SWIG Java backend debug DB
  RPC layer library
  RPC layer library (debug)
  RPC layer debug DB
  Serialization layer library
  Serialization layer library (debug)
  Serialization layer debug DB
  Utilities library
  Utilities library (debug)
  Utilities debug DB


Libraries for Android 2.2 or greater
  
  JNI library
  JNI library (debug)
  JAR for Android (without JNI binaries)
Language Bindings
  JAR for all desktops (JNI inside)
  Managed Code DLL for C#
Prebuilt libraries (not needed?)











Sample Programs

  umundo-pingpong for Android
  
  
  
  
  
  
  
  Libraries in here by mistake :(
  
  
  Consider these to be place-holders and
  replace by current libraries from above
  Replace the JAR as well, sorry ..
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  C# Samples
    umundo-pingpong in C#
    
    
    
    
    
    
    
    Serialization in C#











    Serialization in C# (outdated)









  Java Samples
  
    Chat using the core layer






    Chat using the rpc layer









    Chat using the s11n layer

</pre>
</td>

<td valign="top">
<pre>
├── bin
│   ├── protoc-umundo-cpp-rpc.exe
│   ├── protoc-umundo-java-rpc.exe
│   ├── umundo-monitor.exe
│   └── umundo-pingpong.exe
├── include
│   └── umundo
│       ├── core.h
│       ├── rpc.h
│       ├── s11n.h
│       └── util.h
├── lib
│   ├── umundoNativeCSharp.dll
│   ├── umundoNativeCSharp.lib
│   ├── umundoNativeCSharp_d.dll
│   ├── umundoNativeCSharp_d.lib
│   ├── umundoNativeJava.dll
│   ├── umundoNativeJava.lib
│   ├── umundoNativeJava_d.dll
│   ├── umundoNativeJava_d.lib
│   ├── umundocore.lib
│   ├── umundocore_d.lib
│   ├── umundocore_d.pdb
│   ├── umundonativecsharp_d.pdb
│   ├── umundonativejava_d.pdb
│   ├── umundorpc.lib
│   ├── umundorpc_d.lib
│   ├── umundorpc_d.pdb
│   ├── umundoserial.lib
│   ├── umundoserial_d.lib
│   ├── umundoserial_d.pdb
│   ├── umundoutil.lib
│   ├── umundoutil_d.lib
│   └── umundoutil_d.pdb
└── share
    └── umundo
        ├── android-8
        │   ├── armv5te
        │   │   ├── libumundoNativeJava.so
        │   │   └── libumundoNativeJava_d.so
        │   └── umundo.jar
        ├── lib
        │   ├── umundo.jar
        │   └── umundoCSharp.dll
        ├── prebuilt
        │   ├── dnssd.lib
        │   ├── dnssd.lib.pdb
        │   ├── libzmq.lib
        │   ├── libzmq_d.lib
        │   ├── libzmq_d.pdb
        │   ├── mDNSEmbedded.lib
        │   ├── mDNSEmbedded_d.lib
        │   ├── mdnsresponder.vc100.pdb
        │   ├── pcre.lib
        │   ├── pcred.lib
        │   └── pcred.pdb
        └── samples
            ├── android
            │   └── umundo-pingpong
            │       ├── AndroidManifest.xml
            │       ├── gen
            │       │   └── org
            │       │       └── umundo
            │       │           └── samples
            │       │               ├── BuildConfig.java
            │       │               └── R.java
            │       ├── libs
            │       │   ├── android-support-v4.jar
            │       │   ├── armeabi
            │       │   │   ├── libumundoNativeJava.so
            │       │   │   └── libumundoNativeJava_d.so
            │       │   └── umundo.jar
            │       ├── proguard-project.txt
            │       ├── project.properties
            │       ├── res
            │       │   ├── drawable-hdpi
            │       │   │   ├── ic_action_search.png
            │       │   │   └── ic_launcher.png
            │       │   ├── drawable-mdpi
            │       │   │   ├── ic_action_search.png
            │       │   │   └── ic_launcher.png
            │       │   ├── drawable-xhdpi
            │       │   │   ├── ic_action_search.png
            │       │   │   └── ic_launcher.png
            │       │   ├── layout
            │       │   │   └── activity_umundo_android.xml
            │       │   └── values
            │       │       └── strings.xml
            │       └── src
            │           └── org
            │               └── umundo
            │                   └── samples
            │                       └── UMundoAndroidActivity.java
            ├── csharp
            │   ├── umundo-pingpong
            │   │   ├── Program.cs
            │   │   ├── Properties
            │   │   │   └── AssemblyInfo.cs
            │   │   ├── umundo-pingpong.csproj
            │   │   └── umundo-pingpong.csproj.user
            │   ├── umundo-pingpong.sln
            │   ├── umundo-pingpong.suo
            │   ├── umundo-s11ndemo
            │   │   ├── umundo-s11ndemo
            │   │   │   ├── Program.cs
            │   │   │   ├── Properties
            │   │   │   │   └── AssemblyInfo.cs
            │   │   │   ├── amessage.cs
            │   │   │   ├── amessage.proto
            │   │   │   ├── umundo-s11ndemo.csproj
            │   │   │   ├── umundo-s11ndemo.csproj.user
            │   │   │   └── umundoNativeCSharp_d.dll
            │   │   ├── umundo-s11ndemo.sln
            │   │   └── umundo-s11ndemo.suo
            │   ├── umundo.s11n
            │   │   ├── ITypedReceiver.cs
            │   │   ├── Properties
            │   │   │   └── AssemblyInfo.cs
            │   │   ├── TypedPublisher.cs
            │   │   ├── TypedSubscriber.cs
            │   │   ├── umundo.s11n.csproj
            │   │   └── umundo.s11n.csproj.user
            │   ├── umundo.s11n.sln
            │   └── umundo.s11n.suo
            └── java
                ├── core
                │   └── chat
                │       ├── build.properties
                │       ├── build.xml
                │       └── src
                │           └── org
                │               └── umundo
                │                   └── Chat.java
                ├── rpc
                │   └── chat
                │       ├── build.properties
                │       ├── build.xml
                │       ├── proto
                │       │   └── ChatS11N.proto
                │       └── src
                │           └── org
                │               └── umundo
                │                   └── Chat.java
                └── s11n
                    └── chat
                        ├── build.properties
                        ├── build.xml
                        ├── proto
                        │   └── ChatS11N.proto
                        └── src
                            └── org
                                └── umundo
                                    └── Chat.java
</td>
</tr>
</pre>
</table>