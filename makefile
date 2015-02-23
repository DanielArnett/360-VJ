DIR_SRC = ./
DIR_FFGL = ./FFGL-1.6

DEBUG = 0

SHADERMAKER_SRCS = $(DIR_SRC)/ShaderMaker.cpp

SHADERMAKER_OBJS = $(notdir $(SHADERMAKER_SRCS:%cpp=%o))

COMMON_SRCS = $(DIR_FFGL)/FFGLPluginInfo.cpp $(DIR_FFGL)/FFGLPluginInfoData.cpp \
	$(DIR_FFGL)/FFGL.cpp $(DIR_FFGL)/FFGLShader.cpp \
	$(DIR_FFGL)/FFGLExtensions.cpp \
	$(DIR_FFGL)/FFGLPluginManager.cpp $(DIR_FFGL)/FFGLPluginSDK.cpp

COMMON_OBJS = $(notdir $(COMMON_SRCS:%cpp=%o))

OBJS = $(COMMON_OBJS) $(SHADERMAKER_OBJS)

vpath %.cpp $(DIR_SRC):$(DIR_FFGL)

#CCPP = @g++
CCPP = clang++
CPPFLAGS = -Wall -Wno-unknown-pragmas -pedantic \
	-I$(DIR_SRC) -I$(DIR_FFGL) -DTARGET_OS_MAC

CSHLIB = $(CCPP) -o $@ -dynamiclib -framework GLUT -framework OpenGL

#	-lc -lX11 -lGL -lglut

ifeq ($(DEBUG), 1)
	CPPFLAGS += -ggdb2 -O0 -D_DEBUG=1
else
	CPPFLAGS += -g0 -O3
endif

all: ShaderMaker.dylib

%.o: %.cpp
	$(CCPP) -c $(CPPFLAGS) -o $@ $<

ShaderMaker.dylib: $(SHADERMAKER_OBJS) $(COMMON_OBJS)
	$(CSHLIB)

.PHONY: clean

clean:
	-rm -rf $(OBJS) *.dylib

