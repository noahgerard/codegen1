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
a := 13
b := 5
result := a % b
retval := call print_int result # DO NOT CHANGE
return 0 # DO NOT CHANGE
end function # DO NOT CHANGE
EOF

correctoutput=3

gcc -o main main.s ${iolib}
output=$(./main)
if [[ $output -eq $correctoutput ]]; then
  echo "success"
  exit 0
else
  echo "failure"
  exit 1
fi
