// The MIT License
// Copyright © 2013 Inigo Quilez
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


// A list of useful distance function to simple primitives. All
// these functions (except for ellipsoid) return an exact
// euclidean distance, meaning they produce a better SDF than
// what you'd get if you were constructing them from boolean
// operations.
//
// More info here:
//
// http://www.iquilezles.org/www/articles/distfunctions/distfunctions.htm

#define HW_PERFORMANCE 0

#if HW_PERFORMANCE==0
#define AA 1
#else
#define AA 2   // make this 2 or 3 for antialiasing
#endif

#include "interface.h"
DRAW_IMAGE(draw_image);

#define MAIN_WINDOW_WIDTH  400
#define MAIN_WINDOW_HEIGHT 300
#include "main.cpp"

#include "../libberdip/drawing.cpp"

struct BasicState
{
    RandomSeriesPCG randomizer;
    f32 seconds;
    u32 ticks;
    u32 prevMouseDown;
};

global u64 gMapCalls = 0;

//------------------------------------------------------------------

internal f32
smoothstep(f32 min, f32 t, f32 max)
{
    f32 x = clamp01((t - min) / (max - min));
    f32 result = x * x * (3.0f - 2.0f * x);
    return result;
}

internal v2
fraction(v2 v)
{
    v2 result;
    result.x = fraction(v.x);
    result.y = fraction(v.y);
    return result;
}

internal v2
operator /(v2 a, v2 b)
{
    v2 result;
    result.x = a.x / b.x;
    result.y = a.y / b.y;
    return result;
}

internal v3
operator /(v3 a, v3 b)
{
    v3 result;
    result.x = a.x / b.x;
    result.y = a.y / b.y;
    result.z = a.z / b.z;
    return result;
}

internal v3
operator /(f32 a, v3 b)
{
    v3 result;
    result.x = a / b.x;
    result.y = a / b.y;
    result.z = a / b.z;
    return result;
}

internal v3
reflect(v3 i, v3 n)
{
    v3 result;
    result = i - 2.0f * dot(n, i) * n;
    return result;
}

internal v2
vec_xz(v3 v)
{
    v2 result;
    result.x = v.x;
    result.y = v.z;
    return result;
}

internal f32
sdPlane(v3 p)
{
	return p.y;
}

internal f32
sdSphere( v3 p, f32 s )
{
    return length(p)-s;
}

internal f32
sdBox( v3 p, v3 b )
{
    v3 d = absolute(p) - b;
    return minimum(maximum3(d.x,d.y,d.z),0.0f) + length(V3(maximum(d.x,0.0f), maximum(d.y, 0.0f), maximum(d.z, 0.0f)));
}

internal f32
sdEllipsoid( v3 p, v3 r ) // approximated
{
    f32 k0 = length(p/r);
    f32 k1 = length(p/hadamard(r,r));
    return k0*(k0-1.0f)/k1;
    
}

internal f32
sdTorus( v3 p, v2 t )
{
    return length( V2(length(vec_xz(p))-t.x,p.y) )-t.y;
}

internal f32
sdCappedTorus(v3 p, v2 sc, f32 ra, f32 rb)
{
    p.x = absolute(p.x);
    f32 k = (sc.y*p.x>sc.x*p.y) ? dot(p.xy,sc) : length(p.xy);
    return square_root( dot(p,p) + ra*ra - 2.0f*ra*k ) - rb;
}

internal f32
sdHexPrism( v3 p, v2 h )
{
    v3 q = absolute(p);
    
    const v3 k = V3(-0.8660254f, 0.5f, 0.57735f);
    p = absolute(p);
    p.xy -= 2.0f*minimum(dot(k.xy, p.xy), 0.0f)*k.xy;
    v2 d = V2(
              length(p.xy - V2(clamp(-k.z*h.x, p.x, k.z*h.x), h.x))*sign_of(p.y - h.x),
              p.z-h.y );
    return minimum(maximum(d.x,d.y),0.0f) + length(V2(maximum(d.x,0.0f), maximum(d.y, 0.0f)));
}

internal f32
sdCapsule( v3 p, v3 a, v3 b, f32 r )
{
	v3 pa = p-a, ba = b-a;
	f32 h = clamp01( dot(pa,ba)/dot(ba,ba));
	return length( pa - ba*h ) - r;
}

internal f32
sdRoundCone( v3 p, f32 r1, f32 r2, f32 h )
{
    v2 q = V2( length(vec_xz(p)), p.y );
    
    f32 b = (r1-r2)/h;
    f32 a = square_root(1.0f-b*b);
    f32 k = dot(q,V2(-b,a));
    
    if( k < 0.0f ) return length(q) - r1;
    if( k > a*h ) return length(q-V2(0.0f,h)) - r2;
    
    return dot(q, V2(a,b) ) - r1;
}

internal f32
sdRoundCone(v3 p, v3 a, v3 b, f32 r1, f32 r2)
{
    // sampling independent computations (only depend on shape)
    v3  ba = b - a;
    f32 l2 = dot(ba,ba);
    f32 rr = r1 - r2;
    f32 a2 = l2 - rr*rr;
    f32 il2 = 1.0f/l2;
    
    // sampling dependant computations
    v3 pa = p - a;
    f32 y = dot(pa,ba);
    f32 z = y - l2;
    f32 x2 = length_squared( pa*l2 - ba*y );
    f32 y2 = y*y*l2;
    f32 z2 = z*z*l2;
    
    // single square root!
    f32 k = sign_of(rr)*rr*rr*x2;
    if( sign_of(z)*a2*z2 > k ) return  square_root(x2 + z2)        *il2 - r2;
    if( sign_of(y)*a2*y2 < k ) return  square_root(x2 + y2)        *il2 - r1;
    return (square_root(x2*a2*il2)+y*rr)*il2 - r1;
}



internal f32
sdTriPrism( v3 p, v2 h )
{
    const f32 k = square_root(3.0f);
    h.x *= 0.5f*k;
    p.xy /= h.x;
    p.x = absolute(p.x) - 1.0f;
    p.y = p.y + 1.0f/k;
    if( p.x+k*p.y>0.0f ) p.xy=V2(p.x-k*p.y,-k*p.x-p.y)/2.0f;
    p.x -= clamp(-2.0f, p.x, 0.0f );
    f32 d1 = length(p.xy)*sign_of(-p.y)*h.x;
    f32 d2 = absolute(p.z)-h.y;
    return length(V2(maximum(d1, 0.0f), maximum(d2,0.0f))) + minimum(maximum(d1,d2), 0.0f);
}

// vertical
internal f32
sdCylinder( v3 p, v2 h )
{
    v2 d = absolute(V2(length(vec_xz(p)),p.y)) - h;
    return minimum(maximum(d.x,d.y),0.0f) + length(V2(maximum(d.x, 0.0f), maximum(d.y, 0.0f)));
}

// arbitrary orientation
internal f32
sdCylinder(v3 p, v3 a, v3 b, f32 r)
{
    v3 pa = p - a;
    v3 ba = b - a;
    f32 baba = dot(ba,ba);
    f32 paba = dot(pa,ba);
    
    f32 x = length(pa*baba-ba*paba) - r*baba;
    f32 y = absolute(paba-baba*0.5f)-baba*0.5f;
    f32 x2 = x*x;
    f32 y2 = y*y*baba;
    f32 d = (maximum(x,y)<0.0f)?-minimum(x2,y2):(((x>0.0f)?x2:0.0f)+((y>0.0f)?y2:0.0f));
    return sign_of(d)*square_root(absolute(d))/baba;
}

// vertical
internal f32
sdCone( v3 p, v3 c )
{
    v2 q = V2( length(vec_xz(p)), p.y );
    f32 d1 = -q.y-c.z;
    f32 d2 = maximum( dot(q,c.xy), q.y);
    return length(V2(maximum(d1, 0.0f), maximum(d2,0.0f))) + minimum(maximum(d1,d2), 0.0f);
}

internal f32
sdCone( v3 p, f32 h, f32 r1, f32 r2 )
{
    v2 q = V2( length(vec_xz(p)), p.y );
    
    v2 k1 = V2(r2,h);
    v2 k2 = V2(r2-r1,2.0f*h);
    v2 ca = V2(q.x-minimum(q.x,(q.y < 0.0f)?r1:r2), absolute(q.y)-h);
    v2 cb = q - k1 + k2*clamp01( dot(k1-q,k2)/length_squared(k2));
    f32 s = (cb.x < 0.0f && ca.y < 0.0f) ? -1.0f : 1.0f;
    return s*square_root( minimum(length_squared(ca),length_squared(cb)) );
}

// http://iquilezles.org/www/articles/distfunctions/distfunctions.htm
internal f32
sdCone(v3 p, v3 a, v3 b, f32 ra, f32 rb)
{
    f32 rba  = rb-ra;
    f32 baba = dot(b-a,b-a);
    f32 papa = dot(p-a,p-a);
    f32 paba = dot(p-a,b-a)/baba;
    
    f32 x = square_root( papa - paba*paba*baba );
    
    f32 cax = maximum(0.0f,x-((paba<0.5f)?ra:rb));
    f32 cay = absolute(paba-0.5f)-0.5f;
    
    f32 k = rba*rba + baba;
    f32 f = clamp01( (rba*(x-ra)+paba*baba)/k);
    
    f32 cbx = x-ra - f*rba;
    f32 cby = paba - f;
    
    f32 s = (cbx < 0.0f && cay < 0.0f) ? -1.0f : 1.0f;
    
    return s*square_root( minimum(cax*cax + cay*cay*baba,
                                  cbx*cbx + cby*cby*baba) );
}

// c is the sin/cos of the desired cone angle
internal f32
sdSolidAngle(v3 pos, v2 c, f32 ra)
{
    v2 p = V2( length(V2(pos.x, pos.z)), pos.y );
    f32 l = length(p) - ra;
	f32 m = length(p - c*clamp(0.0f, dot(p,c),ra) );
    return maximum(l,m*sign_of(c.y*p.x-c.x*p.y));
}

internal f32
sdOctahedron(v3 p, f32 s)
{
    p = absolute(p);
    f32 m = p.x + p.y + p.z - s;
    
    // exact distance
#if 0
    v3 o = min(3.0f*p - m, 0.0f);
    o = max(6.0f*p - m*2.0f - o*3.0f + (o.x+o.y+o.z), 0.0f);
    return length(p - s*o/(o.x+o.y+o.z));
#endif
    
    // exact distance
#if 1
    v3 q;
    if( 3.0f*p.x < m ) q = p;
    else if( 3.0f*p.y < m ) q = V3(p.y, p.z, p.x);
    else if( 3.0f*p.z < m ) q = V3(p.z, p.x, p.y);
    else return m*0.57735027f;
    f32 k = clamp(0.0f, 0.5f*(q.z-q.y+s),s);
    return length(V3(q.x,q.y-s+k,q.z-k));
#endif
    
    // bound, not exact
#if 0
	return m*0.57735027f;
#endif
}

internal f32
sdPyramid( v3 p, f32 h )
{
    f32 m2 = h*h + 0.25f;
    
    // symmetry
    p.x = absolute(p.x);
    p.z = absolute(p.z);
    if (p.z > p.x) {
        f32 temp = p.x;
        p.x = p.z;
        p.z = temp;
    }
    p.x -= 0.5f;
    p.z -= 0.5f;
	
    // project into face plane (2D)
    v3 q = V3( p.z, h*p.y - 0.5f*p.x, h*p.x + 0.5f*p.y);
    
    f32 s = maximum(-q.x,0.0f);
    f32 t = clamp01( (q.y-0.5f*p.z)/(m2+0.25f));
    
    f32 a = m2*(q.x+s)*(q.x+s) + q.y*q.y;
	f32 b = m2*(q.x+0.5f*t)*(q.x+0.5f*t) + (q.y-m2*t)*(q.y-m2*t);
    
    f32 d2 = minimum(q.y,-q.x*m2-q.y*0.5f) > 0.0f ? 0.0f : minimum(a,b);
    
    // recover 3D and scale, and add sign
    return square_root( (d2+q.z*q.z)/m2 ) * sign_of(maximum(q.z,-p.y));;
}

internal f32
ndot(v2 a, v2 b )
{
    return a.x*b.x - a.y*b.y;
}

// la,lb=semi axis, h=height, ra=corner
internal f32
sdRhombus(v3 p, f32 la, f32 lb, f32 h, f32 ra)
{
    p = absolute(p);
    v2 b = V2(la,lb);
    f32 f = clamp(-1.0f, (ndot(b,b-2.0f*vec_xz(p)))/dot(b,b), 1.0f );
	v2 q = V2(length(vec_xz(p)-0.5f*hadamard(b,V2(1.0f-f,1.0f+f)))*sign_of(p.x*b.y+p.z*b.x-b.x*b.y)-ra, p.y-h);
    return minimum(maximum(q.x,q.y),0.0f) + length(V2(maximum(q.x,0.0f), maximum(q.y, 0.0f)));
}


//------------------------------------------------------------------

internal v2
opU( v2 d1, v2 d2 )
{
	return (d1.x<d2.x) ? d1 : d2;
}

//------------------------------------------------------------------

//------------------------------------------------------------------

internal v2
map( v3 pos )
{
    ++gMapCalls;
    
    v2 res = V2( 1.0e10f, 0.0f);
    
    if( pos.x>-2.5f && pos.x<0.5f )
    {
        res = opU( res, V2( sdPyramid( 2.5f*(pos-V3(-1.0f,0.15f,-3.0f)), 1.1f )/2.5f, 13.56f ) );
        res = opU( res, V2( sdOctahedron( pos-V3(-1.0f,0.15f,-2.0f), 0.35f ), 23.56f ) );
        res = opU( res, V2( sdTriPrism(  pos-V3(-1.0f,0.25f,-1.0f), V2(0.25f,0.05f) ),43.5f ) );
        res = opU( res, V2( sdHexPrism(  pos-V3(-1.0f,0.20f, 1.0f), V2(0.25f,0.05f) ),17.0f ) );
        res = opU( res, V2( sdEllipsoid( pos-V3(-1.0f,0.30f, 0.0f), V3(0.2f, 0.25f, 0.05f) ), 43.17f ) );
    }
    if( pos.x>-1.5f && pos.x<1.5f )
    {
        res = opU( res, V2( sdSphere(    pos-V3( 0.0f,0.25f, 0.0f), 0.25f ), 46.9f ) );
        res = opU( res, V2( sdTorus(     pos-V3( 0.0f,0.25f, 1.0f), V2(0.2f,0.05f) ), 25.0f ) );
        res = opU( res, V2( sdCone(      pos-V3( 0.0f,0.50f,-1.0f), V3(0.8f,0.6f,0.3f) ), 55.0f ) );
        res = opU( res, V2( sdCone(      pos-V3( 0.0f,0.35f,-2.0f), 0.15f, 0.2f, 0.1f ), 13.67f ) );
        res = opU( res, V2( sdSolidAngle(pos-V3( 0.0f,0.20f,-3.0f), V2(3,4)/5.0f, 0.4f ), 49.13f ) );
    }
    if( pos.x>-0.5f && pos.x<2.5f )
    {
        v3 pCapTor = pos - V3( 1.0f,0.20f, 1.0f);
        res = opU( res, V2( sdCappedTorus(V3(pCapTor.x, pCapTor.z, pCapTor.y), V2(0.866025f,-0.5f), 0.2f, 0.05f), 8.5f) );
        res = opU( res, V2( sdBox(       pos-V3( 1.0f,0.25f, 0.0f), V3(0.25f, 0.25f, 0.25f) ), 3.0f ) );
        res = opU( res, V2( sdCapsule(   pos-V3( 1.0f,0.00f,-1.0f),V3(-0.1f,0.1f,-0.1f), V3(0.2f,0.4f,0.2f), 0.1f  ), 31.9f ) );
        res = opU( res, V2( sdCylinder(  pos-V3( 1.0f,0.30f,-2.0f), V2(0.1f,0.2f) ), 8.0f ) );
        v3 pRhombus = pos-V3( 1.0f,0.40f,-3.0f);
        res = opU( res, V2( sdRhombus( V3(pRhombus.x, pRhombus.z, pRhombus.y), 0.15f, 0.25f, 0.04f, 0.08f ), 18.4f ) );
    }
    if( pos.x>0.5f )
    {
        res = opU( res, V2( sdCylinder(  pos-V3( 2.0f,0.20f,-2.0f), V3(0.1f,-0.1f,0.0f), V3(-0.1f,0.3f,0.1f), 0.08f), 31.2f ) );
        res = opU( res, V2( sdCone(      pos-V3( 2.0f,0.20f,-1.0f), V3(0.1f,0.0f,0.0f), V3(-0.1f,0.3f,0.1f), 0.15f, 0.05f), 46.1f ) );
        res = opU( res, V2( sdRoundCone( pos-V3( 2.0f,0.20f, 0.0f), V3(0.1f,0.0f,0.0f), V3(-0.1f,0.3f,0.1f), 0.15f, 0.05f), 51.7f ) );
        res = opU( res, V2( sdRoundCone( pos-V3( 2.0f,0.20f, 1.0f), 0.2f, 0.1f, 0.3f ), 37.0f ) );
    }
    
    
	//res = min( res, sdBox(pos-v3(0.5,0.4,-0.5), v3(2.0,0.41,2.0) ) );
    return res;
}

// http://iquilezles.org/www/articles/boxfunctions/boxfunctions.htm
internal v2
iBox( v3 ro, v3 rd, v3 rad )
{
    v3 m = 1.0f/rd;
    v3 n = hadamard(m, ro);
    v3 k = hadamard(absolute(m),rad);
    v3 t1 = -n - k;
    v3 t2 = -n + k;
	return V2(maximum3( t1.x, t1.y, t1.z ),
              minimum3( t2.x, t2.y, t2.z ));
}

global const f32 maxHei = 0.8f;

internal v2
castRay( v3 ro, v3 rd )
{
    v2 res = V2(-1.0f,-1.0f);
    
    f32 tmin = 1.0f;
    f32 tmax = 10.0f;
    
    // raytrace floor plane
    f32 tp1 = (0.0f-ro.y)/rd.y;
    if( tp1>0.0f )
    {
        tmax = minimum( tmax, tp1 );
        res = V2( tp1, 1.0f );
    }
    //else return res;
    
    // raymarch primitives
    v2 tb = iBox( ro-V3(0.5f,0.4f,-0.5f), rd, V3(2.0f,0.41f,3.0f) );
    if( tb.x<tb.y && tb.y>0.0f && tb.x<tmax)
    {
        tmin = maximum(tb.x,tmin);
        tmax = minimum(tb.y,tmax);
        
        f32 t = tmin;
        for( int i=0; i<70 && t<tmax; i++ )
        {
            v2 h = map( ro+rd*t );
            if( absolute(h.x)<(0.0001f*t) )
            {
                res = V2(t,h.y);
                break;
            }
            t += h.x;
        }
    }
    
    return res;
}


// http://iquilezles.org/www/articles/rmshadows/rmshadows.htm
internal f32
calcSoftshadow( v3 ro, v3 rd, f32 mint, f32 tmax )
{
    // bounding volume
    f32 tp = (maxHei-ro.y)/rd.y;
    if( tp>0.0f ) tmax = minimum( tmax, tp );
    
    f32 res = 1.0f;
    f32 t = mint;
    for( int i=0; i<16; i++ )
    {
		f32 h = map( ro + rd*t ).x;
        f32 s = clamp01(8.0f*h/t);
        res = minimum( res, s*s*(3.0f-2.0f*s) );
        t += clamp( 0.02f, h, 0.10f );
        if( res<0.005f || t>tmax ) break;
    }
    return clamp01( res );
}

// http://iquilezles.org/www/articles/normalsSDF/normalsSDF.htm
internal v3
calcNormal( v3 pos )
{
#if 0
    // inspired by tdhooper and klems - a way to prevent the compiler from inlining map() 4 times
    v3 n = V3(0,0,0);
    for( int i=0; i<4; i++ )
    {
        v3 e = 0.5773*(2.0*V3((((i+3)>>1)&1),((i>>1)&1),(i&1))-1.0);
        n += e*map(pos+0.0005*e).x;
    }
#else
    v3 ex = 0.5773f*(2.0f*V3(1, 0, 0) - 1.0f);
    v3 ey = 0.5773f*(2.0f*V3(0, 1, 0) - 1.0f);
    v3 ez = 0.5773f*(2.0f*V3(0, 0, 1) - 1.0f);
    
    v3 n = ex*map(pos+0.0005f*ex).x + ey*map(pos+0.0005f*ey).x + ez*map(pos+0.0005f*ez).x;
    
#endif
    return normalize_or_zero(n);
}

internal f32
calcAO( v3 pos, v3 nor )
{
	f32 occ = 0.0f;
    f32 sca = 1.0f;
    for ( int i=0; i<5; i++ )
    {
        f32 hr = 0.01f + 0.12f*(f32)i/4.0f;
        v3 aopos =  nor * hr + pos;
        f32 dd = map( aopos ).x;
        occ += -(dd-hr)*sca;
        sca *= 0.95f;
    }
    return clamp01( 1.0f - 3.0f*occ ) * (0.5f+0.5f*nor.y);
}

// http://iquilezles.org/www/articles/checkerfiltering/checkerfiltering.htm
internal f32
checkersGradBox( v2 p, v2 dpdx, v2 dpdy )
{
    // filter kernel
    v2 w = absolute(dpdx)+absolute(dpdy) + 0.001f;
    // analytical integral (box filter)
    v2 i = 2.0f*(absolute(fraction((p-0.5f*w)*0.5f)-0.5f)-absolute(fraction((p+0.5f*w)*0.5f)-0.5f))/w;
    // xor pattern
    return 0.5f - 0.5f*i.x*i.y;
}

internal v3
render( v3 ro, v3 rd, v3 rdx, v3 rdy )
{
    v3 col = V3(0.7f, 0.7f, 0.9f) - maximum(rd.y,0.0f)*0.3f;
    v2 res = castRay(ro,rd);
    f32 t = res.x;
	f32 m = res.y;
    if( m>-0.5f )
    {
        v3 pos = ro + t*rd;
        v3 nor = (m<1.5f) ? V3(0.0f,1.0f,0.0f) : calcNormal( pos );
        v3 ref = reflect( rd, nor );
        
        // material
		col = V3(0.2f, 0.2f, 0.2f) + 0.18f*V3(sin_pi(0.05f)*(m-1.0f),sin_pi(0.08f)*(m-1.0f),sin_pi(0.1f)*(m-1.0f) );
        //col = v3(0.2);
        col = V3(0.2f, 0.2f, 0.2f) + 0.18f*V3(sin_pi(m*2.0f + 0.0f),sin_pi(m*2.0f + 0.5f),sin_pi(m*2.0f + 1.0f));
        if( m<1.5f )
        {
            // project pixel footprint into the plane
            v3 dpdx = ro.y*(rd/rd.y-rdx/rdx.y);
            v3 dpdy = ro.y*(rd/rd.y-rdy/rdy.y);
            
            f32 f = checkersGradBox( 5.0f*vec_xz(pos), 5.0f*vec_xz(dpdx), 5.0f*vec_xz(dpdy) );
            col = V3(0.15f, 0.15f, 0.15f) + f*V3(0.05f, 0.05f, 0.05f);
        }
        
        // lighting
        f32 occ = calcAO( pos, nor );
		v3  lig = normalize_or_zero( V3(-0.5f, 0.4f, -0.6f) );
        v3  hal = normalize_or_zero( lig-rd );
		f32 amb = square_root(clamp01( 0.5f+0.5f*nor.y ));
        f32 dif = clamp01( dot( nor, lig ) );
        f32 bac = clamp01( dot( nor, normalize_or_zero(V3(-lig.x,0.0f,-lig.z))) )*clamp01( 1.0f-pos.y);
        f32 dom = smoothstep( -0.2f, 0.2f, ref.y );
        f32 fre = pow( clamp01(1.0f+dot(nor,rd)), 2.0f );
        
        dif *= calcSoftshadow( pos, lig, 0.02f, 2.5f );
        dom *= calcSoftshadow( pos, ref, 0.02f, 2.5f );
        
		f32 spe = pow( clamp01( dot( nor, hal ) ),16.0f)*
            dif *
            (0.04f + 0.96f*pow( clamp01(1.0f+dot(hal,rd)), 5.0f ));
        
		v3 lin = V3(0, 0, 0);
        lin += 3.80f*dif*V3(1.30f,1.00f,0.70f);
        lin += 0.55f*amb*V3(0.40f,0.60f,1.15f)*occ;
        lin += 0.85f*dom*V3(0.40f,0.60f,1.30f)*occ;
        lin += 0.55f*bac*V3(0.25f,0.25f,0.25f)*occ;
        lin += 0.25f*fre*V3(1.00f,1.00f,1.00f)*occ;
		col = hadamard(col,lin);
		col += 7.00f*spe*V3(1.10f,0.90f,0.70f);
        
        col = lerp( col, 1.0f-exp( -0.0001f*t*t*t ), V3(0.7f,0.7f,0.9f) );
    }
    
	return clamp01(col);
}

struct Camera
{
    v3 u;
    v3 v;
    v3 w;
};

internal Camera
setCamera( v3 ro, v3 ta, f32 cr )
{
    Camera result;
    result.w = normalize_or_zero(ta-ro);
	v3 cp = V3(sin_pi(cr), cos_pi(cr), 0.0f);
	result.u = normalize_or_zero(cross(result.w, cp));
    result.v = cross(result.u, result.w);
    return result;
}

internal v3
operator *(Camera c, v3 p)
{
    v3 result;
#if 1
    result.x = c.u.x * p.x + c.v.x * p.y + c.w.x * p.z;
    result.y = c.u.y * p.x + c.v.y * p.y + c.w.y * p.z;
    result.z = c.u.z * p.x + c.v.z * p.y + c.w.z * p.z;
#else
    result.x = dot(c.u, p);
    result.y = dot(c.v, p);
    result.z = dot(c.w, p);
#endif
    return result;
}

internal v4
mainImage(v2 fragCoord, v2 mouse, v2 screenSize, f32 timeAt)
{
    v2 mo = mouse / screenSize;
	f32 time = 15.0f + timeAt*1.5f;
    
    // camera
    v3 ta = V3( 0.5f, -0.4f, -0.5f );
    v3 ro = ta + V3( 4.5f*cos_pi(0.1f*time + 6.0f*mo.x), 1.0f + 2.0f*mo.y, 4.5f*sin_pi(0.1f*time + 6.0f*mo.x) );
    // camera-to-world transformation
    Camera ca = setCamera( ro, ta, 0.0f );
    
    f32 oneOverHeight = 1.0f / screenSize.y;
    
    v3 tot = V3(0,0,0);
#if AA>1
    for( int m=0; m<AA; m++ )
        for( int n=0; n<AA; n++ )
    {
        // pixel coordinates
        v2 o = V2((f32)m,(f32)n) / (f32)AA - 0.5f;
        v2 p = (2.0f*(fragCoord+o)-screenSize) * oneOverHeight;
#else
        v2 p = (2.0f*fragCoord-screenSize) * oneOverHeight;
#endif
        
        // ray direction
        v3 rd = ca * normalize_or_zero(V3(p, 2.5f));
        
        // ray differentials
        v2 px = (2.0f*(fragCoord+V2(1.0f,0.0f))-screenSize) * oneOverHeight;
        v2 py = (2.0f*(fragCoord+V2(0.0f,1.0f))-screenSize) * oneOverHeight;
        v3 rdx = ca * normalize_or_zero( V3(px,2.5f) );
        v3 rdy = ca * normalize_or_zero( V3(py,2.5f) );
        
        // render
        v3 col = render( ro, rd, rdx, rdy );
        
		// gamma
        //col = pow( col, V3(0.4545) );
        
        tot += col;
#if AA>1
    }
    tot /= (f32)(AA*AA);
#endif
    
    
    return V4( tot, 1.0 );
}

internal v2
normalized_screen(v2 screenP, v2 screenSize)
{
    f32 oneOverHeight = 1.0f / screenSize.y;
    v2 result = (2.0f * screenP - screenSize) * oneOverHeight;
    result.y = -result.y;
    return result;
}

DRAW_IMAGE(draw_image)
{
    i_expect(sizeof(BasicState) <= state->memorySize);
    
    v2 size = V2((f32)image->width, (f32)image->height);
    
    BasicState *basics = (BasicState *)state->memory;
    if (!state->initialized)
    {
        // basics->randomizer = random_seed_pcg(129301597412ULL, 1928649128658612912ULL);
        basics->randomizer = random_seed_pcg(time(0), 1928649128658612912ULL);
        
        state->initialized = true;
    }
    
    gMapCalls = 0;
    
    fill_rectangle(image, 0, 0, image->width, image->height, V4(0, 0, 0, 1));
    
    v3 cameraPos = V3(0, 0, -1);
    v3 cameraTarget = V3(0, 0, 0);
    f32 cameraPerspective = 2.0f;
    
    v3 cameraForward = normalize_or_zero(cameraTarget - cameraPos);
    v3 cameraRight = normalize_or_zero(cross(V3(0, 1, 0), cameraForward));
    v3 cameraUp = normalize_or_zero(cross(cameraForward, cameraRight));
    
    u32 *pixels = image->pixels;
    for (u32 y = 0; y < image->height; ++y)
    {
        u32 *pixelRow = pixels;
        for (u32 x = 0; x < image->width; ++x)
        {
            v2 at = V2((f32)x, (f32)y);
            at.y = size.y-at.y-1.0f;
            
            //v2 uv = normalized_screen(at, size);
            v4 colour = mainImage(at, 0.5f * size /*mouse.pixelPosition*/, size, basics->seconds);
            //v3 rayDir = get_camera_ray_dir(uv, cameraPerspective, cameraForward, cameraRight, cameraUp);
            
            //v3 colour = render(cameraPos, rayDir, basics->seconds);
            //draw_pixel(pixelRow++, sRGB_from_linear(V4(colour, 1.0f)));
            //draw_pixel(pixelRow++, V4(colour, 1.0f));
            draw_pixel(pixelRow++, sRGB_from_linear(colour));
        }
        pixels += image->width;
    }
    
    basics->prevMouseDown = mouse.mouseDowns;
    basics->seconds += dt;
    ++basics->ticks;
    if (basics->seconds > 60.0f)
    {
        basics->seconds -= 60.0f;
        fprintf(stdout, "Ticks: %4u | Time: %fms | Calls: %.0f\n", basics->ticks,
                60000.0f / (f32)basics->ticks, (f64)gMapCalls);
        basics->ticks = 0;
    }
}
