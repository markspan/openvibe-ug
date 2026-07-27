#!/usr/bin/env bash
###
#
#  @file build.sh
#  @copyright 2023-2024 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
#                       Univ. Bordeaux. All rights reserved.
#
#  @version 1.2.4
#  @author Mathieu Faverge
#  @author Florent Pruvost
#  @date 2024-05-29
#
###
set -e

CI_FLAGS=""

# specific compilation flags on linux for linters
if [[ "$SYSTEM" == "linux" ]]; then
    CI_FLAGS="-O0 -g -Wall -fPIC -fno-inline --coverage"
fi

pixi install

# Configure with CMake
pixi run ci-configure \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_FLAGS_DEBUG="$CI_FLAGS" \
  -DCMAKE_CXX_FLAGS_DEBUG="$CI_FLAGS" \
  -Wno-dev

# Compile openvibe

pixi run ci-build -j2 2>&1 | tee build.log

# Static analysis on the compile_commands.json produced above (linux only)
if [[ "$SYSTEM" == "linux" ]]; then
    pixi run ci-analyze
fi
