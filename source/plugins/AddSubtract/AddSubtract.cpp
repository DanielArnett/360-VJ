#include "AddSubtract.h"
#include <fstream>// std::ifstream

using namespace ffglex;
const std::string _fragmentShaderCode = { 
#include "Reprojection.hlsl"
};
enum ParamType : FFUInt32
{
	PT_INPUT_PROJECTION,
	PT_OUTPUT_PROJECTION,
	PT_RED,
	PT_GREEN,
	PT_BLUE,
	PT_FOV
};

static CFFGLPluginInfo PluginInfo(
	PluginFactory< AddSubtract >,// Create method
	"RPRJ",                      // Plugin unique ID of maximum length 4.
	"Reprojection2",            // Plugin name
	2,                           // API major version number
	1,                           // API minor version number
	1,                           // Plugin major version number
	0,                           // Plugin minor version number
	FF_EFFECT,                   // Plugin type
	"Change image projection",   // Plugin description
	"Modify projections"      // About
);

static const char _vertexShaderCode[] = R"(#version 410 core
uniform vec2 MaxUV;

layout( location = 0 ) in vec4 vPosition;
layout( location = 1 ) in vec2 vUV;

out vec2 uv;

void main()
{
	gl_Position = vPosition;
	uv = vUV * MaxUV;
}
)";

AddSubtract::AddSubtract() :
	inputProjection( 0 ), outputProjection( 0 ), r( 0.5f ), g( 0.5f ), b( 0.5f ), fov( 1.0 )
{
	SetMinInputs( 1 );
	SetMaxInputs( 1 );

	SetOptionParamInfo( PT_INPUT_PROJECTION, "InputProjection", 3, inputProjection );
	SetParamElementInfo( PT_INPUT_PROJECTION, 0, "Equirectangular", 0 );
	SetParamElementInfo( PT_INPUT_PROJECTION, 1, "Fisheye", 1 );
	SetParamElementInfo( PT_INPUT_PROJECTION, 2, "Flat", 2 );
	SetParamElementInfo( PT_INPUT_PROJECTION, 3, "Cubemap", 3 );
	SetOptionParamInfo(  PT_OUTPUT_PROJECTION, "OutputProjection", 3, outputProjection );
	SetParamElementInfo( PT_OUTPUT_PROJECTION, 0, "Equirectangular", 0 );
	SetParamElementInfo( PT_OUTPUT_PROJECTION, 1, "Fisheye", 1 );
	SetParamElementInfo( PT_OUTPUT_PROJECTION, 2, "Flat", 2 );
	SetParamElementInfo( PT_OUTPUT_PROJECTION, 3, "Cubemap", 3 );

	SetParamInfof( PT_RED, "Pitch", FF_TYPE_BRIGHTNESS );
	SetParamInfof( PT_GREEN, "Roll", FF_TYPE_GREEN );
	SetParamInfof( PT_BLUE, "Yaw", FF_TYPE_BLUE );
	SetParamInfof( PT_FOV, "fov Out", FF_TYPE_STANDARD );

	FFGLLog::LogToHost( "Created AddSubtract effect" );
}
AddSubtract::~AddSubtract()
{
}

FFResult AddSubtract::InitGL( const FFGLViewportStruct* vp )
{
	if( !shader.Compile( _vertexShaderCode, _fragmentShaderCode ) )
	{
		DeInitGL();
		return FF_FAIL;
	}
	if( !quad.Initialise() )
	{
		DeInitGL();
		return FF_FAIL;
	}
	
	//Use base-class init as success result so that it retains the viewport.
	return CFFGLPlugin::InitGL( vp );
}
FFResult AddSubtract::ProcessOpenGL( ProcessOpenGLStruct* pGL )
{
	if( pGL->numInputTextures < 1 )
		return FF_FAIL;

	if( pGL->inputTextures[ 0 ] == NULL )
		return FF_FAIL;

	//FFGL requires us to leave the context in a default state on return, so use this scoped binding to help us do that.
	ScopedShaderBinding shaderBinding( shader.GetGLID() );
	//The shader's sampler is always bound to sampler index 0 so that's where we need to bind the texture.
	//Again, we're using the scoped bindings to help us keep the context in a default state.
	ScopedSamplerActivation activateSampler( 0 );
	Scoped2DTextureBinding textureBinding( pGL->inputTextures[ 0 ]->Handle );

	shader.Set( "InputTexture", 0 );

	//The input texture's dimension might change each frame and so might the content area.
	//We're adopting the texture's maxUV using a uniform because that way we dont have to update our vertex buffer each frame.
	FFGLTexCoords maxCoords = GetMaxGLTexCoords( *pGL->inputTextures[ 0 ] );
	shader.Set( "MaxUV", maxCoords.s, maxCoords.t );

	glUniform3f( shader.FindUniform( "Brightness" ), r, g, b );
	glUniform1f( shader.FindUniform( "fovOut" ), fov );
	glUniform1i( shader.FindUniform( "inputProjection" ), inputProjection );
	glUniform1i( shader.FindUniform( "outputProjection" ), outputProjection );

	quad.Draw();

	return FF_SUCCESS;
}
FFResult AddSubtract::DeInitGL()
{
	shader.FreeGLResources();
	quad.Release();

	return FF_SUCCESS;
}

FFResult AddSubtract::SetFloatParameter( unsigned int dwIndex, float value )
{
	switch( dwIndex )
	{
	case PT_RED:
		r = value;
		break;
	case PT_GREEN:
		g = value;
		break;
	case PT_BLUE:
		b = value;
		break;
	case PT_INPUT_PROJECTION:
		inputProjection = value;
		break;
	case PT_OUTPUT_PROJECTION:
		outputProjection = value;
		break;
	case PT_FOV:
		fov = value;
		break;
	default:
		return FF_FAIL;
	}

	return FF_SUCCESS;
}

float AddSubtract::GetFloatParameter( unsigned int index )
{
	switch( index )
	{
	case PT_RED:
		return r;
	case PT_GREEN:
		return g;
	case PT_BLUE:
		return b;
	case PT_INPUT_PROJECTION:
		return inputProjection;
	case PT_OUTPUT_PROJECTION:
		return outputProjection;
	case PT_FOV:
		return fov;
	}

	return 0.0f;
}
