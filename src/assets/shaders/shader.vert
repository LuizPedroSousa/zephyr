#version 450

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 texture_coordinates;
layout (location = 0) out vec3 frag_color;

layout(binding = 0) uniform UniformBufferObject{
  mat4 model;
  mat4 view;
  mat4 projection;
} ubo;

const vec3 colors[6] = vec3[6](
  vec3(1.0, 0.0, 0.0),
  vec3(0.0, 1.0, 0.0),
  vec3(0.0, 0.0, 1.0),
  vec3(1.0, 1.0, 0.0),
  vec3(1.0, 0.0, 1.0),
  vec3(0.0, 1.0, 1.0)
);

const float scales[3] = float[3](
    0.8,
    0.5,
    0.3
);

void main(){
  gl_Position = ubo.projection * ubo.view * ubo.model * vec4(in_position, 1.0);

  float alternate = 0.25;

  uint scale_hash =
    floatBitsToUint(floor(gl_Position.x)/alternate) ^ floatBitsToUint(floor(gl_Position.y)/0.5);

  scale_hash ^= scale_hash >> 16;

  float scale = scales[scale_hash % 3];

  uint color_hash = floatBitsToUint(floor(gl_Position.x / scale) * scale)
         ^ floatBitsToUint(floor(gl_Position.y / scale) * scale);
  color_hash ^= color_hash >> 16;

  frag_color = colors[color_hash % 6];
}
