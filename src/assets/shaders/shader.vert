#version 450

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 texture_coordinates;
layout (location = 0) out vec3 frag_color;

void main(){
  gl_Position = vec4(in_position, 1.0);
  frag_color = vec3(1.0, 1.0, 1.0);
}
