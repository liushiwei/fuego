#!/bin/bash
#-----------------------------------------------------------------------------
# Run regression tests
#-----------------------------------------------------------------------------

RESULT_DIR="html"
TESTSUITE_DEFAULT="basics.suite"
FUEGO_REL="../build/gmake/build/release/fuego -srand 1"
FUEGO_DBG="../build/gmake/build/debug/fuego -srand 1"
FUEGOTEST_REL="../build/gmake/build/release/fuego_test -srand 1"
FUEGOTEST_DBG="../build/gmake/build/debug/fuego_test -srand 1"

#-----------------------------------------------------------------------------
# Functions
#-----------------------------------------------------------------------------

usage() {
    cat >&2 <<EOF
Usage: $0 [options] [testsuite]

Options:
  -d Append date to name of ouput directory
  -h Print help and exit
  -l Long output
  -p Program, using full command or a program abbreviation
     (fuego,fuego_test). Default is fuego.
  -r Use release version of program (only for program abbreviations)
  -t Single test file to run (without extension .tst or .list)

The test files are expected to have the file endings .tst or .list.
Lists can contain other lists (file names prepended with @, see
gogui-regress documentation).

The test and program to run can be either specified with the -p and -t
options or by giving a test suite argument to allow runs with different
programs. A test suite is a file containing rows with a test file name and
a program name (only program abbreviations are allowed). If neither is
specified, the test suite $TESTSUITE_DEFAULT is used.

This script returns 0, if no test fails.
EOF
}

# Set variables PROGRAM_CMD and RESULT_EXT depending on PROGRAM
# and RELEASE
setprogram() {
    case "$PROGRAM" in
	fuego)
	    if (( $RELEASE != 0 )); then
		PROGRAM_CMD="$FUEGO_REL"
		RESULT_EXT="fuego"
	    else
		PROGRAM_CMD="$FUEGO_DBG"
		RESULT_EXT="fuego_dbg"
	    fi
	    ;;
	fuego_test)
	    if (( $RELEASE != 0 )); then
		PROGRAM_CMD="$FUEGOTEST_REL"
		RESULT_EXT="fuego_test"
	    else
		PROGRAM_CMD="$FUEGOTEST_DBG"
		RESULT_EXT="fuego_test_dbg"
	    fi
	    ;;
	*)
	    PROGRAM_CMD="$PROGRAM"
	    RESULT_EXT=""
	    ;;
    esac
}

# Set TESTARG from TESTFILE.
# Adds extension .tst or .list, if a file with that name exists. Prepends
# the argument with @ for lists as required by gogui-regress
settestarg() {
    FILE_EXT=${TESTFILE##*.}
    if [[ "$FILE_EXT" == "$TESTFILE" && -f "$TESTFILE.list" ]]; then
	FILE_EXT="list"
	TESTFILE="$TESTFILE.list"
    fi
    
    if [[ "$FILE_EXT" == "list" ]]; then
	TESTARG="@$TESTFILE"
    elif [[ "$FILE_EXT" == "tst" ]]; then
	TESTARG="$TESTFILE"
    else
	echo >&2 "Invalid test name '$TESTFILE'"
	exit 1
    fi
}

runtest() {
    PROGRAM="$1"
    TESTFILE="$2"

    setprogram
    settestarg

    TEST_SUBDIR=$TESTFILE
    # If the test case name is the name of a single test file, use only the
    # file name for the name of the subdirectory in RESULT_DIR, not the full
    # path (allows to specify test files in parent or sibling directories
    # and the output will still be in RESULT_DIR)
    if [[ -f "$TESTFILE" ]]; then
	TEST_SUBDIR=$(basename $TESTFILE)
    fi
    TEST_DIR="$RESULT_DIR/$TEST_SUBDIR"
    if [[ "$RESULT_EXT" != "" ]]; then
	TEST_DIR="$TEST_DIR-$RESULT_EXT"
    fi
    if (( $APPEND_DATE != 0 )); then
        DATE=$(date +'%Y%m%d')
	TEST_DIR="$TEST_DIR-$DATE"
    fi
    mkdir -p "$TEST_DIR"
    
    if (( $LONG_OUTPUT != 0 )); then
	OPTIONS="-long"
    fi
    cat >&2 <<EOF
Program: '$PROGRAM_CMD'
Test: $TESTARG
Output:  $TEST_DIR/index.html
EOF
    gogui-regress $OPTIONS -output "$TEST_DIR" "$PROGRAM_CMD" "$TESTARG"
    if [[ "$?" != 0 ]]; then
	RESULT=1
    else
	echo "OK"
    fi
    echo
}

#-----------------------------------------------------------------------------
# Parse options and arguments
#-----------------------------------------------------------------------------

RELEASE=0
LONG_OUTPUT=0
APPEND_DATE=0
PROGRAM=""
TESTFILE=""
while getopts "dhlp:rt:" OPT; do
case "$OPT" in
    d)   APPEND_DATE=1;;
    h)   usage; exit 0;;
    l)   LONG_OUTPUT=1;;
    p)   PROGRAM="$OPTARG";;
    r)   RELEASE=1;;
    t)   TESTFILE="$OPTARG";;
    [?]) usage; exit 1;;
esac
done

TESTSUITE=""
shift $(($OPTIND - 1))
if (( $# > 1 )); then
    usage
    exit 1
fi
if (( $# == 1 )); then
    if [[ "$TESTFILE" != "" || "$PROGRAM" != "" ]]; then
	echo >&2 "Cannot use both a test suite and a test file/program"
	exit 1
    fi
    TESTSUITE="$1"
else
    if [[ "$TESTFILE" == "" && "$PROGRAM" == "" ]]; then
	TESTSUITE="$TESTSUITE_DEFAULT"
    elif [[ "$TESTFILE" == "" || "$PROGRAM" == "" ]]; then
	echo >&2 "Need either test suite or test file/program"
	exit 1
    fi
fi

#-----------------------------------------------------------------------------
# Run tests
#-----------------------------------------------------------------------------

RESULT=0
if [[ "$TESTSUITE" != "" ]]; then
    if [[ ! -f "$TESTSUITE" ]]; then
	echo >&2 "File '$TESTSUITE' not found"
	exit 1
    fi
    LINENO=0
    while read LINE; do
	LINENO=$(( LINENO + 1 ))
	shopt -s extglob
	if [[ "${LINE%%*( )#*}" == "" || "${LINE%%*( )}" == "" ]]; then
	    continue # Skip comment and empty lines
	fi
	runtest $LINE
    done < "$TESTSUITE"
else
    runtest "$PROGRAM" "$TESTFILE"
fi
if [[ $RESULT == 0 ]]; then
    echo "*** ALL OK ***"
else
    echo "*** TEST FAILS OCCURRED ***"
fi

exit $RESULT

#-----------------------------------------------------------------------------