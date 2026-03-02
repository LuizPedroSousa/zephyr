#version 450

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_texture_coordinates;
layout (location = 0) out vec3 frag_color;
layout (location = 1) out vec2 texture_coordinates;
layout (location = 2) out vec3 frag_normal;
layout (location = 3) out vec3 view_position;
layout (location = 4) out vec3 frag_world_pos;
layout (location = 5) out vec3 camera_forward;
layout (location = 6) out vec2 ndc_pos;

layout(binding = 0) uniform UniformBufferObject{
  mat4 model;
  mat4 view;
  mat4 projection;
  float time;
  vec3 view_position;
  vec3 camera_forward;
} ubo;

void main(){
  gl_Position = ubo.projection * ubo.view * ubo.model * vec4(in_position, 1.0);


  frag_normal = transpose(inverse(mat3(ubo.model))) * in_normal;

  frag_world_pos = vec3(ubo.model * vec4(in_position, 1.0));

  float dist = length(in_position);
  float attenuation = 1.0 / (1.0 + dist * dist);

  float wave = 0.5 + attenuation * 0.5 * sin(ubo.time * 1.5 + dot(vec3(2.094, 1.094, 4.189), in_position));

  frag_color = vec3(wave);

  texture_coordinates = in_texture_coordinates;

  view_position = ubo.view_position;

  camera_forward = ubo.camera_forward;
  ndc_pos = gl_Position.xy / gl_Position.w;
}
