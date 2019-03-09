#!/bin/sh

tests=0
vcompleted_test=0
echo ""
for i in $(find . -exec file {} \; | grep -i elf | awk '{print $1}' | awk '{print substr($0, 1, length($0)-1)}'); do
    echo "***** RUNNING vagrind $i *****";
    valgrind --error-exitcode=1 $i
    if [ $? == 0 ]; then
        vcompleted_test=$((vcompleted_test + 1))
    fi;
    tests=$((tests + 1))
    echo ""
    echo ""
done;

if [ $tests == 0 ]; then
    echo "No test available. Make sure to run 'make' before 'make run'";
    exit 0
fi;

rm *.img
completed_test=0
for i in $(find . -exec file {} \; | grep -i elf | awk '{print $1}' | awk '{print substr($0, 1, length($0)-1)}'); do
    echo "***** RUNNING $i *****";
    $i
    if [ $? == 0 ]; then
        completed_test=$((completed_test + 1))
    fi;
    echo ""
    echo ""
done;

echo "VALGRIND COMPLETED TEST: $vcompleted_test/$tests";
echo "COMPLETED TEST: $completed_test/$tests";
