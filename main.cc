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
#include "load.h"


int main (int argc, char * argv[])
{
  float * xyz;
  int np; 
  unsigned int nt;
  unsigned int * ind;

  float * xyz_l;
  int np_l;
  unsigned int nt_l;
  unsigned int * ind_l;

  const int width = 1024, height = 1024;
  int w, h;
  unsigned char * r = NULL;
  int Nj = atoi (argv[1]);

  gensphere1 (Nj, &np, &xyz, &nt, &ind, &r);


  np_l = 360;
  xyz_l = (float *)malloc (np_l * 3 * sizeof (float));
  ind_l = (unsigned int *)malloc (np_l * 2 * sizeof (unsigned int));
  for (int i = 0; i < np_l; i++)
    {
      xyz_l[3*i+0] = cos (2.0f * M_PI * i / 360.0f);
      xyz_l[3*i+1] = sin (2.0f * M_PI * i / 360.0f);
      xyz_l[3*i+2] = 0.0f; 
      ind_l[2*i+0] = i;
      ind_l[2*i+1] = (i + 1) % np_l;
    }


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



  // Sphere
  GLuint VertexArrayID;
  GLuint vertexbuffer, colorbuffer, elementbuffer;

  glGenVertexArrays (1, &VertexArrayID);
  glBindVertexArray (VertexArrayID);

  glGenBuffers (1, &vertexbuffer);
  glBindBuffer (GL_ARRAY_BUFFER, vertexbuffer);
  glBufferData (GL_ARRAY_BUFFER, 3 * np * sizeof (float), xyz, GL_STATIC_DRAW);
  glEnableVertexAttribArray (0); 
  glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, NULL); 

  const int ncol = 1;
  glGenBuffers (1, &colorbuffer);
  glBindBuffer (GL_ARRAY_BUFFER, colorbuffer);
  glBufferData (GL_ARRAY_BUFFER, ncol * np * sizeof (unsigned char), r, GL_STATIC_DRAW);
  glEnableVertexAttribArray (1); 
  glVertexAttribPointer (1, ncol, GL_UNSIGNED_BYTE, GL_TRUE, ncol * sizeof (unsigned char), NULL); 

  
  glGenBuffers (1, &elementbuffer);
  glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
  glBufferData (GL_ELEMENT_ARRAY_BUFFER, 3 * nt * sizeof (unsigned int), ind , GL_STATIC_DRAW);

  // Line
  GLuint VertexArrayID_l;
  GLuint vertexbuffer_l, elementbuffer_l;

  glGenVertexArrays (1, &VertexArrayID_l);
  glBindVertexArray (VertexArrayID_l);

  glGenBuffers (1, &vertexbuffer_l);
  glBindBuffer (GL_ARRAY_BUFFER, vertexbuffer_l);
  glBufferData (GL_ARRAY_BUFFER, 3 * np_l * sizeof (float), xyz_l, GL_STATIC_DRAW);
  glEnableVertexAttribArray (0); 
  glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, NULL); 

  glGenBuffers (1, &elementbuffer_l);
  glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, elementbuffer_l);
  glBufferData (GL_ELEMENT_ARRAY_BUFFER, 2 * np_l * sizeof (unsigned int), ind_l, GL_STATIC_DRAW);

  GLuint programID = shader 
(
R"CODE(
#version 330 core

in vec4 fragmentColor;

out vec4 color;

void main()
{
  color.r = fragmentColor.r;
  color.g = fragmentColor.g;
  color.b = fragmentColor.b;
  color.a = fragmentColor.a;
}
)CODE",
R"CODE(
#version 330 core

layout(location = 0) in vec3 vertexPos;
layout(location = 1) in vec3 vertexCol;

out vec4 fragmentColor;

uniform mat4 MVP;

void main()
{
  gl_Position =  MVP * vec4 (vertexPos, 1);

  fragmentColor.r = vertexCol.r;
  fragmentColor.g = vertexCol.g;
  fragmentColor.b = vertexCol.b;
  fragmentColor.a = 1.;

}
)CODE");

  GLuint programID_l = shader 
(
R"CODE(
#version 330 core

out vec4 color;

void main()
{
  color.r = 0.;
  color.g = 1.;
  color.b = 0.;
  color.a = 1.;
}
)CODE",
R"CODE(
#version 330 core

layout(location = 0) in vec3 vertexPos;

uniform mat4 MVP;

void main()
{
  gl_Position =  MVP * vec4 (vertexPos, 1);
}
)CODE");



  int count = 0;
  while (1) 
    {   
      glm::mat4 Projection = glm::perspective (glm::radians (20.0f), 1.0f, 0.1f, 100.0f);
      glm::mat4 View       = glm::lookAt (glm::vec3 (6.0f * cos (count * M_PI / 180.0f), 
                                                     6.0f * sin (count * M_PI / 180.0f),
                                                     0.0f), 
                                          glm::vec3 (0,0,0), glm::vec3 (0,0,1));
      glm::mat4 Model      = glm::mat4 (1.0f);
      glm::mat4 MVP = Projection * View * Model; 

      glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      glUseProgram (programID);
      glUniformMatrix4fv (glGetUniformLocation (programID, "MVP"), 1, GL_FALSE, &MVP[0][0]);
      glBindVertexArray (VertexArrayID);
      glDrawElements (GL_TRIANGLES, 3 * nt, GL_UNSIGNED_INT, NULL);

      glUseProgram (programID_l);
      glUniformMatrix4fv (glGetUniformLocation (programID_l, "MVP"), 1, GL_FALSE, &MVP[0][0]);
      glBindVertexArray (VertexArrayID_l);
      glDrawElements (GL_LINES, 2 * np_l, GL_UNSIGNED_INT, NULL);
      glBindVertexArray (0);


      glfwSwapBuffers (window);
      glfwPollEvents (); 
  
      if (glfwGetKey (window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        break;
      if (glfwWindowShouldClose (window) != 0)  
        break;

      count++;
      count %= 360;
    }   


  glfwTerminate ();


  return 0;
}
