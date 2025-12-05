#Usage python kp_coverage.py
import os
import sys

PROJECT_ROOT = os.path.abspath(os.path.abspath(__file__) + "/../../")
lcovcmd = "cd %s;lcov -c -d ./ -o unit-test.info --rc lcov_branch_coverage=1 --exclude '*/test/*' --exclude '*bits/*' --exclude '*ext/*' --exclude '*/parallel/*' --exclude '*/common/*' --exclude '*/device/*' --exclude '*/kuaf_crypto.c' --exclude '*/kuaf_shm.c'" % PROJECT_ROOT
gencmd = "cd %s;genhtml unit-test.info -o all --branch-coverage --rc lcov_branch_coverage=1" % PROJECT_ROOT

os.system(lcovcmd)
os.system(gencmd)