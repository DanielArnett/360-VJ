R"(
#version 410 core
uniform sampler2D InputTexture;
uniform vec3 Rotation;

in vec2 uv;
uniform vec2 MaxUV;
out vec4 fragColor;
uniform int inputProjection, outputProjection, width, height;
uniform float fovOut, fovIn;
//precision highp float;
vec4 TRANSPARENT_PIXEL = vec4( 0.0, 0.0, 0.0, 0.0 );
float PI = 3.14159265359;
const int EQUI          = 0;
const int FISHEYE       = 1;
const int FLAT          = 2;
const int CUBEMAP       = 3;
const int GRIDLINES_OFF = 0;
const int GRIDLINES_ON  = 1;
vec2 SET_TO_TRANSPARENT = vec2( -1.0, -1.0 );
bool isTransparent      = false;// A global flag indicating if the pixel should just set to transparent and return immediately.
// uniform vec3 InputRotation;
// A transformation matrix rotating about the x axis by th degrees.
mat3 Rx( float th )
{
	return mat3( 1, 0, 0, 0, cos( th ), -sin( th ), 0, sin( th ), cos( th ) );
}
// A transformation matrix rotating about the y axis by th degrees.
mat3 Ry( float th )
{
	return mat3( cos( th ), 0, sin( th ), 0, 1, 0, -sin( th ), 0, cos( th ) );
}
// A transformation matrix rotating about the z axis by th degrees.
mat3 Rz( float th )
{
	return mat3( cos( th ), -sin( th ), 0, sin( th ), cos( th ), 0, 0, 0, 1 );
}
// Rotate a point vector by th.x then th.y then th.z, and return the rotated point.
vec3 rotatePoint( vec3 p, vec3 th )
{
	return Rx( th.x ) * Ry( th.y ) * Rz( th.z ) * p;
}
// Convert a 3D point on the unit sphere into latitude and longitude.
// In more mathy terms we're converting from "Cartesian Coordinates" to "Spherical Coordinates"
vec2 pointToLatLon( vec3 point )
{
	float r = distance( vec3( 0.0, 0.0, 0.0 ), point );
	vec2 latLon;
	latLon.x = asin( point.z / r );
	latLon.y = atan( point.x, point.y );
	return latLon;
}
// Convert latitude, longitude into a 3d point on the unit-sphere.
// In more mathy terms we're converting from  "Spherical Coordinates" to "Cartesian Coordinates"
vec3 latLonToPoint( vec2 latLon )
{
	float lat = latLon.x;
	float lon = latLon.y;
	vec3 point;
	point.x = cos( lat ) * sin( lon );
	point.y = cos( lat ) * cos( lon );
	point.z = sin( lat );
	return point;
}
// Convert pixel coordinates from an Equirectangular image into latitude/longitude coordinates.
vec2 equiUvToLatLon( vec2 local_uv )
{
	return vec2( local_uv.y * PI - PI / 2.0,
				 local_uv.x * 2.0 * PI - PI );
}

// Convert latitude, longitude to x, y pixel coordinates on an equirectangular image.
vec2 latLonToEquiUv( vec2 latLon )
{
	vec2 local_uv;
	local_uv.x = ( latLon.y + PI ) / ( 2.0 * PI );
	local_uv.y = ( latLon.x + PI / 2.0 ) / PI;
	// Set to transparent if out of bounds
	if( local_uv.x < -1.0 || local_uv.y < -1.0 || local_uv.x > 1.0 || local_uv.y > 1.0 )
	{
		// Return a isTransparent pixel
		isTransparent = true;
		return SET_TO_TRANSPARENT;
	}
	return local_uv;
}

// Convert  pixel coordinates from an Fisheye image into latitude/longitude coordinates.
vec2 fisheyeUvToLatLon(vec2 local_uv, float fovOutput) {
	vec2 pos = 2.0 * local_uv - 1.0;
	// The distance from the source pixel to the center of the image
	float r = distance(vec2(0.0,0.0),pos.xy);
	// Don't bother with pixels outside of the fisheye circle
	if (1.0 < r) {
		isTransparent = true;
		return SET_TO_TRANSPARENT;
	}
	float theta = atan(r, 1.0);
	r = tan(theta/fovOutput);
	vec2 latLon;
	latLon.x = (1.0 - r) * (PI/2.0);
	// Calculate longitude
	latLon.y = PI + atan(-pos.x, pos.y);
      
	if (latLon.y < 0.0) {
		latLon.y += 2.0*PI;
	}
	vec3 point = latLonToPoint(latLon);
	point = rotatePoint(point, vec3(PI/2.0, 0.0, 0.0));
	latLon = pointToLatLon(point);
	return latLon;
}

// Convert latitude, longitude to x, y pixel coordinates on the source fisheye image.
vec2 pointToFisheyeUv( vec3 point, float fovIn )
{
	point = rotatePoint( point, vec3( -PI / 2.0, 0.0, 0.0 ) );
	// Phi and theta are flipped depending on where you read about them.
	float theta = atan( distance( vec2( 0.0, 0.0 ), point.xy ), point.z );
	// The distance from the source pixel to the center of the image
	float r = ( 2.0 / PI ) * ( theta / fovIn );

	// phi is the angle of r on the unit circle. See polar coordinates for more details
	float phi = atan( -point.y, point.x );
	// Get the position of the source pixel
	vec2 sourcePixel;
	sourcePixel.x = r * cos( phi );
	sourcePixel.y = r * sin( phi );
	// Normalize the output pixel to be in the range [0,1]
	sourcePixel += 1.0;
	sourcePixel /= 2.0;
	// Don't bother with source pixels outside of the fisheye circle
	if( 1.0 < r || sourcePixel.x < 0.0 || sourcePixel.y < 0.0 || sourcePixel.x > 1.0 || sourcePixel.y > 1.0 )
	{
		// Return a isTransparent pixel
		isTransparent = true;
		return SET_TO_TRANSPARENT;
	}
	return sourcePixel;
}

bool outOfFlatBounds( vec2 xy, float lower, float upper )
{
	vec2 lowerBound = vec2( lower, lower );
	vec2 upperBound = vec2( upper, upper );
	return ( any( lessThan( xy, lowerBound ) ) || any( greaterThan( xy, upperBound ) ) );
}

vec2 flatImageUvToLatLon( vec2 local_uv, float fovOutput )
{
	// Position of the source pixel in uv coordinates in the range [-1,1]
	vec2 pos          = 2.0 * local_uv - 1.0;
	float aspectRatio = float( width ) / float( height );
	vec3 point        = vec3( pos.x * aspectRatio, 1.0 / fovOutput, pos.y );
	return pointToLatLon( point );
}

// Convert latitude, longitude into a 3d point on the unit-sphere.
vec3 flatLatLonToPoint( vec2 latLon )
{
	vec3 point = latLonToPoint( latLon );
	// Get phi of this point, see polar coordinate system for more details.
	float phi = atan( point.x, -point.y );
	// With phi, calculate the point on the image plane that is also at the angle phi
	point.x = sin( phi ) * tan( PI / 2.0 - latLon.x );
	point.y = cos( phi ) * tan( PI / 2.0 - latLon.x );
	point.z = 1.0;
	return point;
}

vec2 latLonToFlatUv( vec2 latLon, float fovInput )
{
	vec3 point        = rotatePoint( latLonToPoint( latLon ), vec3( -PI / 2.0, 0.0, 0.0 ) );
	latLon            = pointToLatLon( point );
	float aspectRatio = float( width ) / float( height );
	vec2 xyOnImagePlane;
	vec3 p;
	if( latLon.x < 0.0 )
	{
		isTransparent = true;
		return SET_TO_TRANSPARENT;
	}
	// Derive a 3D point on the plane which correlates with the latitude and longitude in the fisheye image.
	p = flatLatLonToPoint( latLon );
	p.x /= aspectRatio;
	// Control the scale with the user's fov input parameter.
	p.xy *= fovInput;
	// Position of the source pixel in the source image in the range [-1,1]
	xyOnImagePlane = p.xy / 2.0 + 0.5;
	if( outOfFlatBounds( xyOnImagePlane, 0.0, 1.0 ) )
	{
		isTransparent = true;
		return SET_TO_TRANSPARENT;
	}
	return xyOnImagePlane;
}

void main()
{
	// Latitude and Longitude of the destination pixel (uv)
	vec2 latLon;
	if( outputProjection == EQUI )
	{
		latLon = equiUvToLatLon( uv );
	}
	else if( outputProjection == FISHEYE )
	{
		latLon = fisheyeUvToLatLon( uv, fovOut );
	}

	else if( outputProjection == FLAT ) {
		latLon = flatImageUvToLatLon( uv, fovOut );
	}
	if( latLon == SET_TO_TRANSPARENT )
	{
		fragColor = TRANSPARENT_PIXEL;
		return;
	}
	// Create a point on the unit-sphere from the latitude and longitude
		// X increases from left to right [-1 to 1]
		// Y increases from bottom to top [-1 to 1]
		// Z increases from back to front [-1 to 1]
	vec3 point = latLonToPoint(latLon);
	// Rotate the point based on the user input in radians
	point = rotatePoint(point, Rotation.xyz);
	// Convert back to latitude and longitude
	latLon = pointToLatLon(point);
	// Convert back to the normalized pixel coordinate
	vec2 sourcePixel;
	if( inputProjection == EQUI )
		sourcePixel = latLonToEquiUv( latLon );
	else if( inputProjection == FISHEYE )
		sourcePixel = pointToFisheyeUv( point, fovIn );
	else if( inputProjection == FLAT )
		sourcePixel = latLonToFlatUv( latLon, fovIn );

	if( sourcePixel == SET_TO_TRANSPARENT ) {
		fragColor = TRANSPARENT_PIXEL;
		return;
	}
	// Applying the MaxUV after our opterations fixes the "seam" from
	// https://github.com/DanielArnett/360-VJ/issues/10
	sourcePixel *= MaxUV;
	// Set the color of the destination pixel to the color of the source pixel
	fragColor = texture( InputTexture, sourcePixel );
}
//)"