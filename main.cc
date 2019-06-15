#include <GL/glew.h>
#include <GLFW/glfw3.h>


#include "gensphere.h"
#include "shader.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <unistd.h>
#include <sys/time.h>


static float lonc = 0.0f;
static float latc = 0.0f;
static float R = 6.0f;
static float fov = 20.0f;
static bool wireframe = false;
static bool rotate = false;
static bool hide = false;

static 
void key_callback (GLFWwindow * window, int key, int scancode, int action, int mods)
{
  if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
      switch (key)
        {
          case GLFW_KEY_UP:
            latc += 5;
	    break;
          case GLFW_KEY_DOWN:
            latc -= 5;
	    break;
          case GLFW_KEY_LEFT:
            lonc += 5;
	    break;
          case GLFW_KEY_RIGHT:
            lonc -= 5;
	    break;
	  case GLFW_KEY_SPACE:
            lonc  = 0; latc = 0;
	    break;
	  case GLFW_KEY_F1:
            fov += 1;
	    break;
	  case GLFW_KEY_F2:
            fov -= 1;
	    break;
	  case GLFW_KEY_F3:
            wireframe = ! wireframe;
	    break;
	  case GLFW_KEY_F4:
            hide = ! hide;
	    break;
	  case GLFW_KEY_TAB:
            rotate = ! rotate;
	    break;
	}
    }
}

bool endsWith (std::string const & fullString, std::string const & ending) 
{
  if (fullString.length () >= ending.length ()) 
    return (0 == fullString.compare (fullString.length ()  
            - ending.length (), ending.length (), ending));
  return false;
}

int main (int argc, char * argv[])
{
  int np; 
  float * xyz;
  unsigned int nt;
  unsigned int * ind;
  const int width = 1024, height = 1024;
  int w, h;
  geom_t geom;
  geom.Nj = atoi (argv[1]);

  typedef struct
  {
    int w = 0, h = 0;
    unsigned char * rgb = NULL;
    unsigned int texture;
  } contour_t;

  const int N = 4;
  contour_t cont[N];

  std::string type = "XxYxZ";

  if (argc > 2)
    type = argv[2];
  bool fill = argc > 3;


  for (int k = 0; k < N; k++)
    {

      cont[k].w = 100;
      cont[k].h = 100;

      for (int l = N; l > k; l--)
        cont[k].h *= 2;

      cont[k].rgb = (unsigned char *)malloc (sizeof (unsigned char) * cont[k].w * cont[k].h * 3);
     
      if(fill){
      const int col[10][3] = {
                               {255,   0,   0}, {  0, 255,   0},
                               {  0,   0, 255}, {  0,   0,   0},
                               {255, 255, 255}, {  0,   0,   0},
                               {255, 255,   0}, {255,   0, 255},
                               {  0, 255, 255}, {255, 127, 127} 
                             };
     
     
      const int s = cont[k].h / 10;
      for (int i = 0; i < cont[k].w; i++)
        for (int j = 0; j < cont[k].h; j++)
          {
            cont[k].rgb[3*(cont[k].w*j+i)+0] = col[(j/s)%10][0];
            cont[k].rgb[3*(cont[k].w*j+i)+1] = col[(j/s)%10][1];
            cont[k].rgb[3*(cont[k].w*j+i)+2] = col[(j/s)%10][2];
          }
      }else{
     
      const int s = cont[k].h / 10;
      for (int i = 0; i < cont[k].w; i++)
        for (int j = 0; j < cont[k].h; j++)
          {
            cont[k].rgb[3*(cont[k].w*j+i)+0] = (j % s == 0) ? 0 : 255;
            cont[k].rgb[3*(cont[k].w*j+i)+1] = (j % s == 0) ? 0 : 255;
            cont[k].rgb[3*(cont[k].w*j+i)+2] = (j % s == 0) ? 0 : 255;
          }
     
      }
    }


  float * F;

  if (argc > 2)
    type = argv[2];

  if (endsWith (type, std::string (".grb")))
    gensphere_grib (&geom, &np, &xyz, &nt, &ind, &F, type);
  else
    gensphere (&geom, &np, &xyz, &nt, &ind, &F, type);


  float maxval = *std::max_element (F, F + np);
  float minval = *std::min_element (F, F + np);

  std::cout << " minval = " << minval << std::endl;
  std::cout << " maxval = " << maxval << std::endl;

  for (int i = 0; i < np; i++)
    F[i] = (F[i] - minval) / (maxval - minval);

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


  glGenBuffers (1, &colorbuffer);
  glBindBuffer (GL_ARRAY_BUFFER, colorbuffer);
  glBufferData (GL_ARRAY_BUFFER, np * sizeof (float), F, GL_STATIC_DRAW);
  glEnableVertexAttribArray (1); 
  glVertexAttribPointer (1, 1, GL_FLOAT, GL_FALSE, 0, NULL); 


  
  glGenBuffers (1, &elementbuffer);
  glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
  glBufferData (GL_ELEMENT_ARRAY_BUFFER, 3 * nt * sizeof (unsigned int), ind , GL_STATIC_DRAW);

  GLuint programID = shader 
(
R"CODE(
#version 330 core

in vec3 fragmentPos;
in float fragmentVal;
out vec4 color;

uniform sampler2D texture;
uniform bool fill = false;

void main ()
{
  vec4 col = texture2D (texture, 0.90 * vec2 (0.5, fragmentVal) + 0.05);
  if(fill){
  color.r = col.r;
  color.g = col.g;
  color.b = col.b;
  color.a = 1.;
  }else{
  color.r = 0.;
  color.g = 0.;
  color.b = 1.;
  color.a = 1. - col.r;
  }
}
)CODE",
R"CODE(

#version 330 core

layout (location = 0) in vec3 vertexPos;
layout (location = 1) in float vertexVal;

out vec3 fragmentPos;
out float fragmentVal;

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
  fragmentVal = vertexVal;

}


)CODE");

  GLuint programID1 = shader
(
R"CODE(
#version 330 core

in vec3 fragmentPos;
out vec4 color;

void main ()
{
  color.r = 1.;
  color.g = 0.;
  color.b = 0.;
  color.a = 1.;
}
)CODE",
R"CODE(

#version 330 core

layout (location = 0) in vec3 vertexPos;

uniform mat4 MVP;

void main()
{
  vec3 normedPos;

  float x = vertexPos.x;
  float y = vertexPos.y;
  float z = vertexPos.z;
  float r = 0.99 / sqrt (x * x + y * y + z * z); 
  normedPos.x = x * r;
  normedPos.y = y * r;
  normedPos.z = z * r;
  gl_Position =  MVP * vec4 (normedPos, 1);
}


)CODE");


  for (int i = 0; i < N; i++)
    {
      glGenTextures (1, &cont[i].texture);
      glBindTexture (GL_TEXTURE_2D, cont[i].texture); 
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, cont[i].w, cont[i].h, 0, GL_RGB, GL_UNSIGNED_BYTE, cont[i].rgb);
    }



  while (1) 
    {   
      glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      glm::mat4 Projection = glm::perspective (glm::radians (fov), 1.0f, 0.1f, 100.0f);
      glm::mat4 View       = glm::lookAt (glm::vec3 (R * cos (lonc * M_PI / 180.0f) * cos (latc * M_PI / 180.0f), 
                                                     R * sin (lonc * M_PI / 180.0f) * cos (latc * M_PI / 180.0f),
                                                     R * sin (latc * M_PI / 180.0f)), 
                                          glm::vec3 (0,0,0), glm::vec3 (0,0,1));
      glm::mat4 Model      = glm::mat4 (1.0f);
      
      glm::mat4 MVP = Projection * View * Model; 

      if (! hide){
      glUseProgram (programID1);
      glUniformMatrix4fv (glGetUniformLocation (programID1, "MVP"), 1, GL_FALSE, &MVP[0][0]);
      glBindVertexArray (VertexArrayID);
      glDrawElements (GL_TRIANGLES, 3 * nt, GL_UNSIGNED_INT, NULL);
      }

      glUseProgram (programID);
      glUniformMatrix4fv (glGetUniformLocation (programID, "MVP"), 1, GL_FALSE, &MVP[0][0]);
      glUniform1i (glGetUniformLocation (programID, "fill"), fill);

      if (wireframe)
        glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
      else
        glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);

      glActiveTexture (GL_TEXTURE0);

      if (fov < 8)
        glBindTexture (GL_TEXTURE_2D, cont[0].texture);
      else if (fov < 15)
        glBindTexture (GL_TEXTURE_2D, cont[1].texture);
      else if (fov < 25)
        glBindTexture (GL_TEXTURE_2D, cont[2].texture);
      else 
        glBindTexture (GL_TEXTURE_2D, cont[3].texture);
      glUniform1i (glGetUniformLocation (programID, "texture"), 0);

      glBindVertexArray (VertexArrayID);
      glDrawElements (GL_TRIANGLES, 3 * nt, GL_UNSIGNED_INT, NULL);


      glfwSwapBuffers (window);
      glfwPollEvents (); 
  
      if (glfwGetKey (window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        break;
      if (glfwWindowShouldClose (window) != 0)  
        break;


      if (rotate)
        lonc += 1;
    }   





  glfwTerminate ();


  return 0;
}
