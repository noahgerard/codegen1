#!/bin/bash

echo "${0}"

cmd=${1}

iolib=$(realpath $(dirname ${0})/.iolib.c)
getref=$(realpath $(dirname ${0})/.getref.gdb)

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
result := &a
retval := call print_int result # DO NOT CHANGE
return 0 # DO NOT CHANGE
end function # DO NOT CHANGE
EOF

gcc -g -o main main.s ${iolib}

# use gdb to get the value of rbp after the prologue, print it out, then run the rest of the program to print out the value of the variable holding the reference.  the remaining steps pick out the two numbers and subtract them with bc
gdb --batch --command=${getref} ./main 2>/dev/null >main.out
output=$(cat main.out | tail -n3 | head -n2 | sed 's/$1 = //' | tr '\n' ' ' | awk -F' ' '{print$1"-"$2}' | bc -lq)

# since the program gets the ref of the first local variable, its offset should be 16 (which skips over the rbx callee-saved register on the stack
correctoutput=16

if [[ $output -eq $correctoutput ]]; then
	echo "success"
	exit 0
else
	echo "failure"
	exit 1
fi
