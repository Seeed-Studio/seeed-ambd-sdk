#!/bin/sh
for file in `find . -type f -name "*.sh"`; do
fileName=${file}
echo ${fileName}
chmod +x ${fileName}
done