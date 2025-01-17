#!/usr/bin/env perl

$hostos   = `uname -s | sed -e s/\-.*//`;    chop($hostos);

#
# 1. Not specified
#   1.1 Automatically detect, then check compiler
#   1.2 If no fortran compiler is detected, gfortran is default with NOFORTRAN definition
# 2. Specified
#   2.1 If path is correct, check compiler
#   2.2 If path is not correct, but still valid compiler name, force setting
#     2.2.2 Path is not correct, invalid compiler name, then gfortran is default with NOFORTRAN definition
#

$makefile = shift(@ARGV);
$config   = shift(@ARGV);

$nofortran = 0;

$compiler = join(" ", @ARGV);
$compiler_bin = shift(@ARGV);
 
# f77 is too ambiguous
$compiler = "" if $compiler eq "f77";

@path = split(/:/, $ENV{"PATH"});

if ($compiler eq "") {

    @lists = ("gfortran", "g95", "frt", "fort", "openf90", "openf95",
	      "sunf77", "sunf90", "sunf95",
              "xlf95", "xlf90", "xlf",
              "ppuf77", "ppuf95", "ppuf90", "ppuxlf",
	      "pathf90", "pathf95",
	      "pgf95", "pgf90", "pgf77", "pgfortran", "nvfortran",
	      "flang", "egfortran",
              "ifort", "nagfor", "ifx", "ftn", "crayftn");

OUTER:
    foreach $lists (@lists) {
        foreach $path (@path) {
            if (-x $path . "/" . $lists) {
                $compiler = $lists;
                $compiler_bin = $lists;
                last OUTER;
            }
        }
    }

}

if ($compiler eq "") {

    $nofortran = 1;
    $compiler = "gfortran";
    $vendor = GFORTRAN;
    $bu       = "_";

} else {

    $data = `which $compiler_bin > /dev/null 2> /dev/null`;
    $vendor = "";

    if (!$?) {

	$data = `$compiler -O2 -S ftest.f > /dev/null 2>&1 && cat ftest.s && rm -f ftest.s`;
	if ($data eq "") {
		$data = `$compiler -O2 -S ftest.f > /dev/null 2>&1 && cat ftest.c && rm -f ftest.c`;
	}
	if ($data =~ /zhoge_/) {
	    $bu       = "_";
	}

	if ($data =~ /Fujitsu/) {

	    $vendor = FUJITSU;
	    $openmp = "-Kopenmp";

	} elsif ($data =~ /GNU/ || $data =~ /GCC/ ) {

            $data =~ s/\(+.*?\)+//g;
	    $data =~ /(\d+)\.(\d+).(\d+)/;
	    $major = $1;
	    $minor = $2;

	    if ($major >= 4) {
		$vendor = GFORTRAN;
		$openmp = "-fopenmp";
	    } else {
		if ($compiler =~ /flang/) {
		    $vendor = FLANG;
		    $openmp = "-fopenmp";
	    } elsif ($compiler =~ /ifort/ || $compiler =~ /ifx/) {
		    $vendor = INTEL;
		    $openmp = "-fopenmp";
	    } elsif ($compiler =~ /pgf/ || $compiler =~ /nvf/) {
		    $vendor = PGI;
		    $openmp = "-mp";
		} else {
		    $vendor = G77;
		    $openmp = "";
		}
	    }
	} elsif ($data =~ /Cray/) {

	    $vendor = CRAY;
	    $openmp = "-fopenmp";

	}

	if ($data =~ /g95/) {
	    $vendor = G95;
	    $openmp = "";
	}

	if ($data =~ /Intel/) {
	    $vendor = INTEL;
	    $openmp = "-fopenmp";
	}

        if ($data =~ /Sun Fortran/) {
            $vendor = SUN;
	    $openmp = "-xopenmp=parallel";
        }

	if ($data =~ /PathScale/) {
	    $vendor = PATHSCALE;
	    $openmp = "-openmp";
	}

	if ($data =~ /Open64/) {
	    $vendor = OPEN64;
	    $openmp = "-mp";
	}

	if ($data =~ /PGF/ || $data =~ /NVF/) {
	    $vendor = PGI;
	    $openmp = "-mp";
	}

	if ($data =~ /IBM XL/) {
	    $vendor = IBM;
	    $openmp = "-openmp";
	}

	if ($data =~ /NAG/) {
	    $vendor = NAG;
	    $openmp = "-openmp";
	}

	# for embedded underscore name, e.g. zho_ge, it may append 2 underscores.
	$data = `$compiler -O2 -S ftest3.f > /dev/null 2>&1 && cat ftest3.s && rm -f ftest3.s`;
	if ($data eq "") {
		$data = `$compiler -O2 -S ftest3.f > /dev/null 2>&1 && cat ftest3.c && rm -f ftest3.c`;
	}
	if ($data =~ / zho_ge__/) {
	    $need2bu       = 1;
	}
	if ($vendor =~ /G95/) {
    	  if ($ENV{NO_LAPACKE} != 1) {
		$need2bu = "";
	  }
	}
    }

    if ($vendor eq "") {

	if ($compiler =~ /g77/) {
	    $vendor = G77;
	    $bu       = "_";
	    $openmp = "";
	}

	if ($compiler =~ /g95/) {
	    $vendor = G95;
	    $bu       = "_";
	    $openmp = "";
	}

	if ($compiler =~ /gfortran/) {
	    $vendor = GFORTRAN;
	    $bu       = "_";
	    $openmp = "-fopenmp";
	}

	if ($compiler =~ /ifort/ || $compiler =~ /ifx/) {
	    $vendor = INTEL;
	    $bu       = "_";
	    $openmp = "-fopenmp";
	}

	if ($compiler =~ /pathf/) {
	    $vendor = PATHSCALE;
	    $bu       = "_";
	    $openmp = "-mp";
	}

	if ($compiler =~ /pgf/ || $compiler =~ /nvf/) {
	    $vendor = PGI;
	    $bu       = "_";
	    $openmp = "-mp";
	}

	if ($compiler =~ /ftn/) {
	    $vendor = PGI;
	    $bu       = "_";
	    $openmp = "-openmp";
	}

	if ($compiler =~ /frt/) {
	    $vendor = FUJITSU;
	    $bu       = "_";
	    $openmp = "-openmp";
	}

	if ($compiler =~ /sunf77|sunf90|sunf95/) {
	    $vendor = SUN;
	    $bu       = "_";
	    $openmp = "-xopenmp=parallel";
	}

	if ($compiler =~ /ppuf/) {
	    $vendor = IBM;
	    $openmp = "-openmp";
	}

	if ($compiler =~ /xlf/) {
	    $vendor = IBM;
	    $openmp = "-openmp";
	}

	if ($compiler =~ /open64/) {
	    $vendor = OPEN64;
	    $openmp = "-mp";
	}

	if ($compiler =~ /flang/) {
	    $vendor = FLANG;
	    $bu     = "_";
	    $openmp = "-fopenmp";
	}

	if ($compiler =~ /nagfor/) {
	    $vendor = NAG;
	    $bu     = "_";
	    $openmp = "-openmp";
	}

	if ($vendor eq "") {
	    $nofortran = 1;
	    $compiler = "gfortran";
	    $vendor = GFORTRAN;
	    $bu       = "_";
	    $openmp = "";
	}

    }
}

$data = `which $compiler_bin > /dev/null 2> /dev/null`;

if (!$?) {

    $binary = $ENV{"BINARY"};

    $openmp = "" if $ENV{USE_OPENMP} != 1;

    if ($binary == 32) {
	$link = `$compiler $openmp -m32 -v ftest2.f 2>&1 && rm -f a.out a.exe`;
	if ($?) {
	    $link = `$compiler $openmp -q32 -v ftest2.f 2>&1 && rm -f a.out a.exe`;
	}
        # for AIX
	if ($?) {
	    $link = `$compiler $openmp -maix32 -v ftest2.f 2>&1 && rm -f a.out a.exe`;
	}
       #For gfortran MIPS
	if ($?) {
    $mips_data = `$compiler_bin -E -dM - < /dev/null`;
    if ($mips_data =~ /_MIPS_ISA_MIPS64/) {
        $link = `$compiler $openmp -mabi=n32 -v ftest2.f 2>&1 && rm -f a.out a.exe`;
    } else {
        $link = `$compiler $openmp -mabi=32 -v ftest2.f 2>&1 && rm -f a.out a.exe`;
    }
	}
	$binary = "" if ($?);
    }

    if ($binary == 64) {
	$link = `$compiler $openmp -m64 -v ftest2.f 2>&1 && rm -f a.out a.exe`;
	if ($?) {
	    $link = `$compiler $openmp -q64 -v ftest2.f 2>&1 && rm -f a.out a.exe`;
	}
        # for AIX
	if ($?) {
	    $link = `$compiler $openmp -maix64 -v ftest2.f 2>&1 && rm -f a.out a.exe`;
	}
       #For gfortran MIPS
	if ($?) {
	    $link = `$compiler $openmp -mabi=64 -v ftest2.f 2>&1 && rm -f a.out a.exe`;
	}
       #For nagfor
	if ($?) {
	    $link = `$compiler $openmp -dryrun ftest2.f 2>&1 && rm -f a.out a.exe`;
        }
	$binary = "" if ($?);
    }
    if ($binary eq "") {
	$link = `$compiler $openmp -v ftest2.f 2>&1 && rm -f a.out a.exe`;
    }
}

if ( $vendor eq "NAG") {
	    $link = `$compiler $openmp -dryrun ftest2.f 2>&1 && rm -f a.out a.exe`;
    }
if ( $vendor eq "CRAY") {
	    $link = `$compiler $openmp -hnopattern ftest2.f 2>&1 && rm -f a.out a.exe`;
    }
$linker_L = "";
$linker_l = "";
$linker_a = "";

if ($link ne "") {

    $link =~ s/\-Y\sP\,/\-Y/g;
    
    $link =~ s/\-R\s*/\-rpath\%/g;

    $link =~ s/\-rpath\s+/\-rpath\%/g;

    $link =~ s/\-rpath-link\s+/\-rpath-link\%/g;

    @flags = split(/[\s\,\n]/, $link);
    # remove leading and trailing quotes from each flag.
    @flags = map {s/^['"]|['"]$//g; $_} @flags;

    foreach $flags (@flags) {
	if (
	    ($flags =~ /^\-L/)
	    && ($flags !~ /^-LIST:/)
	    && ($flags !~ /^-LANG:/)
	    ) {
	    $linker_L .= $flags . " ";
	}

	if ($flags =~ /^\-Y/) {
	    next if ($hostos eq 'SunOS');
	    $linker_L .= "-Wl,". $flags . " ";
        }

	if ($flags =~ /^\--exclude-libs/) {
	    $linker_L .= "-Wl,". $flags . " ";
	    $flags="";
	}


	if ($flags =~ /^\-rpath\%/) {
	    $flags =~ s/\%/\,/g;
	    $linker_L .= "-Wl,". $flags . " " ;
	}

	if ($flags =~ /^\-rpath-link\%/) {
	    $flags =~ s/\%/\,/g;
	    $linker_L .= "-Wl,". $flags . " " ;
	}
	if ($flags =~ /-lgomp/ && $ENV{"CC"} =~ /clang/) {
	    $flags = "-lomp";
	}

	if (
	    ($flags =~ /^\-l/)
	    && ($flags !~ /ibrary/)
	    && ($flags !~ /gfortranbegin/)
	    && ($flags !~ /flangmain/)
	    && ($flags !~ /frtbegin/)
	    && ($flags !~ /pathfstart/)
	    && ($flags !~ /crt[0-9]/)
	    && ($flags !~ /gcc/)
	    && ($flags !~ /user32/)
	    && ($flags !~ /kernel32/)
	    && ($flags !~ /advapi32/)
	    && ($flags !~ /shell32/)
	    && ($flags !~ /omp/ || ($vendor !~ /PGI/ && $vendor !~ /FUJITSU/ && $flags =~ /omp/))
	    && ($flags !~ /[0-9]+/ || ($vendor == FUJITSU && $flags =~ /^-lfj90/))
		&& ($flags !~ /^\-l$/)
	    ) {
	    $linker_l .= $flags . " ";
	}

	if ( $flags =~ /quickfit.o/ && $vendor == NAG) {
	    $linker_l .= $flags . " ";
	}
	if ( $flags =~ /safefit.o/ && $vendor == NAG) {
	    $linker_l .= $flags . " ";
	}
	if ( $flags =~ /thsafe.o/ && $vendor == NAG) {
	    $linker_l .= $flags . " ";
	}

	$linker_a .= $flags . " " if $flags =~ /\.a$/;
    }

}

if ($vendor eq "FLANG"){
    $linker_a .= "-lflang"
}

open(MAKEFILE, ">> $makefile") || die "Can't append $makefile";
open(CONFFILE, ">> $config"  ) || die "Can't append $config";

print MAKEFILE "F_COMPILER=$vendor\n";
print MAKEFILE "FC=$compiler\n";
print MAKEFILE "BU=$bu\n" if $bu ne "";
print MAKEFILE "NOFORTRAN=1\n" if $nofortran == 1;

print CONFFILE "#define BUNDERSCORE\t$bu\n" if $bu ne "";
print CONFFILE "#define NEEDBUNDERSCORE\t1\n" if $bu ne "";
print CONFFILE "#define NEED2UNDERSCORES\t1\n" if $need2bu ne "";

print MAKEFILE "NEED2UNDERSCORES=1\n" if $need2bu ne "";

if (($linker_l ne "") || ($linker_a ne "")) {
    print MAKEFILE "FEXTRALIB=$linker_L $linker_l $linker_a\n";
}

close(MAKEFILE);
close(CONFFILE);
