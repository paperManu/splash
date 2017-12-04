#!/bin/bash

usage(){
  echo Usage: profile_flamegraph.sh [PROFILING_DATA_FILE] 
  echo Mandatory argument:
  echo -e "\tPROFILING_DATA_FILE: path of the file containing the flamegraph-formatted profiling data"
  exit 1
}

if [ $# -ne 1 ]
then
  usage
fi

if [ -w $(dirname $1) ]
then
  if [ -e "$1" ]
  then
    $(dirname $0)/../external/flamegraph/flamegraph.pl $1 > $1.svg && firefox $1.svg
  else
    echo "Could not find profiling data file $1."
    exit 1
  fi
else
  echo "Directory $(dirname $1) is not writable."
  exit 1
fi

