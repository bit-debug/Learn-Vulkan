#!/bin/bash


BRIGHT_RED="\x1b[38;5;9m"
CLEAR="\x1b[0m"

cd build
rm -r *

printf "${BRIGHT_RED}Compiling vertex shader......\n${CLEAR}"
glslc -fshader-stage=vertex ../shaders/main.vs -o vertex.spv
printf "${BRIGHT_RED}Vertex shader is compiled, 'vertex.spv' is generated\n${CLEAR}"

printf "${BRIGHT_RED}Compiling fragment shader......\n\x1b[0m"
glslc -fshader-stage=fragment ../shaders/main.fs -o fragment.spv
printf "${BRIGHT_RED}Fragment shader is compiled, 'fragment.spv' is generated\n${CLEAR}"

printf "${BRIGHT_RED}Running cmake......\n${CLEAR}"
cmake ../.

printf "${BRIGHT_RED}Running make......\n${CLEAR}"
make

cd ..