/*!\file window.cpp
 *
 * \brief Utilisation du CascadeClassifier pour détecter des visages et les yeux
 *
 * \author Farès BELHADJ, amsi@ai.univ-paris8.fr
 * \date October 15 2015
 */
#include <stdio.h>
#include <assert.h>
#include <GL4D/gl4du.h>
#include <GL4D/gl4dg.h>
#include <GL4D/gl4duw_SDL2.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <objdetect.hpp>

using namespace cv;
using namespace std;

//openCV déclaration :
Mat ci;
vector<Rect> faces;

//GL4Dum delcaration :

//prototype
int InitGL4(int argc, char **argv);
void drawItem(float x, float y, float z, GLuint el);
void TextureCV(Mat ci);

/* Prototypes des fonctions statiques contenues dans ce fichier C */
static void init(void);
static void resize(int w, int h);
static void draw(void);
static void quit(void);
 
/*!\brief dimensions de la fenêtre */
static int _windowWidth = 800, _windowHeight = 531;

/*!\brief identifiants des (futurs) GLSL programs */
static GLuint _pId = 0;

//identifiant texture
static GLuint _topencvId = 0;
static GLuint _tHatId = 0;
//static GLuint _tMustacheId = 0;

/*!\brief Création de la fenêtre et paramétrage des fonctions callback.*/
static GLuint _square = 0;
static GLuint _sphere = 0;


////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

int main(int argc, char ** argv) {
  CascadeClassifier * face_cc = new CascadeClassifier("haarcascade_frontalface_default.xml");
  CascadeClassifier * eye_cc = new CascadeClassifier("haarcascade_eye.xml");
  if(face_cc == NULL || eye_cc == NULL)
    return 1;
  ci = imread("visages.jpg");
  Mat gsi = imread("visages.jpg", CV_LOAD_IMAGE_GRAYSCALE);
  cvNamedWindow("Face detection", CV_WINDOW_AUTOSIZE);
  
  face_cc->detectMultiScale(gsi, faces, 1.3, 5);
  for (vector<Rect>::iterator fc = faces.begin(); fc != faces.end(); ++fc) {
    rectangle(ci, (*fc).tl(), (*fc).br(), Scalar(0, 255, 0), 2, CV_AA);
  }
  
  ////////////////////////////////////////////////////////////////////
  if(InitGL4(argc, argv) != 0){
	printf("Erreur dans InitGL4()");
	return EXIT_FAILURE;
  }

  return 0;
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

void TextureCV(Mat ci){
	glBindTexture(GL_TEXTURE_2D, _topencvId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, ci.cols, ci.rows, 0, GL_BGR, GL_UNSIGNED_BYTE, ci.ptr());
	glUniform1i(glGetUniformLocation(_pId, "myTexture"), 0);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

//permet de charger une image bmp et de l'utiliser comme texture
int loadPNGTexture(char*bmp, char* shaderName, GLuint* tex){
	int result = 0;
	SDL_Surface* text = SDL_LoadBMP(bmp);
	if(text == NULL){
		fprintf(stderr,"\nErreur de chargement lors du chargement du fichier %s",bmp);
		result = -1;
	} else{
		glBindTexture(GL_TEXTURE_2D, *tex);//on rattache à tex la texture souhaitée
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, text->w, text->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, text->pixels);//load data
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);//texture param
	  	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	  	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		
		glUniform1i(glGetUniformLocation(_pId, shaderName), 0);//on envoie les données au GPU
    		SDL_FreeSurface(text);
	}
	return result;
}

//permet de convertir les coordonnées d'un repère 2D -> 3D
void convertCoord(float x, float width, float y, float height, GLuint el){	
	float xp = 2 * (x / (_windowWidth - 1.0)) - 1;//changement de repère [0;1] vers [-1;1]
	float yp = 2 * ((_windowHeight - y) / (_windowHeight - 1.0)) - 1;//changement de repère [0;1] vers [-1;1]
	float rp = (width * height) / ((GLfloat)_windowHeight * _windowWidth);//on recule l'objet dans la scène en fonction de la place occuper à l'ecran
	
	gl4duBindMatrix("projectionMatrix");
	gl4duPushMatrix();//sauve la matrice
	GLfloat* pdatamat = (GLfloat*) gl4duGetMatrixData(), pmatinv[16], rt[4], v[4] = {xp, yp, 0.0, 1.0}, w[4] = {(rp-1)/2, (rp-1)/2, (rp-1)/2, 1.0}, rs[4];
	memcpy(pmatinv, pdatamat, sizeof pmatinv);//on copie la matrice de données
	gl4duPopMatrix();//load la matrice
	MMAT4INVERSE(pmatinv);//on calcul la matrice inverse
	
	MMAT4XVEC4(rt, pmatinv, v);//produit entre la matrice et le vecteur v
	MVEC4WEIGHT(rt);
	MMAT4XVEC4(rs, pmatinv, w);//produit entre la matrice et le veceur w
	MVEC4WEIGHT(rs);
	
	glEnable(GL_DEPTH_TEST);
	gl4duBindMatrix("modelViewMatrix");
	gl4duPushMatrix();{//sauvegarde l'état courant de la matrice
		  GLfloat rouge[] = {1, 0, 0, 1};
	  	  gl4duLoadIdentityf();
		  gl4duTranslatef(rt[0], rt[1], rt[2]);
		  gl4duScalef(rs[0], rs[1], MIN(rs[0], rs[1]));
		  gl4duSendMatrices();
		  glUniform4fv(glGetUniformLocation(_pId, "couleur"), 1, rouge);
		  gl4dgDraw(el);	
	}gl4duPopMatrix();//on retourne à l'état initiale avant l'appel de la fonction
}

void drawItem(float x, float y, float z, GLuint el){  

}

void addLum(GLfloat pos[]){
	GLfloat lumPos[4], *mat;
	gl4duBindMatrix("modelViewMatrix");
	gl4duPushMatrix();{
	  	gl4duLoadIdentityf();
		gl4duTranslatef(0, 0, -3);
		mat = (GLfloat*) gl4duGetMatrixData();
		MMAT4XVEC4(lumPos, mat, pos);//multiplication de la matrice avec un vecteur 4 afin que la lumière suive ma vue
		glUniform4fv(glGetUniformLocation(_pId, "lumPos"), 1, lumPos);
	}gl4duPopMatrix();
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

//initialisation de GL4Dummies
int InitGL4(int argc, char**argv){
  if(!gl4duwCreateWindow(argc, argv, "GL4Dummies", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, _windowWidth, _windowHeight, SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN))  return 1;
  init();
  atexit(quit);
  gl4duwResizeFunc(resize);
  gl4duwDisplayFunc(draw);
  gl4duwMainLoop();
  return 0;
}

/*!\brief Initialise les paramètres OpenGL.*/
static void init(void) {
  _pId  = gl4duCreateProgram("<vs>shaders/basic.vs", "<fs>shaders/basic.fs", NULL);
  	
  glGenTextures(1, &_topencvId);//on génère un identifiant de texture
  glGenTextures(1, &_tHatId);
  //glGenTextures(1, &_tMustacheId);
  
  TextureCV(ci);//_topencvId contient l'image
  char filename[] = "hat.bmp", shadername[] = "hat";//évites un warning à la ligne du dessous
  loadPNGTexture(filename, shadername, &_tHatId);//fichier, nom dans le fragment shader, ou sauvegarder la texture

  //param sphère
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  gl4duGenMatrix(GL_FLOAT, "modelViewMatrix");//crétaion d'une matrice de plaçage des objets
  gl4duGenMatrix(GL_FLOAT, "projectionMatrix");//création d'une matrice d'affichage
  resize(_windowWidth, _windowHeight);
  
  //génère les formes
  _sphere = gl4dgGenSpheref(10, 10);
  _square = gl4dgGenQuadf();
}

static void resize(int w, int h) {
  _windowWidth  = w; _windowHeight = h;
  glViewport(0, 0, _windowWidth, _windowHeight);
  gl4duBindMatrix("projectionMatrix");
  gl4duLoadIdentityf();
  gl4duFrustumf(-0.5, 0.5, -0.5 * _windowHeight / _windowWidth, 0.5 * _windowHeight / _windowWidth, 1.0, 1000.0);
  gl4duBindMatrix("modelViewMatrix");
  gl4duLoadIdentityf();
}

/*!\brief Dessin de la géométrie texturée. */
static void draw(void) {
  glUseProgram(_pId);
  glDisable(GL_DEPTH_TEST);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, _topencvId);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  gl4duBindMatrix("projectionMatrix");
  gl4duPushMatrix(); {
	  gl4duLoadIdentityf();//permet de retirer la perspective temporairement
	  gl4duBindMatrix("modelViewMatrix");//toute modification matricielle modifie modelViewMatrix
	  gl4duPushMatrix(); {//sauve l'état de la matrice
		  gl4duTranslatef(0,0,-1);
		  gl4duScalef(1,-1,1); 
		  gl4duSendMatrices();
		  
		 glUniform1i(glGetUniformLocation(_pId, "tex"), 0);
		 glUniform1i(glGetUniformLocation(_pId, "totext"), 1);//la valeur de totext est maintenant 1 du côté GPU
		  
		  gl4dgDraw(_square);
	  } gl4duPopMatrix();//on retourne à la matrice précédente
	  
	  gl4duBindMatrix("projectionMatrix");
  } gl4duPopMatrix();//on retourne à la matrice précédente
  gl4duBindMatrix("modelViewMatrix");
  
  glUniform1i(glGetUniformLocation(_pId, "totext"), 0);//la valeur de totext est maintenant 0 du côté GPU
  glBindTexture(GL_TEXTURE_2D, _tHatId);//on selectionne la texture que l'on souhaite utiliser
  
  for (unsigned int i = 0; i < faces.size(); i++) {
  	//convertCoord(position, (float) (faces[i].x + (faces[i].width /2)), (float) (faces[i].y + (faces[i].height / 2)), size);
  	convertCoord((float) faces[i].x, (float) faces[i].width, (float) faces[i].y, (float) faces[i].height, _square);
//  	drawItem(position[0], position[1], position[2], _sphere);
  }
  
  //drawItem(0.0,0.0,-2.0,_sphere);
  
  GLfloat lumPos[4] = {-0.5, 0.5, 1.5, 1.0};
  addLum(lumPos);
}

static void quit(void) {
  gl4duClean(GL4DU_ALL);
}

/*	Note de cours :

	qu'est ce qui modifie une matrice ? :

	gl4LoadIdentifyf() load une matrice dans la matrice courante.
	gl4duRotatef(ANGLE_DEGRES,ax,ay,az)
	gl4duScalef(sx,sy,sz);
	
	on a en plus de ça une pile de matrice. l'appel de gl4duPushMatrix() sauve la matrice à l'état courant
	l'appel de gl4duPopMatrix() restore l'état précédent de la matrice

*/
