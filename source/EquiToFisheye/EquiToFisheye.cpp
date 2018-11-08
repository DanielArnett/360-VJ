#include <FFGL.h>
#include <FFGLLib.h>

#include "EquiToFisheye.h"
#include "../../lib/ffgl/utilities/utilities.h"

//#define FFPARAM_BrightnessR  (0)
//#define FFPARAM_BrightnessG	 (1)
//#define FFPARAM_BrightnessB	 (2)

////////////////////////////////////////////////////////////////////////////////////////////////////
//  Plugin information
////////////////////////////////////////////////////////////////////////////////////////////////////

static CFFGLPluginInfo PluginInfo(
	EquiToFisheye::CreateInstance,		// Create method
	"EQ2F",								// Plugin unique ID
	"360 To Fisheye",					// Plugin name
	1,						   			// API major version number 													
	500,								// API minor version number
	1,									// Plugin major version number
	000,								// Plugin minor version number
	FF_EFFECT,							// Plugin type
	"Convert 360 Videos into Fisheye Videos",	  // Plugin description
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
	uniform sampler2D InputTexture;

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
	latLon.y += PI / 2.0;
	return latLon;
}
void main()
{
	vec2 uv = gl_TexCoord[0].xy;

	vec2 pos = 2.0 * uv - 1.0;
	float r = distance(vec2(0.0, 0.0), pos);
	// Don't bother with pixels outside of the fisheye circle
	if (1.0 < r) {
		// Return a transparent pixel
		gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
		return;
	}
	vec3 p;
	vec2 sourcePixel;
	// Get the latitude and longitude for the destination pixel.
	vec2 latLon = getLatLonFromFisheyeUv(pos, r);
	p = latLonToPoint(latLon);
	p.z = p.y;
	p.y = p.z;

	// Get the lat lon for the source pixel
	latLon = pointToLatLon(p);

	sourcePixel.x = (latLon.y + PI) / (2.0 * PI);
	sourcePixel.y = (latLon.x + PI / 2.0) / PI;

	gl_FragColor = texture2D(InputTexture, sourcePixel);
}
);

EquiToFisheye::EquiToFisheye()
	:CFreeFrameGLPlugin(),
	m_initResources(1),
	m_inputTextureLocation(-1)//,
	//m_BrightnessLocation(-1)
{
	// Input properties
	SetMinInputs(1);
	SetMaxInputs(1);

	// Parameters
	/*SetParamInfo(FFPARAM_BrightnessR, "R", FF_TYPE_RED, 0.5f);
	m_BrightnessR = 0.5f;

	/*SetParamInfo(FFPARAM_BrightnessG, "G", FF_TYPE_GREEN, 0.5f);
	m_BrightnessG = 0.5f;

	SetParamInfo(FFPARAM_BrightnessB, "B", FF_TYPE_BLUE, 0.5f);
	m_BrightnessB = 0.5f;*/

}

EquiToFisheye::~EquiToFisheye()
{

}

FFResult EquiToFisheye::InitGL(const FFGLViewportStruct *vp)
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
	//m_BrightnessLocation = m_shader.FindUniform("brightness");

	//the 0 means that the 'inputTexture' in
	//the shader will use the texture bound to GL texture unit 0
	glUniform1i(m_inputTextureLocation, 0);

	m_shader.UnbindShader();

	return FF_SUCCESS;
}

FFResult EquiToFisheye::DeInitGL()
{
	m_shader.FreeGLResources();


	return FF_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//  Methods
////////////////////////////////////////////////////////////////////////////////////////////////////



FFResult EquiToFisheye::ProcessOpenGL(ProcessOpenGLStruct *pGL)
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

	//assign the Brightness
	glUniform1f(m_BrightnessLocation,
		-1.0f + (m_BrightnessR * 2.0f)/*,
		-1.0f + (m_BrightnessG * 2.0f),
		-1.0f + (m_BrightnessB * 2.0f)*/
	);


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

float EquiToFisheye::GetFloatParameter(unsigned int dwIndex)
{
	float retValue = 0.0;

	switch (dwIndex)
	{
		/*case FFPARAM_BrightnessR:
			retValue = m_BrightnessR;
			return retValue;
		/*case FFPARAM_BrightnessG:
			retValue = m_BrightnessG;
			return retValue;
		case FFPARAM_BrightnessB:
			retValue = m_BrightnessB;
			return retValue;*/
	default:
		return retValue;
	}
}

FFResult EquiToFisheye::SetFloatParameter(unsigned int dwIndex, float value)
{
	switch (dwIndex)
	{
		/*case FFPARAM_BrightnessR:
			m_BrightnessR = value;
			break;
		/*case FFPARAM_BrightnessG:
			m_BrightnessG = value;
			break;
		case FFPARAM_BrightnessB:
			m_BrightnessB = value;
			break;*/
	default:
		return FF_FAIL;
	}

	return FF_SUCCESS;
}
