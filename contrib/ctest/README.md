# uMundo CTest scripts

If you want to contribute a build-slave, just create a file called 
<tt>hosts/&lt;HOSTNAME>.ctest</tt> - have a look at the existing host
files. Then setup your crontab as follows:

    50       */4   *   *   *       CTEST_SUBMIT_TYPE="Experimental"	/var/builds/umundo/contrib/ctest/run-tests.cron
    0        2     *   *   *       CTEST_SUBMIT_TYPE="Nightly"      /var/builds/umundo/contrib/ctest/run-tests.cron
    */2      *     *   *   *       CTEST_SUBMIT_TYPE="Continuous"   /var/builds/umundo/contrib/ctest/run-tests.cron
    0        2     *   *   *       /var/builds/umundo/contrib/ctest/deploy-umundo-docs.cron 

<b>Note:</b> Be aware that <tt>run-tests.cron</tt> is under version control and 
might get updated from git with potentially surprising content. Copy the whole 
ctest directory someplace safe if you are concerned and make sure to specify 
<tt>UMUNDO_SOURCE_DIR=/umundo/checkout/here</tt> in the crontab line.

<b>Warning:</b> Do not use the source directory you are developing in with
<tt>run-tests.cron</tt>. It will force sync the local repository with the
current GIT head. Use a dedicated test checkout.