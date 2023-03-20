# Model to Voxel
This project converts conventional 3D models consisting of triangles and a texture into a voxel representation with diffuse color, transparancy, and per-voxel normals.

This relies on [VoxGL](https://github.com/jfriedson/voxgl) for voxel octree data structure and OpenGL interaction.

![Screenshot of voxelized gallery](screenshots/gallery.png?raw=true)
![Screenshot of voxelized house](screenshots/house.png?raw=true)


## How it works
The new convertOctree.comp allow octree generation on the gpu directly, while extracting the same visual details as the previous method.  This dramatically increases the speed of filling the octree, on the order of ~4 times faster, although my implementation may be overly cautious of race conditions.

Previously, the conversion shader fills a buffer with voxel data containing the voxel position, color from the texture, and the smoothed normal of the triangle from it's vertex normals.  This data is then transfered to the CPU and the octree is filled.  The octree is then sent to the GPU for drawing.

My next goal for this project is to use morton code as a key for an on-disk hash table. This would enable constant access time from a large storage device in a concurrent manner. This enables the generation of voxel spaces much larger than of those that can fit inside of system and GPU RAM. To visualize this data, unloaded visible voxels will be requested by the GPU and streamed from disk.


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
- The maximum tree depth attainable is around 11 or 12 levels depending on the space the model occupies. A tree depth of 12 translates to a voxel space resolution of (2^12)^3, 4096^3, which is almost 69 billion voxels in total.

- The model being converted may contain multiple meshes, however, the conversion shader only accepts a single diffuse texture, so textures must be baked together.


## Future Improvements
- Compress voxel data with DAG
- Stream data from HDD to CPU, and CPU to GPU
- Improve rendering performance by storing parent index
- There are 8 unused bits in the conversion buffer that can be utilized for something. Maybe roughness or metallic.

## Dependencies
[VoxGL](https://github.com/jfriedson/voxgl)
