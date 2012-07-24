# uMundo CTest scripts

These CTest scripts serve as a starting point if you want to setup a build-slave. You will have to copy them some place
where they can be executed from cron. There are hard-coded paths to <tt>/var/builds</tt> within and you most likely want
to adapt these.

    50       *   *   *   *       UMUNDO_SOURCE_DIR=/var/builds/umundo /var/builds/scripts/test-umundo-experimental.cron
    0        2   *   *   *       UMUNDO_SOURCE_DIR=/var/builds/umundo /var/builds/scripts/test-umundo-nightly.cron
    */2      *   *   *   *       UMUNDO_SOURCE_DIR=/var/builds/umundo /var/builds/scripts/test-umundo-continuous.cron
    58       *   *   *   *       UMUNDO_SOURCE_DIR=/var/builds/umundo /var/builds/scripts/deploy-umundo-docs.cron 
