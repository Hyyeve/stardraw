This is a (hopefully, eventually!) portable graphics abstraction that I initially started as a better rendering system for [hyengine](https://github.com/Hyyeve/hyengine).
With a bigger scope in mind, it's now been moved into a separate project.

It's still extremely WIP, but current features include:

- A simple abstracted command / descriptor API with the ability to be data-driven
- Commands and descriptors covering buffer management, vertex specification, and basic graphics state
- A usable GL 4.5 implementation for most of the defined commands and descriptors.
