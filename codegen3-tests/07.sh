#!/bin/bash

echo "${0}"

cmd=${1}

testcmd="/usr/bin/ls"

initialize () {
  mkdir testdir
	cd testdir
}

cleanup () {
	cd ../
	rm -rf testdir
}

initialize
trap "cleanup" EXIT SIGHUP SIGINT SIGQUIT SIGABRT SIGALRM SIGTERM

cat <<EOF | ${cmd} > main.s
function main
localVariables retval
retval := call callee
return retval
end function
EOF

cat <<EOF | ${cmd} > callee.s
function callee
localVariables v
v := 4
return v
end function
EOF

correctexit=4

gcc -o main main.s callee.s

./main
exitcode=$?

if [[ $exitcode -eq $correctexit ]]; then
	echo "success"
	exit 0
else
	echo "failure"
	exit 1
fi
