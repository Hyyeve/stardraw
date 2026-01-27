This is a framework and engine I've been developing along with other C++ projects to be a highly performant and flexible graphics/game fundamentals library.
It's very subject to large and breaking changes, but is in a fully usable state - I have multiple projects that depend on it.

It features:
- Standard game loop mechanism with configurable fixed physics update rate and dynamic & interpolated frame rate
- Flexible graphics abstraction on top of modern OpenGL, written with DSA and AZDO principles in mind
- Virtualized input abstraction with full support for keyboard/mouse/controllers and built-in handling for input buffering
- Threadpool based task system for utilizing multithreading
- Low-overhead pretty-printing logger system
- Custom asset loading and saving mechanisms
- Various math and data structure utilities.

There's also some things that are partially implemented / I want to add:
- Custom UI layout and rendering engine (partially implemented)
- Custom audio engine (not implemented)
- Custom 2D/3D physics (not implemented here; WIP in other projects)
