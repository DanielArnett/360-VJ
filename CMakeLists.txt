add_ffgl_plugin(Reprojection
VERSION
1.0.1
SOURCES
Reprojection/Reprojection.h
Reprojection/Reprojection.cpp
Reprojection/Rhader.h
)


set_target_properties(Reprojection PROPERTIES 
FOLDER "External"
)


add_ffgl_plugin(MirrorDome
VERSION
1.0.1
SOURCES
MirrorDome/MirrorDome.h
MirrorDome/MirrorDome.cpp
Reprojection/Shader.h
)


set_target_properties(MirrorDome PROPERTIES 
FOLDER "External"
)

