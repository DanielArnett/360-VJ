#include "FlatToFisheye.h"
#include <ffgl/FFGLLib.h>
#include <ffglex/FFGLScopedShaderBinding.h>
#include <ffglex/FFGLScopedSamplerActivation.h>
#include <ffglex/FFGLScopedTextureBinding.h>
using namespace ffglex;

#define FFPARAM_Roll ( 0 )
//#define FFPARAM_Aspect_Ratio ( 1 )
//#define FFPARAM_Yaw ( 2 )

static CFFGLPluginInfo PluginInfo(
	PluginFactory< EquiToFisheye >,// Create method
	"FTFV",                       // Plugin unique ID of maximum length 4.
	"Flat to Fisheye",				 	  // Plugin name
	2,                            // API major version number
	1,                            // API minor version number
	1,                            // Plugin major version number
	0,                            // Plugin minor version number
	FF_EFFECT,                    // Plugin type
	"Apply a fisheye effect to flat videos", // Plugin description
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
	uniform float fieldOfView;
	
	// vector with elements in the range [0,1]
	// uv == vec2(0.0, 0.0) at the bottom left corner of the image
	in vec2 uv;
	out vec4 fragColor;

	float PI = 3.14159;

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
		// Get phi of this point, see polar coordinate system for more details.
		float phi = atan(point.y, point.x);
		// With phi, calculate the point on the image plane that is also at the angle phi
		point.x = cos(phi) * tan(PI / 2.0 - latLon.x);
		point.y = sin(phi) * tan(PI / 2.0 - latLon.x);
		point.z = 1.0;
		return point;
	}
	bool outOfBounds(vec2 xy, float lower, float upper)
	{
		vec2 lowerBound = vec2(lower, lower);
		vec2 upperBound = vec2(upper, upper);
		return (any(lessThan(xy, lowerBound)) || any(greaterThan(xy, upperBound)));
	}
	void main()
	{
		ivec2 textureSize2d = textureSize(InputTexture,0);
		vec2 textureSize = textureSize2d.xy;
		float ASPECT_RATIO = textureSize.x / textureSize.y;
		float fovInput = fieldOfView; // 0.0 to 1.0
		float FOV = 180.0 * fovInput; // 60 degrees to 180 degrees

		// Position of the destination pixel in xy coordinates in the range [0,1]
		vec2 pos;
		pos.x = 2.0 * (uv.x - 0.5);
		pos.y = 2.0 * (uv.y - 0.5);

		vec2 imagePlaneDimensions = vec2(tan(radians(FOV / 2.0)) * 2.0, (tan(radians(FOV / 2.0)) * 2.0) / ASPECT_RATIO);

		// Distance from the center to this pixel. The 'radius' of the pixel.
		float r = distance(vec2(0.0, 0.0), pos);
		// Mask off the fisheye ring.
		if (r > 1.0) {
			// Return a transparent pixel
			fragColor = vec4(0.0,0.0,0.0,0.0);
			return;
		}
		// Calculate the 3D position of the source pixel on the image plane.
		// First get the latitude and longitude of the destination pixel in the fisheye image.
		vec2 latLon = getLatLonFromFisheyeUv(pos, r);
		vec2 xyOnImagePlane;
		vec3 p;

		// Derive a 3D point on the plane which correlates with the latitude and longitude in the fisheye image.
		p = latLonToPoint(latLon);
		
		// Position of the source pixel in the source image in the range [-1,1]
		xyOnImagePlane = p.xy / (imagePlaneDimensions / 2.0);
		if (outOfBounds(xyOnImagePlane, -1.0, 1.0)) 
		{
			fragColor = vec4(0.0, 0.0, 0.0, 0.0);
			return;
		}

		// Normalize the pixel coordinates to [0,1]
		xyOnImagePlane +=  1.0; 
		xyOnImagePlane /= 2.0;

		// Return the source pixel as a vec4 with the r, g, b, and alpha values
		fragColor = texture( InputTexture, xyOnImagePlane );
	}
	)";

EquiToFisheye::EquiToFisheye() :
	maxUVLocation( -1 ),
	fieldOfViewLocation( -1 ),
	roll( 0.5f )
	//aspectRatio( 0.5f ),
	//yaw( 0.5f )
{
	// Input properties
	SetMinInputs( 1 );
	SetMaxInputs( 1 );

	// Parameters
	SetParamInfof( FFPARAM_Roll, "Field of View", FF_TYPE_STANDARD );
	//SetParamInfof( FFPARAM_Aspect_Ratio, "Aspect Ratio", FF_TYPE_STANDARD );
	//SetParamInfof( FFPARAM_Yaw, "Yaw", FF_TYPE_STANDARD );
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
	fieldOfViewLocation = shader.FindUniform( "fieldOfView" );

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

	glUniform1f( fieldOfViewLocation,
				 ( roll ));

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
	/*case FFPARAM_Aspect_Ratio:
		aspectRatio = value;
		break;
	case FFPARAM_Yaw:
		yaw = value;
		break;*/
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
	/*case FFPARAM_Aspect_Ratio:
		return aspectRatio;
	case FFPARAM_Yaw:
		return yaw;*/
	case FFPARAM_Roll:
		return roll;

	default:
		return 0.0f;
	}
}
