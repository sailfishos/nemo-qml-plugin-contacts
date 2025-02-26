#!/bin/sh

export TESTDIR=$1
export TESTCASE=$2
export DEVICEUSER=$(getent passwd $(grep '^UID_MIN' /etc/login.defs |  tr -s ' ' | cut -d ' ' -f2) | sed 's/:.*//')
export LIBCONTACTS_TEST_MODE=1
/usr/sbin/run-blts-root /bin/su -g privileged -c "/opt/tests/$TESTDIR/$TESTCASE" "$DEVICEUSER"
