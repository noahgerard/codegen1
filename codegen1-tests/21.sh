#!/bin/bash

echo "${0}"

cmd=${1}

iolib=$(realpath $(dirname ${0})/.iolib.c)



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
function main # DO NOT CHANGE
localVariables a b c d e f g result retval # DO NOT CHANGE
a := 9
b := 4
if a < 10 goto true1  # true
c := a + b
goto end1
true1:
c := a - b  # 5
end1:
if a > 10 goto true2  # false
d := a * b  # 36
goto end2
true2:
d := a / b
end2:
result := a
retval := call print_int result # DO NOT CHANGE
return 0 # DO NOT CHANGE
end function # DO NOT CHANGE
EOF

correctoutput=9

gcc -o main main.s ${iolib}

output=$(./main)

if [[ $output -eq $correctoutput ]]; then
	echo "success"
	exit 0
else
	echo "failure"
	exit 1
fi
