# Model to Voxel
This project converts conventional 3D model consisting of triangles and textures into a voxel octree with textures baked into 32 bit color resolution leaves. It does not yet support alpha transparency conversion.

This relies on [VoxGL](https://github.com/jfriedson/voxgl) for voxel octree data structure and OpenGL interaction.


## Limitations
Supports octrees up to a depth of 9 for most 3D models before hitting the limit of the vector containing 31 bit resolution octree indices.  This translates to a grid of 512 x 512 x 512, which is still a decent resolution.


## Dependencies
[VoxGL](https://github.com/jfriedson/voxgl)
