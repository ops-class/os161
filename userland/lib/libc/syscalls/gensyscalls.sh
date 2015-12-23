#!/bin/sh
#
# gensyscalls.sh
# Usage: ./gensyscalls.sh < syscalls.h
#
# Parses the kernel's syscalls.h into the body of syscalls.S
#

# tabs to spaces, just in case
tr '\t' ' ' |\
awk '
    # Do not read the parts of the file that are not between the markers.
    /^\/\*CALLBEGIN\*\// { look=1; }
    /^\/\*CALLEND\*\// { look=0; }

    # And, do not read lines that do not match the approximate right pattern.
    look && /^#define SYS_/ && NF==3 {
	sub("^SYS_", "", $2);
	# print the name of the call and the number.
	print $2, $3;
    }
' | awk '{
	# output something simple that will work in syscalls.S.
	printf "SYSCALL(%s, %s)\n", $1, $2;
}'
