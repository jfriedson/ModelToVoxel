# Model to Voxel
This project converts conventional 3D model consisting of triangles and a texture into a voxel representation with color, transparancy, and per-voxel normals.

This relies on [VoxGL](https://github.com/jfriedson/voxgl) for voxel octree data structure and OpenGL interaction.

![Screenshot of voxelized gallery](screenshots/gallery.png?raw=true)
![Screenshot of voxelized house](screenshots/house.png?raw=true)


## How it works
The new gpu_gen.cpp and convertOctree.comp allow octree generation on the gpu directly.  This dramatically increases the speed of creating the octree.

Previously, the conversion shader fills a buffer with voxel data containing the voxel position, color from the texture, and the smoothed normal of the triangle from it's vertex normals.  This data is then transfered to the CPU and the octree is filled.  The octree is then sent to the GPU for drawing.

I am still considering the old conversion shader to combine morton code with an on-disk hash table. This would enable nearly instant access time from disk.


## Features
- About 5-12 milliseconds to render an octree of depth 12 with shadows, ~3-7 ms without shadows.
- 8bpc color and transparancy per leaf
- Per-voxel normals derived from interpolated vertex normals
- Diffuse lighting uses the per-voxel normals to calculate shading.
- Realtime shadows are calculated by bouncing the primary ray towards the light source
- Small shadows show up as a few black pixels. This looks ugly. To try and remediate this, I copied a FXAA implementation but the small black pixels still stand out. FXAA does help with shimmering while the camera is moving.

## Controls
WASD - move around

Mouse - look around

L - set light direction to camera's perspective


## Limitations
- The maximum tree depth obtainable is around 12 or 13 levels. I need to look into the memory usage more, but this could be due to the conversion shader filling the storage buffer, or the octree surpasing the size of the storage buffer. A tree depth of 12 translates to a voxel space resolution of (2^12)^3, 4096^3, which is almost 69 billion voxels in total.

- The model being converted may contain multiple meshes, however, the conversion shader only accepts a single diffuse texture, so textures must be baked together.


## Future Improvements
- Compress voxel data with DAG
- Stream data from HDD to CPU, and CPU to GPU
- Improve rendering performance by storing parent index
- There are 8 unused bits in the conversion buffer that can be used for something. Maybe roughness or metallic.
- Make gpu octree gen more reliable

## Dependencies
[VoxGL](https://github.com/jfriedson/voxgl)
