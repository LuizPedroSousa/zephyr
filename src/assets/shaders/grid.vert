#version 450

layout(location = 0) out vec3 near_point;
layout(location = 1) out vec3 far_point;

layout(binding = 0) uniform UniformBufferObject {
  mat4 model;
  mat4 view;
  mat4 projection;
  float time;
  vec3 view_position;
  vec3 camera_forward;
} ubo;

vec3 unproject(float x, float y, float z) {
  vec4 p = inverse(ubo.view) * inverse(ubo.projection) * vec4(x, y, z, 1.0);
  return p.xyz / p.w;
}

void main() {
  vec2 verts[3] = vec2[](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
  vec2 p = verts[gl_VertexIndex];

  near_point = unproject(p.x, p.y, 0.0);
  far_point  = unproject(p.x, p.y, 1.0);

  gl_Position = vec4(p, 1.0, 1.0);
}
