#pragma once

#include <FFGLShader.h>
#include "FFGLPluginSDK.h"
#include <string>

class FlatToFisheye : public CFreeFrameGLPlugin
{
public:
	FlatToFisheye();
	~FlatToFisheye();

	///////////////////////////////////////////////////
	// FreeFrame plugin methods
	///////////////////////////////////////////////////

	FFResult SetFloatParameter(unsigned int dwIndex, float value) override;
	float GetFloatParameter(unsigned int index) override;
	FFResult ProcessOpenGL(ProcessOpenGLStruct* pGL) override;
	FFResult InitGL(const FFGLViewportStruct *vp) override;
	FFResult DeInitGL() override;

	///////////////////////////////////////////////////
	// Factory method
	///////////////////////////////////////////////////

	static FFResult __stdcall CreateInstance(CFreeFrameGLPlugin **ppOutInstance)
	{
		*ppOutInstance = new FlatToFisheye();
		if (*ppOutInstance != NULL)
			return FF_SUCCESS;
		return FF_FAIL;
	}


protected:
	// Parameters
	int m_initResources;
	float fieldOfView;
	FFGLShader m_shader;
	GLint m_inputTextureLocation;
	GLint fieldOfViewLocation;
	GLint maxUVLocation;

};

//#pragma once
//#include <string>
//#include <ffgl/FFGLPluginSDK.h>
//#include <ffglex/FFGLShader.h>
//#include <ffglex/FFGLScreenQuad.h>
//
//class EquiToFisheye : public CFreeFrameGLPlugin
//{
//public:
//	EquiToFisheye();
//	~EquiToFisheye();
//
//	//CFreeFrameGLPlugin
//	FFResult InitGL( const FFGLViewportStruct* vp ) override;
//	FFResult ProcessOpenGL( ProcessOpenGLStruct* pGL ) override;
//	FFResult DeInitGL() override;
//
//	FFResult SetFloatParameter( unsigned int dwIndex, float value ) override;
//
//	float GetFloatParameter( unsigned int index ) override;
//
//private:
//	float pitch;
//	float yaw;
//	float roll;
//
//	ffglex::FFGLShader shader;   //!< Utility to help us compile and link some shaders into a program.
//	ffglex::FFGLScreenQuad quad; //!< Utility to help us render a full screen quad.
//	GLint maxUVLocation;
//	GLint fieldOfViewLocation;
//};
