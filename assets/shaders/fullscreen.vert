// OpenGL ES 2.0 vertex shader
// No version line required on Vita; ES 2.0 by default.
attribute vec2 aPos;
attribute vec2 aUV;

varying vec2 vUV;

void main() {
    vUV = aUV;
    gl_Position = vec4(aPos, 0.0, 1.0);
}