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
localVariables retval p1 p2
p1 := 9
p2 := 11
retval := call callee p1 p2
return retval
end function
EOF

cat <<EOF | ${cmd} > callee.s
function callee
localVariables inp1 inp2
parameters inp1 inp2
return inp2
end function
EOF

correctexit=11

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
