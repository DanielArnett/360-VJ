#include "FisheyeToFlat.h"
#include <ffgl/FFGLLib.h>
#include <ffglex/FFGLScopedShaderBinding.h>
#include <ffglex/FFGLScopedSamplerActivation.h>
#include <ffglex/FFGLScopedTextureBinding.h>
using namespace ffglex;

#define FFPARAM_Roll ( 0 )
//#define FFPARAM_Aspect_Ratio ( 1 )
//#define FFPARAM_Yaw ( 2 )

static CFFGLPluginInfo PluginInfo(
	PluginFactory< FisheyeToFlat >,// Create method
	"F2FE",                       // Plugin unique ID of maximum length 4.
	"Fisheye To Flat",				 	  // Plugin name
	2,                            // API major version number
	1,                            // API minor version number
	1,                            // Plugin major version number
	0,                            // Plugin minor version number
	FF_EFFECT,                    // Plugin type
	"Rectify a Fisheye Image", // Plugin description
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

	float PI = 3.14159265359;
	vec2 OUT_OF_BOUNDS = vec2(-1.0, -1.0);
	vec4 TRANSPARENT = vec4(0.0, 0.0, 0.0, 0.0);
	// Convert a 3D point on the unit sphere into latitude and longitude.
	vec2 pointToLatLon(vec3 point) 
	{
		float r = distance(vec3(0.0, 0.0, 0.0), point);
		vec2 latLon;
		latLon.y = atan(point.y, point.x);
		latLon.x = acos(point.z / r);
		return latLon;
	}
	vec3 flatImageUvToPoint(vec2 pos, float fieldOfView, float aspectRatio)
	{
		// Position of the source pixel in uv coordinates in the range [-1,1]
		pos.x = 2.0 * (pos.x - 0.5);
		pos.y = 2.0 * (pos.y - 0.5);
		vec2 imagePlaneDimensions = vec2(tan(fieldOfView / 2.0) * 2.0, (tan(fieldOfView / 2.0) * 2.0) / aspectRatio);
		
		// Position of the source pixel on the image plane
		pos *= imagePlaneDimensions;
		return vec3(pos.x, pos.y, 1.0);
	}
	vec2 latLonToFisheyeXY(vec2 latLon)
	{
		// Convert latitude and longitude into polar coordinates
		float r = latLon.x/(PI / 2.0);
		float phi = latLon.y;
		vec2 xy;
		// Get the position of the source pixel
		xy.x = r * cos(phi);
		xy.y = r * sin(phi);
		// Normalize the output pixel to be in the range [0,1]
		xy = (xy + 1.0) / 2.0;
		if (r > 1.0) {
			return OUT_OF_BOUNDS;
		}
		return xy;
	}
	void main()
	{
		ivec2 textureSize2d = textureSize(InputTexture,0);
		vec2 textureSize = textureSize2d.xy;
		float aspectRatio = textureSize.x / textureSize.y;
		vec3 point = flatImageUvToPoint(uv, radians(180.0 * fieldOfView), aspectRatio);
		vec2 latLon = pointToLatLon(point);
		vec2 xy = latLonToFisheyeXY(latLon);
		// Set the color of the destination pixel to the color of the source pixel.
		if (xy != OUT_OF_BOUNDS)
		{
			fragColor = texture(InputTexture, xy);
		}
		else
		{
			fragColor = TRANSPARENT;
		}
	}
	)";

FisheyeToFlat::FisheyeToFlat() :
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
FisheyeToFlat::~FisheyeToFlat()
{
}

FFResult FisheyeToFlat::InitGL( const FFGLViewportStruct* vp )
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
	return CFreeFrameGLPlugin::InitGL( vp );
}
FFResult FisheyeToFlat::ProcessOpenGL( ProcessOpenGLStruct* pGL )
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
FFResult FisheyeToFlat::DeInitGL()
{
	shader.FreeGLResources();
	quad.Release();
	maxUVLocation = -1;
	fieldOfViewLocation = -1;

	return FF_SUCCESS;
}

FFResult FisheyeToFlat::SetFloatParameter( unsigned int dwIndex, float value )
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

float FisheyeToFlat::GetFloatParameter( unsigned int dwIndex )
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
