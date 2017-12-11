#version 330

in vec2 vertexPosition;
in vec4 vertexColor;
in vec2 vertexUV;

out vec2 fragmentPosition;
out vec4 fragmentColor;
out vec2 fragmentUV;

uniform mat4 transformationMatrix;


void main() {
   
    gl_Position = transformationMatrix * vec4(vertexPosition, 0.0, 1.0);
   
    fragmentPosition = vertexPosition;
    fragmentColor = vertexColor;
	fragmentUV = vec2(vertexUV.x,1-vertexUV.y);

}