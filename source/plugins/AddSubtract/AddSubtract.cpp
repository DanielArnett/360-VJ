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
	PT_PITCH,
	PT_ROLL,
	PT_YAW,
	PT_FOV_IN,
	PT_FOV_OUT
};

static CFFGLPluginInfo PluginInfo(
	PluginFactory< AddSubtract >,// Create method
	"RPRJ",                      // Plugin unique ID of maximum length 4.
	"Reprojection",            // Plugin name
	2,                           // API major version number
	1,                           // API minor version number
	1,                           // Plugin major version number
	0,                           // Plugin minor version number
	FF_EFFECT,                   // Plugin type
	"Change image projection",   // Plugin description
	"Modify projections"      // About
);

static const char _vertexShaderCode[] = R"(#version 410 core

layout( location = 0 ) in vec4 vPosition;
layout( location = 1 ) in vec2 vUV;

out vec2 uv;

void main()
{
	gl_Position = vPosition;
	uv = vUV;
}
)";

AddSubtract::AddSubtract() :
	inputProjection( 0 ), outputProjection( 0 ), pitch( 0.5f ), roll( 0.5f ), yaw( 0.5f ), fovOut( 0.5 ), fovIn( 0.5 )
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

	SetParamInfof( PT_PITCH, "Pitch", FF_TYPE_STANDARD );
	SetParamInfof( PT_ROLL, "Roll", FF_TYPE_STANDARD );
	SetParamInfof( PT_YAW, "Yaw", FF_TYPE_STANDARD );
	SetParamInfof( PT_FOV_OUT, "fov Out", FF_TYPE_STANDARD );
	SetParamInfof( PT_FOV_IN, "fov In", FF_TYPE_STANDARD );

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
	//SetParamDisplayName( PT_RED, std::to_string( pGL->inputTextures[ 0 ]->Width ).c_str(), true );
	glUniform3f( shader.FindUniform( "Rotation" ), (pitch-0.5)*2.0*3.14159265359,
		                                           (roll -0.5)*2.0*3.14159265359,
		                                           (yaw  -0.5)*2.0*3.14159265359 );
	glUniform1f( shader.FindUniform( "fovOut" ), fovOut * 3.14159269359 / 2.0 );
	glUniform1f( shader.FindUniform( "fovIn" ), fovIn * 3.14159269359 / 2.0 );
	glUniform1i( shader.FindUniform( "inputProjection" ), inputProjection );
	glUniform1i( shader.FindUniform( "outputProjection" ), outputProjection );
	glUniform1i( shader.FindUniform( "width" ), pGL->inputTextures[ 0 ]->Width );
	glUniform1i( shader.FindUniform( "height" ), pGL->inputTextures[ 0 ]->Height );


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
	case PT_PITCH:
		pitch = value;
		break;
	case PT_ROLL:
		roll = value;
		break;
	case PT_YAW:
		yaw = value;
		break;
	case PT_INPUT_PROJECTION:
		inputProjection = value;
		break;
	case PT_OUTPUT_PROJECTION:
		outputProjection = value;
		break;
	case PT_FOV_OUT:
		fovOut = value;
		break;
	case PT_FOV_IN:
		fovIn = value;
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
	case PT_PITCH:
		return pitch;
	case PT_ROLL:
		return roll;
	case PT_YAW:
		return yaw;
	case PT_INPUT_PROJECTION:
		return inputProjection;
	case PT_OUTPUT_PROJECTION:
		return outputProjection;
	case PT_FOV_OUT:
		return fovOut;
	case PT_FOV_IN:
		return fovIn;
	}

	return 0.0f;
}

/**
* This will print a double value behind the slider in resolume. 
*/
void AddSubtract::printDoubleToResolumeBuffer( char ( &buffer )[ 15 ], double value )
{
#if defined( WIN32 ) || defined( _WIN32 ) || defined( __WIN32__ ) || defined( __NT__ )
	sprintf_s( buffer, "%f", value );
#elif __APPLE__
	sprintf( buffer, "%f", value );
#endif
}

char* AddSubtract::GetParameterDisplay( unsigned int index )
{
	static char displayValueBuffer[ 15 ];
	/**
	 * We're not returning ownership over the string we return, so we have to somehow guarantee that
	 * the lifetime of the returned string encompasses the usage of that string by the host. Having this static
	 * buffer here keeps previously returned display string alive until this function is called again.
	 * This happens to be long enough for the hosts we know about.
	 */
	memset( displayValueBuffer, 0, sizeof( displayValueBuffer ) );
	switch( index )
	{
	case PT_PITCH:
		printDoubleToResolumeBuffer( displayValueBuffer, pitch * 360.0f - 180.0f );
		return displayValueBuffer;
	case PT_YAW:
		printDoubleToResolumeBuffer( displayValueBuffer, yaw * 360.0f - 180.0f );
		return displayValueBuffer;
	case PT_ROLL:
		printDoubleToResolumeBuffer( displayValueBuffer, roll * 360.0f - 180.0f );
		return displayValueBuffer;
	case PT_FOV_OUT:
		printDoubleToResolumeBuffer( displayValueBuffer, fovOut );
		return displayValueBuffer;
	case PT_FOV_IN:
		printDoubleToResolumeBuffer( displayValueBuffer, fovIn );
		return displayValueBuffer;
	default:
		return CFFGLPlugin::GetParameterDisplay( index );
	}
}
