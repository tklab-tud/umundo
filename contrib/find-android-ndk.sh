#
# Try to find the android sdk
#
# First look in the environment variable ANDROID_NDK_ROOT, then try some 
# locations from different development deployments. Add your own elif 
# block if you happen to use it often.
#

if [ ! -f ${ANDROID_NDK_ROOT}/ndk-build ]; then
	# try some convinient default locations
	if [ -d /opt/android-ndk-r7c ]; then
		ANDROID_NDK_ROOT="/opt/android-ndk-r7c"
	elif [ -d /Developer/Applications/android-ndk-r7c ]; then
		ANDROID_NDK_ROOT="/Developer/Applications/android-ndk-r7c"
	elif [ -d /home/sradomski/Documents/android-dev/android-ndk-r7c ]; then
		ANDROID_NDK_ROOT="/home/sradomski/Documents/android-dev/android-ndk-r8"
	else
		echo
		echo "Cannot find android-ndk, call script as"
		echo "ANDROID_NDK_ROOT=\"/path/to/android/ndk\" ${ME}"
		echo
		exit
	fi
	export ANDROID_NDK="${ANDROID_NDK_ROOT}"
fi

echo ${ANDROID_NDK_ROOT}