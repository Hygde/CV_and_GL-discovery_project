/*!\file basic.fs
 *
 * \brief rendu avec lumière directionnelle diffuse et couleur.
 * \author Farès BELHADJ, amsi@ai.univ-paris8.fr 
 * \date April 15 2016
 */
#version 330
uniform mat4 modelViewMatrix;
uniform sampler2D tex;
uniform sampler2D hat;
uniform sampler2D mustache;

uniform int totext;
uniform vec4 couleur;
uniform vec4 lumPos;
in vec2 vsoTexCoord;
in vec3 vsoNormal;
in vec3 vsoPosition;
out vec4 fragColor;

void main(void) {  
  if(totext == 0) {
	fragColor = texture(hat, vsoTexCoord);
  }
  if(totext == 1){
  	fragColor = texture(mustache, vsoTexCoord);
  }
  if(totext == 2){
  	fragColor = texture(tex, vsoTexCoord);
  }
}
