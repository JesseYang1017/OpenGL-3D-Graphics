# OpenGL 3D Graphics

A collection of C++/OpenGL exercises exploring 3D transformations, viewing, and projection, developed as part of my Computer Graphics coursework at Trinity College Dublin.

## Model Transformations

Explores how transformation matrices affect imported 3D models.

**Features**

- Load and render 3D meshes using Assimp
- Keyboard-controlled translation along the X, Y, and Z axes
- Rotation around the X and Y axes
- Uniform scaling
- Toggle transformation order to observe the difference between applying translation and rotation in different sequences

**Controls**

| Key   | Action                      |
| ----- | --------------------------- |
| W / S | Move along Y                |
| A / D | Move along X                |
| Q / E | Move along Z                |
| I / K | Rotate around X             |
| J / L | Rotate around Y             |
| + / - | Scale up / down             |
| T     | Toggle transformation order |
| R     | Reset                       |

## Viewing and Projection

Explores how view and projection matrices affect the appearance of a 3D scene while keeping the objects stationary.

**Features**

- Render two instances of the same mesh using separate model matrices
- Switch between front and top-down views
- Compare perspective and orthographic projections
- Adjust field of view (30°, 45°, 75°)
- Modify camera distance and far clipping plane
- Construct view and orthographic projection matrices without GLM

**Controls**

| Key       | Action                     |
| --------- | -------------------------- |
| 1 / 2     | Front / Top view           |
| P / O     | Perspective / Orthographic |
| 3 / 4 / 5 | FOV: 30° / 45° / 75°       |
| Up / Down | Adjust camera distance     |
| [ / ]     | Adjust far clipping plane  |
| R         | Reset                      |
| Esc       | Exit                       |

## Technologies

- C++
- OpenGL / GLSL
- FreeGLUT / GLEW
- Assimp
- Custom matrix and vector math utilities

## Setup

The exercises were developed using Visual Studio on Windows.

**Dependencies:** OpenGL, FreeGLUT, GLEW, and Assimp.

The source code uses relative paths for shader and model files. The executable should be run with its working directory set to the corresponding lab folder.

A portable build configuration is not currently included.

## Notes

These exercises build upon a course-provided OpenGL framework and mathematical utilities. The focus is on practicing object transformations, view matrices, projection techniques, and their implementation in C++.

## Demo

Screenshots and demonstrations of model transformations and viewing controls.
