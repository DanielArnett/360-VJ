//
//		ShaderMaker.cpp
//
//		A source code template that can be used to compile FreeframeGL
//		plugins from shader source copied from "GLSL Sandbox" and "ShaderToy".
//
//		------------------------------------------------------------
//		Revisions :
//		21-01-15	Version 1.000
//		26.02.15	Changes for FFGL 1.6
//					change DWORD to FFResult
//					remove "Virtual" from destructor definition
//					Changes  for port to OSX
//					Version 1.001
//		06.03.15	Provided for revised SharedToy spec with mainImage instead of main
//                  See ShaderToy example 4
//					Version 1.002
//		------------------------------------------------------------
//
//		Copyright (C) 2015. Lynn Jarvis, Leading Edge. Pty. Ltd.
//		Ported to OSX by Amaury Hazan (amaury@billaboop.com)
//
//		This program is free software: you can redistribute it and/or modify
//		it under the terms of the GNU Lesser General Public License as published by
//		the Free Software Foundation, either version 3 of the License, or
//		(at your option) any later version.
//
//		This program is distributed in the hope that it will be useful,
//		but WITHOUT ANY WARRANTY; without even the implied warranty of
//		MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//		GNU Lesser General Public License for more details.
//
//		You will receive a copy of the GNU Lesser General Public License along 
//		with this program.  If not, see http://www.gnu.org/licenses/.
//		--------------------------------------------------------------
//
//
#include "ShaderMaker.h"

#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32__))
int (*cross_secure_sprintf)(char *, size_t, const char *,...) = sprintf_s;
#else 
// posix
int (*cross_secure_sprintf)(char *, size_t, const char *, ...) = snprintf;
#endif

#define FFPARAM_SPEED       (0)
#define FFPARAM_MOUSEX      (1)
#define FFPARAM_MOUSEY      (2)
#define FFPARAM_MOUSELEFTX  (3)
#define FFPARAM_MOUSELEFTY  (4)
#define FFPARAM_RED         (5)
#define FFPARAM_GREEN       (6)
#define FFPARAM_BLUE        (7)
#define FFPARAM_ALPHA       (8)

#define STRINGIFY(A) #A

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// +++++++++++ IMPORTANT : DEFINE YOUR PLUGIN INFORMATION HERE +++++++++++
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
static CFFGLPluginInfo PluginInfo ( 
	ShaderMaker::CreateInstance,		// Create method
	"ZZZZ",								// *** Plugin unique ID (4 chars) - this must be unique for each plugin
	"Shader Maker",						// *** Plugin name - make it different for each plugin 
	1,						   			// API major version number 													
	006,								// API minor version number	
	1,									// *** Plugin major version number
	002,								// *** Plugin minor version number
	FF_EFFECT,							// Plugin type is always an effect
	"Wraps ShaderToy and GLSLSandbox shaders into a FFGL plugin", // *** Plugin description - you can expand on this
	"by Lynn Jarvis (spout.zeal.co) OSX port by Amaury Hazan (billaboop.com)"			// *** About - use your own name and details
);


// Common vertex shader code as per FreeFrame examples
char *vertexShaderCode = STRINGIFY (
void main()
{
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_FrontColor = gl_Color;

} );


// Important notes :

// The shader code is pasted into a section of the source file below
// which uses the Stringizing operator (#) (https://msdn.microsoft.com/en-us/library/7e3a913x.aspx)
// This converts the shader code into a string which is then used by the shader compiler on load of the plugin.
// There are some limitations of using the stringizing operator in this way because it depends on the "#" symbol,
// e.g. #( .. code ), Therefore there cannot be any # characters in the code itself.
//
// For example it is common to see :
//
//		#ifdef GL_ES
//		precision mediump float;
//		#endif
//
//	The #ifdef and #endif have to be removed.
//
//		// #ifdef GL_ES
//		// precision mediump float;
//		// #endif
//
// Compile the code as-is to start with and you should get a functioning plugin.
// Several examples can be used below. Only one can be compiled at a time.
//

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// ++++++ COPY/PASTE YOUR GLSL SANDBOX OR SHADERTOY SHADER CODE HERE +++++
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
char *fragmentShaderCode = STRINGIFY (
// ==================== PASTE WITHIN THESE LINES =======================


// Red screen test shader
/*void main(void) {
    gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);

}*/


/*
//
// Shadertoy example 1
//
//
// https://www.shadertoy.com/view/MdfSzn
//
// MetaTunnel
//
// Created by Anatole Duprat - XT95/2014 - http://www.aduprat.com/portfolio/?page=shaders
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
//
// http://www.pouet.net/prod.php?which=52777
// By Frequency - http://www.frequency.fr/
// 1st place at Numerica Artparty in march 2009 in France
//

float time = iGlobalTime*.5;

const float s=0.4; //Density threshold

// The scene define by density
float obj(vec3 p)
{
    float d = 1.0;
    d *= distance(p, vec3(cos(time)+sin(time*0.2),0.3,2.0+cos(time*0.5)*0.5) );
    d  = distance(p,vec3(-cos(time*0.7),0.3,2.0+sin(time*0.5)));
    d *= distance(p,vec3(-sin(time*0.2)*0.5,sin(time),2.0));
    d *=cos(p.y)*cos(p.x)-0.1-cos(p.z*7.+time*7.)*cos(p.x*3.)*cos(p.y*4.)*0.1;
    return d;
}

void main()
{
    vec2 q = gl_FragCoord.xy/iResolution.xy;
    vec2 v = -1.0+2.0*q;
    v.x *= iResolution.x/iResolution.y*.5+.5;
	
    vec3 o = vec3(v.x,v.y,0.0);
    vec3 d = normalize(vec3(v.x+cos(time)*.3,v.y,1.0))/64.0;
	
    vec3 color = vec3(0.0);
    float t = 0.0;
    bool hit = false;
	
    for(int i=0; i<100; i++) {
        if(!hit) {
            if(obj(o+d*t) < s) {
                t-=5.0;
                for(int j=0; j<5; j++)
                    if(obj(o+d*t) > s)
                        t+=1.0;
                vec3 e=vec3(0.01,.0,.0);
                vec3 n=vec3(0.0);
                n.x=obj(o+d*t)-obj(vec3(o+d*t+e.xyy));
                n.y=obj(o+d*t)-obj(vec3(o+d*t+e.yxy));
                n.z=obj(o+d*t)-obj(vec3(o+d*t+e.yyx));
                n = normalize(n);
                color = vec3(1.) * max(dot(vec3(0.0,0.0,-0.5),n),0.0)+max(dot(vec3(0.0,- .5,0.5),n),0.0)*0.5;
                hit=true;
            }
            t+=5.0;
        }
    }

    gl_FragColor= vec4(color,1.)+vec4(0.1,0.2,0.5,1.0)*(t*0.025);

}
*/


/*
//
// Shadertoy example 2 - needs a texture input..
//
// https://www.shadertoy.com/view/Xsl3zn
//
//
// Warping Texture
//
// Created by inigo quilez - iq/2013
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
//
                                      
void main(void)
{
    vec2 uv = 0.5*gl_FragCoord.xy / iResolution.xy;

    float d = length(uv);
    vec2 st = uv*0.1 + 0.2*vec2(cos(0.071*iGlobalTime+d), sin(0.073*iGlobalTime-d));

    vec3 col = texture2D( iChannel0, st ).xyz;
    float w = col.x;
    col *= 1.0 - texture2D( iChannel0, 0.4*uv + 0.1*col.xy  ).xyy;
    col *= w*2.0;
	
    col *= 1.0 + 2.0*d;
    gl_FragColor = vec4(col,1.0);

}
*/


/*
//
// Shadertoy example 3
//
//
// 'Menger Sponge' by iq (2010)
//
// https://www.shadertoy.com/view/4sX3Rn?
//
// Created by inigo quilez
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
//
// http://www.iquilezles.org/apps/shadertoy/index2.html
//
// Three iterations of the famous fractal structure (pretty much inspired by untraceable/TBC).
// See http://www.iquilezles.org/articles/menger/menger.htm for the full explanation of how this was done
//

                                      /*
float maxcomp(in vec3 p ) { return max(p.x,max(p.y,p.z));}
float sdBox( vec3 p, vec3 b )
{
  vec3  di = abs(p) - b;
  float mc = maxcomp(di);
  return min(mc,length(max(di,0.0)));
}

mat3 ma = mat3( 0.60, 0.00,  0.80,
                0.00, 1.00,  0.00,
               -0.80, 0.00,  0.60 );

vec4 map( in vec3 p )
{
    float d = sdBox(p,vec3(1.0));
    vec4 res = vec4( d, 1.0, 0.0, 0.0 );

    float ani = smoothstep( -0.2, 0.2, -cos(0.5*iGlobalTime) );
	float off = 1.5*sin( 0.01*iGlobalTime );
	
    float s = 1.0;
    for( int m=0; m<4; m++ )
    {

        p = mix( p, ma*(p+off), ani );
	   
        vec3 a = mod( p*s, 2.0 )-1.0;
        s *= 3.0;
        vec3 r = abs(1.0 - 3.0*abs(a));
        float da = max(r.x,r.y);
        float db = max(r.y,r.z);
        float dc = max(r.z,r.x);
        float c = (min(da,min(db,dc))-1.0)/s;

        if( c>d )
        {
          d = c;
          res = vec4( d, min(res.y,0.2*da*db*dc), (1.0+float(m))/4.0, 0.0 );
        }
    }

    return res;
}

vec4 intersect( in vec3 ro, in vec3 rd )
{
    float t = 0.0;
    vec4 res = vec4(-1.0);
	vec4 h = vec4(1.0);
    for( int i=0; i<64; i++ )
    {
		if( h.x<0.002 || t>10.0 ) break;
        h = map(ro + rd*t);
        res = vec4(t,h.yzw);
        t += h.x;
    }
	if( t>10.0 ) res=vec4(-1.0);
    return res;
}

float softshadow( in vec3 ro, in vec3 rd, float mint, float k )
{
    float res = 1.0;
    float t = mint;
	float h = 1.0;
    for( int i=0; i<32; i++ )
    {
        h = map(ro + rd*t).x;
        res = min( res, k*h/t );
		t += clamp( h, 0.005, 0.1 );
    }
    return clamp(res,0.0,1.0);
}

vec3 calcNormal(in vec3 pos)
{
    vec3  eps = vec3(.001,0.0,0.0);
    vec3 nor;
    nor.x = map(pos+eps.xyy).x - map(pos-eps.xyy).x;
    nor.y = map(pos+eps.yxy).x - map(pos-eps.yxy).x;
    nor.z = map(pos+eps.yyx).x - map(pos-eps.yyx).x;
    return normalize(nor);
}

void main(void)
{
    vec2 p = -1.0 + 2.0 * gl_FragCoord.xy / iResolution.xy;
    p.x *= 1.33;

    // light
    vec3 light = normalize(vec3(1.0,0.9,0.3));

    float ctime = iGlobalTime;
    // camera
    vec3 ro = 1.1*vec3(2.5*sin(0.25*ctime),1.0+1.0*cos(ctime*.13),2.5*cos(0.25*ctime));
    vec3 ww = normalize(vec3(0.0) - ro);
    vec3 uu = normalize(cross( vec3(0.0,1.0,0.0), ww ));
    vec3 vv = normalize(cross(ww,uu));
    vec3 rd = normalize( p.x*uu + p.y*vv + 2.5*ww );

    // background color
    vec3 col = mix( vec3(0.3,0.2,0.1)*0.5, vec3(0.7, 0.9, 1.0), 0.5 + 0.5*rd.y );
	
    vec4 tmat = intersect(ro,rd);
    if( tmat.x>0.0 )
    {
        vec3  pos = ro + tmat.x*rd;
        vec3  nor = calcNormal(pos);
		
        float occ = tmat.y;
		float sha = softshadow( pos, light, 0.01, 64.0 );

		float dif = max(0.1 + 0.9*dot(nor,light),0.0);
		float sky = 0.5 + 0.5*nor.y;
        float bac = max(0.4 + 0.6*dot(nor,vec3(-light.x,light.y,-light.z)),0.0);

        vec3 lin  = vec3(0.0);
        lin += 1.00*dif*vec3(1.10,0.85,0.60)*sha;
        lin += 0.50*sky*vec3(0.10,0.20,0.40)*occ;
        lin += 0.10*bac*vec3(1.00,1.00,1.00)*(0.5+0.5*occ);
        lin += 0.25*occ*vec3(0.15,0.17,0.20);	 

        vec3 matcol = vec3(
            0.5+0.5*cos(0.0+2.0*tmat.z),
            0.5+0.5*cos(1.0+2.0*tmat.z),
            0.5+0.5*cos(2.0+2.0*tmat.z) );
        col = matcol * lin;
    }

    col = pow( col, vec3(0.4545) );

    gl_FragColor = vec4(col,1.0);
}
*/

//
// Shadertoy example 4
//
// LJ - Example of revised specification using "mainImage" instead of "main"
// A fix can be made to include a main function right here, but it is included
// before compilation in "LoadShader" to be consistent with "ShaderLoader"
//
// https://www.shadertoy.com/view/ldl3W8#
//
// Voronoi - distances
//
// Created by inigo quilez - iq/2013
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

// I've not seen anybody out there computing correct cell interior distances for Voronoi
// patterns yet. That's why they cannot shade the cell interior correctly, and why you've
// never seen cell boundaries rendered correctly. 

// However, here's how you do mathematically correct distances (note the equidistant and non
// degenerated grey isolines inside the cells) and hence edges (in yellow):

// http://www.iquilezles.org/www/articles/voronoilines/voronoilines.htm

// LJ - must avoid hash charaters in the code when using stringify. They are OK in comments
// #define ANIMATE
bool bAnimate = true;

vec2 hash2( vec2 p )
{
    // texture based white noise
    // return texture2D( iChannel0, (p+0.5)/256.0, -100.0 ).xy;
	
    // procedural white noise	
    return fract(sin(vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3))))*43758.5453);
}

vec3 voronoi( in vec2 x )
{
    vec2 n = floor(x);
    vec2 f = fract(x);

    //----------------------------------
    // first pass: regular voronoi
    //----------------------------------
	// LJ - stringify does not like the comma here !
	// Declare variables separately
	// vec2 mg, mr;
	vec2 mg;
	vec2 mr;

    float md = 8.0;

    for( int j=-1; j<=1; j++ ) {
        for( int i=-1; i<=1; i++ ) {
            vec2 g = vec2(float(i),float(j));
            vec2 o = hash2( n + g );
			// LJ avoid the hash character here
			// #ifdef ANIMATE
			if(bAnimate)
			    o = 0.5 + 0.5*sin( iGlobalTime + 6.2831*o );
			// #endif
            vec2 r = g + o - f;
            float d = dot(r,r);

            if( d<md ) {
                md = d;
                mr = r;
                mg = g;
            }
        }
	}

    //----------------------------------
    // second pass: distance to borders
    //----------------------------------
    md = 8.0;
    for( int j=-2; j<=2; j++ ) {
        for( int i=-2; i<=2; i++ ) {
            vec2 g = mg + vec2(float(i),float(j));
            vec2 o = hash2( n + g );
			// LJ avoid the hash character here
			// #ifdef ANIMATE
			if(bAnimate)
				o = 0.5 + 0.5*sin( iGlobalTime + 6.2831*o );
			// #endif
            vec2 r = g + o - f;

            if( dot(mr-r,mr-r)>0.00001 )
                md = min( md, dot( 0.5*(mr+r), normalize(r-mr) ) );
        }
	}

    return vec3( md, mr );
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 p = fragCoord.xy/iResolution.xx;

    vec3 c = voronoi( 8.0*p );

    // isolines
    vec3 col = c.x*(0.5 + 0.5*sin(64.0*c.x))*vec3(1.0);
    
    // borders	
    col = mix( vec3(1.0,0.6,0.0), col, smoothstep( 0.04, 0.07, c.x ) );
    
    // feature points
    float dd = length( c.yz );
    col = mix( vec3(1.0,0.6,0.1), col, smoothstep( 0.0, 0.12, dd) );
    col += vec3(1.0,0.6,0.1)*(1.0-smoothstep( 0.0, 0.04, dd));

    fragColor = vec4(col,1.0);
}



/*
//
// GLSL Sandbox example 1
//
//
//	Duelling Mandelbulbs
//
// http://glsl.herokuapp.com/e#1293.0
// http://www.thealphablenders.com/
// 2012 by Andrew Caudwell
//

// This has to be removed for stringify
// #ifdef GL_ES
// precision mediump float;
// #endif

uniform vec2  resolution;
uniform vec2  mouse;
uniform float time;

struct Ray {
   vec3 o;
   vec3 d;
};

// dueling mandelbulbs
// @acaudwell

// http://www.fractalforums.com/mandelbulb-implementation/realtime-renderingoptimisations/

float mandelbulb(in vec3 p, float power) {
	
    float dr = 1.0;
    float r  = length(p);

    vec3 c = p;
	
    for(int i=0; i<2; i++) {
		    
        float zo0 = asin(p.z / r);
        float zi0 = atan(p.y, p.x);
        float zr  = pow(r, power-1.0);
        float zo  = (zo0) * power;
        float zi  = (zi0) * power;
        float czo = cos(zo);

        dr = zr * dr * power + 1.0;
        zr *= r;

        p = zr * vec3(czo*cos(zi), czo*sin(zi), sin(zo));
	        
	p += c;
	    
        r = length(p);
    }

    return 0.5 * r * log(r) / r;	
}

void main() {

    vec2 p = ((gl_FragCoord.xy / resolution.xy) * 2.0 - 1.0) * 3.5;
	
    float t = time;

    Ray ray1;
    ray1.o = vec3(0.0);
    ray1.d = normalize( vec3((p - 1.5*vec2(sin(t-2.0), cos(t+1.0))) * vec2(resolution.x/resolution.y, 1.0), 1.0 ) );

    Ray ray2;
    ray2.o = vec3(0.0);
    ray2.d = normalize( vec3((p - 1.5*vec2(cos(-t),sin(t))) * vec2(resolution.x/resolution.y, 1.0), 1.0 ) );
		
    ray1.d.xy = vec2( ray1.d.x * cos(t) - ray1.d.y * sin(t), ray1.d.x * sin(t) + ray1.d.y * cos(t)); 
    ray2.d.xy = vec2( ray2.d.x * cos(t) - ray2.d.y * sin(t), ray2.d.x * sin(t) + ray2.d.y * cos(t)); 
	
    float m1 =  mandelbulb(ray1.o + ray1.d, abs(cos(t)*13.0));
    float m2 =  mandelbulb(ray2.o + ray2.d, abs(sin(t)*13.0));
	
    float f = pow(max(m1,m2) , abs(m1-m2));
    vec3  c = m1 > m2 ? vec3(0.0, 0.05, 0.2) : vec3(0.2, 0.05, 0.0);
	
    gl_FragColor = vec4(c*f, 1.0);
}*/

/*
//
// GLSL Sandbox example 2
//
//
//	Relief tunnel
//
// http://glsl.herokuapp.com/e#3259.0
//

// This has to be removed for stringify
// #ifdef GL_ES
// precision mediump float;
// #endif

//gt
uniform float time;
uniform vec2 mouse;
uniform vec2 resolution;

void main( void ) {
	
	vec2 p = -1.0 + 2.0 * gl_FragCoord.xy / resolution.xy;
   	vec2 uv;

	//shadertoy deform "relief tunnel"-gt
   	float r = sqrt( dot(p,p) );
   	float a = atan(p.y,p.x) + 0.9*sin(0.5*r-0.5*time);

	float s = 0.5 + 0.5*cos(7.0*a);
   	s = smoothstep(0.0,1.0,s);
   	s = smoothstep(0.0,1.0,s);
   	s = smoothstep(0.0,1.0,s);
   	s = smoothstep(0.0,1.0,s);

   	uv.x = time + 1.0/( r + .2*s);
  	  //uv.y = 3.0*a/3.1416;
	uv.y = 1.0*a/10.1416;

    float w = (0.5 + 0.5*s)*r*r;
   	// vec3 col = texture2D(tex0,uv).xyz;
    float ao = 0.5 + 0.5*cos(42.0*a);//amp up the ao-g
   	ao = smoothstep(0.0,0.4,ao)-smoothstep(0.4,0.7,ao);
    	ao = 1.0-0.5*ao*r;

	//faux shaded texture-gt
	float px = gl_FragCoord.x/resolution.x;
	float py = gl_FragCoord.y/resolution.y;
	float x = mod(uv.x*resolution.x,resolution.x/3.5);
	float y = mod(uv.y*resolution.y+(resolution.y/2.),resolution.y/3.5);
	float v =  (x / y)-.7;

	gl_FragColor = vec4(vec3(.1-v,.9-v,1.-v)*w*ao,1.0);

}
*/

// ==================== END OF SHADER CODE PASTE =======================


);


////////////////////////////////////////////////////////////////////////////////////////////////////
//  Constructor and destructor
////////////////////////////////////////////////////////////////////////////////////////////////////
ShaderMaker::ShaderMaker():CFreeFrameGLPlugin()
{

	/*
	// Debug console window so printf works
	FILE* pCout; // should really be freed on exit 
	AllocConsole();
	freopen_s(&pCout, "CONOUT$", "w", stdout); 
	printf("Shader Maker Vers 1.002\n");
	printf("GLSL version [%s]\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
	*/

	// Input properties allow for no texture or for four textures
	SetMinInputs(0);
	SetMaxInputs(2); // TODO - 4 inputs

	// Parameters
	SetParamInfo(FFPARAM_SPEED,         "Speed",         FF_TYPE_STANDARD, 0.5f); m_UserSpeed = 0.5f;
	SetParamInfo(FFPARAM_MOUSEX,        "X mouse",       FF_TYPE_STANDARD, 0.5f); m_UserMouseX = 0.5f;
	SetParamInfo(FFPARAM_MOUSEY,        "Y mouse",       FF_TYPE_STANDARD, 0.5f); m_UserMouseY = 0.5f;
	SetParamInfo(FFPARAM_MOUSELEFTX,    "X mouse left",  FF_TYPE_STANDARD, 0.5f); m_UserMouseLeftX = 0.5f;
	SetParamInfo(FFPARAM_MOUSELEFTY,    "Y mouse left",  FF_TYPE_STANDARD, 0.5f); m_UserMouseLeftY = 0.5f;
	SetParamInfo(FFPARAM_RED,           "Red",           FF_TYPE_STANDARD, 0.5f); m_UserRed = 0.5f;
	SetParamInfo(FFPARAM_GREEN,         "Green",         FF_TYPE_STANDARD, 0.5f); m_UserGreen = 0.5f;
	SetParamInfo(FFPARAM_BLUE,          "Blue",          FF_TYPE_STANDARD, 0.5f); m_UserBlue = 0.5f;
	SetParamInfo(FFPARAM_ALPHA,         "Alpha",         FF_TYPE_STANDARD, 1.0f); m_UserAlpha = 1.0f;

	// Set defaults
	SetDefaults();

	// Flags
	bInitialized = false;

}

////////////////////////////////////////////////////////////////////////////////////////////////////
//  Methods
////////////////////////////////////////////////////////////////////////////////////////////////////
FFResult ShaderMaker::InitGL(const FFGLViewportStruct *vp)
{
	// initialize gl extensions and make sure required features are supported
	m_extensions.Initialize();
	if (m_extensions.multitexture==0 || m_extensions.ARB_shader_objects==0)
		return FF_FAIL;

	// Set the viewport size
	m_vpWidth  = (float)vp->width;
	m_vpHeight = (float)vp->height;

	// Start the clock
	StartCounter();

	// Load the shader
	std::string shaderString = fragmentShaderCode;
	bInitialized = LoadShader(shaderString);

	return FF_SUCCESS;
}

ShaderMaker::~ShaderMaker()
{
	// Not using this but it is here just in case
}


FFResult ShaderMaker::DeInitGL()
{
	if(bInitialized)
		m_shader.UnbindShader();

	m_shader.FreeGLResources();

	if(m_fbo) m_extensions.glDeleteFramebuffersEXT(1, &m_fbo);
	if(m_glTexture0) glDeleteTextures(1, &m_glTexture0);
	if(m_glTexture1) glDeleteTextures(1, &m_glTexture1);
	if(m_glTexture2) glDeleteTextures(1, &m_glTexture2);
	if(m_glTexture3) glDeleteTextures(1, &m_glTexture3);
	m_glTexture0 = 0;
	m_glTexture1 = 0;
	m_glTexture2 = 0;
	m_glTexture3 = 0;
	m_fbo = 0;
	bInitialized = false;

	return FF_SUCCESS;
}

FFResult ShaderMaker::ProcessOpenGL(ProcessOpenGLStruct *pGL)
{
	FFGLTextureStruct Texture0;
	FFGLTextureStruct Texture1;
	// TODO
	// FFGLTextureStruct Texture2;
	// FFGLTextureStruct Texture3;
	FFGLTexCoords maxCoords;
	time_t datime;
	struct tm tmbuff;

	if(bInitialized) {

		// To the host this is an effect plugin, but it can be either a source or an effect
		// and will work without any input, so we still start up if even there is no input texture

		// Is there is texture needed by the shader ?
		if(m_inputTextureLocation >= 0 || m_inputTextureLocation1 >= 0) {

			// Is there a texture available ?
			if(m_inputTextureLocation >= 0 && pGL->numInputTextures > 0 && pGL->inputTextures[0] != NULL) {

				Texture0 = *(pGL->inputTextures[0]);
				maxCoords = GetMaxGLTexCoords(Texture0);

				// Delete the local texture if the incoming size is different
				if((int)m_channelResolution[0][0] != Texture0.Width || (int)m_channelResolution[0][1] != Texture0.Height) {
					if(m_glTexture0 > 0) glDeleteTextures(1, &m_glTexture0);
				}

				// Set the resolution of the first texture size
				m_channelResolution[0][0] = (float)Texture0.Width;
				m_channelResolution[0][1] = (float)Texture0.Height;

				// For a power of two texture, the size will be different to the hardware size.
				// The shader will not compensate for this, so we have to create another texture
				// the same size as the resolution we set to the shader. Also the shader needs 
				// textures created with wrapping REPEAT rather than CLAMP to edge
				// So we create such a texture if the size changes and use it for every frame.
				CreateRectangleTexture(Texture0, maxCoords, m_glTexture0, GL_TEXTURE0, m_fbo, pGL->HostFBO);

				// Now we have a local texture of the right size and type
				// Filled with the data from the incoming Freeframe texture
			}

			// Repeat if there is a second incoming texture and the shader needs it
			if(m_inputTextureLocation1 >= 0 && pGL->numInputTextures > 1 && pGL->inputTextures[1] != NULL) {
				Texture1 = *(pGL->inputTextures[1]);
				maxCoords = GetMaxGLTexCoords(Texture1);
				if((int)m_channelResolution[1][0] != Texture1.Width || (int)m_channelResolution[1][1] != Texture1.Height) {
					if(m_glTexture1 > 0) glDeleteTextures(1, &m_glTexture1);
				}
				// Set the channel resolution of the second texture size
				m_channelResolution[1][0] = (float)Texture1.Width;
				m_channelResolution[1][1] = (float)Texture1.Height;
				CreateRectangleTexture(Texture1, maxCoords, m_glTexture1, GL_TEXTURE1, m_fbo, pGL->HostFBO);
			}

			/*
			// And for textures 3 and 4
			if(m_inputTextureLocation2 >= 0 && pGL->numInputTextures > 2 && pGL->inputTextures[2] != NULL) {
				Texture2 = *(pGL->inputTextures[2]);
				maxCoords = GetMaxGLTexCoords(Texture2);
				if((int)m_channelResolution[2][0] != Texture2.Width || (int)m_channelResolution[2][1] != Texture2.Height) {
					if(m_glTexture2 > 0) glDeleteTextures(1, &m_glTexture2);
				}
				// Set the channel resolution of the second texture size
				m_channelResolution[2][0] = (float)Texture2.Width;
				m_channelResolution[2][1] = (float)Texture2.Height;
				CreateRectangleTexture(Texture2, maxCoords, m_glTexture2, GL_TEXTURE1, m_fbo, pGL->HostFBO);
			}

			if(m_inputTextureLocation3 >= 0 && pGL->numInputTextures > 3 && pGL->inputTextures[3] != NULL) {
				Texture3 = *(pGL->inputTextures[3]);
				maxCoords = GetMaxGLTexCoords(Texture2);
				if((int)m_channelResolution[3][0] != Texture3.Width || (int)m_channelResolution[3][1] != Texture3.Height) {
					if(m_glTexture3 > 0) glDeleteTextures(1, &m_glTexture3);
				}
				// Set the channel resolution of the second texture size
				m_channelResolution[3][0] = (float)Texture3.Width;
				m_channelResolution[3][1] = (float)Texture3.Height;
				CreateRectangleTexture(Texture3, maxCoords, m_glTexture3, GL_TEXTURE1, m_fbo, pGL->HostFBO);
			}
			*/

		} // endif shader uses a texture
	
		// Calculate elapsed time
		lastTime = elapsedTime;
		elapsedTime = GetCounter()/1000.0; // In seconds - higher resolution than timeGetTime()
		m_time = m_time + (float)(elapsedTime-lastTime)*m_UserSpeed*2.0f; // increment scaled by user input 0.0 - 2.0

		// Just pass elapsed time for individual channel times
		m_channelTime[0] = m_time;
		m_channelTime[1] = m_time;
		m_channelTime[2] = m_time;
		m_channelTime[3] = m_time;

		// Calculate date vars
		time(&datime);
#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32__))
		localtime_s(&tmbuff, &datime);
#else
        localtime_r(&datime, &tmbuff);
#endif
		m_dateYear = (float)tmbuff.tm_year;
		m_dateMonth = (float)tmbuff.tm_mon+1;
		m_dateDay = (float)tmbuff.tm_mday;
		m_dateTime = (float)(tmbuff.tm_hour*3600 + tmbuff.tm_min*60 + tmbuff.tm_sec);

		// activate our shader
		m_shader.BindShader();

		//
		// Assign values and set the uniforms to the shader
		//

		//
		// Common
		//

		// First input texture
		// The shader will use the first texture bound to GL texture unit 0
		if(m_inputTextureLocation >= 0 && Texture0.Handle > 0) {
			m_extensions.glUniform1iARB(m_inputTextureLocation, 0);
		}

		// Second input texture
		// The shader will use the texture bound to GL texture unit 1
		if(m_inputTextureLocation1 >= 0 && Texture1.Handle > 0)
			m_extensions.glUniform1iARB(m_inputTextureLocation1, 1);

		/*
		// 4 channels
		if(m_inputTextureLocation2 >= 0 && Texture2.Handle > 0)
			m_extensions.glUniform1iARB(m_inputTextureLocation2, 2);

		if(m_inputTextureLocation3 >= 0 && Texture3.Handle > 0)
			m_extensions.glUniform1iARB(m_inputTextureLocation3, 3);
		*/

		// Elapsed time
		if(m_timeLocation >= 0) 
			m_extensions.glUniform1fARB(m_timeLocation, m_time);
	
		//
		// GLSL sandbox
		//
		// resolution (viewport size)
		if(m_screenLocation >= 0) 
			m_extensions.glUniform2fARB(m_screenLocation, m_vpWidth, m_vpHeight); 

		// mouse - Mouse position
		if(m_mouseLocation >= 0) { // Vec2 - normalized
			m_mouseX = m_UserMouseX;
			m_mouseY = m_UserMouseY;
			m_extensions.glUniform2fARB(m_mouseLocation, m_mouseX, m_mouseY); 
		}

		// surfaceSize - Mouse left drag position - in pixel coordinates
		if(m_surfaceSizeLocation >= 0) {
			m_mouseLeftX = m_UserMouseLeftX*m_vpWidth;
			m_mouseLeftY = m_UserMouseLeftY*m_vpHeight;
			m_extensions.glUniform2fARB(m_surfaceSizeLocation, m_mouseLeftX, m_mouseLeftY);
		}

		//
		// Shadertoy

		// iMouse
		// xy contain the current pixel coords (if LMB is down);
		// zw contain the click pixel.
		// Modified here equivalent to mouse unclicked or left button dragged
		// The mouse is not being simulated, they are just inputs that can be used within the shader.
		if(m_mouseLocationVec4 >= 0) {
			// Convert from 0-1 to pixel coordinates for ShaderToy
			// Here we use the resolution rather than the screen
			m_mouseX     = m_UserMouseX*m_vpWidth;
			m_mouseY     = m_UserMouseY*m_vpHeight;
			m_mouseLeftX = m_UserMouseLeftX*m_vpWidth;
			m_mouseLeftY = m_UserMouseLeftY*m_vpHeight;
			m_extensions.glUniform4fARB(m_mouseLocationVec4, m_mouseX, m_mouseY, m_mouseLeftX, m_mouseLeftY); 
		}

		// iResolution - viewport resolution
		if(m_resolutionLocation >= 0) // Vec3
			m_extensions.glUniform3fARB(m_resolutionLocation, m_vpWidth, m_vpHeight, 1.0); 

		// Channel resolutions are linked to the actual texture resolutions - the size is set in ProcessOpenGL
		// Global resolution is the viewport
		if(m_channelresolutionLocation >= 0) {
			// uniform vec3	iChannelResolution[4]
			// 4 channels Vec3. Float array is 4 rows, 3 cols
			// TODO - 4 channels - 2 & 3 are unused so will not have a texture anyway
			m_channelResolution[2][0] = m_vpWidth;
			m_channelResolution[2][1] = m_vpHeight;
			m_channelResolution[2][2] = 1.0;
			m_channelResolution[3][0] = m_vpWidth;
			m_channelResolution[3][1] = m_vpHeight;
			m_channelResolution[3][2] = 1.0;
			m_extensions.glUniform3fvARB(m_channelresolutionLocation, 4, (GLfloat *)m_channelResolution);
		}

		// iDate - vec4
		if(m_dateLocation >= 0) 
			m_extensions.glUniform4fARB(m_dateLocation, m_dateYear, m_dateMonth, m_dateDay, m_dateTime);

		// Channel elapsed time - vec4
		if(m_channeltimeLocation >= 0)
			m_extensions.glUniform1fvARB(m_channeltimeLocation, 4, m_channelTime);

		// Extras - input colour is linked to the user controls Red, Green, Blue, Alpha
		if(m_inputColourLocation >= 0)
			m_extensions.glUniform4fARB(m_inputColourLocation, m_UserRed, m_UserGreen, m_UserBlue, m_UserAlpha);

		// Bind a texture if the shader needs one
		if(m_inputTextureLocation >= 0 && Texture0.Handle > 0) {
			m_extensions.glActiveTexture(GL_TEXTURE0);
			// For a power of two texture we will have created a local texture
			if(m_glTexture0 > 0)
				glBindTexture(GL_TEXTURE_2D, m_glTexture0);
			else
				glBindTexture(GL_TEXTURE_2D, Texture0.Handle);
		}

		// If there is a second texture, bind it to texture unit 1
		if(m_inputTextureLocation1 >= 0 && Texture1.Handle > 0) {
			m_extensions.glActiveTexture(GL_TEXTURE1);
			if(m_glTexture1 > 0)
				glBindTexture(GL_TEXTURE_2D, m_glTexture1);
			else
				glBindTexture(GL_TEXTURE_2D, Texture1.Handle);
		}

		/*
		// Texture units 2 and 3
		if(m_inputTextureLocation2 >= 0 && Texture2.Handle > 0) {
			m_extensions.glActiveTexture(GL_TEXTURE2);
			if(m_glTexture2 > 0)
				glBindTexture(GL_TEXTURE_2D, m_glTexture2);
			else
				glBindTexture(GL_TEXTURE_2D, Texture2.Handle);
		}

		if(m_inputTextureLocation3 >= 0 && Texture3.Handle > 0) {
			m_extensions.glActiveTexture(GL_TEXTURE3);
			if(m_glTexture3 > 0)
				glBindTexture(GL_TEXTURE_2D, m_glTexture3);
			else
				glBindTexture(GL_TEXTURE_2D, Texture3.Handle);
		}
		*/

		// Do the draw for the shader to work
		glEnable(GL_TEXTURE_2D);
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);	
		glVertex2f(-1.0, -1.0);
		glTexCoord2f(0.0, 1.0);	
		glVertex2f(-1.0,  1.0);
		glTexCoord2f(1.0, 1.0);	
		glVertex2f( 1.0,  1.0);
		glTexCoord2f(1.0, 0.0);	
		glVertex2f( 1.0, -1.0);
		glEnd();
		glDisable(GL_TEXTURE_2D);

		/*
		// unbind input texture 3
		if(m_inputTextureLocation3 >= 0 && Texture3.Handle > 0) {
			m_extensions.glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		// unbind input texture 2
		if(m_inputTextureLocation2 >= 0 && Texture2.Handle > 0) {
			m_extensions.glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
		*/

		// unbind input texture 1
		if(m_inputTextureLocation1 >= 0 && Texture1.Handle > 0) {
			m_extensions.glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		// unbind input texture 0
		m_extensions.glActiveTexture(GL_TEXTURE0); // default
		if(m_inputTextureLocation >= 0 && Texture0.Handle > 0)
			glBindTexture(GL_TEXTURE_2D, 0);

		// unbind the shader
		m_shader.UnbindShader();

	} // endif bInitialized

	return FF_SUCCESS;
}

char * ShaderMaker::GetParameterDisplay(DWORD dwIndex) {

	memset(m_DisplayValue, 0, 15);
	switch (dwIndex) {

		case FFPARAM_SPEED:
			cross_secure_sprintf(m_DisplayValue, 16, "%d", (int)(m_UserSpeed*100.0));
			return m_DisplayValue;
	
		case FFPARAM_MOUSEX:
			cross_secure_sprintf(m_DisplayValue, 16, "%d", (int)(m_UserMouseX*m_vpWidth));
			return m_DisplayValue;

		case FFPARAM_MOUSEY:
			cross_secure_sprintf(m_DisplayValue, 16, "%d", (int)(m_UserMouseY*m_vpHeight));
			return m_DisplayValue;

		case FFPARAM_MOUSELEFTX:
			cross_secure_sprintf(m_DisplayValue, 16, "%d", (int)(m_UserMouseLeftX*m_vpWidth));
			return m_DisplayValue;

		case FFPARAM_MOUSELEFTY:
			cross_secure_sprintf(m_DisplayValue, 16, "%d", (int)(m_UserMouseLeftY*m_vpHeight));
			return m_DisplayValue;

		case FFPARAM_RED:
			cross_secure_sprintf(m_DisplayValue, 16, "%d", (int)(m_UserRed*256.0));
			return m_DisplayValue;

		case FFPARAM_GREEN:
			cross_secure_sprintf(m_DisplayValue, 16, "%d", (int)(m_UserGreen*256.0));
			return m_DisplayValue;

		case FFPARAM_BLUE:
			cross_secure_sprintf(m_DisplayValue, 16, "%d", (int)(m_UserBlue*256.0));
			return m_DisplayValue;

		case FFPARAM_ALPHA:
			cross_secure_sprintf(m_DisplayValue, 16, "%d", (int)(m_UserAlpha*256.0));
			return m_DisplayValue;

		default:
			return m_DisplayValue;
	}
	return NULL;
}

FFResult ShaderMaker::GetInputStatus(DWORD dwIndex)
{
	DWORD dwRet = FF_INPUT_NOTINUSE;

	switch (dwIndex) {

		case 0 :
			if(m_inputTextureLocation >= 0)
				dwRet = FF_INPUT_INUSE;
			break;

		case 1 :
			if(m_inputTextureLocation1 >= 0)
				dwRet = FF_INPUT_INUSE;
			break;

		/* TODO - 4 channels
		case 2 :
			if(m_inputTextureLocation2 >= 0)
				dwRet = FF_INPUT_INUSE;
			break;

		case 3 :
			if(m_inputTextureLocation3 >= 0)
				dwRet = FF_INPUT_INUSE;
			break;
		*/


		default :
			break;

	}

	return dwRet;

}

float ShaderMaker::GetFloatParameter(unsigned int index)
{
	switch (index) {

		case FFPARAM_SPEED:
			return  m_UserSpeed;
	
		case FFPARAM_MOUSEX:
			return  m_UserMouseX;

		case FFPARAM_MOUSEY:
			return  m_UserMouseY;

		case FFPARAM_MOUSELEFTX:
			return m_UserMouseLeftX;

		case FFPARAM_MOUSELEFTY:
			return m_UserMouseLeftY;

		case FFPARAM_RED:
			return m_UserRed;

		case FFPARAM_GREEN:
			return m_UserGreen;

		case FFPARAM_BLUE:
			return m_UserBlue;

		case FFPARAM_ALPHA:
			return m_UserAlpha;

		default:
			return FF_FAIL;
	}
}

FFResult ShaderMaker::SetFloatParameter(unsigned int index, float value)
{
		switch (index) {

			case FFPARAM_SPEED:
				m_UserSpeed = value;
				break;

			case FFPARAM_MOUSEX:
				m_UserMouseX = value;
				break;

			case FFPARAM_MOUSEY:
				m_UserMouseY = value;
				break;

			case FFPARAM_MOUSELEFTX:
				m_UserMouseLeftX = value;
				break;

			case FFPARAM_MOUSELEFTY:
				m_UserMouseLeftY = value;
				break;

			case FFPARAM_RED:
				m_UserRed = value;
				break;

			case FFPARAM_GREEN:
				m_UserGreen = value;
				break;

			case FFPARAM_BLUE:
				m_UserBlue = value;
				break;

			case FFPARAM_ALPHA:
				m_UserAlpha = value;
				break;

			default:
				return FF_FAIL;
		}
		return FF_SUCCESS;
}

void ShaderMaker::SetDefaults() {

    elapsedTime            = 0.0;
    lastTime               = 0.0;
#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32__))
	PCFreq                 = 0.0;
	CounterStart           = 0;
#else
    start = std::chrono::steady_clock::now();
#endif

	m_mouseX               = 0.5;
	m_mouseY               = 0.5;
	m_mouseLeftX           = 0.5;
	m_mouseLeftY           = 0.5;

	m_UserMouseX           = 0.5;
	m_UserMouseY           = 0.5;
	m_UserMouseLeftX       = 0.5;
	m_UserMouseLeftY       = 0.5;

	m_time                 = 0.0;
	m_dateYear             = 0.0;
	m_dateMonth            = 0.0;
	m_dateDay              = 0.0;
	m_dateTime             = 0.0;

	m_channelTime[0]       = 0.0;
	m_channelTime[1]       = 0.0;
	m_channelTime[2]       = 0.0;
	m_channelTime[3]       = 0.0;

	// ShaderToy -  Vec3 - 4 channels 
	m_channelResolution[0][0] = 0.0; // 0 is width
	m_channelResolution[0][1] = 0.0; // 1 is height
	m_channelResolution[0][2] = 1.0; // 2 is depth

	m_channelResolution[1][0] = 0.0;
	m_channelResolution[1][1] = 0.0;
	m_channelResolution[1][2] = 1.0;

	m_channelResolution[2][0] = 0.0;
	m_channelResolution[2][1] = 0.0;
	m_channelResolution[2][2] = 1.0;

	m_channelResolution[3][0] = 0.0;
	m_channelResolution[3][1] = 0.0;
	m_channelResolution[4][2] = 1.0;

	m_UserSpeed               = 0.5;
	m_UserMouseX              = 0.5;
	m_UserMouseY              = 0.5;
	m_UserMouseLeftX          = 0.5;
	m_UserMouseLeftY          = 0.5;

	// OpenGL
	m_glTexture0              = 0;
	m_glTexture1              = 0;
	m_glTexture2              = 0;
	m_glTexture3              = 0;
	m_fbo                     = 0;

}

bool ShaderMaker::LoadShader(std::string shaderString) {
		
		std::string stoyUniforms;

		//
		// Extra uniforms specific to ShaderMaker for buth GLSL Sandbox and ShaderToy
		// For GLSL Sandbox, the extra "inputColour" uniform has to be typed into the shader
		//		uniform vec4 inputColour
		static char *extraUniforms = { "uniform vec4 inputColour;\n" };
		
		// Is it a GLSL Sandbox file?
		// look for "uniform float time;". If it does not exist it is a ShaderToy file
		// This is an exact string, so the shader has to have it.
		if(strstr(shaderString.c_str(), "uniform float time;") == 0) {
			//
			// ShaderToy file
			//
			// Shadertoy does not include uniform variables in the source file so add them here
			//
			// uniform vec3			iResolution;			// the rendering resolution (in pixels)
			// uniform float		iGlobalTime;			// current time (in seconds)
			// uniform vec4		 	iMouse;					// xy contain the current pixel coords (if LMB is down). zw contain the click pixel.
			// uniform vec4			iDate;					// (year, month, day, time in seconds)
			// uniform float		iChannelTime[4];		// channel playback time (in seconds)
			// uniform vec3			iChannelResolution[4];	// channel resolution (in pixels)
			// uniform sampler2D	iChannel0;				// sampler for input texture 0.
			// uniform sampler2D	iChannel1;				// sampler for input texture 1.
			// uniform sampler2D	iChannel2;				// sampler for input texture 2.
			// uniform sampler2D	iChannel3;				// sampler for input texture 3.
			static char *uniforms = { "uniform vec3 iResolution;\n"
									  "uniform float iGlobalTime;\n"
									  "uniform vec4 iMouse;\n"
									  "uniform vec4 iDate;\n"
									  "uniform float iChannelTime[4];\n"
									  "uniform vec3 iChannelResolution[4];\n"
									  "uniform sampler2D iChannel0;\n"
									  "uniform sampler2D iChannel1;\n"
									  "uniform sampler2D iChannel2;\n"
									  "uniform sampler2D iChannel3;\n" };
			
			stoyUniforms = uniforms;
			stoyUniforms += extraUniforms;
			stoyUniforms += shaderString; // add the rest of the shared content

			// It might be a revised ShaderToy file with "mainImage" instead of "main"
			if(strstr(shaderString.c_str(), "void mainImage") != 0) {
				//
				// If it is a revised spec ShaderToy file, add a fix at the end for GLSL compatibility
				//
				// Credit Eric Newman 
				// http://magicmusicvisuals.com/forums/viewtopic.php?f=2&t=196
				//
				static char *stoyMainFunction = { "void main(void) {\n"
												  "    mainImage(gl_FragColor, gl_FragCoord.xy);\n"
												  "}\n" };
				stoyUniforms += stoyMainFunction;
			}

			shaderString = stoyUniforms; // the final string
		}
	
		// initialize gl shader
		m_shader.SetExtensions(&m_extensions);
		if (!m_shader.Compile(vertexShaderCode, shaderString.c_str())) {
			// SelectSpoutPanel("Shader compile error");
			return false;
		}
		else {
			// activate our shader
			bool success = false;
			if (m_shader.IsReady()) {
				if (m_shader.BindShader())
					success = true;
			}

			if (!success) {
				// SelectSpoutPanel("Shader bind error");
				return false;
			}
			else {
				// Set uniform locations to -1 so that they are only used if necessary
				m_timeLocation				 = -1;
				m_channeltimeLocation		 = -1;
				m_mouseLocation				 = -1;
				m_mouseLocationVec4			 = -1;
				m_dateLocation				 = -1;
				m_resolutionLocation		 = -1;
				m_channelresolutionLocation  = -1;
				m_inputTextureLocation		 = -1;
				m_inputTextureLocation1		 = -1;
				m_inputTextureLocation2		 = -1;
				m_inputTextureLocation3		 = -1;
				m_screenLocation			 = -1;
				m_surfaceSizeLocation		 = -1;
				// m_surfacePositionLocation	= -1; // TODO
				// m_vertexPositionLocation    = -1; // TODO

				// Extras
				// Input colour is linked to the user controls Red, Green, Blue, Alpha
				m_inputColourLocation        = -1;


				// lookup the "location" of each uniform

				//
				// GLSL Sandbox
				//
				// Normalized mouse position. Components of this vector are always between 0.0 and 1.0.
				//	uniform vec2 mouse;
				// Screen (Viewport) resolution.
				//	uniform vec2 resolution;
				// Used for mouse left drag currently
				//	uniform vec2 surfaceSize;
				//  TODO uniform vec2 surfacePosition;

				// Input textures do not appear to be in the GLSL Sandbox spec
				// but are allowed for here

				// From source of index.html on GitHub
				if(m_inputTextureLocation < 0)
					m_inputTextureLocation = m_shader.FindUniform("texture");

				// Preferred names tex0 and tex1 which are commonly used
				if(m_inputTextureLocation < 0)
					m_inputTextureLocation = m_shader.FindUniform("tex0");

				if(m_inputTextureLocation1 < 0)
					m_inputTextureLocation1 = m_shader.FindUniform("tex1");

				// TODO tex2 and tex3 ?

				// Backbuffer is not supported and is mapped to Texture unit 0
				// From source of index.html on GitHub
				// https://github.com/mrdoob/glsl-sandbox/blob/master/static/index.html
				if(m_inputTextureLocation < 0)
					m_inputTextureLocation = m_shader.FindUniform("backbuffer");

				// From several sources
				if(m_inputTextureLocation < 0)
					m_inputTextureLocation = m_shader.FindUniform("bbuff");

				// Time
				if(m_timeLocation < 0)
					m_timeLocation = m_shader.FindUniform("time");

				// Mouse move
				if(m_mouseLocation < 0)
					m_mouseLocation = m_shader.FindUniform("mouse");

				// Screen size
				if(m_screenLocation < 0) // Vec2
					m_screenLocation = m_shader.FindUniform("resolution"); 

				// Mouse left drag
				if(m_surfaceSizeLocation < 0)
					m_surfaceSizeLocation = m_shader.FindUniform("surfaceSize");
				
				/*
				// TODO
				// surfacePosAttrib is the attribute, surfacePosition is the varying var
				m_surfacePositionLocation = m_shader.FindAttribute("surfacePosAttrib"); 
				if(m_surfacePositionLocation < 0) printf("surfacePosition attribute not found\n");
				if(m_surfacePositionLocation >= 0) {
					// enable the attribute
					m_extensions.glEnableVertexAttribArrayARB(m_surfacePositionLocation);
				}
				m_vertexPositionLocation = m_shader.FindAttribute("position");
				if(m_vertexPositionLocation < 0) printf("vertexPosition attribute not found\n");
				if(m_vertexPositionLocation >= 0) {
					// enable the attribute
					m_extensions.glEnableVertexAttribArrayARB(m_vertexPositionLocation);
				}
				*/

				//
				// Shadertoy
				//

				
				//
				// Texture inputs iChannelx
				//
				if(m_inputTextureLocation < 0)
					m_inputTextureLocation = m_shader.FindUniform("iChannel0");
				
				if(m_inputTextureLocation1 < 0)
					m_inputTextureLocation1 = m_shader.FindUniform("iChannel1");

				if(m_inputTextureLocation2 < 0)
					m_inputTextureLocation2 = m_shader.FindUniform("iChannel2");

				if(m_inputTextureLocation3 < 0)
					m_inputTextureLocation3 = m_shader.FindUniform("iChannel3");

				// iResolution
				if(m_resolutionLocation < 0) // Vec3
					m_resolutionLocation = m_shader.FindUniform("iResolution");

				// iMouse
				if(m_mouseLocationVec4 < 0) // Shadertoy is Vec4
					m_mouseLocationVec4 = m_shader.FindUniform("iMouse");

				// iGlobalTime
				if(m_timeLocation < 0)
					m_timeLocation = m_shader.FindUniform("iGlobalTime");

				// iDate
				if(m_dateLocation < 0)
					m_dateLocation = m_shader.FindUniform("iDate");

				// iChannelTime
				if(m_channeltimeLocation < 0)
					m_channeltimeLocation = m_shader.FindUniform("iChannelTime[4]");
				if(m_channeltimeLocation < 0)
					m_channeltimeLocation = m_shader.FindUniform("iChannelTime[0]");
				if(m_channeltimeLocation < 0)
					m_channeltimeLocation = m_shader.FindUniform("iChannelTime[1]");
				if(m_channeltimeLocation < 0)
					m_channeltimeLocation = m_shader.FindUniform("iChannelTime[2]");
				if(m_channeltimeLocation < 0)
					m_channeltimeLocation = m_shader.FindUniform("iChannelTime[3]");

				// iChannelResolution
				if(m_channelresolutionLocation < 0) // Vec3 width, height, depth * 4
					m_channelresolutionLocation = m_shader.FindUniform("iChannelResolution[4]");
				if(m_channelresolutionLocation < 0)
					m_channelresolutionLocation = m_shader.FindUniform("iChannelResolution[0]");
				if(m_channelresolutionLocation < 0)
					m_channelresolutionLocation = m_shader.FindUniform("iChannelResolution[1]");
				if(m_channelresolutionLocation < 0)
					m_channelresolutionLocation = m_shader.FindUniform("iChannelResolution[2]");
				if(m_channelresolutionLocation < 0)
					m_channelresolutionLocation = m_shader.FindUniform("iChannelResolution[3]");

				// inputColour - linked to user input
				if(m_inputColourLocation < 0)
					m_inputColourLocation = m_shader.FindUniform("inputColour");

				m_shader.UnbindShader();

				// Delete the local texture because it might be a different size
				if(m_glTexture0 > 0) glDeleteTextures(1, &m_glTexture0);
				if(m_glTexture1 > 0) glDeleteTextures(1, &m_glTexture1);
				if(m_glTexture2 > 0) glDeleteTextures(1, &m_glTexture2);
				if(m_glTexture3 > 0) glDeleteTextures(1, &m_glTexture3);
				m_glTexture0 = 0;
				m_glTexture1 = 0;
				m_glTexture2 = 0;
				m_glTexture3 = 0;

				// Start the clock again to start from zero
				StartCounter();

				// printf("shader Loaded OK\n");

				return true;

			} // bind shader OK
		} // compile shader OK
		// =============================================

		// printf("shader Load failed\n");

		return false;
}

void ShaderMaker::StartCounter()
{

#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32__))
    LARGE_INTEGER li;
	// Find frequency
    QueryPerformanceFrequency(&li);
    PCFreq = double(li.QuadPart)/1000.0;
	// Second call needed
    QueryPerformanceCounter(&li);
    CounterStart = li.QuadPart;
#else
    // posix c++11
    start = std::chrono::steady_clock::now();
#endif

}

double ShaderMaker::GetCounter()
{

#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32__))
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return double(li.QuadPart-CounterStart)/PCFreq;
#else
    // posix c++11
    end = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()/1000.;
#endif
    return 0;
}

void ShaderMaker::CreateRectangleTexture(FFGLTextureStruct Texture, FFGLTexCoords maxCoords, GLuint &glTexture, GLenum texunit, GLuint &fbo, GLuint hostFbo)
{
	// First create an fbo and a texture the same size if they don't exist
	if(fbo == 0) {
		m_extensions.glGenFramebuffersEXT(1, &fbo); 
	}

	if(glTexture == 0) {
		glGenTextures(1, &glTexture);
		m_extensions.glActiveTexture(texunit);
		glBindTexture(GL_TEXTURE_2D, glTexture);
		glTexImage2D(GL_TEXTURE_2D, 0,  GL_RGBA, Texture.Width, Texture.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D, 0);
		m_extensions.glActiveTexture(GL_TEXTURE0);
	} // endif created a new texture
				
	// Render the incoming texture to the local one via the fbo
	m_extensions.glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
	m_extensions.glFramebufferTexture2DEXT(GL_READ_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, glTexture, 0);
	glBindTexture(GL_TEXTURE_2D, Texture.Handle);
				
	glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);
	//
	// Must refer to maxCoords here because the texture
	// could be smaller than the hardware size containing it
	//
	//lower left
	glTexCoord2f(0.0, 0.0);
	glVertex2f(-1.0, -1.0);
	//upper left
	glTexCoord2f(0.0, (float)maxCoords.t);
	glVertex2f(-1.0, 1.0);
	// upper right
	glTexCoord2f((float)maxCoords.s, (float)maxCoords.t);
	glVertex2f(1.0, 1.0);
	//lower right
	glTexCoord2f((float)maxCoords.s, 0.0);
	glVertex2f(1.0, -1.0);
	glEnd();
	glDisable(GL_TEXTURE_2D);

	// unbind the input texture
	glBindTexture(GL_TEXTURE_2D, 0);

	// unbind the fbo
	if(hostFbo)
		m_extensions.glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, hostFbo);
	else
		m_extensions.glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

}

