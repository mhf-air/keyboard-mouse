#!/usr/bin/env bash

# set -o xtrace
set -o errexit
set -o pipefail
readonly ARGS="$@"
readonly ARGS_COUNT="$#"
println() {
	printf "$@\n"
}
# ================================================================================
#

main() {
	./.a
}

main
