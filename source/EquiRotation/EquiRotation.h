#pragma once

#include <FFGLShader.h>
#include "FFGLPluginSDK.h"
#include <string>

class EquiRotation : public CFreeFrameGLPlugin
{
public:
	EquiRotation();
	~EquiRotation();

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
		*ppOutInstance = new EquiRotation();
		if (*ppOutInstance != NULL)
			return FF_SUCCESS;
		return FF_FAIL;
	}

protected:
	float pitch;
	float yaw;
	float roll;


	int m_initResources;
	FFGLShader m_shader;
	GLint m_inputTextureLocation;
	GLint m_BrightnessLocation;
	GLint maxUVLocation;
	GLint fieldOfViewLocation;

};
