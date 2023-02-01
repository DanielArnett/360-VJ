#include "EquiToFisheye.h"
#include <ffgl/FFGLLib.h>
#include <ffglex/FFGLScopedShaderBinding.h>
#include <ffglex/FFGLScopedSamplerActivation.h>
#include <ffglex/FFGLScopedTextureBinding.h>
using namespace ffglex;

//#define FFPARAM_Roll ( 0 )
//#define FFPARAM_Pitch ( 1 )
//#define FFPARAM_Yaw ( 2 )

static CFFGLPluginInfo PluginInfo(
	PluginFactory< EquiToFisheye >,// Create method
	"EQ2F",                       // Plugin unique ID of maximum length 4.
	"360 to Fisheye",		 	  // Plugin name
	2,                            // API major version number
	1,                            // API minor version number
	1,                            // Plugin major version number
	0,                            // Plugin minor version number
	FF_EFFECT,                    // Plugin type
	"Convert 360 Videos into Fisheye Videos",	  // Plugin description
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

in vec2 uv;

out vec4 fragColor;
float PI = 3.14159265359;

vec2 getLatLonFromFisheyeUv(vec2 uv, float r)
{
	vec2 latLon;
	latLon.x = (1.0 - r)*(PI / 2.0);
	// Calculate longitude
	if (r == 0.0) {
		latLon.y = 0.0;
	}
	else if (uv.x < 0.0) {
		latLon.y = PI - asin(uv.y / r);
	}
	else if (uv.x >= 0.0) {
		latLon.y = asin(uv.y / r);
	}
	if (latLon.y < 0.0) {
		latLon.y += 2.0*PI;
	}

	return latLon;
}
// Convert latitude, longitude into a 3d point on the unit-sphere.
vec3 latLonToPoint(vec2 latLon)
{
	vec3 point;
	// 3D position of the destination pixel on the unit sphere.
	point.x = cos(latLon.x) * cos(latLon.y);
	point.y = cos(latLon.x) * sin(latLon.y);
	point.z = sin(latLon.x);

	return point;
}
vec2 pointToLatLon(vec3 p)
{
	vec2 latLon;
	latLon.x = asin(p.z);
	latLon.y = -acos(p.x / cos(latLon.x));
	// Rotate by a quarter turn to align the fisheye image with the center of the 360 image
	latLon.y += PI/2.0;
	return latLon;
}
void main()
{
	vec2 pos = 2.0 * uv - 1.0;
	float r = distance(vec2(0.0, 0.0), pos);
	// Don't bother with pixels outside of the fisheye circle
	if (1.0 < r) {
		// Return a transparent pixel
		fragColor = vec4(0.0,0.0,0.0,0.0);
		return;
	}
	vec3 p;
	vec2 sourcePixel;
	// Get the latitude and longitude for the destination pixel.
	vec2 latLon = getLatLonFromFisheyeUv(pos, r);
	p = latLonToPoint(latLon);

	// TODO align my equirectangular coordinate system with the fisheye coordinate system. Here's a temporary fix.
	p.z = p.y;
	p.y = p.z;

	// Get the lat lon for the source pixel
	latLon = pointToLatLon(p);

	sourcePixel.x = (latLon.y + PI) / (2.0 * PI);
	sourcePixel.y = (latLon.x + PI/2.0) / PI;

	fragColor = texture(InputTexture, sourcePixel);
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
	/*SetParamInfof( FFPARAM_Roll, "Roll", FF_TYPE_STANDARD );
	SetParamInfof( FFPARAM_Pitch, "Pitch", FF_TYPE_STANDARD );
	SetParamInfof( FFPARAM_Yaw, "Yaw", FF_TYPE_STANDARD );*/
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
	fieldOfViewLocation = shader.FindUniform( "Brightness" );

	//Use base-class init as success result so that it retains the viewport.
	return CFFGLPlugin::InitGL( vp );
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
	/*case FFPARAM_Pitch:
		aspectRatio = value;
		break;
	case FFPARAM_Yaw:
		yaw = value;
		break;
	case FFPARAM_Roll:
		fieldOfView = value;
		break;*/
	default:
		return FF_FAIL;
	}

	return FF_SUCCESS;
}

float EquiToFisheye::GetFloatParameter( unsigned int dwIndex )
{
	switch( dwIndex )
	{
	/*case FFPARAM_Pitch:
		return aspectRatio;
	case FFPARAM_Yaw:
		return yaw;
	case FFPARAM_Roll:
		return fieldOfView;*/

	default:
		return 0.0f;
	}
}
