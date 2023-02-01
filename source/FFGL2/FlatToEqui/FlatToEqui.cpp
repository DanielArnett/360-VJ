#include "FlatToEqui.h"
#include <ffgl/FFGLLib.h>
using namespace ffglex;

#define FFPARAM_Roll   ( 0 )
#define FFPARAM_Pitch  ( 1 )
#define FFPARAM_Yaw    ( 2 )
#define FFPARAM_FOV  ( 3 )
#define FFPARAM_Aspect  ( 4 )

static CFFGLPluginInfo PluginInfo(
	PluginFactory< FlatToEqui >,// Create method
	"FL2E",                       // Plugin unique ID of maximum length 4.
	"Flat To 360",				 	  // Plugin name
	2,                            // API major version number
	1,                            // API minor version number
	1,                            // Plugin major version number
	0,                            // Plugin minor version number
	FF_EFFECT,                    // Plugin type
	"Convert flat videos to equirectangular videos.",		  // Plugin description
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
uniform float fieldOfView;
uniform float aspectRatio;
in vec2 uv;
out vec4 fragColor;
vec4 GREEN = vec4(0.0, 1.0, 0.0, 1.0);
bool isTransparent = false; // A global flag indicating if the pixel should just set to transparent and return immediately. 

vec2 SET_TO_TRANSPARENT = vec2(-1.0, -1.0);
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
	return Rx(th.x) * Ry(th.y) * Rz(th.z) * p;
}

// Convert x, y pixel coordinates from an Equirectangular image into latitude/longitude coordinates.
vec2 uvToLatLon(vec2 localUv)
{
	return vec2(localUv.y * PI - PI/2.0,
				localUv.x * 2.0*PI - PI);
}

// Convert latitude, longitude into a 3d point on the unit-sphere.
// In more mathy terms we're converting from  "Spherical Coordinates" to "Cartesian Coordinates"
vec3 latLonToPoint(vec2 latLon)
{
    float lat = latLon.x;
    float lon = latLon.y;
    vec3 point;
    point.x = cos(lat) * sin(lon);
    point.y = cos(lat) * cos(lon);
    point.z = sin(lat);
    return point;
}

// Convert a 3D point on the unit sphere into latitude and longitude.
// In more mathy terms we're converting from "Cartesian Coordinates" to "Spherical Coordinates"
vec2 pointToLatLon(vec3 point)
{
    float r = distance(vec3(0.0, 0.0, 0.0), point);
    vec2 latLon;
    latLon.x = asin(point.z / r);
    latLon.y = atan(point.x, point.y);
    return latLon;
}

// Convert latitude, longitude to x, y pixel coordinates on an equirectangular image.
vec2 latLonToEquiUv(vec2 latLon)
{
    vec2 local_uv;
    local_uv.x = (latLon.y + PI)/(2.0*PI);
    local_uv.y = (latLon.x + PI/2.0)/PI;
    // Set to transparent if out of bounds
    //if (local_uv.x < -1.0 || local_uv.y < -1.0 || local_uv.x > 1.0 || local_uv.y > 1.0) {
    //    // Return a isTransparent pixel
    //    isTransparent = true;
    //    return SET_TO_TRANSPARENT;
    //}
    return local_uv;
}
bool outOfFlatBounds(vec2 xy, float lower, float upper)
{
	vec2 lowerBound = vec2(lower, lower);
	vec2 upperBound = vec2(upper, upper);
	return (any(lessThan(xy, lowerBound)) || any(greaterThan(xy, upperBound)));
}
// Convert latitude, longitude into a 3d point on the unit-sphere.
vec3 flatLatLonToPoint(vec2 latLon)
{
	vec3 point = latLonToPoint(latLon);
	// Get phi of this point, see polar coordinate system for more details.
	float phi = atan(point.x, -point.y);
	// With phi, calculate the point on the image plane that is also at the angle phi
	point.x = sin(phi) * tan(PI / 2.0 - latLon.x);
	point.y = cos(phi) * tan(PI / 2.0 - latLon.x);
	point.z = 1.0;
	return point;
}
vec2 latLonToFlatUv(vec2 latLon, float fovInput)
{
	vec3 point = rotatePoint(latLonToPoint(latLon), vec3(-PI/2.0, 0.0, 0.0));
	latLon = pointToLatLon(point);
	//float aspectRatio = float(width)/float(height);
	vec2 xyOnImagePlane;
	vec3 p;
	if (latLon.x < 0.0) 
	{
		isTransparent = true;
		return SET_TO_TRANSPARENT;
	}
	// Derive a 3D point on the plane which correlates with the latitude and longitude in the fisheye image.
	p = flatLatLonToPoint(latLon);
	p.x /= aspectRatio;
	// Control the scale with the user's fov input parameter.
	p.xy /= fovInput;
	// Position of the source pixel in the source image in the range [-1,1]
	xyOnImagePlane = p.xy / 2.0 + 0.5;
	if (outOfFlatBounds(xyOnImagePlane, 0.0, 1.0)) 
	{
		isTransparent = true;
		return SET_TO_TRANSPARENT;
	}
	return xyOnImagePlane;
	//return vec2(1.0,1.0);
}

void main()
{	
	//if (uv.x > 0.5) {
	//	fragColor = texture( InputTexture, uv );
	//}
	//else {
	//	fragColor = vec4(1.0, 0.0, 0.0, 1.0);
	//}
	//return;
	// Latitude and Longitude of the destination pixel (uv)
	vec2 latLon = uvToLatLon(uv);
	// Create a point on the unit-sphere from the latitude and longitude
		// X increases from left to right [-1 to 1]
		// Y increases from bottom to top [-1 to 1]
		// Z increases from back to front [-1 to 1]
	vec3 point = latLonToPoint(latLon);
	// Rotate the point based on the user input in radians
	point = rotatePoint(point, InputRotation.xyz * PI);
	// Convert back to latitude and longitude
	latLon = pointToLatLon(point);
	// Convert back to the normalized pixel coordinate

	vec2 sourcePixel = latLonToFlatUv(latLon, fieldOfView*2.0*PI);
	if (isTransparent) {
		fragColor = vec4(0.0, 0.0, 1.0, 1.0);
	}
	//vec2 sourcePixel = uv;
	/*if (sourcePixel.x >= 1.0 || sourcePixel.x <= 0.0 || sourcePixel.y >= 1.0 || sourcePixel.x <= 0.0) {
		fragColor = vec4(0.0, 1.0, 0.0, 1.0);
		return;
	}*/	
	//vec2 croppedUv = sourcePixel;
	//croppedUv = vec2(1.0, 1.0) - croppedUv;

	// Set the color of the destination pixel to the color of the source pixel
	fragColor = texture( InputTexture, sourcePixel );
	/*fragColor = GREEN;
	return;*/
}
)";

FlatToEqui::FlatToEqui() :
	maxUVLocation( -1 ),
	inputRotationLocation( -1 ),
	pitch( 0.5f ),
	yaw( 0.5f ),
	roll( 0.5f ),
	fieldOfViewLocation(0.5f),
	aspectRatioLocation((16.0f/9.0f)/2.0)
{
	// Input properties
	SetMinInputs( 1 );
	SetMaxInputs( 1 );

	// Parameters
	SetParamInfof( FFPARAM_Roll, "Yaw", FF_TYPE_STANDARD );
	SetParamInfof( FFPARAM_Pitch, "Pitch", FF_TYPE_STANDARD );
	SetParamInfof(FFPARAM_Yaw, "Roll", FF_TYPE_STANDARD);
	SetParamInfof(FFPARAM_FOV, "Field of View", FF_TYPE_STANDARD);
	SetParamInfof(FFPARAM_Aspect, "Aspect Ratio", FF_TYPE_STANDARD);
}
FlatToEqui::~FlatToEqui()
{
}

FFResult FlatToEqui::InitGL( const FFGLViewportStruct* vp )
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
	glUniform1i(shader.FindUniform("InputTexture"), 0);

	//We need to know these uniform locations because we need to set their value each frame.
	maxUVLocation = shader.FindUniform( "MaxUV" );
	inputRotationLocation = shader.FindUniform("InputRotation");
	fieldOfViewLocation = shader.FindUniform("fieldOfView");
	aspectRatioLocation = shader.FindUniform("aspectRatio");

	//Use base-class init as success result so that it retains the viewport.
	return CFFGLPlugin::InitGL( vp );
}
FFResult FlatToEqui::ProcessOpenGL( ProcessOpenGLStruct* pGL )
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

	glUniform3f( inputRotationLocation,
				 -1.0f + ( pitch * 2.0f ),
				 -1.0f + ( yaw * 2.0f ),
				 -1.0f + ( roll * 2.0f ) );
	glUniform1f(fieldOfViewLocation, fieldOfView * 3.14159274101257324);
	glUniform1f(aspectRatioLocation, aspectRatio * 2.0);

	//The shader's sampler is always bound to sampler index 0 so that's where we need to bind the texture.
	//Again, we're using the scoped bindings to help us keep the context in a default state.
	ScopedSamplerActivation activateSampler( 0 );
	Scoped2DTextureBinding textureBinding( Texture.Handle );

	quad.Draw();

	return FF_SUCCESS;
}
FFResult FlatToEqui::DeInitGL()
{
	shader.FreeGLResources();
	quad.Release();
	maxUVLocation = -1;
	inputRotationLocation = -1;

	return FF_SUCCESS;
}

FFResult FlatToEqui::SetFloatParameter( unsigned int dwIndex, float value )
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
	case FFPARAM_FOV:
		fieldOfView = value;
		break;
	case FFPARAM_Aspect:
		aspectRatio = value;
		break;
	default:
		return FF_FAIL;
	}

	return FF_SUCCESS;
}

float FlatToEqui::GetFloatParameter( unsigned int dwIndex )
{
	switch( dwIndex )
	{
	case FFPARAM_Pitch:
		return pitch;
	case FFPARAM_Yaw:
		return yaw;
	case FFPARAM_Roll:
		return roll;
	case FFPARAM_FOV:
		return fieldOfView;
	case FFPARAM_Aspect:
		return aspectRatio;

	default:
		return 0.0f;
	}
}
char* FlatToEqui::GetParameterDisplay(unsigned int index)
{
	/**
	 * We're not returning ownership over the string we return, so we have to somehow guarantee that
	 * the lifetime of the returned string encompasses the usage of that string by the host. Having this static
	 * buffer here keeps previously returned display string alive until this function is called again.
	 * This happens to be long enough for the hosts we know about.
	 */
	static char displayValueBuffer[15];
	memset(displayValueBuffer, 0, sizeof(displayValueBuffer));
	switch (index)
	{
	case FFPARAM_Pitch:
		sprintf(displayValueBuffer, "%f", pitch * 360.0f - 180.0f);
		return displayValueBuffer;
	case FFPARAM_Yaw:
		sprintf(displayValueBuffer, "%f", yaw * 360.0f - 180.0f);
		return displayValueBuffer;
	case FFPARAM_Roll:
		sprintf(displayValueBuffer, "%f", roll * 360.0f - 180.0f);
		return displayValueBuffer;
	case FFPARAM_FOV:
		sprintf(displayValueBuffer, "%f", fieldOfView * 180.0f);
		return displayValueBuffer;
	case FFPARAM_Aspect:
		sprintf(displayValueBuffer, "%f", aspectRatio * 2.0);
		return displayValueBuffer;
	default:
		return CFFGLPlugin::GetParameterDisplay(index);
	}
}
