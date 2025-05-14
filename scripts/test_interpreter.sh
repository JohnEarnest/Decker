#!/usr/bin/env bash
# integration tests for lil based on
# a collection of blessed reference outputs and error logs.

INTERPRETER=$1
echo "running tests against ${INTERPRETER}..."

positive_test() {
	rm -rf temp.out
	rm -rf temp.err
	$INTERPRETER "$1" >> temp.out 2>> temp.err
	ec=$?
	if [ -s temp.err ]; then
		echo "errors running test ${1}:"
		cat temp.err
		exit 1
	elif [ $ec != 0 ]; then
		echo "crash running test ${1}."
		exit 1
	elif [ ! -f temp.out ]; then
		echo "no output for ${1}."
		exit 1
	elif ! cmp -s temp.out $2; then
		echo "reference input doesn't match for ${1}:"
		diff --strip-trailing-cr $2 temp.out
		exit 1
	fi
}

negative_test() {
	rm -rf temp.out
	rm -rf temp.err
	$INTERPRETER "$1" >> temp.out 2>> temp.err
	ec=$?
	if ! diff -q --strip-trailing-cr temp.err $2; then
		echo "reference error doesn't match for ${1}:"
		echo "expected:"
		cat $2
		echo "got:"
		cat temp.err
		exit 1
	fi
}

for filename in tests/*.lil; do
	if [ -f ${filename%.*}.out ]; then
		positive_test $filename ${filename%.*}.out
	elif [ -f ${filename%.*}.err ]; then
		negative_test $filename ${filename%.*}.err
	else
		echo "no reference file found for test ${filename}!"
		exit 1
	fi
done
echo "all interpreter tests passed."
rm -rf temp.out
rm -rf temp.err
