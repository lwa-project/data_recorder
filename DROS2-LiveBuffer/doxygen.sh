#!/bin/bash
FILE=/home/chwolfe2/workspace-2013/DROS2-LiveBuffer/doxyfile
if [ `hostname` == "xps-dev" ]; then
	echo "Generating documentation..."
	doxygen $FILE

	echo "Publishing documentation to webserver..."
	rsync -e "ssh -i /home/chwolfe2/.ssh/apache_push_id_rsa" -raz /home/chwolfe2/DROS_docs/output-livebuffer/* chwolfe2@192.168.1.10:/home/chwolfe2/DROS_docs/output-livebuffer/.

	echo "Finished Documentaiton"
else
	echo "Bypassing Doxygen generation (wrong build host)... "
fi
