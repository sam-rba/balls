#version 130

in  vec4 in_coords;
in vec3 in_color;
out vec3 new_color;

void
main(void) {
	new_color = in_color;

	gl_Position = in_coords;
}
