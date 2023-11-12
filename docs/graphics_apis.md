Graphics APIs support implementation
====================================

This document goes quickly through how the support for multiple graphics APIs is implemented into Splash. The goal is to facilitate maintainance, improvement to already supported APIs, and addition of new APIs.

At the time of writing this documentation, Splash supports OpenGL 4.5 and OpenGL ES 3.2.

A bit of history
----------------

Splash was started at a time where OpenGL 4.4 was all new, OpenGL 4.1 was still supported on Mac OS X (not yet macOS), and Vulkan was not a thing. The obvious choice was to go for OpenGL as the rendering API for Splash, which is what we did. More precisely, OpenGL 3.3 as none of the more modern features was considered useful for Splash at the time.

This led to the rendering pipeline of Splash to be built around the OpenGL API. The various classes which were involved in the rendering contained a lot of calls to OpenGL API, with logic code being intertwined with triangle crunching.

Nowadays the _graphics API of choice_ is not even a concept anymore, with all the possibilities. And on top of that embedded platforms are now capable of running Splash correctly, with OpenGL ES 3.2 and the multiple variations of Pi boards.

So came the time to diversify, and allow for adding other graphics APIs. Which involves quite a lot of refactoring, and the very welcome contribution of Tarek Yasser who did the most difficult: starting this process.

Overview
--------

This doc will (hopefully) explain the rationale behind the `gfx::FooGfxImpl`/`gfx::FooImpl` thing strewn around graphics related classes.

Splash graphics code is structured as follows:

1. An application specific class (e.g. `Splash::Texture_Image`), housing a pointer to an impl class (e.g. `Splash::gfx::Texture_ImageImpl`).
2. An impl class that defines an API that all current/future graphics APIs should adhere to (`Splash::gfx::Texture_ImageImpl`). 
3. An impl for each supported graphics API (`Splash::gfx::<API>::Texture_ImageImpl`).
4. A renderer base class, `Splash::gfx::Renderer`, housing functionality common between all renderers. It also houses creation methods for `Texture_ImageImpl` and other API specific code that each API's renderer is to override.
5. A renderer specialization for each API.

This extra level of indirection allows us to define an interface for all graphics API implementations to follow without having to change the application code itself. 

The old structure was fine if you want to support OpenGL/GLES only, but Splash is an old-ish program (~10 years old at the time of writing), so it will need to support another API at some time to work with future hardware. This raised the need to split application logic from graphics API specific logic. And the main idea in my mind to do that was to define an interface for different graphics API implementations to follow, then implement that interface separately for each API.

De-intertwining OpenGL code
---------------------------

Let's analyze an example. Say you have a class `Foo` that does some application logic mixed with OpenGL logic. You can follow these steps to pull out OpenGL's code into its own class, and define an API for other graphics APIs to use later on:

1. Go through the class's code and pull out the chunks responsible for OpenGL calls into functions, making sure to pass any non-OpenGL variables to the function (by value or reference depends on the control flow of the original function)
2. Create a class `Splash::gfx::FooGfxImpl`, move graphics functions to it, as well as any needed data members. Make sure everything is initialized exactly as it was in the original code. This class is moved to a file at `src/graphics/api/foo_gfx_impl.{h,cpp}`. Keeping everything in one place makes testing and modification faster. You can split the code to header and source files once finalizing your work.
3. Add `std::unique_ptr<Splash::gfx::FooGfxImpl` somewhere in `Foo`. I usually call this variable `_gfxImpl`.
4. Make sure you initialize `_gfxImpl` somewhere in the class, you can hardcode it in the constructor's initializer list as `_gfxImpl(std::make_unique<gfx::FooGfxImpl>(..))` for testing purposes, but it won't work when `FooGfxImpl` is turned into an abstract class later on. 
5. Make sure your implementation still "works" by running Splash and testing the codepaths you just moved around.
6. If step 5 leads you to bugs or a broken implementation, you can compare the code flow again with the original code. Sometimes branching and whatnot can be very tricky.
7. If your implementation works properly, you can begin moving the API specific code to `src/graphics/api/opengl/foo_gfx_impl.{h,cpp}` under a class named `Splash::gfx::opengl::FooGfxImpl`.
8. If this class must be created from a class which is not already API-specific, add a creator function in the renderer of your API. I usually name this `createFooGfxImpl(..)`.
9. In place of the hardcoded `std::make_unique` in step 4, call `createFooGfxImpl(..)` through the renderer. You might need to modify the calling code to pass the renderer down the call stack, but it should be fine.

At this point, you should have at least two new files:

1. `src/graphics/api/foo_gfx_impl.h`: Houses the public API that each graphics API needs to implement.
2. `src/graphics/api/<API>/foo_gfx_impl.h`: Houses the graphics API specific implementation of the aforementioned public API.

The code execution should go as follows:

1. Splash starts up, constructs some instance of `Foo`.
2. While initializing the instance of `Foo` (can be either at construction time or later on), you call `gfx::Renderer::createFooGfxImpl(..)` to initialize `Foo::_gfxImpl`.
3. All calls to the graphics API should be through `_gfxImpl`.

Adding a new graphics API
-------------------------

Right now, adding a new rendering API (beside already existing OpenGL and OpenGL ES) is a lot of work, as all OpenGL code has not be moved out of the logic code yet. So the very first step (and it is a big one) is to finalize the separation of logic and OpenGL / OpenGL ES code.

Once this is done, the process to add a new API should be something along these lines:

- Add the implementation of all classes already present in any of the subdirectory of `src/graphics/api/` into `src/graphics/api/<NEW API>`. So for example, if there are `api/opengl/foo_gfx_impl.h` and `api/opengl/bar_gfx_impl.h`, you'll need to create `api/<NEW API>/foo_gfx_impl.h` and `api/<NEW API>/bar_gfx_impl.h`
- Definition (`.cpp`) files should be added for compilation to `src/CMakeLists.txt`
- Add the new API to the base `Renderer` class in `src/graphics/api/renderer.{h,cpp}`
- Add a new choice for renderer selection in `src/splash-app.cpp`

The best move is probably to first add all declarations for the new API, then implement each class bit by bit, starting with the most structurally important one. Which would probably be `WindowGfxImpl` to be able to see something, then `Texture_ImageImpl`. Using a graphics debugger like [RenderDoc](https://renderdoc.org/) might be very helpful.

Additional notes
----------------

- If you can't figure out a shared API to use with different graphics APIs, don't stress too much. The main purpose of this is to split out API specific code so its easier to work with.
- Its fine if you have no idea what the class does at first if you haven't worked with it before. Analyzing and pulling out graphics code should allow you to slowly figure out what the class does and how it interacts with the graphics API.
- Don't try to be smart, make the code work in the dumbest/simplest way you can, commit, then do all the fancy tricks you wish. The most you can do before the code works is to reduce duplication by moving repeated code to a function or breaking down if-else if-else chains to some table lookup/switch case if possible. Dont forget to commit frequently :)
