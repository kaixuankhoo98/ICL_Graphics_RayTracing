// From the spec
attribute vec4 vertex_worldSpace;

uniform float canvasWidth;
uniform float canvasHeight;
uniform vec3 cameraPosition;
uniform mat3 cameraRotation;
uniform bool isOrthographicProjection;
uniform float orthographicFOV;
uniform float perspectiveFOV;

varying vec3 origin;
varying vec3 dir;

void main() {
    float aspectRatio = canvasWidth/canvasHeight;
    vec3 origin_camSpace, dir_camSpace;

    if (isOrthographicProjection) {
        origin_camSpace = vec3(vertex_worldSpace.x*orthographicFOV*aspectRatio,
                               vertex_worldSpace.y*orthographicFOV, 
                               0);
        dir_camSpace = vec3(0, 0, -1);
    }
    else { // perspective projection
        origin_camSpace = vec3(0);
        dir_camSpace = vec3(vertex_worldSpace.x*aspectRatio,
                            vertex_worldSpace.y,
                            -1.0/tan(radians(perspectiveFOV)));
    }

    origin = cameraPosition + cameraRotation*origin_camSpace;
    dir = normalize(cameraRotation*dir_camSpace);
    gl_Position = vertex_worldSpace;
}
