#!/bin/sh
#
# gencalls.sh - generate calls.c and calls.h
#
# Usage: gencalls.sh callspecs-file

if [ "x$1" = x ]; then
    echo "Usage: $0 callspecs-file"
    exit 1
fi

awk < $1 '

    BEGIN {
	type["ptr"] = "void *";
	type["int"] = "int";
	type["off"] = "off_t";
	type["size"] = "size_t";
	fmt["ptr"] = "%p";
	fmt["int"] = "%d";
	fmt["off"] = "%lld";
	fmt["size"] = "%lu";
	cast["ptr"] = "";
	cast["int"] = "";
	cast["off"] = "(long long)";
	cast["size"] = "(unsigned long)";

	printf "/* Automatically generated file; do not edit */\n";
	printf "#include <sys/types.h>\n";
	printf "#include <sys/stat.h>\n";
	printf "#include <assert.h>\n";
	printf "#include <unistd.h>\n";
	printf "#include <stdio.h>\n";
	printf "#include <stdlib.h>\n";
	printf "#include <errno.h>\n";
	printf "#include <err.h>\n";
	printf "\n";
	printf "#include \"extern.h\"\n";
	printf "\n";

	printf "typedef void (*tryfunc)(int dofork);\n";
	printf "\n";

	n=0;
    }

    {
	printf "static\n";
	printf "void\n";
	printf "try_%s(int dofork)\n", $2;
	printf "{\n";
	for (i=3; i<=NF; i++) {
	    printf "\t%s a%d = rand%s();\n", type[$i], i-3, $i;
	}
	printf "\tint result, pid, status;\n";
	printf "\tchar buf[128];\n";
	printf "\n";

	printf "\tsnprintf(buf, sizeof(buf), \"%s(", $2;
	for (i=3; i<=NF; i++) {
	    printf "%s", fmt[$i];
	    if (i<NF) printf ", ";
	}
	printf ")\",\n\t\t";
	for (i=3; i<=NF; i++) {
	    printf "%s(a%d)", cast[$i], i-3;
	    if (i<NF) printf ", ";
	}
	printf ");\n";
	printf"\tprintf(\"%%-47s\", buf);\n";
	#printf "\tfflush(stdout);\n";
	printf "\n";

	printf "\tpid = dofork ? fork() : 0;\n";
	printf "\tif (pid<0) {\n";
	printf "\t\terr(1, \"fork\");\n";
	printf "\t}\n";
	printf "\tif (pid>0) {\n";
	printf "\t\twaitpid(pid, &status, 0);\n";
	printf "\t\treturn;\n";
	printf "\t}\n";
	printf "\n";

	printf "\tresult = %s(", $2;
	for (i=3; i<=NF; i++) {
	    printf "a%d", i-3;
	    if (i<NF) printf ", ";
	}
	printf ");\n";

	printf "\tprintf(\" result %%d, errno %%d\\n\", result, errno);\n";
	printf "\tif (dofork) {\n";
	printf "\t\texit(0);\n";
	printf "\t}\n";

	printf "}\n";
	printf "\n";

	asst[$2] = $1;
	all[n++] = $2;
    }

    END {
	for (a=2; a<=5; a++) {
	    printf "static tryfunc funcs%d[] = {\n", a;
	    for (i=0; i<n; i++) {
		if (asst[all[i]] <= a) {
		    printf "\ttry_%s,\n", all[i];
		}
	    }
	    printf "\tNULL\n";
	    printf "};\n";
	    printf "\n";
	}

	printf "static tryfunc *tables[4] = {\n";
	printf "\tfuncs2,\n";
	printf "\tfuncs3,\n";
	printf "\tfuncs4,\n";
	printf "\tfuncs5,\n";
	printf "};\n";
	printf "\n";

	printf "void\n";
	printf "trycalls(int asst, int dofork, int count)\n"
	printf "{\n";
	printf "\ttryfunc *list;\n";
	printf "\tint i, j;\n";
	printf "\n";

	printf "\tassert(asst>=2 && asst<=5);\n";
	printf "\tlist = tables[asst-2];\n";
	printf "\n";

	printf "\tfor (i=0; i<count; i++) {\n";
	printf "\t\tfor (j=0; list[j]; j++) {\n";
	printf "\t\t\t(*list[j])(dofork);\n";
	printf "\t\t}\n";
	printf "\t}\n";
	printf "}\n";
	printf "\n";
    }
'
