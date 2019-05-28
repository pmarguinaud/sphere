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
  float * uv;
  unsigned int nt;
  unsigned int * ind;
  const int width = 1024, height = 1024;
  int w, h;
  unsigned char * rgb = NULL;
  unsigned char md5[MD5_DIGEST_LENGTH];

  bmp ("Whole_world_-_land_and_oceans_8000.bmp", &rgb, &w, &h);

  gensphere1 (Nj, &np, &xyz, &nt, &ind);

  if (! glfwInit ()) 
    {   
      fprintf (stderr, "Failed to initialize GLFW\n");
      return -1;
    }   


  uv = (float *)malloc (sizeof (float) * 2 * np);

  for (int i = 0; i < np; i++)
    {
      uv[2*i+0] = 0.10;
      uv[2*i+1] = 0.10;
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
  GLuint vertexbuffer, uvbuffer;

  glGenVertexArrays (1, &VertexArrayID);
  glBindVertexArray (VertexArrayID);

  glGenBuffers (1, &vertexbuffer);
  glBindBuffer (GL_ARRAY_BUFFER, vertexbuffer);
  glBufferData (GL_ARRAY_BUFFER, 3 * np * sizeof (float), xyz, GL_STATIC_DRAW);
  glEnableVertexAttribArray (0); 
  glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, NULL); 
  glVertexAttribDivisor (0, 1);  
  
  glGenBuffers (1, &uvbuffer);
  glBindBuffer (GL_ARRAY_BUFFER, uvbuffer);
  glBufferData (GL_ARRAY_BUFFER, 2 * np * sizeof (float), uv, GL_STATIC_DRAW);
  glEnableVertexAttribArray (1); 
  glVertexAttribPointer (1, 2, GL_FLOAT, GL_FALSE, 0, NULL); 
  glVertexAttribDivisor (1, 1);  
  
  //

  GLuint programID = shader 
(
R"CODE(
#version 330 core

out vec4 color;

void main ()
{
  color.r = 1.;
  color.g = 1.;
  color.b = 1.;
  color.a = 1.;
}
)CODE",
R"CODE(

#version 330 core

layout (location = 0) in vec3 vertexPos;
layout (location = 1) in vec2 uv;

uniform mat4 MVP;

vec3 vprod (vec3 u, vec3 v)
{
  return vec3 (u.y * v.z - u.z * v.y, 
               u.z * v.x - u.x * v.z, 
               u.x * v.y - u.y * v.x);
}

void main()
{
  vec3 u = normalize (vprod (vec3 (0., 0., 1.), vertexPos));
  vec3 v = normalize (vprod (vertexPos, u));

  vec3 pos;

  if (gl_VertexID == 0)
    pos = vec3 (0., 0., 0.);
  else if (gl_VertexID == 1)
    pos = vec3 (1., 0., 0.);

  pos = vertexPos + (pos.x * uv.x - pos.y * uv.y) * u + (pos.x * uv.y + pos.y * uv.x) * v;

  gl_Position =  MVP * vec4 (pos, 1);

}


)CODE");

  glUseProgram (programID);


  float lon = 0., lat = 0.;

  bool ortho = true;

  while (1) 
    {   
      float coslon = cos (glm::radians (lon)), sinlon = sin (glm::radians (lon));
      float coslat = cos (glm::radians (lat)), sinlat = sin (glm::radians (lat));
      float x = 6.0 * coslon * coslat, y = 6.0 * sinlon * coslat, z = 6.0 * sinlat;

      glm::mat4 Projection = ortho ? glm::ortho(-1.0f, +1.0f, -1.0f, +1.0f, 0.1f, 100.0f)
                                   : glm::perspective (glm::radians (20.0f), (float)width/(float)height, 0.1f, 100.0f);
      glm::mat4 View       = glm::lookAt (glm::vec3 (x, y, z), glm::vec3 (0,0,0), glm::vec3 (0,0,1));
      glm::mat4 Model      = glm::mat4 (1.0f);
      glm::mat4 MVP = Projection * View * Model; 
      glUniformMatrix4fv (glGetUniformLocation (programID, "MVP"), 1, GL_FALSE, &MVP[0][0]);

      glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      glBindVertexArray (VertexArrayID);
      glDrawArraysInstanced (GL_LINES, 0, 2, np); 

      glfwSwapBuffers (window);
      glfwPollEvents (); 
  
      if (glfwGetKey (window, GLFW_KEY_SPACE) == GLFW_PRESS)
        {
          lat = 0.; lon = 0.;
        }
      if (glfwGetKey (window, GLFW_KEY_UP) == GLFW_PRESS)
        lat += 1;
      if (glfwGetKey (window, GLFW_KEY_DOWN) == GLFW_PRESS)
        lat -= 1;
      if (glfwGetKey (window, GLFW_KEY_LEFT) == GLFW_PRESS)
        lon += 1;
      if (glfwGetKey (window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        lon -= 1;
      if (glfwGetKey (window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        break;
      if (glfwGetKey (window, GLFW_KEY_O) == GLFW_PRESS)
        ortho = ! ortho;
      if (glfwWindowShouldClose (window) != 0)  
        break;
    }   


  glfwTerminate ();


  return 0;
}
