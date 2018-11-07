#include "EquiRotation.h"
#include <ffgl/FFGLLib.h>
#include <ffglex/FFGLScopedShaderBinding.h>
#include <ffglex/FFGLScopedSamplerActivation.h>
#include <ffglex/FFGLScopedTextureBinding.h>
using namespace ffglex;

#define FFPARAM_Roll ( 0 )
#define FFPARAM_Pitch ( 1 )
#define FFPARAM_Yaw ( 2 )

static CFFGLPluginInfo PluginInfo(
	PluginFactory< EquiToFisheye >,// Create method
	"360V",                       // Plugin unique ID of maximum length 4.
	"360 VJ",				 	  // Plugin name
	2,                            // API major version number
	1,                            // API minor version number
	1,                            // Plugin major version number
	0,                            // Plugin minor version number
	FF_EFFECT,                    // Plugin type
	"Rotate 360 videos",		  // Plugin description
	"Written by Daniel Arnett, go to https://github.com/DanielArnett/360-VJ/releases for more detail."      // About
);

static const char vertexShaderCode[] = R"(#version 410 core
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

static const char fragmentShaderCode[] = R"(#version 410 core
float PI = 3.14159265359;
uniform sampler2D InputTexture;
uniform vec3 InputRotation;

in vec2 uv;
mat3 Rx(float th) 
{
	return mat3(1, 0, 0,
				0, cos(th), -sin(th),
				0, sin(th), cos(th));
}
mat3 Ry(float th) 
{
	return mat3(cos(th), 0, sin(th),
				   0,    1,    0,
				-sin(th), 0, cos(th));
}
mat3 Rz(float th) 
{
	return mat3(cos(th), -sin(th), 0,
				sin(th),  cos(th), 0,
				  0,         0   , 1);
}
vec3 rotatePoint(vec3 p, vec3 th)
{
	mat3 rotation = Rx(th.r) * Ry(th.g) * Rz(th.b);
	return rotation * p;
}
vec2 uvToLatLon(vec2 uv)
{
	return vec2(uv.y * PI - PI/2.0,
				uv.x * 2.0*PI - PI);
}

vec3 latLonToPoint(vec2 latLon)
{
	vec3 point;
	point.x = cos(latLon.x) * sin(latLon.y);
	point.y = sin(latLon.x);
	point.z = cos(latLon.x) * cos(latLon.y);
	return point;
}
vec2 pointToLatLon(vec3 point) 
{
	vec2 latLon;
	latLon.x = asin(point.y);
	latLon.y = atan(point.x, point.z);
	return latLon;
}
vec2 latLonToUv(vec2 latLon)
{	
	vec2 uv;
	uv.x = (latLon.y + PI)/(2.0*PI);
	uv.y = (latLon.x + PI/2.0)/PI;
	return uv;
}
out vec4 fragColor;
void main()
{
	// Latitude and Longitude of the destination pixel (uv)
	vec2 latLon = uvToLatLon(uv);
	// Create a point on the unit-sphere from the latitude and longitude
		// X increases from left to right [-1 to 1]
		// Y increases from bottom to top [-1 to 1]
		// Z increases from back to front [-1 to 1]
	vec3 point = latLonToPoint(latLon);
	// Rotate the point based on the user input in radians
	point = rotatePoint(point, InputRotation.rgb * PI);
	// Convert back to latitude and longitude
	latLon = pointToLatLon(point);
	// Convert back to the normalized pixel coordinate
	vec2 sourcePixel = latLonToUv(latLon);
	// Set the color of the destination pixel to the color of the source pixel
	fragColor = texture( InputTexture, sourcePixel );
}
)";

EquiToFisheye::EquiToFisheye() :
	maxUVLocation( -1 ),
	fieldOfViewLocation( -1 ),
	pitch( 0.5f ),
	yaw( 0.5f ),
	roll( 0.5f )
{
	// Input properties
	SetMinInputs( 1 );
	SetMaxInputs( 1 );

	// Parameters
	SetParamInfof( FFPARAM_Roll, "Roll", FF_TYPE_STANDARD );
	SetParamInfof( FFPARAM_Pitch, "Pitch", FF_TYPE_STANDARD );
	SetParamInfof( FFPARAM_Yaw, "Yaw", FF_TYPE_STANDARD );
}
EquiToFisheye::~EquiToFisheye()
{
}

FFResult EquiToFisheye::InitGL( const FFGLViewportStruct* vp )
{
	if( !shader.Compile( vertexShaderCode, fragmentShaderCode ) )
	{
		DeInitGL();
		return FF_FAIL;
	}
	if( !quad.Initialise() )
	{
		DeInitGL();
		return FF_FAIL;
	}

	//FFGL requires us to leave the context in a default state on return, so use this scoped binding to help us do that.
	ScopedShaderBinding shaderBinding( shader.GetGLID() );

	//We're never changing the sampler to use, instead during rendering we'll make sure that we're always
	//binding the texture to sampler 0.
	glUniform1i( shader.FindUniform( "inputTexture" ), 0 );

	//We need to know these uniform locations because we need to set their value each frame.
	maxUVLocation = shader.FindUniform( "MaxUV" );
	fieldOfViewLocation = shader.FindUniform( "InputRotation" );

	//Use base-class init as success result so that it retains the viewport.
	return CFreeFrameGLPlugin::InitGL( vp );
}
FFResult EquiToFisheye::ProcessOpenGL( ProcessOpenGLStruct* pGL )
{
	if( pGL->numInputTextures < 1 )
		return FF_FAIL;

	if( pGL->inputTextures[ 0 ] == NULL )
		return FF_FAIL;

	//FFGL requires us to leave the context in a default state on return, so use this scoped binding to help us do that.
	ScopedShaderBinding shaderBinding( shader.GetGLID() );

	FFGLTextureStruct& Texture = *( pGL->inputTextures[ 0 ] );

	//The input texture's dimension might change each frame and so might the content area.
	//We're adopting the texture's maxUV using a uniform because that way we dont have to update our vertex buffer each frame.
	FFGLTexCoords maxCoords = GetMaxGLTexCoords( Texture );
	glUniform2f( maxUVLocation, maxCoords.s, maxCoords.t );

	glUniform3f( fieldOfViewLocation,
				 -1.0f + ( pitch * 2.0f ),
				 -1.0f + ( yaw * 2.0f ),
				 -1.0f + ( roll * 2.0f ) );

	//The shader's sampler is always bound to sampler index 0 so that's where we need to bind the texture.
	//Again, we're using the scoped bindings to help us keep the context in a default state.
	ScopedSamplerActivation activateSampler( 0 );
	Scoped2DTextureBinding textureBinding( Texture.Handle );

	quad.Draw();

	return FF_SUCCESS;
}
FFResult EquiToFisheye::DeInitGL()
{
	shader.FreeGLResources();
	quad.Release();
	maxUVLocation = -1;
	fieldOfViewLocation = -1;

	return FF_SUCCESS;
}

FFResult EquiToFisheye::SetFloatParameter( unsigned int dwIndex, float value )
{
	switch( dwIndex )
	{
	case FFPARAM_Pitch:
		pitch = value;
		break;
	case FFPARAM_Yaw:
		yaw = value;
		break;
	case FFPARAM_Roll:
		roll = value;
		break;
	default:
		return FF_FAIL;
	}

	return FF_SUCCESS;
}

float EquiToFisheye::GetFloatParameter( unsigned int dwIndex )
{
	switch( dwIndex )
	{
	case FFPARAM_Pitch:
		return pitch;
	case FFPARAM_Yaw:
		return yaw;
	case FFPARAM_Roll:
		return roll;

	default:
		return 0.0f;
	}
}
char* EquiToFisheye::GetParameterDisplay(unsigned int index)
{
	/**
	 * We're not returning ownership over the string we return, so we have to somehow guarantee that
	 * the lifetime of the returned string encompasses the usage of that string by the host. Having this static
	 * buffer here keeps previously returned display string alive until this function is called again.
	 * This happens to be long enough for the hosts we know about.
	 */
	static char displayValueBuffer[15];
	memset(displayValueBuffer, 0, sizeof(displayValueBuffer));
	switch (index)
	{
	case FFPARAM_Pitch:
		sprintf_s(displayValueBuffer, "%f", pitch * 360 - 180);
		return displayValueBuffer;
	case FFPARAM_Yaw:
		sprintf_s(displayValueBuffer, "%f", yaw * 360 - 180);
		return displayValueBuffer;
	case FFPARAM_Roll:
		sprintf_s(displayValueBuffer, "%f", roll * 360 - 180);
		return displayValueBuffer;
	default:
		return CFreeFrameGLPlugin::GetParameterDisplay(index);
	}
}