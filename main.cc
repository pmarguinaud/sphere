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


void calc_md5 (const char * buf, int len, unsigned char out[])
{
  MD5_CTX c;
  MD5_Init (&c);
  MD5_Update (&c, buf, len);
  MD5_Final (out, &c);
}


int main (int argc, char * argv[])
{
  int Nj = atoi (argv[1]);
  int np; 
  float * xyz;
  unsigned int nt;
  unsigned int * ind;
  const int width = 800, height = 800;
  int w, h;
  unsigned char md5[MD5_DIGEST_LENGTH];

  gensphere1 (Nj, &np, &xyz, &nt, &ind);

  if (! glfwInit ()) 
    {   
      fprintf (stderr, "Failed to initialize GLFW\n");
      return -1;
    }   

  GLFWwindow * window1, * window2;
  
  glfwWindowHint (GLFW_SAMPLES, 4);
  glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint (GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  window1 = glfwCreateWindow (width, height, "", NULL, NULL);
  window2 = glfwCreateWindow (width, height, "", NULL, window1);
    
  if (window1 == NULL)
    { 
      fprintf (stderr, "Failed to open GLFW window1\n");
      glfwTerminate ();
      return -1;
    }
  
  glfwMakeContextCurrent (window1);
  glfwSwapInterval(1);


  
  glewExperimental = true; 
  if (glewInit () != GLEW_OK)
    {
      fprintf (stderr, "Failed to initialize GLEW\n");
      glfwTerminate ();
      return -1;
    }

  GLuint VertexArrayID1;
  GLuint vertexbuffer, elementbuffer;

  glGenVertexArrays (1, &VertexArrayID1);
  glBindVertexArray (VertexArrayID1);

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

out vec4 fragmentColor;

uniform mat4 MVP;

void main()
{
  gl_Position =  MVP * vec4 (vertexPos, 1);

  fragmentColor.r = 1.;
  fragmentColor.g = 0.;
  fragmentColor.b = 1.;
  fragmentColor.a = 1.;

}
)CODE");


  glm::mat4 Projection = glm::perspective (glm::radians (20.0f), 1.0f / 1.0f, 0.1f, 100.0f);
  glm::mat4 View       = glm::lookAt (glm::vec3 (6.0f,0.0f,0.0f), glm::vec3 (0,0,0), glm::vec3 (0,0,1));
  glm::mat4 Model      = glm::mat4 (1.0f);

  glm::mat4 MVP = Projection * View * Model; 

  glUseProgram (programID);
  glUniformMatrix4fv (glGetUniformLocation (programID, "MVP"), 1, GL_FALSE, &MVP[0][0]);


  glfwMakeContextCurrent (window2);
  glUseProgram (programID);
  glUniformMatrix4fv (glGetUniformLocation (programID, "MVP"), 1, GL_FALSE, &MVP[0][0]);

  GLuint VertexArrayID2;

  glGenVertexArrays (1, &VertexArrayID2);
  glBindVertexArray (VertexArrayID2);

  glBindBuffer (GL_ARRAY_BUFFER, vertexbuffer);
  glEnableVertexAttribArray (0); 
  glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, NULL); 
  
  glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, elementbuffer);


  while (1) 
    {   
      //
      glfwMakeContextCurrent (window1);
      glViewport (0, 0, width, height);
      glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      glBindVertexArray (VertexArrayID1);
      glDrawElements (GL_TRIANGLES, 3 * nt, GL_UNSIGNED_INT, NULL);

      glfwSwapBuffers (window1);
      glfwPollEvents (); 
  
      //
      glfwMakeContextCurrent (window2);
      glViewport (0, 0, width, height);
      glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      glBindVertexArray (VertexArrayID2);
      glDrawElements (GL_TRIANGLES, 3 * nt, GL_UNSIGNED_INT, NULL);

      glfwSwapBuffers (window2);
      glfwPollEvents (); 
  
      if (glfwGetKey (window1, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        break;
      if (glfwWindowShouldClose (window1) != 0)  
        break;
    }   


  glfwTerminate ();


  return 0;
}
