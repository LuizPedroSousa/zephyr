#!/bin/bash

SHADERS_DIR=src/assets/shaders

OUTPUT_DIR=build/assets/shaders

if [[ ! -d $OUTPUT_DIR ]]; then
  mkdir -p $OUTPUT_DIR
fi

glslc $SHADERS_DIR/shader.vert -o $OUTPUT_DIR/shader.vert.spv
glslc $SHADERS_DIR/shader.frag -o $OUTPUT_DIR/shader.frag.spv
