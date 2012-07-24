# umundo.s11n ReadMe

## Using ProtoBuf on Windows

1. Download protobuf-2.4.1.zip from google and unpack it at a convenient location.
2. Open the MS Visual Studio solution in vsprojects/.
3. Build the whole solution (not just libprotobuf), once for "Debug" and once for "Release".
4. Ignore 1755 errors and 20 warnings (they are from gtest).

That is supposed to create all necessary libraries and executables. To get the umundo build process to use them:

1. Start the CMake-GUI.
2. Setup build and source directory as usual and hit "Configure".
3. Enable the "Advanced" checkbox in the GUI.
4. Find and provide values for the single <tt>PROTOBUF_SRC_ROOT_FOLDER</tt> field - the rest is automatically determine by CMake.
5. Click "Configure" again.
6. Click "Generate".

