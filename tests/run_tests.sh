#!/bin/sh

echo ""
for i in $(find . -exec file {} \; | grep -i elf | awk '{print $1}' | awk '{print substr($0, 1, length($0)-1)}'); do
    echo "***** RUNNING vagrind $i *****";
    valgrind $i
    echo ""
    echo ""
done;

rm *.img

for i in $(find . -exec file {} \; | grep -i elf | awk '{print $1}' | awk '{print substr($0, 1, length($0)-1)}'); do
    echo "***** RUNNING $i *****";
    $i
    echo ""
    echo ""
done;
