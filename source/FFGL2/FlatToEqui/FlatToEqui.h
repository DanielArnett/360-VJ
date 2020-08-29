#pragma once
#include <string>
#include <FFGLSDK.h>

class FlatToEqui : public CFFGLPlugin
{
public:
	FlatToEqui();
	~FlatToEqui();

	//CFreeFrameGLPlugin
	FFResult InitGL( const FFGLViewportStruct* vp ) override;
	FFResult ProcessOpenGL( ProcessOpenGLStruct* pGL ) override;
	FFResult DeInitGL() override;

	FFResult SetFloatParameter( unsigned int dwIndex, float value ) override;

	float GetFloatParameter( unsigned int index ) override;
	char* GetParameterDisplay(unsigned int index) override;


private:
	float pitch;
	float yaw;
	float roll;
	float fieldOfView = 0.05;
	float aspectRatio = (16.0f/9.0f)/2.0;

	ffglex::FFGLShader shader;   //!< Utility to help us compile and link some shaders into a program.
	ffglex::FFGLScreenQuad quad; //!< Utility to help us render a full screen quad.
	GLint maxUVLocation;
	GLint inputRotationLocation;
	GLint fieldOfViewLocation;
	GLint aspectRatioLocation;
};
