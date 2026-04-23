#!/bin/bash

echo "${0}"

cmd=${1}

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
localVariables retval a b c d e f g
a := 8
b := 2
c := 23
d := 43
e := 10
f := 9
g := 11
retval := call callee a b c d e f g
return retval
end function
EOF

cat <<EOF | ${cmd} > callee.s
function callee
localVariables p1 p2 p3 p4 p5 p6 p7
parameters p1 p2 p3 p4 p5 p6 p7
return p7
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
