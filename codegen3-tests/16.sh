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
localVariables retval a b c d e f g h i
a := 8
b := 2
c := 23
d := 43
e := 10
f := 9
g := 11
h := 13
i := 15
retval := call callee a b c d e f g h i
return retval
end function
EOF

cat <<EOF | ${cmd} > callee.s
function callee
localVariables p1 p2 p3 p4 p5 p6 p7 p8 p9 var1 var2
parameters p1 p2 p3 p4 p5 p6 p7 p8 p9
var1 := p9
var2 := var1
return var2
end function
EOF

correctexit=15

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
