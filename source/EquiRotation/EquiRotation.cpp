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
uniform sampler2D InputTexture;
uniform vec3 InputRotation;

in vec2 uv;
/*
 * This is used to manipulate 360 VR videos, also known as equirectangular videos.
 * This shader can pitch, roll, and yaw the camera within a 360 image.
*/
vec3 PRotateX(vec3 p, float theta)
{
   vec3 q;
   q.x = p.x;
   q.y = p.y * cos(theta) + p.z * sin(theta);
   q.z = -p.y * sin(theta) + p.z * cos(theta);
   return(q);
}

vec3 PRotateY(vec3 p, float theta)
{
   vec3 q;
   q.x = p.x * cos(theta) - p.z * sin(theta);
   q.y = p.y;
   q.z = p.x * sin(theta) + p.z * cos(theta);
   return(q);
}

vec3 PRotateZ(vec3 p, float theta)
{
   vec3 q;
   q.x = p.x * cos(theta) + p.y * sin(theta);
   q.y = -p.x * sin(theta) + p.y * cos(theta);
   q.z = p.z;
   return(q);
}
out vec4 fragColor;
float PI = 3.14159265359;
void main()
{
	ivec2 textureSize2d = textureSize(InputTexture,0);
    vec2 textureSize = textureSize2d.xy;
	vec2 rotation = vec2(InputRotation.r, InputRotation.g);
	vec3 input = InputRotation.rgb / 2.0;
	if (input.r == 0.5) { input.r -= 0.00001; }
	if (input.g == 0.5) { input.g -= 0.00001; }
	if (input.b == 0.5) { input.b -= 0.00001; }

	// Position of the destination pixel in xy coordinates in the range [0,1]
	vec2 pos;
	// Position of the source pixel in xy coordinates
	vec2 outCoord;
	// The ray from the camera into the 360 virtual screen
	// X increases from left to right [-1 to 1]
	// Y increases from bottom to top [-1 to 1]
	// Z increases from back to front [-1 to 1]
	vec3 ray;

	float latitude = uv.y * PI - PI/2.0;
	float longitude = uv.x * 2.0*PI - PI;
	// Create a ray from the latitude and longitude
	ray.x = cos(latitude) * sin(longitude);
	ray.y = sin(latitude);
	ray.z = cos(latitude) * cos(longitude);
	// Rotate the ray based on the user input
	if (input.r != 0.5) {
		ray = PRotateX(ray, input.r * 2.0*PI);
	}
	if (input.g != 0.5) {
		ray = PRotateY(ray, input.g * 2.0*PI);
	}
	if (input.b != 0.5) {
		ray = PRotateZ(ray, input.b * 2.0*PI);
	}
	// Convert back to latitude and longitude
	latitude = asin(ray.y);
	// Manually implement `longitude = atan2(ray.x, ray.z);`
	if (ray.z > 0.0) {
		longitude = atan(ray.x/ray.z);
	}
	else if (ray.z < 0.0 && ray.x >= 0.0) {
		longitude = atan(ray.x/ray.z) + PI;
	}
	else if (ray.z < 0.0 && ray.x < 0.0) {
		longitude = atan(ray.x/ray.z) - PI;
	}
	else if (ray.z == 0.0 && ray.x > 0.0) {
		longitude = PI/2.0;
	}
	else if (ray.z == 0.0 && ray.x < 0.0) {
		longitude = -PI/2.0;
	}
	// Convert back to the normalized pixel coordinate
	outCoord.x = (longitude + PI)/(2.0*PI);
	outCoord.y = (latitude + PI/2.0)/PI;

	vec4 color = texture( InputTexture, outCoord );
	fragColor = color;
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