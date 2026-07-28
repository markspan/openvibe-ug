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

pixi install

# pixi tasks run in chain, so running ci-analyze will trigger configure and build too.
# This is done on linux for code analysis. On other unix systems, we run ci-build (which triggers configure beforehand).
if [[ "$SYSTEM" == "linux" ]]; then
    pixi run ci-analyze 2>&1 | tee build.log
else
    pixi run ci-build 2>&1 | tee build.log
fi
