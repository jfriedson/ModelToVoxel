// Needs fixing. Something to do with depth and order of traversal

#version 460

precision highp float;
precision highp int;


layout(local_size_x = 16, local_size_y = 16) in;


struct InnerOctant {
    uint children[8];
};

struct Leaf {
	uint color;
};

layout(binding = 0, std430) restrict readonly buffer InnerOctants {
    InnerOctant innerOctants[];
};

layout(binding = 1, std430) restrict readonly buffer Leaves {
    Leaf leaves[];
};


uniform ivec2 resolution;
uniform vec3 cameraPos;
uniform mat3 cameraMat;

layout(rgba8) restrict writeonly uniform image2D texture;


const uint leafBit = 0x80000000, leafData = 0x7FFFFFFF;
const uint stackDepth = 23;

float minComponent(in vec3 v) {
    return min(min(v.x, v.y), v.z);
}

float maxComponent(in vec3 v) {
    return max(max(v.x, v.y), v.z);
}

vec3 castRay(vec3 origin, vec3 dir) {
    vec3 inverseDir = 1.f / dir;
    vec3 bias = inverseDir * origin;

    int stackPointer = 0;

    uint nodeStack[stackDepth];
    float nodeDepthStack[stackDepth];
    uint childIndexStack[stackDepth];

    uint node = 0;
    uint childIndex = 0;

    vec3 pos = vec3(0.f);
    float side = 0.5f;

    vec3 total = vec3(0.f);

    while (true) {
        //uint child = innerOctants[node].children[childIndex];
        vec3 boxMin = pos * inverseDir - bias;
        vec3 boxMax = (pos + side) * inverseDir - bias;

        float tMin = maxComponent(min(boxMin, boxMax));
        float tMax = minComponent(max(boxMin, boxMax));

        if (tMin < tMax && tMax > 0) {
            if (innerOctants[node].children[childIndex] >= leafBit) {
                vec4 color = unpackUnorm4x8(leaves[innerOctants[node].children[childIndex] & leafData].color);

                if(color.a > 0.f)
          		    return color.rgb;
               //total = color.rgb;
            }
            else {
                if (childIndex != 7) {
                    nodeStack[stackPointer] = node;
                    nodeDepthStack[stackPointer] = side;
                    childIndexStack[stackPointer] = childIndex;
                    stackPointer++;
                }

                side *= 0.5f;
                node = innerOctants[node].children[childIndex];
                childIndex = 0;
                continue;
            }
        }

        if (childIndex == 7) {
            stackPointer--;
            if (stackPointer < 0)
                break;

            node = nodeStack[stackPointer];
            side = nodeDepthStack[stackPointer];
            childIndex = childIndexStack[stackPointer];
        }

        pos -= mod(pos, side * 2.f);
        childIndex++;
        pos += mix(vec3(0.f), vec3(side), notEqual(uvec3(childIndex) & uvec3(1, 2, 4), uvec3(0)));
    }

    return total;
}

void main() {
    vec3 rayVec = normalize(vec3((gl_GlobalInvocationID.xy-(0.5f*resolution))/resolution.y, 1.f)) * cameraMat;

    vec3 color = castRay(cameraPos, rayVec);

    imageStore(texture, ivec2(gl_GlobalInvocationID.xy), vec4(color, 0.f));
}
