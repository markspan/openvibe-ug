#!/usr/bin/env bash
###
# OpenViBE – SonarQube analysis script
#
# Generates coverage + sonar-project.properties + runs sonar-scanner
###

set -e

source "$(conda info --base)/etc/profile.d/conda.sh"
conda activate openvibe

PROJECT="$CI_PROJECT_NAME"

#######################################
# 1. Generate coverage
#######################################

# NOT used by SonarQube Server 24.12 CFamily plugin
# gcovr \
#   --xml-pretty \
#   --exclude-unreachable-branches \
#   --print-summary \
#   -o coverage.xml \
#   --root "$CI_PROJECT_DIR" \
#   --delete \
#   --exclude $BUILD_FOLDER \
#   --gcov-executable=gcov \
#   --gcov-ignore-parse-errors=negative_hits.warn_once_per_file

# GCC_INC=$(x86_64-conda-linux-gnu-c++ -v -E - < /dev/null 2>&1 \
#   | sed -n '/#include <...> search starts here:/,/End of search list/p' \
#   | grep -E '^\s*/' \
#   | tr -d ' ' \
#   | tr '\n' ',')

#######################################
# 2. Generate sonar-project.properties
#######################################
cat > sonar-project.properties << EOF
sonar.host.url=https://sonarqube.inria.fr/sonarqube
sonar.verbose=false
sonar.qualitygate.wait=true

# Project metadata
sonar.projectKey=openvibe_meta_AZJXpMOysbMNg1jXgnB3
sonar.projectName=OpenViBE Meta
sonar.projectVersion=0.1
sonar.projectDescription=OpenViBE – software for real-time neurosciences

# Pull request integration
# sonar.pullrequest.key=$CI_MERGE_REQUEST_IID
# sonar.pullrequest.branch=$PULL_REQUEST_BRANCH_NAME

# Links
sonar.links.homepage=$CI_PROJECT_URL
sonar.links.scm=$CI_REPOSITORY_URL
sonar.links.ci=$CI_PROJECT_URL/pipelines
sonar.links.issue=$CI_PROJECT_URL/issues

# SCM
sonar.scm.provider=git

###############################################
# SOURCE FILES
###############################################
sonar.sources=$BUILD_FOLDER/../designer,$BUILD_FOLDER/../sdk,$BUILD_FOLDER/../extras,$BUILD_FOLDER/../CMake
sonar.sourceEncoding=UTF-8
sonar.cxx.file.suffixes=.cxx,.cpp,.cc,.c,.hxx,.hpp,.h,.hh

###############################################
# EXCLUSIONS – cleaned & deduplicated
###############################################
sonar.exclusions=\
**/build*/**,\
**/conda/**,\
**/CMakeFiles/**,\
**/cmake_modules/**,\
**/doc/**,\
**/designer-legacy-2023/**,\
**/Scalp.obj.hpp,\
**/loreta_anatomic_voxels.hpp,\
**/*.png,\
**/*.svg,\
**/*.txt,\
**/*.json,\
**/*.md,\
**/*.html,\
**/*.css,\
**/*.js,\
**/*.ts,\
**/*.xml,\
**/*.ov,\
**/qt6/**,\
**/deprecated/**

###############################################
# COMPILATION DATABASE
###############################################
sonar.cfamily.compile-commands=$BUILD_FOLDER/compile_commands.json

###############################################
# TEST REPORTS
###############################################
# NOT used by SonarQube Server 24.12 CFamily plugin
# sonar.cxx.xunit.reportPaths=$BUILD_FOLDER/test.xml
# sonar.cxx.cobertura.reportPaths=$BUILD_FOLDER/coverage.xml
# sonar.cxx.cppcheck.reportPaths=$BUILD_FOLDER/../cppcheck.xml
# sonar.cxx.clangsa.reportPaths=$BUILD_FOLDER/analyzer_reports/*/*.plist
# sonar.cxx.valgrind.reportPaths=$BUILD_FOLDER/../valgrind.xml

###############################################
# GCC LOG PARSING
###############################################
# NOT used by SonarQube Server 24.12 CFamily plugin
# sonar.cxx.gcc.reportPaths=$BUILD_FOLDER/build.log
# sonar.cxx.gcc.encoding=UTF-8

###############################################
# INCLUDE DIRECTORIES (correct paths)
###############################################
# NOT used by SonarQube Server 24.12 CFamily plugin
# sonar.cxx.includeDirectories=\
#   ${GCC_INC} \
#   $BUILD_FOLDER/../sdk/openvibe/include/,\
#   $BUILD_FOLDER/../sdk/toolkit/include/,\
#   $BUILD_FOLDER/../sdk/common/include/,\
#   $BUILD_FOLDER/../sdk/modules/xml/include/,\
#   $BUILD_FOLDER/../extras/applications/examples/vrpn-to-openvibe/include/,\
#   $BUILD_FOLDER/../designer/visualization-toolkit/include

sonar.cfamily.includeDirectories=\
/builds/miniconda3/envs/openvibe/include,\
/builds/miniconda3/envs/openvibe/include/qt,\
$BUILD_FOLDER/../sdk/common/include, \
$BUILD_FOLDER/../sdk/openvibe/include,\
/builds/miniconda3/envs/openvibe/include/qt6/QtQml/6.8.3/QtQml, \
/builds/miniconda3/envs/openvibe/include/qt6/QtCore/6.8.3/QtCore
EOF

echo "====== sonar-project.properties ========"
cat sonar-project.properties
echo "========================================"


#######################################
# 3. Run SonarQube Scanner
#######################################

/builds/sonar-scanner-cli/bin/sonar-scanner -Dsonar.token="$SONAR_TOKEN" -Dsonar.projectBaseDir=$BUILD_FOLDER/.. -Dproject.settings=$BUILD_FOLDER/../sonar-project.properties  2>&1 | tee sonar.log