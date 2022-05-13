//vertex coordinates in world space for the render quad
attribute vec3 vertex_worldSpace;
//texture coordinate for this vertex and the render quad
attribute vec2 textureCoordinate_input;

//texture coordinate needs to be passed on to the R2T fragment shader
varying vec2 varyingTextureCoordinate;

uniform mat4 mMatrix;
//view Matrix
uniform mat4 vMatrix;
//projection Matrix
uniform mat4 pMatrix;

uniform float canvasWidth;
uniform float canvasHeight;

uniform vec3 cameraPosition;
uniform mat3 cameraRotation;

uniform bool isOrthographicProjection;
uniform float orthographicFOV;
uniform float perspectiveFOV;

varying vec3 origin;
varying vec3 dir;


//main program for each vertex of the render quad
void main() {

  float aspectRatio = canvasWidth/canvasHeight;
  vec3 origin_camSpace, dir_camSpace;

  if (isOrthographicProjection) {
    origin_camSpace = vec3( vertex_worldSpace.x*orthographicFOV*aspectRatio,
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
  gl_Position = vec4(vertex_worldSpace, 1.0);

  varyingTextureCoordinate = textureCoordinate_input;
}