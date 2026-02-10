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

SCAN_BUILD=""
CI_FLAGS=""

# specific compilation flags on linux for linters
if [[ "$SYSTEM" == "linux" ]]; then
    CI_FLAGS="-O0 -g -Wall -fPIC -fno-inline --coverage"
    SCAN_BUILD="scan-build --status-bugs -plist -analyze-headers -o analyzer_reports "
fi

if [[ "$SYSTEM" == "linux" ]]; then
    conda env update --name openvibe --file conda/env_linux.yaml --prune
elif [[ "$SYSTEM" == "macosx" ]]; then
    conda env update --name openvibe --file conda/env_osx.yaml --prune
else
    echo "The SYSTEM environment variable is $SYSTEM. It is not one of: linux, macosx, windows -> exit 1."
    exit 1
fi

source "$(conda info --base)/etc/profile.d/conda.sh"
conda activate openvibe
rm -rf $BUILD_FOLDER
mkdir $BUILD_FOLDER
cd $BUILD_FOLDER

# Configure with CMake
cmake .. \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_FLAGS_DEBUG="$CI_FLAGS" \
  -DCMAKE_CXX_FLAGS_DEBUG="$CI_FLAGS" \
  -DCMAKE_INSTALL_PREFIX=$BUILD_FOLDER/openvibe_install \
  -Wno-dev

# Compile openvibe
# $TEST_PIPELINE_ONLY defined in common.yml

TARGET=""

if [[ "$TEST_PIPELINE_ONLY" == "true" ]]; then
    TARGET=" openvibe"
fi

${SCAN_BUILD}make -j2 ${TARGET} 2>&1 | tee build.log
# sed '/^scan-build/d' build.log > test; mv test build.log 

# Install
if [[ "$TEST_PIPELINE_ONLY" == "false" ]]; then
    make install
fi
