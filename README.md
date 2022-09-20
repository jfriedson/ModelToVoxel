# Model to Voxel
This project converts conventional 3D model consisting of triangles and textures into a voxel octree with textures baked into 32 bit color resolution leaves. It does not yet support alpha transparency conversion.

This relies on [VoxGL](https://github.com/jfriedson/voxgl) for voxel octree data structure and OpenGL interaction.

![Screenshot of voxelized house](screenshots/house.png?raw=true)
![Screenshot of voxelized gallery](screenshots/gallery.png?raw=true)


## Controls
WASD - move around

Mouse - look around


## Limitations
Supports octrees up to a depth of 8 before cracks appear in the voxelization.  This translates to a grid of 256 x 256 x 256.  The issue is that the rasterization pipeline invokes a fragment shader for pixel in the opengl render (1280 x 720). What is really needed is about one fragment shader invocation per voxel space; however, even this method will contain artifacts for very complex models with overlapping triangles.  A more foolproof solution would be to implement a voxelizer in a compute shader.


## Dependencies
[VoxGL](https://github.com/jfriedson/voxgl)
