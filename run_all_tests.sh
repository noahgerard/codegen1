for i in codegen2-tests/*.sh; do # loop over all test scripts
  bash $i $(realpath CodeGen)    # run the test
  echo $?                        # emit the exit code
  echo ""                        # give some extra space
done                             # end the loop
