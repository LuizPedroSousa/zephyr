#version 450

layout (location = 0) out vec4 out_color;
layout (location = 0) in vec3 color;
layout (location = 1) in vec2 texture_coordinates;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec3 view_position;
layout (location = 4) in vec3 frag_world_pos;
layout (location = 5) in vec3 camera_forward;
layout (location = 6) in vec2 ndc_pos;

layout (binding = 1) uniform sampler2D tex_sampler;

void main(){
  vec4 tex = texture(tex_sampler, texture_coordinates);

  float dist = length(ndc_pos);

  float facing_intensity = pow(clamp(1.0 - dist, 0.0, 1.0), 0.3);

  vec3 to_fragment = normalize(frag_world_pos - view_position);

  float proj = dot(normalize(camera_forward), to_fragment);
  float facing = clamp(proj, 0.0, 1.0) * facing_intensity;

  float luma = dot(tex.rgb,  vec3(0.2126, 0.7152, 0.0722));
  vec3 white_direction = normalize(vec3(1.0));
  float scalar_proj_color = clamp(dot(color, tex.rgb - (color * 0.05)),0.0, 1.0);

  vec3 gray = vec3(luma) * scalar_proj_color;
  vec3 shaded = tex.rgb * scalar_proj_color;

  out_color = vec4(mix(gray,shaded,facing), tex.w);
}
