precision mediump float;

// structs ========================================================
struct Sphere {
    vec3 centre;
    float radius;
    vec3 colour;
};

struct Plane {
    vec3 point;
    vec3 normal;
    vec3 colour;
};

struct Ray {
    vec3 origin;
    vec3 dir;
};

struct Intersection {
    float t;
    bool hit;
    vec3 pos;
    vec3 normal;
    vec3 colour;
};

Intersection initIntersection() {
    Intersection i;
    i.pos = vec3(0,0,0);
    i.normal = vec3(0,0,0);
    i.colour = vec3(0,0,0);
    i.hit = false;
    i.t = -1.0;
    return i;
}

// global declarations ============================================
varying vec3 origin;
varying vec3 dir;

#define PI 3.1415926589793238462643383
const float EPSILON = 1e-4;
const float MAX_DIST = 10000.0;
const int MAXIMUM_RAY_DEPTH = 42;
const int NUM_SPHERES = 6;

uniform mat4 mMatrix;
uniform mat4 vMatrix;

// uniform vec3 lightPosition; // to be controlled by user
const int NUM_LIGHTSOURCES = 9;
vec3 lightPosition[NUM_LIGHTSOURCES];

vec4 ambient = vec4(vec3(0.1), 1.);
vec4 specular = vec4(1.);
float checkerSpacing = 3.;
float falloff = 0.6; //reflection falloff

float k_a = 0.4; //ambient coefficient
float k_d = 0.9; //diffuse coefficient
float k_s = 0.2; //specular coefficient
float s = 600.; //distance heuristic constant
float shininess = 15.; //specular exponent
float phi = 5000.; //light intensity

// SPHERES AND PLANES DECLARATION
Sphere sphere[NUM_SPHERES];
Plane plane;

// functions ======================================================
bool castShadowRay(Intersection intersect);

void initializeLightPosition(float distribution) {
    lightPosition[0] = vec3(6,6,4);
    for (int i=1; i < NUM_LIGHTSOURCES; ++i) {
        lightPosition[i] = lightPosition[i-1] + distribution;
    }
} // light sources in a diagonal line instead of an area for simplicity

float rnd() {
  int seed = int(mod(origin.x*1123.0+619.0, 420.0));
  return 1./(float(seed)/420.0);
}

vec3 hitPoint(vec3 origin, vec3 direction, float t) {
    return origin + t * direction;
}

vec3 checkerPattern(vec3 pt, vec3 c) {
    float chessboard = floor(pt.x*checkerSpacing) + floor(pt.z*checkerSpacing);
    chessboard = (chessboard * 0.5)-floor(chessboard*0.5);
    if (mod(chessboard, checkerSpacing) == 0.0) {
        return c;
    }
    else return c*0.5;
}

//DIRECT ILLUMINATION
vec3 directIllumination(Intersection i, Ray ray) {
    // soft shadow
    vec3 lightDir[NUM_LIGHTSOURCES];
    float distance_av = 0.;
    for (int j=0; j < NUM_LIGHTSOURCES; j++) {
        lightDir[j] = vec3(0.);
        lightDir[j] += lightPosition[j] - i.pos;

        distance_av += length(lightDir[j]) / float(NUM_LIGHTSOURCES);

        lightDir[j] = normalize(lightDir[j]);
    }

    //ambient ----
    vec3 ambient_component = k_a * ambient.xyz;
    //diffuse ----
    vec3 diffuse_component = vec3(0,0,0);
    for (int j=0; j < NUM_LIGHTSOURCES; j++) {
        if (!castShadowRay(i)) {
            float diff_val = k_d * max(dot(i.normal, lightDir[j]), 0.0);
            diffuse_component += i.colour * diff_val/float(NUM_LIGHTSOURCES);
        }
    }
    
    //specular ----
    vec3 reflect = reflect(lightDir[0], i.normal);
    float spec_val = k_s * pow((max(dot(normalize(ray.dir), reflect), 0.0)), shininess);
    vec3 specular_component = specular.xyz * spec_val;

    float d_a = phi / (4.0 * PI * (distance_av + s)); // s is heuristic constant
    return (ambient_component + diffuse_component + specular_component) * d_a;
}

// Intersect sphere from lecture notes
Intersection sphereIntersect(Ray ray, Sphere s) {
    Intersection i = initIntersection();

    vec3 a = ray.origin - s.centre;
    float b = dot(ray.dir, a); // d dot deltap    
    float d = pow(dot(ray.dir, a), 2.0) - pow(length(a), 2.0) + (s.radius * s.radius);

    if(d > 0.0) {
        float t = -b - sqrt(d); // minimum intersect with the sphere
        if (t > 0.0){
            i.hit = true;
            i.pos = hitPoint(ray.origin, ray.dir, t);
            i.normal = normalize(i.pos - s.centre);

            // rainbow colored spheres
            vec3 rainbow = vec3(0.);
            for (int j = 0; j < 100; j++) {
                rainbow += i.normal*rnd() / 400.;
            }
            rainbow *= s.colour * 3.;

            i.colour = rainbow;
            i.t = t;
        }
    }
    return i;
}

Intersection reflectAllSpheres(Ray ray) {
    Intersection result = initIntersection();

    for (int i = 0; i < NUM_SPHERES; i++) {
         Intersection currentI = sphereIntersect(ray, sphere[i]);
         if (currentI.hit)
             if (!result.hit || currentI.t < result.t) 
                result = currentI;
    }
    return result;
}

// intersect plane from lecture notes
Intersection planeIntersect(Ray ray) {
    Intersection result = initIntersection();

    float denom = dot(ray.dir, plane.normal);

    vec3 a = ray.origin - plane.point;
    float t = -1.0 * (dot(plane.normal, a) / denom);
    
    if (t > 0.0 && t < MAX_DIST) {
        result.hit = true;
        result.normal = plane.normal;
        result.pos = hitPoint(ray.origin,ray.dir, t);
        result.t = t;
        result.colour = checkerPattern(result.pos, plane.colour);
    }
    return result;
}

// Intersection with all objects
Intersection intersectAllObjects(Ray ray) {
    Intersection i_sphere = reflectAllSpheres(ray);
    Intersection i_plane = planeIntersect(ray);

    if (!i_plane.hit){
        return i_sphere;
    } 
    if (!i_sphere.hit) {
        return i_plane;
    }
    if (i_plane.t < i_sphere.t) {
        return i_plane;
    }
    return i_sphere;
}

int seed = 0;
float rng() {
  seed = int(mod(float(seed)*1364.0+626.0, 509.0));
  return 1./(float(seed)/509.0);
}

bool castShadowRay(Intersection intersect) {
    // use of rng for soft shadowing
    vec3 direction = normalize(lightPosition[0]*rng() - intersect.pos*rng());
    Ray shadowRay;
    shadowRay.origin = intersect.pos + intersect.normal * EPSILON;
    shadowRay.dir = direction;

    Intersection shadowIntersect = intersectAllObjects(shadowRay);
   
    if (shadowIntersect.hit) return true;
    return false;
}

vec3 rayTrace(Ray ray) {
    float weight = 1.0;
    vec3 totalColor = vec3(0.0, 0.0, 0.0); //accumulated light intensity
    for (int i = 0; i < MAXIMUM_RAY_DEPTH; ++i) {
        Intersection intersection = intersectAllObjects(ray);
        if (!intersection.hit) break;

        totalColor += directIllumination(intersection, ray) * weight;
        weight *= falloff; // pow(falloff, float(i));
        vec3 newDir = normalize(reflect(ray.dir, intersection.normal));
        Ray reflectedRay;
        reflectedRay.origin = intersection.pos + intersection.normal*EPSILON;
        reflectedRay.dir = newDir;
        
        ray = reflectedRay;
    }
    return totalColor;
} 

vec3 fog(vec3 colour, float dist) {
    vec3 fog_color = vec3(0.2,0.2,0.4); // adding a purple fog
    float k_fog = 0.1;
    float fogAmount = 1. - exp(-dist * k_fog);

    if (dist < 0.0) return fog_color;
    return mix(colour, fog_color, fogAmount);
}

// MAIN ========================================================
void main() {
    // scene definition
    sphere[0].centre = vec3(-2.0, 1.5, -3.5);
    sphere[0].radius = 1.5;
    sphere[0].colour = vec3(0.8,0.8,0.8);
    sphere[1].centre = vec3(-0.5, 0.0, -2.0);
    sphere[1].radius = 0.6;
    sphere[1].colour = vec3(0.3,0.8,0.3);
    sphere[2].centre = vec3(1.0, 0.7, -2.2);
    sphere[2].radius = 0.8;
    sphere[2].colour = vec3(0.3,0.8,0.8);
    sphere[3].centre = vec3(0.7, -0.3, -1.2);
    sphere[3].radius = 0.2;
    sphere[3].colour = vec3(0.8,0.8,0.3);
    sphere[4].centre = vec3(-0.7, -0.3, -1.2);
    sphere[4].radius = 0.2;
    sphere[4].colour = vec3(0.8,0.3,0.3);
    sphere[5].centre = vec3(0.2, -0.2, -1.2);
    sphere[5].radius = 0.3;
    sphere[5].colour = vec3(0.8,0.3,0.8);

    plane.point = vec3(0.0, -0.5, 0.0);
    plane.normal = vec3(0, 1.0, 0);
    plane.colour = vec3(1, 1, 1);

    Ray ray;
    ray.dir = normalize((mMatrix* vec4(dir, 0.)).xyz);
    ray.origin = origin;
    // scene definition end

    // Soft shadows
    initializeLightPosition(0.1); // 0.1 spacing between each of the light sources

    //RAYTRACE
    vec3 finalColor = rayTrace(ray);

    // fog
    Intersection i = intersectAllObjects(ray);
    float dist = -1.;
    if(i.hit) {
        dist = length(hitPoint(ray.origin, ray.dir, i.t) - ray.origin);
    }

    finalColor = fog(finalColor, dist);
    gl_FragColor = vec4(finalColor,1.);
}