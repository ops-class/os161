#!/bin/sh
# installheaders.sh - install header files
# usage: installheaders.sh srcdir destdir
# srcdir/*.h is copied to destdir, if different.
#

if [ $# != 2 ]; then
    echo "$0: Usage: $0 srcdir destdir" 1>&2
    exit 1
fi

for H in "$1"/*.h; do
    BH=`basename "$H"`
    if diff "$H" "$2/$BH" >/dev/null 2>&1; then
        :
    else
	echo cp "$H" "$2/$BH"
	cp "$H" "$2/$BH"
    fi
done
