#include "FisheyeToEqui.h"
#include <ffgl/FFGLLib.h>
#include <ffglex/FFGLScopedShaderBinding.h>
#include <ffglex/FFGLScopedSamplerActivation.h>
#include <ffglex/FFGLScopedTextureBinding.h>
using namespace ffglex;

#define FFPARAM_Roll ( 0 )
#define FFPARAM_Pitch ( 1 )
#define FFPARAM_Yaw ( 2 )
#define FFPARAM_FadeInner ( 3 )
#define FFPARAM_FadeOuter ( 4 )

static CFFGLPluginInfo PluginInfo(
	PluginFactory< FisheyeToEqui >,// Create method
	"F2EQ",                       // Plugin unique ID of maximum length 4.
	"Fisheye To 360",		 	  // Plugin name
	2,                            // API major version number
	1,                            // API minor version number
	1,                            // Plugin major version number
	0,                            // Plugin minor version number
	FF_EFFECT,                    // Plugin type
	"Convert fisheye videos into Equirectangular 360 videos.",	  // Plugin description
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
uniform vec2 InputFade;
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
vec3 rotatePoint(vec3 p, vec3 th, vec3 offset)
{
	th += offset;
	return Rx(th.r) * Ry(th.g) * Rz(th.b) * p;
}

// Convert x, y pixel coordinates from an Equirectangular image into latitude/longitude coordinates.
vec2 uvToLatLon(vec2 uv)
{
	return vec2(uv.y * PI - PI/2.0,
				uv.x * 2.0*PI - PI);
}

// Convert latitude, longitude into a 3d point on the unit-sphere.
vec3 latLonToPoint(vec2 latLon)
{
	vec3 point;
	point.x = cos(latLon.x) * sin(latLon.y);
	point.y = sin(latLon.x);
	point.z = cos(latLon.x) * cos(latLon.y);
	return point;
}

// Convert a 3D point on the unit sphere into latitude and longitude.
vec2 pointToLatLon(vec3 point) 
{
	vec2 latLon;
	latLon.x = asin(point.y);
	latLon.y = atan(point.x, point.z);
	return latLon;
}
float getRFromLatLon(vec2 latLon)
{
	return 1.0 - latLon.x/(PI / 2.0);
}
// Convert latitude, longitude to x, y pixel coordinates on the source fisheye image.
vec2 latLonToFisheyeUv(vec2 latLon, vec3 point)
{	
	// The distance from the source pixel to the center of the image
	float r;
	// phi is the angle of r on the unit circle. See polar coordinates for more details
	float phi;
	// Get the position of the source pixel
	vec2 sourcePixel;

	// Get the source pixel radius from center
	r = getRFromLatLon(latLon);
	// Don't bother with source pixels outside of the fisheye circle
	if (1.0 < r) {
		// Return a transparent pixel
		return vec2(-1.0, -1.0);
	}
	phi = atan(-point.z, point.x);
	
	sourcePixel.x = r * cos(phi);
	sourcePixel.y = r * sin(phi);
	// Normalize the output pixel to be in the range [0,1]
	sourcePixel += 1.0;
	sourcePixel /= 2.0;

	return sourcePixel;
}
float getFadeCoefficient(float r)
{
	if (r < InputFade.x)
	{
		return 1.0;
	}
	else if (InputFade.x < r && r < InputFade.y)
	{
		return (InputFade.y - r) / (InputFade.y - InputFade.x);
	}
	else
	{
		return 0.0;
	}
}
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
	point = rotatePoint(point, InputRotation.xyz * PI, vec3(PI/2.0, 0.0, 0.0));
	// Convert back to latitude and longitude
	latLon = pointToLatLon(point);
	// Convert back to the normalized pixel coordinate
	vec2 sourcePixel = latLonToFisheyeUv(latLon, point);
	// Set the color of the destination pixel to the color of the source pixel if it's in the fisheye circle.
	if (sourcePixel.x != -1.0)
	{
		fragColor = texture( InputTexture, sourcePixel );
		// Set the opacity of the pixel
		fragColor.a = getFadeCoefficient(getRFromLatLon(latLon));
		fragColor.rgb *= fragColor.a;
	}
	else
	{
		fragColor = vec4(0.0, 0.0, 0.0, 0.0);
	}
}

)";

FisheyeToEqui::FisheyeToEqui() :
	maxUVLocation( -1 ),
	fieldOfViewLocation(-1),
	fadeLocation(-1),
	fadeIn( 0.8f ),
	fadeOut( 1.0f ),
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
	SetParamInfof( FFPARAM_Yaw, "Yaw", FF_TYPE_STANDARD);
	SetParamInfof( FFPARAM_FadeInner, "Inner Radial Fade", FF_TYPE_STANDARD);
	SetParamInfof( FFPARAM_FadeOuter, "Outer Radial Fade", FF_TYPE_STANDARD);
}
FisheyeToEqui::~FisheyeToEqui()
{
}

FFResult FisheyeToEqui::InitGL( const FFGLViewportStruct* vp )
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
	fieldOfViewLocation = shader.FindUniform("InputRotation");
	fadeLocation = shader.FindUniform("InputFade");

	//Use base-class init as success result so that it retains the viewport.
	return CFreeFrameGLPlugin::InitGL( vp );
}
FFResult FisheyeToEqui::ProcessOpenGL( ProcessOpenGLStruct* pGL )
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

	glUniform2f(fadeLocation,
				fadeIn,
				fadeOut);

	//The shader's sampler is always bound to sampler index 0 so that's where we need to bind the texture.
	//Again, we're using the scoped bindings to help us keep the context in a default state.
	ScopedSamplerActivation activateSampler( 0 );
	Scoped2DTextureBinding textureBinding( Texture.Handle );

	quad.Draw();

	return FF_SUCCESS;
}
FFResult FisheyeToEqui::DeInitGL()
{
	shader.FreeGLResources();
	quad.Release();
	maxUVLocation = -1;
	fieldOfViewLocation = -1;

	return FF_SUCCESS;
}

FFResult FisheyeToEqui::SetFloatParameter( unsigned int dwIndex, float value )
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
	case FFPARAM_FadeInner:
		fadeIn = value;
		break;
	case FFPARAM_FadeOuter:
		fadeOut = value;
		break;
	default:
		return FF_FAIL;
	}

	return FF_SUCCESS;
}

float FisheyeToEqui::GetFloatParameter( unsigned int dwIndex )
{
	switch( dwIndex )
	{
	case FFPARAM_Pitch:
		return pitch;
	case FFPARAM_Yaw:
		return yaw;
	case FFPARAM_Roll:
		return roll;
	case FFPARAM_FadeInner:
		return fadeIn;
	case FFPARAM_FadeOuter:
		return fadeOut;

	default:
		return 0.0f;
	}
}
