#!/usr/bin/env bash
set -euo pipefail
stem=$1
shift
exec "$@" > $stem.out 2> $stem.err
