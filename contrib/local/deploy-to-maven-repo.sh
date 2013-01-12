#!/bin/bash
#
# Deploy umundo.jar into maven repository
#
mvn2 deploy:deploy-file -DgroupId=org.umundo -DartifactId=umundocore -Dversion=0.3.1 \
  -DgeneratePom=true \
  -Dpackaging=jar \
  -Dfile=../../package/umundo.jar \
  -DrepositoryId=tu.darmstadt.umundo \
  -Durl=scp://umundo.tk.informatik.tu-darmstadt.de/var/www/html/umundo/maven2
