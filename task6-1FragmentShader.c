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

// Intersection initialization
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
const int NUM_OF_SPHERES = 6;

uniform mat4 mMatrix;
uniform mat4 vMatrix;
uniform vec3 lightPosition; // to be controlled by user

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
Sphere sphere[NUM_OF_SPHERES];
Plane plane;

// functions ======================================================
bool castShadowRay(Intersection intersect);

vec3 hitPoint(vec3 origin, vec3 direction, float t) {
    return t * direction + origin;
}

vec3 checkerPattern(vec3 pt, vec3 c) {
    float chessboard = floor(pt.x*checkerSpacing) + floor(pt.z*checkerSpacing);
    chessboard = (chessboard * 0.5)-floor(chessboard*0.5);
    if (mod(chessboard, checkerSpacing) == 0.0) {
        return c;
    }
    else return c*0.5;
}

vec3 directIllumination(const Intersection i, const Ray ray) {
    vec3 lightDir = lightPosition - i.pos;
    float dist = length(lightDir);
    lightDir = normalize(lightDir);

    //ambient ----
    vec3 ambient_component = k_a * ambient.xyz;
    //diffuse ----
    vec3 diffuse_component = vec3(0,0,0);
    if (!castShadowRay(i)) {
        float diff_val = k_d * max(dot(i.normal, lightDir), 0.0);
        diffuse_component = i.colour * diff_val;
    }
    //specular ----
    vec3 reflect = reflect(lightDir, i.normal);
    float spec_val = k_s * pow((max(0.,dot(normalize(ray.dir), reflect))), shininess);
    vec3 specular_component = specular.xyz * spec_val;

    float d_a = phi / (4.0 * PI * (dist + s)); // s is heuristic constant
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
            i.colour = s.colour;
            i.t = t;
        }
    }
    return i;
}

Intersection reflectAllSpheres(Ray ray) {
    Intersection result = initIntersection();

    for (int i = 0; i < NUM_OF_SPHERES; i++) {
         Intersection currentI = sphereIntersect(ray, sphere[i]);
         if (currentI.hit)
             if (!result.hit || currentI.t < result.t) 
                result = currentI;
    }
    return result;
}

// Intersect plane from lecture notes
Intersection planeIntersect(Ray ray) {
    Intersection result = initIntersection();

    float d = dot(ray.dir, plane.normal);

    vec3 a = ray.origin - plane.point;
    float t = -1.0 * (dot(plane.normal, a) / d);
    
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

bool castShadowRay(Intersection intersect) {
    vec3 direction = normalize(lightPosition - intersect.pos);
    Ray shadowRay;
    shadowRay.origin = intersect.pos + intersect.normal * EPSILON;
    shadowRay.dir = direction;//normalize((mMatrix* vec4(lightPosition, 0.0)).xyz);

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

    //RAYTRACE
    vec3 finalColor = rayTrace(ray);

    gl_FragColor = vec4(finalColor,1.);
}