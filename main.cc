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


class tex
{
public:
  const unsigned int nt = 2;
  const int width = 1024, height = 1024;
  unsigned int ind[3*2] = {0, 1, 2, 0, 2, 3};
  const int Nx = 4, Ny = 4;
  GLuint programID;
  void create ()
  {
    programID = shader 
(
R"CODE(
#version 420 core

in float instance;

out vec4 color;

void main ()
{
  color = vec4 (0., 0., 0., 0.);

  int i = int (instance) % 3;
  
  color[i] = 1.;  

  color.a = 1.;
}
)CODE",
R"CODE(
#version 420 core

uniform int Nx = 0;
uniform int Ny = 0;

out float instance;

void main ()
{
  int i = gl_VertexID;
  int j = gl_InstanceID;
  float dx = 2. / float (Nx);
  float dy = 2. / float (Ny);

  instance = j;

  int jx = j % Nx;
  int jy = j / Ny;

  vec3 vertexPos;
  if (i == 0) 
    vertexPos = vec3 (0.0f, 0.0f, 0.0f);
  else if (i == 1)
    vertexPos = vec3 (0.0f,   dx, 0.0f);
  else if (i == 2)
    vertexPos = vec3 (0.0f,   dx,   dy);
  else if (i == 3)
    vertexPos = vec3 (0.0f, 0.0f,   dy);

  float x = float (jx) * dx;
  float y = float (jy) * dy;

  vertexPos = vertexPos + vec3 (0.0f, x-1.0, y-1.0);

  gl_Position = vec4 (vertexPos.y, vertexPos.z, 0., 1.);
}
)CODE");

    glUseProgram (programID);
    GLuint VertexArrayID;
    GLuint elementbuffer;
    
    glGenVertexArrays (1, &VertexArrayID);
    glBindVertexArray (VertexArrayID);
    
    glUniform1i (glGetUniformLocation (programID, "Nx"), Nx);
    glUniform1i (glGetUniformLocation (programID, "Ny"), Ny);


  }

  void render () const
  {
    glViewport (0, 0, width, height);
    glDrawElementsInstanced (GL_TRIANGLES, 3 * nt, GL_UNSIGNED_INT, ind, Nx * Ny);
  }
};


int main (int argc, char * argv[])
{
  tex tt;
  

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

  window = glfwCreateWindow (tt.width, tt.height, "", NULL, NULL);
    
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

  tt.create ();


  while (1) 
    {   
      glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      tt.render ();

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
