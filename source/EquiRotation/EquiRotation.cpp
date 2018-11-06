#include "EquiRotation.h"
#include <ffgl/FFGLLib.h>
#include <ffglex/FFGLScopedShaderBinding.h>
#include <ffglex/FFGLScopedSamplerActivation.h>
#include <ffglex/FFGLScopedTextureBinding.h>
using namespace ffglex;

#define FFPARAM_Field_Of_View ( 0 )
#define FFPARAM_Aspect_Ratio ( 1 )
#define FFPARAM_Yaw ( 2 )

static CFFGLPluginInfo PluginInfo(
	PluginFactory< FlatToFisheye >,// Create method
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
uniform vec3 Brightness;

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
	vec2 rotation = vec2(Brightness.r, Brightness.g);

	// Given a destination pixel on the screen, we need to find the color of a source pixel used to fill this destination pixel.
	// The color of the destination pixel
	//vec4 color = texture( InputTexture, uv );
	// Position of the destination pixel in xy coordinates in the range [0,1]
	vec2 pos;
	// Position of the source pixel in xy coordinates
	vec2 outCoord;
	// The ray from the camera into the 360 virtual screen
	// X increases from left to right [-1 to 1]
	// Y increases from bottom to top [-1 to 1]
	// Z increases from back to front [-1 to 1]
	vec3 ray;
	// The input given by the user in Resolume
	float xRotateInput = Brightness.r / 2.0;
	float yRotateInput = Brightness.g / 2.0;
	float zRotateInput = Brightness.b / 2.0;
	// Normalize the xy coordinates in the range [0,1]
	pos.x = uv.x;
	pos.y = uv.y;
	
	float latitude = pos.y * PI - PI/2.0;
	float longitude = pos.x * 2.0*PI - PI;
	// Create a ray from the latitude and longitude
	ray.x = cos(latitude) * sin(longitude);
	ray.y = sin(latitude);
	ray.z = cos(latitude) * cos(longitude);
	// Rotate the ray based on the user input
	if (xRotateInput != 0.5) {
		ray = PRotateX(ray, xRotateInput * 2.0*PI);
	}
	if (yRotateInput != 0.5) {
		ray = PRotateY(ray, yRotateInput * 2.0*PI);
	}
	if (zRotateInput != 0.5) {
		ray = PRotateZ(ray, zRotateInput * 2.0*PI);
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
	pos.x = (longitude + PI)/(2.0*PI);
	pos.y = (latitude + PI/2.0)/PI;

	vec4 color = texture( InputTexture, pos );
	fragColor = color;
}
)";

FlatToFisheye::FlatToFisheye() :
	maxUVLocation( -1 ),
	fieldOfViewLocation( -1 ),
	aspectRatio( 0.5f ),
	yaw( 0.5f ),
	fieldOfView( 0.5f )
{
	// Input properties
	SetMinInputs( 1 );
	SetMaxInputs( 1 );

	// Parameters
	SetParamInfof( FFPARAM_Field_Of_View, "Roll", FF_TYPE_STANDARD );
	SetParamInfof( FFPARAM_Aspect_Ratio, "Pitch", FF_TYPE_STANDARD );
	SetParamInfof( FFPARAM_Yaw, "Yaw", FF_TYPE_STANDARD );
}
FlatToFisheye::~FlatToFisheye()
{
}

FFResult FlatToFisheye::InitGL( const FFGLViewportStruct* vp )
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
	fieldOfViewLocation = shader.FindUniform( "Brightness" );

	//Use base-class init as success result so that it retains the viewport.
	return CFreeFrameGLPlugin::InitGL( vp );
}
FFResult FlatToFisheye::ProcessOpenGL( ProcessOpenGLStruct* pGL )
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
				 -1.0f + ( aspectRatio * 2.0f ),
				 -1.0f + ( yaw * 2.0f ),
				 -1.0f + ( fieldOfView * 2.0f ) );

	//The shader's sampler is always bound to sampler index 0 so that's where we need to bind the texture.
	//Again, we're using the scoped bindings to help us keep the context in a default state.
	ScopedSamplerActivation activateSampler( 0 );
	Scoped2DTextureBinding textureBinding( Texture.Handle );

	quad.Draw();

	return FF_SUCCESS;
}
FFResult FlatToFisheye::DeInitGL()
{
	shader.FreeGLResources();
	quad.Release();
	maxUVLocation = -1;
	fieldOfViewLocation = -1;

	return FF_SUCCESS;
}

FFResult FlatToFisheye::SetFloatParameter( unsigned int dwIndex, float value )
{
	switch( dwIndex )
	{
	case FFPARAM_Aspect_Ratio:
		aspectRatio = value;
		break;
	case FFPARAM_Yaw:
		yaw = value;
		break;
	case FFPARAM_Field_Of_View:
		fieldOfView = value;
		break;
	default:
		return FF_FAIL;
	}

	return FF_SUCCESS;
}

float FlatToFisheye::GetFloatParameter( unsigned int dwIndex )
{
	switch( dwIndex )
	{
	case FFPARAM_Aspect_Ratio:
		return aspectRatio;
	case FFPARAM_Yaw:
		return yaw;
	case FFPARAM_Field_Of_View:
		return fieldOfView;

	default:
		return 0.0f;
	}
}
