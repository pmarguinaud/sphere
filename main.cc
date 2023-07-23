#include <GL/glew.h>
#include <GLFW/glfw3.h>


#include "bmp.h"
#include "gensphere.h"
#include "shader.h"
#include <iostream>
#include <vector>
#include <stdexcept>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


class checker
{
public:
  const unsigned int nt = 2;
  const int width, height;
  const std::vector<unsigned int> ind;
  const int Nx, Ny;
  GLuint programID;
  checker (int _width, int _height, int _Nx, int _Ny) : width (_width), height (_height), 
    ind (std::vector<unsigned int>{0, 1, 2, 0, 2, 3}), Nx (_Nx), Ny (_Ny)
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
  int jy = j / Nx;

  vec2 vertexPos;
  if (i == 0) 
    vertexPos = vec2 (0.0f, 0.0f);
  else if (i == 1)
    vertexPos = vec2 (  dx, 0.0f);
  else if (i == 2)
    vertexPos = vec2 (  dx,   dy);
  else if (i == 3)
    vertexPos = vec2 (0.0f,   dy);

  float x = float (jx) * dx;
  float y = float (jy) * dy;

  vertexPos = vertexPos + vec2 (x-1.0, y-1.0);

  gl_Position = vec4 (vertexPos.x, vertexPos.y, 0., 1.);
}
)CODE");

    glUseProgram (programID);
    GLuint VertexArrayID;
    GLuint elementbuffer;
    
    glGenVertexArrays (1, &VertexArrayID);
    glBindVertexArray (VertexArrayID);
    
    std::cout << " Nx = " << Nx << " Ny = " << Ny << std::endl;
    glUniform1i (glGetUniformLocation (programID, "Nx"), Nx);
    glUniform1i (glGetUniformLocation (programID, "Ny"), Ny);


  }

  void render () const
  {
    glUseProgram (programID);
    glDrawElementsInstanced (GL_TRIANGLES, 3 * nt, GL_UNSIGNED_INT, &ind[0], Nx * Ny);
  }

  ~checker ()
  {
    glDeleteProgram (programID);
  }
};

class tex
{
  public:

  int width, height;
  std::vector<unsigned char> rgb;
  unsigned int texture;

  tex (const std::string & path) 
  {
    bmp (path, &rgb, &width, &height);
    glGenTextures (1, &texture);
    glBindTexture (GL_TEXTURE_2D, texture); 
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, &rgb[0]);
  }

  tex (int _width, int _height) : width (_width), height (_height)
  {
    glGenTextures (1, &texture);
    glBindTexture (GL_TEXTURE_2D, texture); 
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
  }

  void bind (GLuint target) const
  {
    if (target != 0)
      throw std::runtime_error ("Unexpected texture target");
    glActiveTexture (GL_TEXTURE0);
    glBindTexture (GL_TEXTURE_2D, texture);
  }

  ~tex ()
  {
    glDeleteTextures (1, &texture);
  }
};

class sphere
{
  public:
  int Nj;
  int np; 
  std::vector<float> xyz;
  unsigned int nt; 
  std::vector<unsigned int> ind;
  GLuint VertexArrayID;
  GLuint vertexbuffer, elementbuffer;
  GLuint programID;
  float lon = 0., lat = 0.;

  sphere (int _Nj)
  {
    Nj = _Nj;

    gensphere (Nj, &np, &xyz, &nt, &ind);

    programID = shader 
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

uniform sampler2D tex;

void main()
{
  float lon = (atan (vertexPos.y, vertexPos.x) / 3.1415926 + 1.0) * 0.5;
  float lat = asin (vertexPos.z) / 3.1415926 + 0.5;
  gl_Position =  MVP * vec4 (vertexPos, 1);

  vec4 col = texture (tex, vec2 (lon, lat));
  fragmentColor.r = col.r;
  fragmentColor.g = col.g;
  fragmentColor.b = col.b;
  fragmentColor.a = 1.;
}
)CODE");

    glUseProgram (programID);

    glGenVertexArrays (1, &VertexArrayID);
    glBindVertexArray (VertexArrayID);

    glGenBuffers (1, &vertexbuffer);
    glBindBuffer (GL_ARRAY_BUFFER, vertexbuffer);
    glBufferData (GL_ARRAY_BUFFER, 3 * np * sizeof (float), &xyz[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray (0); 
    glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, nullptr); 
    
    glGenBuffers (1, &elementbuffer);
    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
    glBufferData (GL_ELEMENT_ARRAY_BUFFER, 3 * nt * sizeof (unsigned int), &ind[0], GL_STATIC_DRAW);

  }

  void render (const tex & tt) const
  {
    glUseProgram (programID);

    glm::mat4 Projection = glm::perspective (glm::radians (20.0f), 1.0f / 1.0f, 0.1f, 100.0f);

    const float deg2rad = M_PI / 180.0f;
    const float R = 6.0;
    float coslon = std::cos (deg2rad * lon), sinlon = std::sin (deg2rad * lon);
    float coslat = std::cos (deg2rad * lat), sinlat = std::sin (deg2rad * lat);
    float X = R * coslon * coslat;
    float Y = R * sinlon * coslat;
    float Z = R *          sinlat;

    glm::mat4 View       = glm::lookAt (glm::vec3 (X,Y,Z), glm::vec3 (0,0,0), glm::vec3 (0,0,1));
    glm::mat4 Model      = glm::mat4 (1.0f);
    glm::mat4 MVP = Projection * View * Model; 
    glUniformMatrix4fv (glGetUniformLocation (programID, "MVP"), 1, GL_FALSE, &MVP[0][0]);

    tt.bind (0);
    glUniform1i (glGetUniformLocation (programID, "tex"), 0);
    glDrawElements (GL_TRIANGLES, 3 * nt, GL_UNSIGNED_INT, nullptr);
  }

  ~sphere ()
  {
    glDeleteProgram (programID);
  }

  void incLon (float dlon)
  {
    lon += dlon;
  }

  void incLat (float dlat)
  {
    lat += dlat;
  }

};

int main (int argc, char * argv[])
{
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

  const int width = 1024, height = 1024;
  window = glfwCreateWindow (width, height, "", nullptr, nullptr);
    
  if (window == nullptr)
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

//tex tt ("Whole_world_-_land_and_oceans_8000.bmp");
  tex tt (800, 400);

  checker ck (tt.width, tt.height, 8, 4);

  if (1){
    unsigned framebuffer;
    glGenFramebuffers (1, &framebuffer);
    glBindFramebuffer (GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tt.texture, 0);

    glViewport (0, 0, ck.width, ck.height);

    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ck.render ();

    glDeleteFramebuffers (1, &framebuffer);

    glBindFramebuffer (GL_FRAMEBUFFER, 0); 


  }else if (1){
  while (1) 
    {   
      glViewport (0, 0, ck.width, ck.height);

      glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      ck.render ();

      glfwSwapBuffers (window);
      glfwPollEvents (); 
  
      if (glfwGetKey (window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        break;
      if (glfwWindowShouldClose (window) != 0)  
        break;
    }   
    return 0;
  }


  int Nj = atoi (argv[1]);

  std::cout << " Nj = " << Nj << std::endl;

  sphere ss (Nj);

  while (1) 
    {   
      glViewport (0, 0, width, height);

      glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      ss.render (tt);

      glfwSwapBuffers (window);
      glfwPollEvents (); 
  
      if (glfwGetKey (window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        break;
      if (glfwGetKey (window, GLFW_KEY_UP) == GLFW_PRESS)
        ss.incLat (+1);
      if (glfwGetKey (window, GLFW_KEY_DOWN) == GLFW_PRESS)
        ss.incLat (-1);
      if (glfwGetKey (window, GLFW_KEY_LEFT) == GLFW_PRESS)
        ss.incLon (-1);
      if (glfwGetKey (window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        ss.incLon (+1);
      if (glfwWindowShouldClose (window) != 0)  
        break;
    }   



  glfwTerminate ();


  return 0;
}
