# 360-VJ
360-VJ is a collection of FFGL shaders for Resolume and other VJing applications. 360-VJ includes shaders for:

* Rotating 360 videos
* Converting 360 videos into Fisheye videos
* Rotating Fisheye videos
* Projecting Flat, Rectilinear (normal), videos into a Fisheye view. Imagine looking at a TV through a fisheye lens.

### Installation
Download the shader from our [Releases page](https://github.com/DanielArnett/flat-to-fisheye-ffgl/releases/), unizip it, and install it into your Resolume Plugin Directory. You can find this directory by looking in Resolume->Preferences->Video->FFGL Plugin Directories. Restart Resolume and look in the Effects tab for the new effects "360 VJ", "360 to Fisheye", "Flat to Fisheye", and "Fisheye Rotation". If they do not appear it is likely because you're using incompatible versions of FFGL. Resolume versions older than 6.0.0 use FFGL 1.5 compiled as 32-bit binaries. Resolume versions 6.0.0-6.1.2 use FFGL 1.5 compiled as 64-bit binaries. Resolume versions 6.1.2 or newer use FFGL 2.0 compiled as 64-bit. TODO: Right now I'm only compiling FFGL 2.0 64-bit. Need to add branches to compile for different Resolume versions.

### Windows
This is a Visual Studio C++ project that can make any number of them.
Download everything and unzip into in any folder, open the VS2017 solution file with
Visual Studio and change to "release", it should build OK as-is.

### Mac
There is an XCode project file in `build/mac` with targets for "ALL" and individual plugins.
It is currently configured to deploy to `10.15`, so if you are using an earlier version of OSX you may need to try changing that.
It's probably worth trying a debug build first to verify the basic configuration is working, but the resulting binaries appear liable to crash in Resolume.
In order to produce working binaries, it seems to be necessary to prepare an archive (`product/Archive` in the XCode menu). If that succeeds, you should see an XCode "Organizer" window containing your build. If you show that in Finder, then "Show Package Contents" and recurse throught the `Products` folder until you find the `.bundle` files that can be copied to `~/Documents/Resolume [Avenue|Arena]/Extra Effects`.

## Credits
Developed and Implemented by Daniel Arnett for the Slippery Rock University Planetarium.

Based on the incredible work of Paul Bourke from the University of Western Australia.
http://paulbourke.net/miscellaneous/sphere2sphere/

Thanks to Lynn Jarvis for helping this project initially get off the ground.
https://github.com/leadedge/ShaderMaker
