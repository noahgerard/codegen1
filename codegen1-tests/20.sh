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
localVariables n fact i retval # DO NOT CHANGE
n := 5
fact := 1
i := 1
loop:
if i > n goto end
fact := fact * i
i := i + 1
goto loop
end:
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
