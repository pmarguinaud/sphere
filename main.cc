#include <GL/glew.h>
#include <GLFW/glfw3.h>


#include "bmp.h"
#include "gensphere.h"
#include "shader.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <unistd.h>
#include <sys/time.h>


using namespace glm;

const float a = 6378137;
const float PI = 3.141592653589793;
const float rad2deg = 180 / M_PI;
const float deg2rad = M_PI / 180;

int webm = 1;
static
void key_callback (GLFWwindow * window, int key, int scancode, int action, int mods)
{
  if ((key == GLFW_KEY_SPACE) && (action == GLFW_PRESS))
    webm = ! webm;
}


int main (int argc, char * argv[])
{
  int Nj = atoi (argv[1]);
  int np; 
  float * xyz;
  unsigned int nt;
  unsigned int * ind;
  const int width = 1024, height = 1024;
  int w0, h0;
  unsigned char * rgb0 = NULL;
  int w1, h1;
  unsigned char * rgb1 = NULL;

  bmp ("Whole_world_-_land_and_oceans_8000.bmp", &rgb0, &w0, &h0);
  bmp ("TileMatrix=5&TileCol=15&TileRow=11.bmp", &rgb1, &w1, &h1);

  gensphere1 (Nj, &np, &xyz, &nt, &ind);

  if (! glfwInit ()) 
    {   
      fprintf (stderr, "Failed to initialize GLFW\n");
      return -1;
    }   

  GLFWwindow * window;
  
  glfwWindowHint (GLFW_SAMPLES, 4);
  glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint (GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  window = glfwCreateWindow (width, height, "", NULL, NULL);
    
  if (window == NULL)
    { 
      fprintf (stderr, "Failed to open GLFW window\n");
      glfwTerminate ();
      return -1;
    }
  
  glfwMakeContextCurrent (window);
  glfwSetKeyCallback (window, key_callback);
  
  glewExperimental = true; 
  if (glewInit () != GLEW_OK)
    {
      fprintf (stderr, "Failed to initialize GLEW\n");
      glfwTerminate ();
      return -1;
    }


  glClearColor (0.0f, 0.0f, 0.0f, 0.0f);
  glEnable (GL_DEPTH_TEST);
  glEnable (GL_BLEND);
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glCullFace (GL_BACK);
  glFrontFace (GL_CCW);
  glEnable (GL_CULL_FACE);
  glDepthFunc (GL_LESS); 



  GLuint VertexArrayID;
  GLuint vertexbuffer, colorbuffer, elementbuffer;

  glGenVertexArrays (1, &VertexArrayID);
  glBindVertexArray (VertexArrayID);

  glGenBuffers (1, &vertexbuffer);
  glBindBuffer (GL_ARRAY_BUFFER, vertexbuffer);
  glBufferData (GL_ARRAY_BUFFER, 3 * np * sizeof (float), xyz, GL_STATIC_DRAW);
  glEnableVertexAttribArray (0); 
  glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, NULL); 
  
  glGenBuffers (1, &elementbuffer);
  glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
  glBufferData (GL_ELEMENT_ARRAY_BUFFER, 3 * nt * sizeof (unsigned int), ind , GL_STATIC_DRAW);

  GLuint programID = shader 
(
R"CODE(
#version 330 core

in vec3 fragmentPos;
out vec4 color;
uniform sampler2D texture0;
uniform sampler2D texture1;


const float PI = 3.141592653589793;
const float rad2deg = 180. / PI;
const float a = 6378137;

uniform bool webm = true;
uniform float F;
uniform float X0 = -20037508.3427892476320267;
uniform float Y0 = +20037508.3427892476320267;
uniform int IX0;
uniform int IY0;

void main ()
{
  float lon = atan (fragmentPos.y, fragmentPos.x);
  float lat = asin (fragmentPos.z);
  float lonmin = -PI;
  float lonmax = +PI;
  float latmin = -PI/2;
  float latmax = +PI/2;

  float x, xmin, xmax;
  float y, ymin, ymax;

  x = lon; xmin = lonmin; xmax = lonmax;
  y = lat; ymin = latmin; ymax = latmax;

  float xt0 = (x - xmin) / (xmax - xmin);
  float yt0 = (y - ymin) / (ymax - ymin);

  vec4 col0 = texture2D (texture0, vec2 (xt0, yt0));

  float X = a * lon;
  float Y = a * log (tan (PI / 4. + lat * 0.5));

  float Z = 256 * F;

  float DX = X - X0;
  float DY = Y0 - Y;

  int IX = int (DX / Z); 
  int IY = int (DY / Z); 

  X = (X - IX) / Z;
  Y = (Y - IY) / Z;


  bool inl = (IX == IX0) && (IY == IY0);

  if (inl && webm){
  vec4 col1 = texture2D (texture1, vec2 (X, Y));
  color.r = col1.r;
  color.g = col1.g;
  color.b = col1.b;
  color.a = 1.;
  }else{
  color.r = col0.r;
  color.g = col0.g;
  color.b = col0.b;
  color.a = 1.;
  }
}
)CODE",
R"CODE(

#version 330 core

layout (location = 0) in vec3 vertexPos;

out vec3 fragmentPos;

uniform mat4 MVP;

const float pi = 3.1415926;

void main()
{
  vec3 normedPos;

  float x = vertexPos.x;
  float y = vertexPos.y;
  float z = vertexPos.z;
  float r = 1. / sqrt (x * x + y * y + z * z); 
  normedPos.x = x * r;
  normedPos.y = y * r;
  normedPos.z = z * r;

  gl_Position =  MVP * vec4 (normedPos, 1);

  fragmentPos = normedPos;

}


)CODE");

  glUseProgram (programID);


  float lonc = 2;
  float latc = 46.7;
  float fov = 5;
  float coslonc = cos (deg2rad * lonc), sinlonc = sin (deg2rad * lonc);
  float coslatc = cos (deg2rad * latc), sinlatc = sin (deg2rad * latc);

  float r = 6;
  float xc = r * coslonc * coslatc;
  float yc = r * sinlonc * coslatc;
  float zc = r *           sinlatc;

  glm::mat4 Projection = glm::perspective (glm::radians (fov), 1.0f, 0.1f, 100.0f);
  glm::mat4 View       = glm::lookAt (glm::vec3 (xc, yc, zc),
		                      glm::vec3 (0,0,0), 
                                      glm::vec3 (0,0,1));
  glm::mat4 Model      = glm::mat4 (1.0f);

  glm::mat4 MVP = Projection * View * Model; 

  glUniformMatrix4fv (glGetUniformLocation (programID, "MVP"), 1, GL_FALSE, &MVP[0][0]);

  int L = 5;
  float F = 2 * PI * a / 256;
  for (int i = 0; i < L; i++)
    F = F / 2;
  int IX0 = 15;
  int IY0 = 11;

  glUniform1f (glGetUniformLocation (programID, "F"), F);
  glUniform1i (glGetUniformLocation (programID, "IX0"), IX0);
  glUniform1i (glGetUniformLocation (programID, "IY0"), IY0);

  unsigned int texture0;
  glGenTextures (1, &texture0);
  glBindTexture (GL_TEXTURE_2D, texture0); 
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, w0, h0, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb0);


  unsigned int texture1;
  glGenTextures (1, &texture1);
  glBindTexture (GL_TEXTURE_2D, texture1); 
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, w1, h1, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb1);

  glActiveTexture (GL_TEXTURE0); 
  glBindTexture (GL_TEXTURE_2D, texture0);
  glUniform1i (glGetUniformLocation (programID, "texture0"), 0);

  glActiveTexture (GL_TEXTURE1); 
  glBindTexture (GL_TEXTURE_2D, texture1);
  glUniform1i (glGetUniformLocation (programID, "texture1"), 1);


  while (1) 
    {   

      glUniform1i (glGetUniformLocation (programID, "webm"), webm);

      glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      glBindVertexArray (VertexArrayID);
      glDrawElements (GL_TRIANGLES, 3 * nt, GL_UNSIGNED_INT, NULL);

      glfwSwapBuffers (window);
      glfwPollEvents (); 
  
      if (glfwGetKey (window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        break;
      if (glfwWindowShouldClose (window) != 0)  
        break;

    }   

  glfwTerminate ();


  return 0;
}
