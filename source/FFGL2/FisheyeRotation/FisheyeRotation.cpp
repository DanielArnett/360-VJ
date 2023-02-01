#include "FisheyeRotation.h"
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
	"FROT",                       // Plugin unique ID of maximum length 4.
	"Fisheye Rotation",		 	  // Plugin name
	2,                            // API major version number
	1,                            // API minor version number
	1,                            // Plugin major version number
	0,                            // Plugin minor version number
	FF_EFFECT,                    // Plugin type
	"Rotate Fisheye videos",	  // Plugin description
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
out vec4 fragColor;
// A transformation matrix rotating about the x axis by `th` degrees.
mat3 Rx(float th) 
{
	return mat3(1, 0, 0,
				0, cos(th), -sin(th),
				0, sin(th), cos(th));
}
// A transformation matrix rotating about the y axis by `th` degrees.
mat3 Ry(float th) 
{
	return mat3(cos(th), 0, sin(th),
				   0,    1,    0,
				-sin(th), 0, cos(th));
}
// A transformation matrix rotating about the z axis by `th` degrees.
mat3 Rz(float th) 
{
	return mat3(cos(th), -sin(th), 0,
				sin(th),  cos(th), 0,
				  0,         0   , 1);
}

// Rotate a point vector by th.x then th.y then th.z, and return the rotated point.
vec3 rotatePoint(vec3 p, vec3 th)
{
	return Rx(th.r) * Ry(th.g) * Rz(th.b) * p;
}
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
	// Perform z rotation.
	/*latLon.y += InputRotation.z * 2.0 * PI;
	if (latLon.y < 0.0) {
		latLon.y += 2.0*PI;
	}*/
	return latLon;
}
// Convert latitude, longitude into a 3d point on the unit-sphere.
vec3 latLonToPoint(vec2 latLon)
{
	vec3 point;
	point.x = cos(latLon.x) * cos(latLon.y);
	point.y = cos(latLon.x) * sin(latLon.y);
	point.z = sin(latLon.x);
	return point;
}

// Convert a 3D point on the unit sphere into latitude and longitude.
vec2 pointToLatLon(vec3 point) 
{
	vec2 latLon;
	latLon.x = asin(point.z);
	latLon.y = -acos(point.x / cos(latLon.x));
	return latLon;
}
void main()
{
	// Position of the destination pixel in xy coordinates in the range [-1,1]
	vec2 pos = 2.0 * uv - 1.0;

	// Radius of the pixel from the center
	float r = distance(vec2(0.0,0.0), pos);
	// Don't bother with pixels outside of the fisheye circle
	if (1.0 < r) {
		// Return a transparent pixel
		fragColor = vec4(0.0,0.0,0.0,0.0);
		return;
	}
	// phi is the angle of r on the unit circle. See polar coordinates for more details
	float phi;
	vec2 latLon = getLatLonFromFisheyeUv(pos, r);
	// The point on the unit-sphere
	vec3 p;
	// The source pixel's coordinates
	vec2 sourcePixel;

	
	// Turn the latitude and longitude into a 3D point
	p = latLonToPoint(latLon);
	
	// Rotate the value based on the user input
	p = rotatePoint(p, 2.0 * PI * InputRotation);
	// Get the latitude and longitude of the rotated point
	latLon = pointToLatLon(p);

	// Get the source pixel radius from center
	r = 1.0 - latLon.x/(PI / 2.0);
	// Don't bother with source pixels outside of the fisheye circle
	if (1.0 < r) {
		// Return a transparent pixel
		fragColor = vec4(0.0,0.0,0.0,0.0);
		return;
	}
	phi = atan(p.y, p.x);
	// Get the position of the output pixel
	sourcePixel.x = r * cos(phi);
	sourcePixel.y = r * sin(phi);
	// Normalize the output pixel to be in the range [0,1]
	sourcePixel += 1.0;
	sourcePixel /= 2.0;
	// Set the color of the destination pixel to the color of the source pixel.
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
