#version 460


in vec3 position;
in vec2 texCoords;

out vec2 vsTexCoords;


void main() {
	vsTexCoords = texCoords;
	gl_Position = vec4(position, 1.f);
}
