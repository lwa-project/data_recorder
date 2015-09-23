#!/bin/bash

ls /LWA/runtime/runtime.log.* | xargs -n1 ~mcsdr/uploadLogfile.py

#dir=`mktemp -d `
#cd ${dir}
#
#files=`ls /LWA/runtime/runtime.log.* ` 
#for file in $files; do 
#	base=`basename ${file} `
#	newname=`echo ${base} | sed -e 's/.tgz//g;' `
#	cp ${file} ${dir}/
#	tar xf ${base}
#	mv runtime.log ${newname}
#	rm ${base}
#	~mcsdr/uploadLogfile.py ${newname}
#done
#
#cd /tmp
#rm -rf ${dir}

