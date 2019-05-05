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

#include <openssl/md5.h>
#include <unistd.h>
#include "load.h"


void calc_md5 (const char * buf, int len, unsigned char out[])
{
  MD5_CTX c;
  MD5_Init (&c);
  MD5_Update (&c, buf, len);
  MD5_Final (out, &c);
}


int main (int argc, char * argv[])
{
  int np; 
  float * xyz;
  unsigned int nt;
  unsigned int * ind;
  const int width = 1024, height = 1024;
  int w, h;
  unsigned char * rgb = NULL;
  unsigned char md5[MD5_DIGEST_LENGTH];

  gensphere1 (argv[1], &np, &xyz, &nt, &ind);
  glgrib_load_rgb (argv[1], &rgb);

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



  GLuint VertexArrayID;
  GLuint vertexbuffer, colorbuffer, elementbuffer;

  glGenVertexArrays (1, &VertexArrayID);
  glBindVertexArray (VertexArrayID);

  glGenBuffers (1, &vertexbuffer);
  glBindBuffer (GL_ARRAY_BUFFER, vertexbuffer);
  glBufferData (GL_ARRAY_BUFFER, 3 * np * sizeof (float), xyz, GL_STATIC_DRAW);
  glEnableVertexAttribArray (0); 
  glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, NULL); 

  const int ncol = 3;
  glGenBuffers (1, &colorbuffer);
  glBindBuffer (GL_ARRAY_BUFFER, colorbuffer);
  glBufferData (GL_ARRAY_BUFFER, ncol * np * sizeof (unsigned char), rgb, GL_STATIC_DRAW);
  glEnableVertexAttribArray (1); 
  glVertexAttribPointer (1, ncol, GL_UNSIGNED_BYTE, GL_TRUE, ncol * sizeof (unsigned char), NULL); 

  
  glGenBuffers (1, &elementbuffer);
  glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
  glBufferData (GL_ELEMENT_ARRAY_BUFFER, 3 * nt * sizeof (unsigned int), ind , GL_STATIC_DRAW);

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

  glUseProgram (programID);

  glm::mat4 Projection = glm::perspective (glm::radians (20.0f), 1.0f / 1.0f, 0.1f, 100.0f);
  glm::mat4 View       = glm::lookAt (glm::vec3 (6.0f,0.0f,0.0f), glm::vec3 (0,0,0), glm::vec3 (0,0,1));
//glm::mat4 Projection = glm::perspective (glm::radians (10.0f), 1.0f / 1.0f, 0.1f, 100.0f);
//glm::mat4 View       = glm::lookAt (glm::vec3 (3.0f,0.0f,5.0f), glm::vec3 (0,0,0), glm::vec3 (0,0,1));
  glm::mat4 Model      = glm::mat4 (1.0f);

  glm::mat4 MVP = Projection * View * Model; 

  glUniformMatrix4fv (glGetUniformLocation (programID, "MVP"), 1, GL_FALSE, &MVP[0][0]);

  while (1) 
    {   
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
