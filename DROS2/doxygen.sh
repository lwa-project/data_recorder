#!/bin/bash
FILE=`dirname $0`/doxyfile
echo "Generating documentation..."
doxygen $FILE

echo "Finished Documentation"
