R"(
#version 410 core
uniform sampler2D InputTexture;
uniform vec3 Brightness;

in vec2 uv;
out vec4 fragColor;
//precision highp float;
vec4 TRANSPARENT_PIXEL = vec4( 0.0, 0.0, 0.0, 0.0 );
float PI = 3.14159265359;
const int EQUI          = 0;
const int FISHEYE       = 1;
const int FLAT          = 2;
const int CUBEMAP       = 3;
const int GRIDLINES_OFF = 0;
const int GRIDLINES_ON  = 1;
int inputProjection     = EQUI;
int outputProjection    = EQUI;
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

void main()
{

	// Latitude and Longitude of the destination pixel (uv)
	vec2 latLon = equiUvToLatLon( uv );
	// Create a point on the unit-sphere from the latitude and longitude
		// X increases from left to right [-1 to 1]
		// Y increases from bottom to top [-1 to 1]
		// Z increases from back to front [-1 to 1]
	vec3 point = latLonToPoint(latLon);
	// Rotate the point based on the user input in radians
	point = rotatePoint(point, Brightness.xyz * PI);
	// Convert back to latitude and longitude
	latLon = pointToLatLon(point);
	// Convert back to the normalized pixel coordinate
	vec2 sourcePixel = latLonToEquiUv( latLon );
	// Set the color of the destination pixel to the color of the source pixel
	fragColor = texture( InputTexture, sourcePixel );
	//The InputTexture contains premultiplied colors, so we need to unpremultiply first to apply our effect on straight colors.
	//( color.a > 0.0 )
	//color.rgb /= color.a;
	//
	//lor.rgb += Brightness * 2. - 1.;
	//
	//The plugin has to output premultiplied colors, this is how we're premultiplying our straight color while also
	//ensuring we aren't going out of the LDR the video engine is working in.
	//lor.rgb = clamp( color.rgb * color.a, vec3( 0.0 ), vec3( color.a ) );
	//agColor = color;
}
)"