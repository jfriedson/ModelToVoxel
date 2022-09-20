#version 460


in vec3 position;
in vec2 texCoords;

out vec3 vsPosition;
out vec2 vsTexCoords;


void main() {
	vsPosition = position;
	vsTexCoords = texCoords;

	gl_Position = vec4(position, 1);
}
