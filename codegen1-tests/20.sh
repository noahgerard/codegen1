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
localVariables a b c d fact # fact must be last for print
a := 5
fact := 1
b := 1
loop:
if b > a goto after
fact := fact * b
b := b + 1
goto loop
after:
retval := call print_int fact # DO NOT CHANGE
return 0 # DO NOT CHANGE
end function # DO NOT CHANGE
EOF

correctoutput=120

gcc -o main main.s ${iolib}
output=$(./main)
if [[ $output -eq $correctoutput ]]; then
  echo "success"
  exit 0
else
  echo "failure"
  exit 1
fi
