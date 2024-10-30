#version 130

in  vec2 in_coords;
in vec3 in_color;
out vec3 new_color;

void
main(void) {
	new_color = in_color;

	gl_Position = vec4(in_coords, 1.0, 1.0);
}
