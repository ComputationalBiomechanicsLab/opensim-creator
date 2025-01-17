#!/usr/bin/env perl

$count = 0;

foreach (@ARGV) {
    print "#define\tinterface_", $_, "\t\t", $count, "\n";
    $count ++;
}

print "#ifdef USE_FUNCTABLE\n";

print "#define MAX_PROF_TABLE ", $count, "\n";

print "static char *func_table[] = {\n";

foreach (@ARGV) {
    print "\"", $_, "\",\n";
}

print "};\n";
print "#endif\n";

