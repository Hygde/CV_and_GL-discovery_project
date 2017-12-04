/*!\file window.cpp
 *
 * \brief Utilisation du CascadeClassifier pour d�tecter des visages et les yeux
 *
 * \author Far�s BELHADJ, amsi@ai.univ-paris8.fr
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

//openCV d�claration :
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
 
/*!\brief dimensions de la fen�tre */
static int _windowWidth = 800, _windowHeight = 531;

/*!\brief identifiants des (futurs) GLSL programs */
static GLuint _pId = 0;

//identifiant texture
static GLuint _topencvId = 0;
static GLuint _tspheresId = 0;

/*!\brief Cr�ation de la fen�tre et param�trage des fonctions callback.*/
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

//permet de convertir les coordonn�es d'un rep�re 2D -> 3D
void convertCoord(float*dest, float x, float width, float y, float height){	
	float xp = 2 * ((_windowWidth - x) / _windowWidth) - 1;//changement de rep�re [0;1] vers [-1;1]
	float yp = 2 * ((_windowHeight - y) / _windowHeight) - 1;//changement de rep�re [0;1] vers [-1;1]
	float zp = (_windowWidth / width) + (_windowHeight / height);//on recule l'objet dans la sc�ne en fonction de la place occuper � l'ecran
	
	*(dest+0) = pow(1.17, zp) * xp;//on d�place les valeurs dans l'emplacement de sauvegarde
	*(dest+1) = yp;//l'�loignement n'affecte pas la hauteur des �l�ments
	
	if(zp > 1.0) zp *= -1.0;//on r�tr�cie la taille de l'�l�ment
	*(dest+2) = zp;
}

void drawItem(float x, float y, float z, GLuint el){  
  gl4duBindMatrix("modelViewMatrix");
  gl4duPushMatrix();{//sauvegarde l'�tat courant de la matrice
	  GLfloat rouge[] = {1, 0, 0, 1};
  	  gl4duLoadIdentityf();
	  gl4duTranslatef(x, y, z);
	  gl4duSendMatrices();
	  glUniform4fv(glGetUniformLocation(_pId, "couleur"), 1, rouge);
	  gl4dgDraw(el);	
  }gl4duPopMatrix();//on retourne � l'�tat initiale avant l'appel de la fonction
}

void addLum(GLfloat pos[]){
	GLfloat lumPos[4], *mat;
	gl4duBindMatrix("modelViewMatrix");
	gl4duPushMatrix();{
	  	gl4duLoadIdentityf();
		gl4duTranslatef(0, 0, -3);
		mat = (GLfloat*) gl4duGetMatrixData();
		MMAT4XVEC4(lumPos, mat, pos);//multiplication de la matrice avec un vecteur 4 afin que la lumi�re suive ma vue
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

/*!\brief Initialise les param�tres OpenGL.*/
static void init(void) {
  _pId  = gl4duCreateProgram("<vs>shaders/basic.vs", "<fs>shaders/basic.fs", NULL);
  	
  glGenTextures(1, &_topencvId);
  glGenTextures(1, &_tspheresId);

  //param sph�re
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  gl4duGenMatrix(GL_FLOAT, "modelViewMatrix");//cr�taion d'une matrice de pla�age des objets
  gl4duGenMatrix(GL_FLOAT, "projectionMatrix");//cr�ation d'une matrice d'affichage
  resize(_windowWidth, _windowHeight);
  
  _sphere = gl4dgGenSpheref(10, 10);
  _square = gl4dgGenQuadf();
  
  TextureCV(ci);//_topencvId contient l'image
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

/*!\brief Dessin de la g�om�trie textur�e. */
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
	  gl4duPushMatrix(); {//sauve l'�tat de la matrice
		  gl4duTranslatef(0,0,-1);
		  gl4duScalef(1,-1,1); 
		  gl4duSendMatrices();
		  
		 glUniform1i(glGetUniformLocation(_pId, "tex"), 0);
		 glUniform1i(glGetUniformLocation(_pId, "totext"), 1);
		  
		  gl4dgDraw(_square);
	  } gl4duPopMatrix();//on retourne � la matrice pr�c�dente
	  
	  gl4duBindMatrix("projectionMatrix");
  } gl4duPopMatrix();//on retourne � la matrice pr�c�dente
  gl4duBindMatrix("modelViewMatrix");
  
  glUniform1i(glGetUniformLocation(_pId, "totext"), 0);
  
  float position[3] = {0};
  for (unsigned int i = 0; i < faces.size(); i++) {
  	//convertCoord(position, (float) (faces[i].x + (faces[i].width /2)), (float) (faces[i].y + (faces[i].height / 2)), size);
  	convertCoord(position, (float) faces[i].x, (float) faces[i].width, (float) faces[i].y, (float) faces[i].height);
  	drawItem(position[0], position[1], position[2], _sphere);
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
	
	on a en plus de �a une pile de matrice. l'appel de gl4duPushMatrix() sauve la matrice � l'�tat courant
	l'appel de gl4duPopMatrix() restore l'�tat pr�c�dent de la matrice

*/
