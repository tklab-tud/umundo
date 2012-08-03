Set shell    = CreateObject( "WScript.Shell" )
Set fso      = CreateObject("Scripting.FileSystemObject")
Set ip       = CreateObject("WScript.Network")
Set procEnv  = shell.Environment("Process")

ME_NAME  = Wscript.ScriptFullName
TEST_DIR = fso.GetParentFolderName(fso.GetFile(ME_NAME))
TESTS    = TEST_DIR + "\tests"
HOSTS    = TEST_DIR + "\hosts"
HOSTNAME = LCase(ip.ComputerName)
TESTFILE = HOSTS + "\" + HOSTNAME + ".ctest"

CTEST_SUBMIT_TYPE = shell.ExpandEnvironmentStrings("%CTEST_SUBMIT_TYPE%")
If CTEST_SUBMIT_TYPE = "%CTEST_SUBMIT_TYPE%" Then
	CTEST_SUBMIT_TYPE = "Experimental"
	procEnv("CTEST_SUBMIT_TYPE") = CTEST_SUBMIT_TYPE
End If

UMUNDO_SOURCE_DIR = shell.ExpandEnvironmentStrings("%UMUNDO_SOURCE_DIR%")
If UMUNDO_SOURCE_DIR = "%UMUNDO_SOURCE_DIR%" Then
	UMUNDO_SOURCE_DIR = fso.GetParentFolderName(fso.GetParentFolderName(TEST_DIR))
	procEnv("UMUNDO_SOURCE_DIR") = UMUNDO_SOURCE_DIR
End If

UMUNDO_SOURCE_DIR = shell.ExpandEnvironmentStrings("%UMUNDO_SOURCE_DIR%")
if (NOT fso.FileExists(UMUNDO_SOURCE_DIR + "\CMakeLists.txt")) Then
	MsgBox "Could not find uMundo Source for " + ME_NAME
	WScript.Quit
End If

if (NOT fso.FileExists(TESTFILE)) Then
	MsgBox "Could not find test file for this host at " + TESTFILE
	WScript.Quit
End If


' Aqcuire lock to avoid concurrent builds
' this will throw a permission denied error :(

Set buildLock = fso.OpenTextFile(TESTFILE, 8, True)

' Check github for updates and quit when nothing's new
if (CTEST_SUBMIT_TYPE = "Continuous") Then
	shell.CurrentDirectory = UMUNDO_SOURCE_DIR
	Set oExec = shell.Exec("git pull")
	GIT_SYNC  = oExec.StdOut.ReadLine
	if (GIT_SYNC = "Already up-to-date.") Then
		WScript.Quit
	End If
End If

shell.CurrentDirectory = TEST_DIR
'"C:\Program Files\Microsoft Visual Studio 10.0\VC\vcvarsall.bat
Set exec = shell.Exec("C:\Program Files\Microsoft Visual Studio 10.0\VC\vcvarsall.bat")
exec.StdOut.ReadAll()
Set exec = shell.Exec("ctest -VV --timeout 100 -S " + TESTFILE)
'Set exec = shell.Exec("cmd.exe /c dir %windir%\*")
Do While exec.Status = 0
    WScript.Sleep 100
    WScript.StdOut.Write(exec.StdOut.ReadLine())
    WScript.StdErr.Write(exec.StdErr.ReadLine())
Loop

MsgBox("CTest:" + exec.StdOut.ReadAll)
