TODO: Move to splash's docs whenever I can get a connection.
WARNING: Naming conventions might change slightly before this code is merged.

This doc will (hopefully) explain the rationale behind the `gfx::FooGfxImpl`/`gfx::FooImpl` thing strewn around graphics related classes.

Before this, Splash's graphics code was split into:
1. A base class for shared OpenGL/GLES functionality, with pure virtual functions for API specific code, along with mixed in application code (`Texture_Image` as an example).
2. (Optional): A base impl class for shared code between OpenGL/GLES implementations (`Splash::gfx::GlBaseTexture_ImageImpl`).
3. OpenGL/GLES implementations of classes (`OpenGLTexture_Image`, `GLESTexture_Image`).
4. A renderer base class housing functionality between all renderers. Houses creation methods for `Texture_ImageImpl` that each API's renderer is to override.
5. A renderer specialization for OpenGL/GLES. Override the methods mentioned in step 3.

After this, Splash's graphics code became structured as follows:
1. An application specific class (`Splash::Texture_Image`), housing a pointer to an impl class (`Splash::gfx::Texture_ImageImpl`).
2. An impl class that defines an API that all current/future graphics APIs should adhere to (`Splash::gfx::Texture_ImageImpl`). 
3. (Optional): A base impl class for shared code between OpenGL/GLES implementations (`Splash::gfx::GlBaseTexture_ImageImpl`).
4. An impl for each supported graphics API (`Splash::gfx::<API>::Texture_ImageImpl`).
5. A renderer base class housing functionality between all renderers. Houses creation methods for `Texture_ImageImpl` that each API's renderer is to override.
6. A renderer specialization for OpenGL/GLES. Override the methods mentioned in step 3.

This extra level of indirection allows us to define an interface for all graphics API implementations to follow without having to change the application code itself. 

The old structure is fine if you want to support OpenGL/GLES only, but splash is an old-ish program (~10 years old at the time of writing), so it might need to support another API at some time to work with future hardware. This raised the need to split application logic from graphics API specific logic. And the main idea in my mind to do that was to define an interface for different graphics API implementations to follow, then implement that interface separately for each API.

Let's analyze an example. Say you have a class `Foo` that does some application logic mixed with OpenGL logic. You can follow these steps to pull out OpenGL's code into its own class, and define an API for other graphics APIs to use later on:
1. Go through the class's code and pull out the chunks responsible for OpenGL calls into functions, making sure to pass any non-OpenGL variables to the function (by value or reference depends on the control flow of the original function)
2. Create a class `Splash::gfx::FooGfxImpl`, move graphics functions to it, as well as any needed data members. Make sure everything is initialized exactly as it was in the original code. I usually house this class in a file at `src/graphics/api/foo_gfx_impl.h`. You can move method implementations to a source file later on once everything works. Keeping everything in one place makes testing and modification faster.
3. Add `std::unique_ptr<Splash::gfx::FooGfxImpl` somewhere in `Foo`. I usually call this variable `_gfxImpl`.
4. Make sure you initialize `_gfxImpl` somewhere in the class, you can hardcode it in the constructor's initializer list as `_gfxImpl(std::make_unique<gfx::FooGfxImpl>(..))` for testing purposes, but it won't work when `FooGfxImpl` is turned into an abstract class later on. 
5. Make sure your implementation still "works" by running Splash and testing the codepaths you just moved around.
6. If step 5 leads you to bugs or a broken implementation, you can compare the code flow again with the original code. Sometimes branching and whatnot can be very tricky.
7. If your implementation works properly, you can begin moving the API specific code to `src/graphics/api/<API>/foo_gfx_impl.h` under a class named `Splash::gfx::<API>::FooGfxImpl`.
8. Add a creator function in the renderer of your API. I usually name this `createFooGfxImpl(..)`.
9. In place of the hardcoded `std::make_unique` in step 4, call `createFooGfxImpl(..)` through the renderer. You might need to modify the calling code to pass the renderer down the call stack, but it should be fine.

At this point, you should have at least two new files:
1. `src/graphics/api/foo_gfx_impl.h`: Houses the public API that each graphics API needs to implement.
2. `src/graphics/api/<API>/foo_gfx_impl.h`: Houses the graphics API specific implementation of the aforementioned public API.

The code execution should go as follows:
1. Splash starts up, constructs some instance of `Foo`.
2. While initializing the instance of `Foo` (can be either at construction time or later on), you call `gfx::Renderer::createFooGfxImpl(..)` to initialize `Foo::_gfxImpl`.
3. All calls to a the graphics API should be through `_gfxImpl`.

Some notes:
1. The naming convention for files and classes is not yet tightend down. Some classes are called `FooImpl`, while others are called `FooGfxImpl`. This should hopefully be taken care of before the code is merged.
2. Creator functions in the renderer are also not very consistent in terms of naming. Some are called `createFoo`, others are called `createFooGfxImpl`.
3. Sometimes `Splash::gfx::FooGfxImpl` contains API specific code. This is either because I wasn't sure if it was possible for other graphics APIs (i.e. Vulkan) to follow the one I had in mind, or I was too lazy :P.
4. Some Impl classes contain code shared between OpenGL and GLES specializations. These are typically called `Splash::gfx::GlBaseFooGfxImpl` or `Splash::gfx::GlBaseFooImpl`. There should be some pure virtual functions left for OpenGL and GLES to override.
5. If you can't figure out a shared API to use with different graphics APIs, don't stress too much. The main purpose of this is to split out API specific code so its easier to work with.
6. Its fine if you have no idea what the class does at first if you haven't worked with it before. Analyzing an pulling out graphics code should allow you to slowly figure out what the class does and how it interacts with the graphics API.
7. Don't try to be smart, make the code work in the dumbest/simplest way you can, commit, then do all the fancy tricky you wish. The most you can do before the code works is to reduce duplication by moving repeated code to a function or breaking down if-else if-else chains to some table lookup/switch case if possible.
