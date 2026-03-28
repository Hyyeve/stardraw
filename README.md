This is a portable graphics abstraction that I initially started as a better rendering system for [hyengine](https://github.com/Hyyeve/hyengine).
With a bigger scope in mind, it's now been moved into a separate project.

Current features include:

- An abstracted command / descriptor API with the ability to be data-driven
- Commands and descriptors cover all modern functionality available in OpenGL
  - multipurpose buffers, textures, sampler states, framebuffers, shaders, vertex layouts/specifications
- A full implementation of all functionality in modern OpenGL 4.5, written with DSA and driver overhead minimization considerations.
- Validation and error reporting of almost all stardraw API. (with a fallback to driver level error reporting for OpenGL)
- A cross-api compatible shader solution using Slang & SPIRV-cross internally.

In future I would like to implement a Vulkan backend (some initial work on that has been done by a friend, but it's non-usable currently),
and potentially other APIs.
