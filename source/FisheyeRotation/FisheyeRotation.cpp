#include "FisheyeRotation.h"
#include <FFGL.h>
#include <FFGLLib.h>
#include "../../ffgl/source/lib/ffgl/utilities/utilities.h"



#define FFPARAM_FOV ( 0 )
#define FFPARAM_Pitch ( 1 )
#define FFPARAM_Yaw ( 2 )

static CFFGLPluginInfo PluginInfo(
	FisheyeRotation::CreateInstance,// Create method
	"FROT",                       // Plugin unique ID of maximum length 4.
	"Fisheye Rotation",		 	  // Plugin name
	1,						   	  // API major version number 													
	500,						  // API minor version number
	1,							  // Plugin major version number
	000,						  // Plugin minor version number
	FF_EFFECT,                    // Plugin type
	"Rotate Fisheye videos",	  // Plugin description
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
float PI = 3.14159265359;
uniform sampler2D InputTexture;
uniform vec3 InputRotation;

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
	vec2 uv = gl_TexCoord[0].xy;
	// Position of the destination pixel in xy coordinates in the range [-1,1]
	vec2 pos = 2.0 * uv - 1.0;

	// Radius of the pixel from the center
	float r = distance(vec2(0.0,0.0), pos);
	// Don't bother with pixels outside of the fisheye circle
	if (1.0 < r) {
		// Return a transparent pixel
		gl_FragColor = vec4(0.0,0.0,0.0,0.0);
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
		gl_FragColor = vec4(0.0,0.0,0.0,0.0);
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

	gl_FragColor = texture2D(InputTexture, sourcePixel);
}
);

FisheyeRotation::FisheyeRotation()
	:CFreeFrameGLPlugin(),
	m_initResources(1),
	m_inputTextureLocation(-1),
	fieldOfViewLocation(-1),
	pitch(0.5f),
	yaw(0.5f),
	roll(0.5f)
{
	// Input properties
	SetMinInputs(1);
	SetMaxInputs(1);

	// Parameters
	SetParamInfo(FFPARAM_FOV, "Roll", FF_TYPE_STANDARD, 0.5f);
	roll = 0.5f;
	SetParamInfo(FFPARAM_Pitch, "Pitch", FF_TYPE_STANDARD, 0.5f);
	pitch = 0.5f;
	SetParamInfo(FFPARAM_Yaw, "Yaw", FF_TYPE_STANDARD, 0.5f);
	yaw = 0.5f;
}
FisheyeRotation::~FisheyeRotation()
{
}

FFResult FisheyeRotation::InitGL( const FFGLViewportStruct* vp )
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
	fieldOfViewLocation = m_shader.FindUniform("InputRotation");

	//the 0 means that the 'inputTexture' in
	//the shader will use the texture bound to GL texture unit 0
	glUniform1i(m_inputTextureLocation, 0);

	m_shader.UnbindShader();

	return FF_SUCCESS;
}
FFResult FisheyeRotation::ProcessOpenGL(ProcessOpenGLStruct *pGL)
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
	glUniform2f( maxUVLocation, maxCoords.s, maxCoords.t );

	glUniform3f( fieldOfViewLocation,
				 -1.0f + ( pitch * 2.0f ),
				 -1.0f + ( yaw * 2.0f ),
				 -1.0f + ( roll * 2.0f ) );


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
FFResult FisheyeRotation::DeInitGL()
{
	m_shader.FreeGLResources();
	maxUVLocation = -1;
	fieldOfViewLocation = -1;

	return FF_SUCCESS;
}

FFResult FisheyeRotation::SetFloatParameter(unsigned int dwIndex, float value)
{
	switch (dwIndex)
	{
	case FFPARAM_Pitch:
		pitch = value;
		break;
	case FFPARAM_Yaw:
		yaw = value;
		break;
	case FFPARAM_FOV:
		roll = value;
		break;
	default:
		return FF_FAIL;
	}

	return FF_SUCCESS;
}

float FisheyeRotation::GetFloatParameter(unsigned int dwIndex)
{
	switch (dwIndex)
	{
	case FFPARAM_Pitch:
		return pitch;
	case FFPARAM_Yaw:
		return yaw;
	case FFPARAM_FOV:
		return roll;

	default:
		return 0.0f;
	}
}
