#pragma once
#include <string>
#include <FFGLSDK.h>

class AddSubtract : public CFFGLPlugin
{
public:
	AddSubtract();
	~AddSubtract();

	//CFFGLPlugin
	FFResult InitGL( const FFGLViewportStruct* vp ) override;
	FFResult ProcessOpenGL( ProcessOpenGLStruct* pGL ) override;
	FFResult DeInitGL() override;

	FFResult SetFloatParameter( unsigned int dwIndex, float value ) override;

	float GetFloatParameter( unsigned int index ) override;
	char* GetParameterDisplay( unsigned int index ) override;
	void printDoubleToResolumeBuffer( char ( &buffer )[ 15 ], double value );


private:
	ffglex::FFGLShader shader;  //!< Utility to help us compile and link some shaders into a program.
	ffglex::FFGLScreenQuad quad;//!< Utility to help us render a full screen quad.
	int inputProjection, outputProjection;
	float pitch, roll, yaw, fovOut, fovIn;
};
