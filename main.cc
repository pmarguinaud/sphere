#include <GL/glew.h>
#include <GLFW/glfw3.h>


#include "shader.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <algorithm>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <unistd.h>
#include <sys/time.h>
#include "load.h"
#include "gensphere.h"


class isoline_data_t
{
public:
  std::vector<unsigned int> ind;
  std::vector<float> xyz;
  std::vector<float> drw;
  void push (const float x, const float y, const float z, const float d = 1.)
  {
    xyz.push_back (x);
    xyz.push_back (y);
    xyz.push_back (z);
    drw.push_back (d);
  }
  void pop ()
  {
    xyz.pop_back ();
    xyz.pop_back ();
    xyz.pop_back ();
    drw.pop_back ();
  }
  void clear ()
  {
    xyz.clear ();
    ind.clear ();
    drw.clear ();
  }
  int size ()
  {
    return xyz.size () / 3;
  }
};

typedef struct
{
  GLuint VertexArrayID;
  GLuint vertexbuffer, elementbuffer, normalbuffer;
  GLuint size;    // Number of indices
  GLuint size_inst;
} isoline_t;

bool triNeigh (const geom_t & geom, const float * r, const float r0, 
               int it, int jglo[3], int itri[3])
{
  int jgloA = geom.ind[3*it+0]; bool bA = r[jgloA] < r0;
  int jgloB = geom.ind[3*it+1]; bool bB = r[jgloB] < r0;
  int jgloC = geom.ind[3*it+2]; bool bC = r[jgloC] < r0;
  
  if ((bA == bB) && (bB == bC))  // All points have same color
    return false;
  
  jlonlat_t jlonlatA = geom.jlonlat (jgloA);
  jlonlat_t jlonlatB = geom.jlonlat (jgloB);
  jlonlat_t jlonlatC = geom.jlonlat (jgloC);
  
  int jglo0, jglo1, jglo2;
  jlonlat_t jlonlat0, jlonlat1, jlonlat2;
  
  if (jlonlatA.jlat == jlonlatB.jlat)
    {
      jglo0    = jgloA;    jglo1    = jgloB;    jglo2    = jgloC;    
      jlonlat0 = jlonlatA; jlonlat1 = jlonlatB; jlonlat2 = jlonlatC;
    }
  else 
  if (jlonlatA.jlat == jlonlatC.jlat)
    {
      jglo0    = jgloA;    jglo1    = jgloC;    jglo2    = jgloB;    
      jlonlat0 = jlonlatA; jlonlat1 = jlonlatC; jlonlat2 = jlonlatB;
    }
  else 
  if (jlonlatB.jlat == jlonlatC.jlat)
    {
      jglo0    = jgloB;    jglo1    = jgloC;    jglo2    = jgloA;    
      jlonlat0 = jlonlatB; jlonlat1 = jlonlatC; jlonlat2 = jlonlatA;
    }
  
  bool up = jlonlat2.jlat < jlonlat0.jlat;
  
  int ntri = geom.pl[jlonlat0.jlat-1] + geom.pl[jlonlat2.jlat-1];                    // Number of triangles on this row
  int lat1 = up ? jlonlat2.jlat : jlonlat0.jlat;
  int otri = lat1 == 1 ? 0 : geom.jglooff[lat1] * 2 - geom.pl[lat1-1] - geom.pl[0];  // Offset of triangles counting
  int ktri = it - otri;                                                              // Rank of current triangle in current row
  int Ltri = ktri == 0      ? ntri-1 : ktri-1; Ltri += otri;                         // Rank of left triangle
  int Rtri = ktri == ntri-1 ?      0 : ktri+1; Rtri += otri;                         // Rank of right triangle
  int Vtri = up ? geom.trid[jglo0] : geom.triu[jglo0];                               // Rank of triangle above of under segment 01

  int itr01 = Vtri, itr12 = Rtri, itr20 = Ltri;
  
  jglo[0] = jglo0; jglo[1] = jglo1; jglo[2] = jglo2;
  itri[0] = itr01; itri[1] = itr12; itri[2] = itr20;

  return true;
}


void process (int it0, const float * r, const float r0, bool * seen, 
              const geom_t & geom, const float * xyz, isoline_data_t * iso)
{
  int count = 0;
  bool cont = true;
  bool edge = false;
  int it = it0; 
  int its[2];
  static int II = 0;
  bool dbg = false;

  while (cont)
    {
      cont = false;

      if (seen[it])
        break;
      
      seen[it] = true;

      int jglo[3], itri[3];

      if (! triNeigh (geom, r, r0, it, jglo, itri))
        break;

      if (count == 0)
        {
          int c = 0;
          for (int i = 0; i < 3; i++)
            {
              int iA = i, iB = (i + 1) % 3;
              int jgloA = jglo[iA], jgloB = jglo[iB];
              bool bA = r[jgloA] < r0, bB = r[jgloB] < r0;
              int itAB = itri[iA];
              if ((bA != bB) && (! seen[itAB]))
                c++;
            }
          edge = c != 2;
        }
      
      for (int i = 0; i < 3; i++)
        {
          int iA = i, iB = (i + 1) % 3;
          int jgloA = jglo[iA], jgloB = jglo[iB];
          bool bA = r[jgloA] < r0, bB = r[jgloB] < r0;
          int itAB = itri[iA];
          if ((bA != bB) && (! seen[itAB]))
            {
              int jgloa = jgloA < jgloB ? jgloA : jgloB;
              int jglob = jgloA < jgloB ? jgloB : jgloA;
              float a = (r0 - r[jgloa]) / (r[jglob] - r[jgloa]);
              // Coordinates of point
              float X = (1 - a) * xyz[3*jgloa+0] + a * xyz[3*jglob+0];  
              float Y = (1 - a) * xyz[3*jgloa+1] + a * xyz[3*jglob+1];
              float Z = (1 - a) * xyz[3*jgloa+2] + a * xyz[3*jglob+2];
              // Normalize
              float R = sqrt (X * X + Y * Y + Z * Z);
              X /= R; Y /= R; Z /= R;
      
              iso->push (X, Y, Z);
      
              if (dbg)
              printf (" %4d %4d %6.2f %6.2f %6.2f %4d\n", count, it, X, Y, Z, itAB);

              if (count < 2)
                its[count] = it;
      
              it = itAB;
              count++;
              cont = true;
              break;
            }
        }

      if ((count == 2) && (! edge))
        seen[its[0]] = false;
      if ((count == 3) && (! edge))
        seen[its[1]] = false;
    }

  if (count > 0)
    {
      iso->push (0., 0., 0., 0.);
      if (dbg)
        printf ("--------------------------------- %d\n", II++);
    }

  return;
}

static bool verbose = false;
static float lonc = 0.0f;
static float latc = 0.0f;
static float R = 6.0f;
static float fov = 20.0f;
static bool wireframe = false;
static bool rotate = false;

static 
void key_callback (GLFWwindow * window, int key, int scancode, int action, int mods)
{
  if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
      switch (key)
        {
          case GLFW_KEY_UP:
            latc += 5;
if (verbose)
std::cout << lonc << " " << latc << std::endl;
	    break;
          case GLFW_KEY_DOWN:
            latc -= 5;
if (verbose)
std::cout << lonc << " " << latc << std::endl;
	    break;
          case GLFW_KEY_LEFT:
            lonc -= 5;
if (verbose)
std::cout << lonc << " " << latc << std::endl;
	    break;
          case GLFW_KEY_RIGHT:
            lonc += 5;
if (verbose)
std::cout << lonc << " " << latc << std::endl;
	    break;
	  case GLFW_KEY_SPACE:
            lonc  = 0; latc = 0; R = 6.0; fov = 20.0f;
	    break;
	  case GLFW_KEY_F1:
            fov += 1;
if (verbose)
std::cout << fov << std::endl;
	    break;
	  case GLFW_KEY_F2:
            fov -= 1;
if (verbose)
std::cout << fov << std::endl;
	    break;
	  case GLFW_KEY_F3:
            wireframe = ! wireframe;
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
  float * xyz;
  int np; 
  unsigned int nt;
  bool remove_open;
 
  const char * REMOVE_OPEN = getenv ("REMOVE_OPEN");

  remove_open = (REMOVE_OPEN && atoi (REMOVE_OPEN));

  geom_t geom;

  const int width = 1024, height = 1024;
  int w, h;
  unsigned char * r = NULL;
  float * F = NULL;

  if (endsWith (argv[1], std::string (".grb")))
    {
      gensphere_grib (&geom, &np, &xyz, &nt, &F, argv[1]);
      if (argc > 2)
        gensphere (&geom, &np, &xyz, &nt, &F, argv[2]);
    }
  else
    {
      geom.Nj = atoi (argv[1]);
      std::string type = "gradx";
      if (argc > 2)
        type = argv[2];
      gensphere (&geom, &np, &xyz, &nt, &F, type);
    }


  int size = 0;
  for (int jlat = 1; jlat <= geom.Nj-1; jlat++)
    size += geom.pl[jlat-1];

  float maxval = *std::max_element (F, F + size);
  float minval = *std::min_element (F, F + size);

  std::cout << minval << " " << maxval << std::endl;

  r = (unsigned char *)malloc (sizeof (unsigned char) * size);
  for (int i = 0; i < size; i++)
    r[i] = 255 * (F[i] - minval) / (maxval - minval);



  const int N = 10;
  isoline_data_t iso_data[N];

#pragma omp parallel for
  for (int i = 0; i < N; i++)
    {
      bool * seen = (bool *)malloc (sizeof (bool) * (nt + 1));
      float F0 = minval + (i + 1) * (maxval - minval) / (N + 1);

      for (int i = 0; i < nt+1; i++)
        seen[i] = false;
      seen[0] = true;
     
      for (int it = 0; it < nt; it++)
        process (it, F, F0, seen+1, geom, xyz, &iso_data[i]);

      free (seen);
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
  
  glfwSetKeyCallback (window, key_callback);

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
  glBufferData (GL_ELEMENT_ARRAY_BUFFER, 3 * nt * sizeof (unsigned int), geom.ind , GL_STATIC_DRAW);

  std::cout << " nt = " << nt << " np = " << np << std::endl;

  // Line 



  isoline_t iso[N];
  for (int i = 0; i < N; i++)
    {
      iso[i].size = iso_data[i].ind.size ();

      iso[i].size_inst = iso_data[i].size () - 1;

      glGenVertexArrays (1, &iso[i].VertexArrayID);
      glBindVertexArray (iso[i].VertexArrayID);
     
      glGenBuffers (1, &iso[i].vertexbuffer);
      glBindBuffer (GL_ARRAY_BUFFER, iso[i].vertexbuffer);
      glBufferData (GL_ARRAY_BUFFER, iso_data[i].xyz.size () * sizeof (float), 
                    iso_data[i].xyz.data (), GL_STATIC_DRAW);

      glEnableVertexAttribArray (0); 
      glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, NULL); 
      glVertexAttribDivisor (0, 1);

      glEnableVertexAttribArray (1); 
      glVertexAttribPointer (1, 3, GL_FLOAT, GL_FALSE, 0, (const void *)(3 * sizeof (float))); 
      glVertexAttribDivisor (1, 1);

      glEnableVertexAttribArray (2); 
      glVertexAttribPointer (2, 3, GL_FLOAT, GL_FALSE, 0, (const void *)(6 * sizeof (float))); 
      glVertexAttribDivisor (2, 1);


      glGenBuffers (1, &iso[i].normalbuffer);
      glBindBuffer (GL_ARRAY_BUFFER, iso[i].normalbuffer);
      glBufferData (GL_ARRAY_BUFFER, iso_data[i].drw.size () * sizeof (float), 
                    iso_data[i].drw.data (), GL_STATIC_DRAW);

      glEnableVertexAttribArray (3); 
      glVertexAttribPointer (3, 1, GL_FLOAT, GL_FALSE, 0, NULL); 
      glVertexAttribDivisor (3, 1);

      glEnableVertexAttribArray (4); 
      glVertexAttribPointer (4, 1, GL_FLOAT, GL_FALSE, 0, (const void *)(sizeof (float))); 
      glVertexAttribDivisor (4, 1);


      iso_data[i].xyz.clear ();
      iso_data[i].ind.clear ();
      

    }

  GLuint programID = shader 
(
R"CODE(
#version 330 core

in vec4 fragmentColor;

out vec4 color;

void main()
{
  color.r = 1.;
  color.g = 1.;
  color.b = 1.;
  color.a = 1.;
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
  vec3 pos = 0.99 * vertexPos;
  gl_Position = MVP * vec4 (pos, 1);

  fragmentColor.r = vertexCol.r;
  fragmentColor.g = 0.;
  fragmentColor.b = 1. - vertexCol.r;
  fragmentColor.a = 1.;

}
)CODE");

  GLuint programID_l_inst = shader 
(
R"CODE(
#version 330 core

out vec4 color;
in vec3 col;
in float instid;
in float norm;

void main()
{
  if(true){
  if (instid < 0.6)
    {
      color.r = 0.;
      color.g = 1.;
      color.b = 0.;
    }
  else
    {
      color.r = 0.;
      color.g = 1.;
      color.b = 0.;
    }
  if (norm < 1.) 
    {
      color.a = 0.;
    }
  else
    {
      color.a = 1.;
    }
  }else{
  color.r = col.r;
  color.g = col.g;
  color.b = col.b;
  color.a = 1.;
  }
}
)CODE",
R"CODE(
#version 330 core

layout(location = 0) in vec3 vertexPos0;
layout(location = 1) in vec3 vertexPos1;
layout(location = 2) in vec3 vertexPos2;
layout(location = 3) in float norm0;
layout(location = 4) in float norm1;

out vec3 col;
out float instid;
out float norm;


uniform mat4 MVP;

void main()
{
  vec3 vertexPos;
  vec3 t = normalize (vertexPos1 - vertexPos0);

  if ((gl_VertexID == 0) || (gl_VertexID == 2))
    vertexPos = vertexPos0;
  else if ((gl_VertexID == 1) || (gl_VertexID == 3))
    vertexPos = vertexPos1;

  vec3 p = normalize (vertexPos);
  vec3 n = cross (t, p);

  if (gl_VertexID == 2)
    vertexPos = vertexPos + 0.05 * n;


  gl_Position =  MVP * vec4 (vertexPos, 1);
  col.x = (1 + vertexPos.x) / 2.0;
  col.y = (1 + vertexPos.y) / 2.0;
  col.z = (1 + vertexPos.z) / 2.0;
  instid = mod (gl_InstanceID, 2);
  norm = min (norm0, norm1);




}
)CODE");


  if(0){
  GLfloat lineWidthRange[2] = {0.0f, 0.0f};
  glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, lineWidthRange);
  std::cout << lineWidthRange[0] << " " << lineWidthRange[1] << std::endl;
  }



  while (1) 
    {   
      glm::mat4 Projection = glm::perspective (glm::radians (fov), 1.0f, 0.1f, 100.0f);
      glm::mat4 View       = glm::lookAt (glm::vec3 (R * cos (lonc * M_PI / 180.0f) * cos (latc * M_PI / 180.0f), 
                                                     R * sin (lonc * M_PI / 180.0f) * cos (latc * M_PI / 180.0f),
                                                     R * sin (latc * M_PI / 180.0f)), 
                                          glm::vec3 (0,0,0), glm::vec3 (0,0,1));
      glm::mat4 Model      = glm::mat4 (1.0f);
      glm::mat4 MVP = Projection * View * Model; 

      glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      if(1){
      // Sphere
      glUseProgram (programID);
      glUniformMatrix4fv (glGetUniformLocation (programID, "MVP"), 1, GL_FALSE, &MVP[0][0]);
      glBindVertexArray (VertexArrayID);
      if (wireframe)
        glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
      else
        glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
      glDrawElements (GL_TRIANGLES, 3 * nt, GL_UNSIGNED_INT, NULL);
      }

      // Line 
      //
      glUseProgram (programID_l_inst);
      glUniformMatrix4fv (glGetUniformLocation (programID_l_inst, "MVP"), 
			  1, GL_FALSE, &MVP[0][0]);
      for (int i = 0; i < N; i++)
        {
          glBindVertexArray (iso[i].VertexArrayID);
          glDrawArraysInstanced (GL_LINE_STRIP, 0, 2, iso[i].size_inst);
          glBindVertexArray (0);
        }


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
