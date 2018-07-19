#!/bin/bash
FILE=./doxyfile
echo "Generating documentation..."
doxygen $FILE

echo "Finished Documentation"
