#!/bin/sh
# fixdepends.sh - munge gcc -M output
# usage: gcc -M foo.c | fixdepends.sh INSTALLTOP host|native > .depend
# (where INSTALLTOP is $(INSTALLTOP) from the build makefiles)

if [ $# != 2 ]; then
    echo "$0: Usage: $0 INSTALLTOP host|native" 1>&2
    exit 1
fi

INSTALLTOP="$1"
MODE="$2"

case "$MODE" in
    host|native) ;;
    *) echo "$0: invalid mode $MODE; should be host or native" 1>&2
       exit 1
       ;;
esac

awk '
    # Any blank lines appear between object files.
    /^$/ {
	printit();
	next;
    }
    # A line beginning with a space is a second or subsequent line of
    # depends.
    /^ / {
	for (i=1;i<=NF;i++) {
	    if ($i != "\\") {
		deps[++ndeps] = $i;
	    }
	}
	next;
    }
    # Any other line begins a new object file.
    {
	# Print the object file we already have, if any.
	printit();
	# Trim the colon after the object file name.
	object = $1;
	sub(":$", "", object);
	# If in host mode, change .o to .ho.
	if (mode == "host") {
	    sub("\\.o$", ".ho", object);
	}
	# Anything else on the line is a depend.
	for (i=2;i<=NF;i++) {
	    if ($i != "\\") {
		deps[++ndeps] = $i;
	    }
	}
	next;
    }
    END {
	# Print the object file we have, if any, so as to not lose the
	# last one.
	printit();
    }
    function printit() {
	if (ndeps == 0) {
	    # We have no object file in progress, so do nothing.
	    return;
	}
	# Insert MYBUILDDIR before the object file name.
	printf "$(MYBUILDDIR)/%s: \\\n", object;
	for (i=1;i<=ndeps;i++) {
	    printf " %s", deps[i];
	    # Lines before the last need continuation.
	    if (i < ndeps) {
		printf " \\";
	    }
	    printf "\n";
	}
	# We no longer have anything to print.
	ndeps = 0;
	object = "/WRONG";
    }
    # Afterwards run sed to substitute INSTALLTOP.
' "mode=$MODE" | sed '
	s,'"$INSTALLTOP"',$(INSTALLTOP),
'
