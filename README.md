# BassGuitar-Harness-OpenGL

A 3D OpenGL visualization of a bass guitar support harness worn by a 
mannequin, built with C and GLUT.

## Features
- **Bezier curves** to model the curved strap geometry realistically
- **Shadow projection matrix** for real-time ground shadows
- **Phong-style lighting** with a positioned light source
- **Texture mapping** on surfaces
- Interactive camera: click-and-drag to rotate, scroll/keys to zoom and pan
- Wireframe mannequin and bass guitar models built from primitive shapes

## Tech Stack
C, OpenGL, GLUT

## How to Run
Requires `freeglut`/`GL` development libraries.
```bash
gcc main.c -o harness -lglut -lGLU -lGL -lm
./harness
```

## Controls
- Left-click + drag: rotate camera
- (add your actual key/mouse bindings here)
