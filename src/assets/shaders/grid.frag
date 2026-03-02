#version 450

layout(location = 0) in vec3 near_point;
layout(location = 1) in vec3 far_point;

layout(location = 0) out vec4 out_color;

layout(binding = 0) uniform UniformBufferObject {
  mat4 model;
  mat4 view;
  mat4 projection;
  float time;
  vec3 view_position;
  vec3 camera_forward;
} ubo;

vec4 grid(vec3 pos, float scale) {
  vec2 coord = pos.xz / scale;

  vec2 deriv = fwidth(coord);

  float line_width = 1.5;
  vec2 lines = abs(fract(coord - 0.5) - 0.5) / (deriv * line_width);
  float line = min(lines.x, lines.y);
  float alpha = 1.0 - min(line, 1.0);

  vec4 color = vec4(0.2, 0.2, 0.2, alpha);

  if (abs(pos.x) < deriv.y * 1.5) color = vec4(0.2, 0.2, 1.0, 1.0); // Z axis
  if (abs(pos.z) < deriv.x * 1.5) color = vec4(1.0, 0.2, 0.2, 1.0); // X axis

  return color;
}

void ground_plane(vec3 direction, float t){
  vec3 pos = near_point + t * direction;
  float fade = max(0.0, 1.0 - length(pos.xz) * 0.05);

  out_color  = grid(pos, 1.0);
  out_color += grid(pos, 5.0) * 0.5;
  out_color.a *= fade;
}

void vertical_line(vec3 direction, float t) {
  vec3 y_point = near_point + t * direction;
  float y_dist = length(y_point.xz);
  float y_alpha = max(0.0, 1.0 - y_dist / (fwidth(y_dist) * 1.5));
  float y_fade  = max(0.0, 1.0 - abs(y_point.y) * 0.05);

  y_alpha *= y_fade;
  out_color.rgb = mix(out_color.rgb, vec3(0.2, 1.0, 0.2), y_alpha);
  out_color.a   = max(out_color.a, y_alpha);
}

void main() {
  vec3 direction = far_point - near_point;

  out_color = vec4(0.0);

  float t = -near_point.y / direction.y;
  if (t >= 0.0) {
    ground_plane(direction, t);
  }

  float t_y = -dot(near_point.xz, direction.xz) / dot(direction.xz, direction.xz);

  if (t_y >= 0.0) {
    vertical_line(direction, t_y);
  }

  if (out_color.a < 0.001) discard;
}
