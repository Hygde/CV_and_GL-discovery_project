/*!\file basic.fs
 *
 * \brief rendu avec lumière directionnelle diffuse et couleur.
 * \author Farès BELHADJ, amsi@ai.univ-paris8.fr 
 * \date April 15 2016
 */
#version 330
uniform mat4 modelViewMatrix;
uniform sampler2D tex;
uniform int totext;
uniform vec4 couleur;
uniform vec4 lumPos;
in vec2 vsoTexCoord;
in vec3 vsoNormal;
in vec3 vsoPosition;
out vec4 fragColor;

void main(void) {  
  if(totext == 0) {
	const float shininess = 10;  
	float diffuse_factor, specular_factor;
	vec4 ambient_color = vec4(0.0, 0, 0, 1); //couleur ambiante = noire
	vec4 specular_color = vec4(1, 1, 1, 1);//couleur de la lumière
	vec4 diffuse_color = couleur;//couleur de la sphere
	
	//calcul
	vec3 N = normalize(vsoNormal);
	vec3 L = normalize(lumPos.xyz); /*vers le bas vers la gauche*/
	vec3 R = reflect(L, N);
	
	diffuse_factor = dot(N, -L);
	diffuse_color *= diffuse_factor;
	
	specular_factor = pow(clamp(dot(-R, vec3(0, 0, 0)), 0, 1), shininess);
	specular_color *= specular_factor;
	
	fragColor = ambient_color + diffuse_color + specular_color;//output	
  }
  else fragColor = texture(tex, vsoTexCoord);
}
