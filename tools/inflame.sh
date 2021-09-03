#!/bin/bash
# This script helps with using Flamegraph (https://github.com/brendangregg/FlameGraph)
# to profile Splash (or, in fact, any other software)

source_directory="$(cd $(dirname ${BASH_SOURCE[0]}) && pwd)"
flamegraph_directory="${source_directory}/../external/flamegraph"
stackcollapse="${flamegraph_directory}/stackcollapse-perf.pl"
flamegraph="${flamegraph_directory}/flamegraph.pl"
inflame_prefix="inflame"

command_line="${@:1}"

check_args()
{
    if (( $# == 0 )); then
        echo "Specify a command line to be ran by this script"
        exit 1
    fi

    if ! command -v $1 > /dev/null 2>&1; then
        echo "The specify executable does not exist"
        exit 1
    fi
}

check_perf()
{
    if ! command -v perf > /dev/null 2>&1; then
        echo "perf must be installed to run this script"
        exit 1
    fi
}

clean_output_files()
{
    rm -f /tmp/${inflame_prefix}.*
}

set_kernel()
{
    if [ $(cat /proc/sys/kernel/perf_event_paranoid) != -1 ]; then
        echo "Activating access to kernel samples..."
        sudo sh -c 'echo -1 >/proc/sys/kernel/perf_event_paranoid'
        sudo sh -c " echo 0 > /proc/sys/kernel/kptr_restrict"
    fi
}

run_perf()
{
    perf record -F 999 -g ${command_line}
    perf script > /tmp/${inflame_prefix}.perf
}

analyse_perf()
{
    echo "Analysing perf measurements..."
    ${stackcollapse} /tmp/${inflame_prefix}.perf > /tmp/${inflame_prefix}.folded
    ${flamegraph} /tmp/${inflame_prefix}.folded > /tmp/${inflame_prefix}.svg
    echo "Analyse finished. Open file located at /tmp/${inflame_prefix}.svg for results"
}

clean_perf_data()
{
    rm -f perf.data*
}

check_args ${command_line}
check_perf
clean_output_files
set_kernel
run_perf ${command_line}
analyse_perf
