#include <FFGL.h>
#include <FFGLLib.h>

#include "EquiRotation.h"
#include "../../lib/ffgl/utilities/utilities.h"


#define FFPARAM_Roll ( 0 )
#define FFPARAM_Pitch ( 1 )
#define FFPARAM_Yaw ( 2 )

static CFFGLPluginInfo PluginInfo(
	EquiRotation::CreateInstance,		// Create method
	"360V",                       // Plugin unique ID of maximum length 4.
	"360 VJ",				 	  // Plugin name
	1,						   	  // API major version number 													
	500,						  // API minor version number
	1,							  // Plugin major version number
	000,						  // Plugin minor version number
	FF_EFFECT,                    // Plugin type
	"Rotate 360 videos",		  // Plugin description
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

// Convert latitude, longitude to x, y pixel coordinates on an equirectangular image.
vec2 latLonToUv(vec2 latLon)
{	
	vec2 uv;
	uv.x = (latLon.y + PI)/(2.0*PI);
	uv.y = (latLon.x + PI/2.0)/PI;
	return uv;
}

void main()
{
	vec2 uv = gl_TexCoord[0].xy;
	// Latitude and Longitude of the destination pixel (uv)
	vec2 latLon = uvToLatLon(uv);
	// Create a point on the unit-sphere from the latitude and longitude
		// X increases from left to right [-1 to 1]
		// Y increases from bottom to top [-1 to 1]
		// Z increases from back to front [-1 to 1]
	vec3 point = latLonToPoint(latLon);
	// Rotate the point based on the user input in radians
	point = rotatePoint(point, InputRotation.rgb * PI);
	// Convert back to latitude and longitude
	latLon = pointToLatLon(point);
	// Convert back to the normalized pixel coordinate
	vec2 sourcePixel = latLonToUv(latLon);
	// Set the color of the destination pixel to the color of the source pixel

	gl_FragColor = texture2D(InputTexture, sourcePixel);
}
);

EquiRotation::EquiRotation()
	:CFreeFrameGLPlugin(),
	m_initResources(1),
	m_inputTextureLocation(-1),
	fieldOfViewLocation( -1 ),
	pitch( 0.5f ),
	yaw( 0.5f ),
	roll( 0.5f )
{
	// Input properties
	SetMinInputs( 1 );
	SetMaxInputs( 1 );

	// Parameters
	SetParamInfo( FFPARAM_Roll, "Roll", FF_TYPE_STANDARD, 0.5f);
	roll = 0.5f;
	SetParamInfo( FFPARAM_Pitch, "Pitch", FF_TYPE_STANDARD, 0.5f);
	pitch = 0.5f;
	SetParamInfo( FFPARAM_Yaw, "Yaw", FF_TYPE_STANDARD, 0.5f);
	yaw = 0.5f;
}
EquiRotation::~EquiRotation()
{
}

FFResult EquiRotation::InitGL( const FFGLViewportStruct* vp )
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
	maxUVLocation = m_shader.FindUniform( "MaxUV" );
	fieldOfViewLocation = m_shader.FindUniform( "InputRotation" );

	//the 0 means that the 'inputTexture' in
	//the shader will use the texture bound to GL texture unit 0
	glUniform1i(m_inputTextureLocation, 0);

	m_shader.UnbindShader();

	return FF_SUCCESS;
}

FFResult EquiRotation::DeInitGL()
{
	m_shader.FreeGLResources();


	return FF_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//  Methods
////////////////////////////////////////////////////////////////////////////////////////////////////



FFResult EquiRotation::ProcessOpenGL(ProcessOpenGLStruct *pGL)
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

FFResult EquiRotation::SetFloatParameter( unsigned int dwIndex, float value )
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

float EquiRotation::GetFloatParameter( unsigned int dwIndex )
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