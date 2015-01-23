For the shaders that you find useful with "ShaderLoader" and will use a lot, it is useful to have a dedicated FreeFrameGL plugin that does not not rely on a shader file. This is a for Visual Studio C++ project that can make any number of them.

Download everything and unzip into in any folder, open the VS2010 solution file with Visual Studio and change to "release", it should build OK as-is. More examples are in the source file.

To make your own shader plugin, all you do is copy/paste the shader code into the source file, change the plugin information and rename the resulting dll. There are some things to take note of, but hopefully the code and documentation are clear enough.
