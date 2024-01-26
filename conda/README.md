# |OpenViBE conda repository| |README|

## HowTo

This conda package in made in order to be able to build and execute OpenVibes on the following plateforms
* Win64
* Mac OS
* Linux 64

It is compose of the corresponding setup files
In order to be able to compile and execute, you have first to get *conda* on your machine 

## Get miniconda

Please refer to the [![Website](https://docs.conda.io/en/latest/miniconda.html#installing)](miniconda web site)

## Setup

Create your environment from config file :
`$ conda env create -n openvibe --file ENV.yml` 

if it failed, try with 
`mamba env update --file ENV.yaml `

Activate it
`$ conda activate openvibe`

## Install

Create a build folder
`$ cd BUILD_FOLDER`

Configure CMake
`$ cmake OPENVIBE_META_FOLDER`

Make with all your processors
`$ make -j` 

Install OpenVibe
`$ make install`

## Tests

`$ cd sdk; ./ctest-launcher.sh `
`$ cd extra; ctest`
