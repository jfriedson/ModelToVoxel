// https://jcgt.org/published/0002/01/02/

#version 460

layout(triangles) in;
layout (triangle_strip, max_vertices = 3) out;


in vec3 vsPosition[];
in vec2 vsTexCoords[];

uniform float voxelResolution;

out vec3 gsVoxelPos;
out vec2 gsTexCoords;


vec3 Project(in vec3 v, in int axis) {
	return axis == 0 ? v.zyx : (axis == 1 ? v.xzy : v.xyz);
}

void main() {
	vec3 axis_weight = abs(cross(vsPosition[1] - vsPosition[0], vsPosition[2] - vsPosition[0]));
	int axis = 2;

	if(axis_weight.x >= axis_weight.y && axis_weight.x > axis_weight.z)
		axis = 0;
	else if(axis_weight.y >= axis_weight.z && axis_weight.y > axis_weight.x)
		axis = 1;

	for(uint i = 0; i < 3; i++) {
		gsVoxelPos = vsPosition[i] * voxelResolution;
		gsTexCoords = vsTexCoords[i];

		gl_Position = vec4(Project(vsPosition[i], axis), 1);

		EmitVertex();
	}

	EndPrimitive();
}
