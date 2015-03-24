DIR_SRC = ../../
DIR_FFGL = ../../FFGL
DIR_BUNDLE = ../../Binaries/osx

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

CCPP = clang++
CPPFLAGS = -Wall -Wno-unknown-pragmas -pedantic \
	-I$(DIR_SRC) -I$(DIR_FFGL) -DTARGET_OS_MAC -std=c++11 -stdlib=libc++ -arch x86_64 -arch i386

CSHLIB = $(CCPP) -o $@ -dynamiclib -framework GLUT -framework OpenGL -arch x86_64 -arch i386 $(OBJS)

ifeq ($(DEBUG), 1)
	CPPFLAGS += -ggdb2 -O0 -D_DEBUG=1
else
	CPPFLAGS += -g0 -Os
endif

%.o: %.cpp
	$(CCPP) -c $(CPPFLAGS) -o $@ $<


.PHONY: clean

all: clean ShaderMaker.dylib clean_package package

ShaderMaker.dylib: $(SHADERMAKER_OBJS) $(COMMON_OBJS)
	$(CSHLIB)

clean:
	rm -rf $(OBJS) *.dylib

clean_package:
	rm -rf "$(DIR_BUNDLE)/ShaderMaker.bundle"

package:
	mkdir -p "$(DIR_BUNDLE)/ShaderMaker.bundle"/Contents/MacOS
	cp Info.plist "$(DIR_BUNDLE)/ShaderMaker.bundle/Contents"
	mv ./ShaderMaker.dylib "$(DIR_BUNDLE)/ShaderMaker.bundle/Contents/MacOS/ShaderMaker"
