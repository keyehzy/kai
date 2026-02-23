#!/usr/bin/env bash

set -e
set -x
set -o pipefail

make all test
