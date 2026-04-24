for i in codegen3-tests/*.sh; do # loop over all test scripts
  bash $i $(realpath CodeGen)    # run the test
  echo $?                        # emit the exit code
  echo ""                        # give some extra space
done                             # end the loop
