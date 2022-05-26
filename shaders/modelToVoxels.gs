// https://jcgt.org/published/0002/01/02/

#version 460

layout(triangles) in;
layout (triangle_strip, max_vertices = 3) out;


in vec2 vsTexCoords[];

uniform ivec3 voxelResolution;

out vec3 gsVoxelPos;
out vec2 gsTexCoords;


vec2 Project(in vec3 v, in int axis) {
	return axis == 0 ? v.yz : (axis == 1 ? v.xz : v.xy);
}

void main() {
	vec3 axis_weight = abs(cross(gl_in[1].gl_Position.xyz - gl_in[0].gl_Position.xyz, gl_in[2].gl_Position.xyz - gl_in[0].gl_Position.xyz));
	int axis = 2;

	if(axis_weight.x >= axis_weight.y && axis_weight.x > axis_weight.z)
		axis = 0;
	else if(axis_weight.y >= axis_weight.z && axis_weight.y > axis_weight.x)
		axis = 1;

	vec3 pos0 = gl_in[0].gl_Position.xyz;
	vec3 pos1 = gl_in[1].gl_Position.xyz;
	vec3 pos2 = gl_in[2].gl_Position.xyz;


	gsTexCoords = vsTexCoords[0];
	//gsNormal = normalize(vsNormal[0]);
	gsVoxelPos = (pos0 + 1.0f) * 0.5f * voxelResolution;
	gl_Position = vec4(Project(pos0, axis), 1.0f, 1.0f);
	EmitVertex();

	gsTexCoords = vsTexCoords[1];
	//gsNormal = normalize(vsNormal[1]);
	gsVoxelPos = (pos1 + 1.0f) * 0.5f * voxelResolution;
	gl_Position = vec4(Project(pos1, axis), 1.0f, 1.0f);
	EmitVertex();

	gsTexCoords = vsTexCoords[2];
	//gsNormal = normalize(vsNormal[2]);
	gsVoxelPos = (pos2 + 1.0f) * 0.5f * voxelResolution;
	gl_Position = vec4(Project(pos2, axis), 1.0f, 1.0f);
	EmitVertex();

	EndPrimitive();
}
