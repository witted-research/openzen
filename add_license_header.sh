#!/bin/bash

for i in $(find src/ -name "*.h" -or -name "*.cpp")
do
  if ! grep -q Copyright $i
  then
    echo "Will add copyright notice to $i"
    cat license_header.txt $i >$i.new && mv $i.new $i
  fi
done

