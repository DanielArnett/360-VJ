#include "FlatToFisheye.h"
#include <ffgl/FFGLLib.h>
#include <ffglex/FFGLScopedShaderBinding.h>
#include <ffglex/FFGLScopedSamplerActivation.h>
#include <ffglex/FFGLScopedTextureBinding.h>
using namespace ffglex;

#define FFPARAM_Field_Of_View ( 0 )
//#define FFPARAM_Aspect_Ratio ( 1 )
//#define FFPARAM_Yaw ( 2 )

static CFFGLPluginInfo PluginInfo(
	PluginFactory< FlatToFisheye >,// Create method
	"FTFV",                       // Plugin unique ID of maximum length 4.
	" Flat to Fisheye",				 	  // Plugin name
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

		float flatImgWidth = tan(radians(FOV / 2.0)) * 2.0;
		float flatImgHeight = flatImgWidth / ASPECT_RATIO;

		// Radius from the center of the image to the source pixel
		float r = distance(vec2(0.0, 0.0), pos);
		// Mask off the fisheye ring.
		if (r > 1.0) {
			// Return a transparent pixel
			fragColor = vec4(0.0,0.0,0.0,0.0);
			return;
		}
		float latitude = (1.0 - r)*(PI / 2.0);
		float longitude;
		float phi;
		float percentX;
		float percentY;
		int u;
		int v;
		vec3 p;
		vec3 col;

		// Calculate the position of the source pixel
		if (r == 0.0) {
			longitude = 0.0;
		}
		else if (pos.x < 0.0) {
			longitude = PI - asin(pos.y / r);
		}
		else if (pos.x >= 0.0) {
			longitude = asin(pos.y / r);
		}
		if (longitude < 0.0) {
			longitude += 2.0*PI;
		}
		p.x = cos(latitude) * cos(longitude);
		p.y = cos(latitude) * sin(longitude);
		p.z = sin(latitude);
		phi = atan(p.y, p.x);
		if (phi < 0.0) {
			phi += 2.0*PI;
		}
		p.x = cos(phi) * tan(PI / 2.0 - latitude);
		p.y = sin(phi) * tan(PI / 2.0 - latitude);
		p.z = 1.0;
		// The x/y position of the pixel, each in the range [-1,1]
		percentX = p.x / (float(flatImgWidth) / 2.0);
		percentY = p.y / (float(flatImgHeight) / 2.0);
		
		// Return blank pixels outside of the projected area
		if (!(-1.0 <= percentX && percentX <= 1.0 && -1.0 <= percentY && percentY <= 1.0)) 
		{
			fragColor = vec4(0.0, 0.0, 0.0, 0.0);
			return;
		}
		// Normalize the coordinates to be in the range [0,1]
		percentX = (percentX + 1.0) / 2.0;
		percentY = (percentY + 1.0) / 2.0;
		
		if (u < 0 || int(textureSize.x) < u || v < 0 || int(textureSize.y) < v) {
			fragColor = vec4(0.0,0.0,0.0,0.0);
			return;
		}
		The position of the source pixel to map to the current pixel
		vec2 outCoord;
		outCoord.x = float(percentX);
		outCoord.y = float(percentY);

		fragColor = texture( InputTexture, outCoord );
	}
	)";

FlatToFisheye::FlatToFisheye() :
	maxUVLocation( -1 ),
	fieldOfViewLocation( -1 ),
	fieldOfView( 0.5f )
	//aspectRatio( 0.5f ),
	//yaw( 0.5f )
{
	// Input properties
	SetMinInputs( 1 );
	SetMaxInputs( 1 );

	// Parameters
	SetParamInfof( FFPARAM_Field_Of_View, "Field of View", FF_TYPE_STANDARD );
	//SetParamInfof( FFPARAM_Aspect_Ratio, "Aspect Ratio", FF_TYPE_STANDARD );
	//SetParamInfof( FFPARAM_Yaw, "Yaw", FF_TYPE_STANDARD );
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
	fieldOfViewLocation = shader.FindUniform( "fieldOfView" );

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

	glUniform1f( fieldOfViewLocation,
				 ( fieldOfView ));

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
	/*case FFPARAM_Aspect_Ratio:
		aspectRatio = value;
		break;
	case FFPARAM_Yaw:
		yaw = value;
		break;*/
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
	/*case FFPARAM_Aspect_Ratio:
		return aspectRatio;
	case FFPARAM_Yaw:
		return yaw;*/
	case FFPARAM_Field_Of_View:
		return fieldOfView;

	default:
		return 0.0f;
	}
}
