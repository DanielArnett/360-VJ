#include "FlatToFisheye.h"
#include <FFGL.h>
#include <FFGLLib.h>
#include "../../ffgl/source/lib/ffgl/utilities/utilities.h"

#define FFPARAM_FOV ( 0 )
//#define FFPARAM_Aspect_Ratio ( 1 )
//#define FFPARAM_Yaw ( 2 )

static CFFGLPluginInfo PluginInfo(
	FlatToFisheye::CreateInstance,// Create method
	"FTFV",                       // Plugin unique ID of maximum length 4.
	"Flat to Fisheye",			  // Plugin name
	1,						   	  // API major version number 													
	500,						  // API minor version number
	1,							  // Plugin major version number
	000,						  // Plugin minor version number
	FF_EFFECT,                    // Plugin type
	"Wubba Lubba Dub Dub", // Plugin description
	"Written by Daniel Arnett, go to https://github.com/DanielArnett/360-VJ/releases for more detail."      // About
);

static const std::string vertexShaderCode = STRINGIFY(
	void main()
{
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_FrontColor = gl_Color;
}
);

static const std::string fragmentShaderCode = STRINGIFY(

//	float PI = 3.14159265359;
//	uniform sampler2D InputTexture;
//	uniform float fieldOfView;
//
//	vec2 getLatLonFromFisheyeUv(vec2 uv, float r)
//	{
//		vec2 latLon;
//		latLon.x = (1.0 - r)*(PI / 2.0);
//		// Calculate longitude
//		if (r == 0.0) {
//			latLon.y = 0.0;
//		}
//		else if (uv.x < 0.0) {
//			latLon.y = PI - asin(uv.y / r);
//		}
//		else if (uv.x >= 0.0) {
//			latLon.y = asin(uv.y / r);
//		}
//		return latLon;
//	}
//	// Convert latitude, longitude into a 3d point on the unit-sphere.
//	vec3 latLonToPoint(vec2 latLon)
//	{
//		vec3 point;
//		// 3D position of the destination pixel on the unit sphere.
//		point.x = cos(latLon.x) * cos(latLon.y);
//		point.y = cos(latLon.x) * sin(latLon.y);
//		point.z = sin(latLon.x);
//		// Get phi of this point, see polar coordinate system for more details.
//		float phi = atan(point.y, point.x);
//		// With phi, calculate the point on the image plane that is also at the angle phi
//		point.x = cos(phi) * tan(PI / 2.0 - latLon.x);
//		point.y = sin(phi) * tan(PI / 2.0 - latLon.x);
//		point.z = 1.0;
//		return point;
//	}
//	bool outOfBounds(vec2 xy, float lower, float upper)
//	{
//		vec2 lowerBound = vec2(lower, lower);
//		vec2 upperBound = vec2(upper, upper);
//		return (any(lessThan(xy, lowerBound)) || any(greaterThan(xy, upperBound)));
//	}
	void main()
	{
		vec2 uv = gl_TexCoord[0].xy;
        gl_FragColor = texture2D( InputTexture, uv);
//		ivec2 textureSize2d = textureSize(InputTexture,0);
//		vec2 textureSize = textureSize2d.xy;
//		float ASPECT_RATIO = textureSize.x / textureSize.y;
//		float fovInput = fieldOfView; // 0.0 to 1.0
//		float FOV = 180.0 * fovInput; // 60 degrees to 180 degrees
//
//		// Position of the destination pixel in xy coordinates in the range [0,1]
//		vec2 pos;
//		pos.x = 2.0 * (uv.x - 0.5);
//		pos.y = 2.0 * (uv.y - 0.5);
//
//		vec2 imagePlaneDimensions = vec2(tan(radians(FOV / 2.0)) * 2.0, (tan(radians(FOV / 2.0)) * 2.0) / ASPECT_RATIO);
//
//		// Distance from the center to this pixel. The 'radius' of the pixel.
//		float r = distance(vec2(0.0, 0.0), pos);
//		// Mask off the fisheye ring.
//		if (r > 1.0) {
//			// Return a transparent pixel
//			gl_FragColor = vec4(0.0,0.0,0.0,0.0);
//			return;
//		}
//		// Calculate the 3D position of the source pixel on the image plane.
//		// First get the latitude and longitude of the destination pixel in the fisheye image.
//		vec2 latLon = getLatLonFromFisheyeUv(pos, r);
//		vec2 xyOnImagePlane;
//		vec3 p;
//
//		// Derive a 3D point on the plane which correlates with the latitude and longitude in the fisheye image.
//		p = latLonToPoint(latLon);
//		
//		// Position of the source pixel in the source image in the range [-1,1]
//		xyOnImagePlane = p.xy / (imagePlaneDimensions / 2.0);
//		if (outOfBounds(xyOnImagePlane, -1.0, 1.0)) 
//		{
//			gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
//			return;
//		}
//
//		// Normalize the pixel coordinates to [0,1]
//		xyOnImagePlane +=  1.0; 
//		xyOnImagePlane /= 2.0;
//
//		// Return the source pixel as a vec4 with the r, g, b, and alpha values
//		gl_FragColor = texture2D( InputTexture, xyOnImagePlane );
	}
	);

FlatToFisheye::FlatToFisheye()
	:CFreeFrameGLPlugin(),
	m_initResources(1),
	m_inputTextureLocation(-1),
	fieldOfViewLocation(-1),
	fieldOfView(0.5f)
{
	// Input properties
	SetMinInputs(1);
	SetMaxInputs(1);

	// Parameters
	SetParamInfo( FFPARAM_FOV, "Field of View", FF_TYPE_STANDARD, 0.5f );
	fieldOfView = 0.5f;
}

FlatToFisheye::~FlatToFisheye()
{
}

FFResult FlatToFisheye::InitGL(const FFGLViewportStruct* vp)
{
	m_initResources = 0;
	//initialize gl shader
	m_shader.Compile(vertexShaderCode, fragmentShaderCode);

	//activate our shader
	m_shader.BindShader();

	//to assign values to parameters in the shader, we have to lookup
	//the "location" of each value.. then call one of the glUniform* methods
	//to assign a value
	m_inputTextureLocation = m_shader.FindUniform("InputTexture");

	//We need to know these uniform locations because we need to set their value each frame.
	maxUVLocation = m_shader.FindUniform("MaxUV");
	fieldOfViewLocation = m_shader.FindUniform("fieldOfView");

	//the 0 means that the 'inputTexture' in
	//the shader will use the texture bound to GL texture unit 0
	glUniform1i(m_inputTextureLocation, 0);

	m_shader.UnbindShader();

	return FF_SUCCESS;
}

FFResult FlatToFisheye::DeInitGL()
{
	m_shader.FreeGLResources();
	maxUVLocation = -1;
	fieldOfViewLocation = -1;

	return FF_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//  Methods
////////////////////////////////////////////////////////////////////////////////////////////////////



FFResult FlatToFisheye::ProcessOpenGL(ProcessOpenGLStruct *pGL)
{
	if (pGL->numInputTextures < 1)
		return FF_FAIL;

	if (pGL->inputTextures[0] == NULL)
		return FF_FAIL;

	//activate our shader
	m_shader.BindShader();

	FFGLTextureStruct &Texture = *(pGL->inputTextures[0]);

	//get the max s,t that correspond to the 
	//width,height of the used portion of the allocated texture space
	FFGLTexCoords maxCoords = GetMaxGLTexCoords(Texture);

	//assign the UV
	glUniform2f(maxUVLocation, maxCoords.s, maxCoords.t);

	glUniform1f(fieldOfViewLocation,
			fieldOfView);


	//activate texture unit 1 and bind the input texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, Texture.Handle);

	//draw the quad that will be painted by the shader/textures
	//note that we are sending texture coordinates to texture unit 1..
	//the vertex shader and fragment shader refer to this when querying for
	//texture coordinates of the inputTexture
	glBegin(GL_QUADS);

	//lower left
	glMultiTexCoord2f(GL_TEXTURE0, 0, 0);
	glVertex2f(-1, -1);

	//upper left
	glMultiTexCoord2f(GL_TEXTURE0, 0, maxCoords.t);
	glVertex2f(-1, 1);

	//upper right
	glMultiTexCoord2f(GL_TEXTURE0, maxCoords.s, maxCoords.t);
	glVertex2f(1, 1);

	//lower right
	glMultiTexCoord2f(GL_TEXTURE0, maxCoords.s, 0);
	glVertex2f(1, -1);
	glEnd();

	//unbind the input texture
	glBindTexture(GL_TEXTURE_2D, 0);


	//unbind the shader
	m_shader.UnbindShader();

	return FF_SUCCESS;
}
FFResult FlatToFisheye::SetFloatParameter(unsigned int dwIndex, float value)
{
	switch (dwIndex)
	{
	case FFPARAM_FOV:
		fieldOfView = value;
		break;
	default:
		return FF_FAIL;
	}

	return FF_SUCCESS;
}

float FlatToFisheye::GetFloatParameter(unsigned int dwIndex)
{
	switch (dwIndex)
	{
	case FFPARAM_FOV:
		return fieldOfView;

	default:
		return 0.0f;
	}
}
