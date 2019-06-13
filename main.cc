#include <GL/glew.h>
#include <GLFW/glfw3.h>


#include "bmp.h"
#include "gensphere.h"
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


static bool uselist = false;

typedef struct
{
  std::vector<unsigned int> ind;
  std::vector<float> xyz1;
} isoline_data_t;

typedef struct
{
  GLuint VertexArrayID;
  GLuint vertexbuffer, elementbuffer;
  GLuint size;
} isoline_t;

void process (const jlonlat_t & jlonlat0, const float * r, const float r0,
              bool * seen, const geom_t & geom, const float * xyz, 
              isoline_data_t * iso, const std::vector<neigh_t> & neighlist)
{
  bool dbg = false;

  int ind_start = iso->xyz1.size () / 3;   // Number of points so far

  neigh_t neigh0 = uselist ?  neighlist[geom.jglo (jlonlat0)] : geom.getNeighbours (jlonlat0);

  jlonlat_t jlonlat = jlonlat0;         // Current position
  neigh_t neigh = neigh0;               // Current neighbours
  neigh_t::pos_t pos1 = neigh_t::I_E;   // Current edge
  neigh_t::rot_t rot1 = neigh_t::P;     // Current direction of rotation 

  int count = 0;                        // Number of points we added for the current line
  while (1)
    {
      int jglo0 = geom.jglo (jlonlat);

      if (neigh.done (&seen[8*jglo0]))  // All edges of current point have been explored
        {
          if (count > 1)                // More than one point have been added; try to close the current line
            {
              int jglo0 = geom.jglo (jlonlat0);
              bool close = false;
              if (jlonlat == jlonlat0)  // We came back to the first point
                close = true;
	      else 
                for (int i = 0; i < 8; i++)
                  if (geom.jglo (neigh.jlonlat[i]) == jglo0)
                    {
                      close = true;    // We came back to a point near to the first point
                      break;
		    }
	      if (close)               // Close the current line
                {
                  iso->ind.push_back (iso->ind.back ());
                  iso->ind.push_back (ind_start);
	        }
	    }
          break;                       // Exit main loop; this is the only place where it is possible to exit
        }

      neigh_t::pos_t pos2 = neigh.next (pos1, rot1);  // Next edge, following current direction of rotation
                                                      // We consider two edges: current edge (pos1), and next edge

      if (seen[8*jglo0+pos1])                         // Current edge has been explored; skip
        goto next;

      {
        int jglo1 = geom.jglo (neigh.jlonlat[pos1]);   
        int jglo2 = geom.jglo (neigh.jlonlat[pos2]);

	// Points are black
        bool b0 = r[jglo0] >= r0;   
        bool b1 = r[jglo1] >= r0;
        bool b2 = r[jglo2] >= r0;
        
	// Points are white
        bool w0 = r[jglo0] <= r0;
        bool w1 = r[jglo1] <= r0;
        bool w2 = r[jglo2] <= r0;
        
	neigh_t::pos_t pos0 = neigh_t::opposite (pos1);  // If current edge points to NE 
	                                                 // then pointed grid-point points back to SW

        seen[8*jglo0+pos1] = true;                       // Mark these edges as visited
        seen[8*jglo1+pos0] = true;

        auto push = [=, &count]()                        
	{ 
          float a = (r0 - r[jglo0]) / (r[jglo1] - r[jglo0]);        // Weight

	  // Coordinates of point
	  //
          float X = (1 - a) * xyz[3*jglo0+0] + a * xyz[3*jglo1+0];  
          float Y = (1 - a) * xyz[3*jglo0+1] + a * xyz[3*jglo1+1];
          float Z = (1 - a) * xyz[3*jglo0+2] + a * xyz[3*jglo1+2];

	  // Normalize
	  float R = sqrt (X * X + Y * Y + Z * Z);
	  X /= R; Y /= R; Z /= R;

          iso->xyz1.push_back (X);
          iso->xyz1.push_back (Y);
          iso->xyz1.push_back (Z);
          
	  // We need at least two points in order to start drawing lines
          if (count > 0)
            {
              iso->ind.push_back (ind_start + count - 1);
              iso->ind.push_back (ind_start + count + 0);
            }

          if(dbg)
            if (count > 0)
              printf (" %8d %7.2f %7.2f %7.2f\n", ind_start + count, X, Y, Z);

          count++;


	};

	// 8 different cases
	
             if (b0 && b1 && b2) // Same colors for all three points; skip
          {
            goto next;
          }
        else if (b0 && b1 && w2) // Same colors for current point and point #1 (pointed by current edge); skip
          {
            goto next;
          }
        else if (b0 && w1 && b2) // A
          {
            push (); 
	    jlonlat = neigh.jlonlat[pos2];                             pos1 = neigh_t::opposite (pos2);
            neigh = uselist ? neighlist[jglo2] : geom.getNeighbours (jlonlat);
	    continue;
          }
        else if (b0 && w1 && w2) // B
          {
            push (); 
	    pos1 = pos2;
	    continue;
          }
        else if (w0 && b1 && b2) // C
          {
            push (); 
	    jlonlat = neigh.jlonlat[pos2]; rot1 = neigh_t::inv (rot1); pos1 = neigh_t::opposite (pos2);
            neigh = uselist ? neighlist[jglo2] : geom.getNeighbours (jlonlat);
	    continue;
          }
        else if (w0 && b1 && w2) // D
          {
            push (); 
	    jlonlat = neigh.jlonlat[pos1]; rot1 = neigh_t::inv (rot1); pos1 = neigh_t::opposite (pos1);
            neigh = uselist ? neighlist[jglo1] : geom.getNeighbours (jlonlat);
	    continue;
          }
        else if (w0 && w1 && w2)
          {
            goto next;
          }
        else if (w0 && w1 && w2)
        {
          goto next;
	}

      }

next:

      pos1 = pos2; // Next edge
    }

// A single point was added; not enough to make a line
if (count == 1)
  {
    iso->xyz1.pop_back ();
    iso->xyz1.pop_back ();
    iso->xyz1.pop_back ();
  }

if (dbg)
if (count > 1)
{

  printf ("> %d\n", count );

  std::cout << " r0 = " << r0 << std::endl;

  neigh0.prn (geom, jlonlat0);

  int jglo0 = geom.jglo (jlonlat0);
  std::cout << " done = " << neigh0.done (&seen[8*jglo0]) << std::endl;
  std::cout << "I_E " << neigh0.jlonlat[neigh_t::I_E].ok () << " " << seen[8*jglo0+0]; if (neigh0.jlonlat[neigh_t::I_E].ok ()) printf ("%30.20f", r[geom.jglo (neigh0.jlonlat[neigh_t::I_E])]); printf ("\n"); 
  std::cout << "INE " << neigh0.jlonlat[neigh_t::INE].ok () << " " << seen[8*jglo0+1]; if (neigh0.jlonlat[neigh_t::INE].ok ()) printf ("%30.20f", r[geom.jglo (neigh0.jlonlat[neigh_t::INE])]); printf ("\n"); 
  std::cout << "IN_ " << neigh0.jlonlat[neigh_t::IN_].ok () << " " << seen[8*jglo0+2]; if (neigh0.jlonlat[neigh_t::IN_].ok ()) printf ("%30.20f", r[geom.jglo (neigh0.jlonlat[neigh_t::IN_])]); printf ("\n"); 
  std::cout << "INW " << neigh0.jlonlat[neigh_t::INW].ok () << " " << seen[8*jglo0+3]; if (neigh0.jlonlat[neigh_t::INW].ok ()) printf ("%30.20f", r[geom.jglo (neigh0.jlonlat[neigh_t::INW])]); printf ("\n"); 
  std::cout << "I_W " << neigh0.jlonlat[neigh_t::I_W].ok () << " " << seen[8*jglo0+4]; if (neigh0.jlonlat[neigh_t::I_W].ok ()) printf ("%30.20f", r[geom.jglo (neigh0.jlonlat[neigh_t::I_W])]); printf ("\n"); 
  std::cout << "ISW " << neigh0.jlonlat[neigh_t::ISW].ok () << " " << seen[8*jglo0+5]; if (neigh0.jlonlat[neigh_t::ISW].ok ()) printf ("%30.20f", r[geom.jglo (neigh0.jlonlat[neigh_t::ISW])]); printf ("\n"); 
  std::cout << "IS_ " << neigh0.jlonlat[neigh_t::IS_].ok () << " " << seen[8*jglo0+6]; if (neigh0.jlonlat[neigh_t::IS_].ok ()) printf ("%30.20f", r[geom.jglo (neigh0.jlonlat[neigh_t::IS_])]); printf ("\n"); 
  std::cout << "ISE " << neigh0.jlonlat[neigh_t::ISE].ok () << " " << seen[8*jglo0+7]; if (neigh0.jlonlat[neigh_t::ISE].ok ()) printf ("%30.20f", r[geom.jglo (neigh0.jlonlat[neigh_t::ISE])]); printf ("\n"); 

  printf ("<");
  neigh.prn (geom, jlonlat);

  int jglo = geom.jglo (jlonlat);
  std::cout << " done = " << neigh0.done (&seen[8*jglo]) << std::endl;
  std::cout << "I_E " << neigh.jlonlat[neigh_t::I_E].ok () << " " << seen[8*jglo+0]; if (neigh.jlonlat[neigh_t::I_E].ok ()) printf ("%30.20f", r[geom.jglo (neigh.jlonlat[neigh_t::I_E])]); printf ("\n"); 
  std::cout << "INE " << neigh.jlonlat[neigh_t::INE].ok () << " " << seen[8*jglo+1]; if (neigh.jlonlat[neigh_t::INE].ok ()) printf ("%30.20f", r[geom.jglo (neigh.jlonlat[neigh_t::INE])]); printf ("\n"); 
  std::cout << "IN_ " << neigh.jlonlat[neigh_t::IN_].ok () << " " << seen[8*jglo+2]; if (neigh.jlonlat[neigh_t::IN_].ok ()) printf ("%30.20f", r[geom.jglo (neigh.jlonlat[neigh_t::IN_])]); printf ("\n"); 
  std::cout << "INW " << neigh.jlonlat[neigh_t::INW].ok () << " " << seen[8*jglo+3]; if (neigh.jlonlat[neigh_t::INW].ok ()) printf ("%30.20f", r[geom.jglo (neigh.jlonlat[neigh_t::INW])]); printf ("\n"); 
  std::cout << "I_W " << neigh.jlonlat[neigh_t::I_W].ok () << " " << seen[8*jglo+4]; if (neigh.jlonlat[neigh_t::I_W].ok ()) printf ("%30.20f", r[geom.jglo (neigh.jlonlat[neigh_t::I_W])]); printf ("\n"); 
  std::cout << "ISW " << neigh.jlonlat[neigh_t::ISW].ok () << " " << seen[8*jglo+5]; if (neigh.jlonlat[neigh_t::ISW].ok ()) printf ("%30.20f", r[geom.jglo (neigh.jlonlat[neigh_t::ISW])]); printf ("\n"); 
  std::cout << "IS_ " << neigh.jlonlat[neigh_t::IS_].ok () << " " << seen[8*jglo+6]; if (neigh.jlonlat[neigh_t::IS_].ok ()) printf ("%30.20f", r[geom.jglo (neigh.jlonlat[neigh_t::IS_])]); printf ("\n"); 
  std::cout << "ISE " << neigh.jlonlat[neigh_t::ISE].ok () << " " << seen[8*jglo+7]; if (neigh.jlonlat[neigh_t::ISE].ok ()) printf ("%30.20f", r[geom.jglo (neigh.jlonlat[neigh_t::ISE])]); printf ("\n"); 


}

  int jglo0 = geom.jglo (jlonlat0);
  if (! neigh.done (&seen[8*jglo0]))  // All edges of current point have not been explored
    process (jlonlat0, r, r0, seen, geom, xyz, iso, neighlist);

}


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
	  case GLFW_KEY_TAB:
            rotate = ! rotate;
	    break;
	}
    }
}


int main (int argc, char * argv[])
{
  float * xyz;
  int np; 
  unsigned int nt;
  unsigned int * ind;

  geom_t geom;

  const int width = 1024, height = 1024;
  int w, h;
  unsigned char * r = NULL;
  float * F = NULL;
  geom.Nj = atoi (argv[1]);


  std::string type = "gradx";
 
  if (argc > 2)
    type = argv[2];

  gensphere (&geom, &np, &xyz, &nt, &ind, &F, type);

  int size = 0;
  for (int jlat = 1; jlat <= geom.Nj-1; jlat++)
    size += geom.pl[jlat-1];

  float maxval = *std::max_element (F, F + size);
  float minval = *std::min_element (F, F + size);

  std::cout << minval << " " << maxval << std::endl;

  r = (unsigned char *)malloc (sizeof (unsigned char) * size);
  for (int i = 0; i < size; i++)
    r[i] = 255 * (F[i] - minval) / (maxval - minval);

  if (0)
  {
  for (int i = 0; i < geom.pl[0]; i++)
    printf ("%4d %7.2f\n", i, F[i]);
  int count = 0;
  for (int i = 0; i < geom.Nj; i++)
    {
      printf ("%d\n", geom.pl[i]);
      count += geom.pl[i];
    }
    printf (" %d\n", count);
  }

  std::vector<neigh_t> neighlist;
  if (uselist)
  neighlist = geom.getNeighbours ();

  const int N = 5;
  isoline_data_t iso_data[N];

  for (int i = 0; i < N; i++)
    {
      bool * seen = (bool *)malloc (sizeof (bool) * 9 * np);
      float F0 = minval + (i + 1) * (maxval - minval) / (N + 1);
      std::cout << F0 << std::endl;

      for (int i = 0; i < 9 * np; i++)
        seen[i] = false;
     
      // Process edges of the domain first
     
      // Top
      for (int jlat = 1; jlat <= 1; jlat++)
        for (int jlon = 1; jlon <= geom.pl[jlat-1]; jlon++)
          process (jlonlat_t (jlon, jlat), F, F0, seen, geom, xyz, 
                   &iso_data[i], neighlist);
     
      // Bottom
      for (int jlat = geom.Nj; jlat <= geom.Nj; jlat++)
        for (int jlon = 1; jlon <= geom.pl[jlat-1]; jlon++)
          process (jlonlat_t (jlon, jlat), F, F0, seen, geom, xyz, 
                   &iso_data[i], neighlist);
     
      // Process center of the domain
      for (int jlat = 2; jlat < geom.Nj; jlat++)
        for (int jlon = 1; jlon <= geom.pl[jlat-1]; jlon++)
          process (jlonlat_t (jlon, jlat), F, F0, seen, geom, xyz, 
                   &iso_data[i], neighlist);

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
  glBufferData (GL_ELEMENT_ARRAY_BUFFER, 3 * nt * sizeof (unsigned int), ind , GL_STATIC_DRAW);

  // Line 

  isoline_t iso[N];
  for (int i = 0; i < N; i++)
    {
      glGenVertexArrays (1, &iso[i].VertexArrayID);
      glBindVertexArray (iso[i].VertexArrayID);
     
      glGenBuffers (1, &iso[i].vertexbuffer);
      glBindBuffer (GL_ARRAY_BUFFER, iso[i].vertexbuffer);
      glBufferData (GL_ARRAY_BUFFER, iso_data[i].xyz1.size () * sizeof (float), 
                    iso_data[i].xyz1.data (), GL_STATIC_DRAW);
      glEnableVertexAttribArray (0); 
      glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, NULL); 
      glGenBuffers (1, &iso[i].elementbuffer);
      glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, iso[i].elementbuffer);
      glBufferData (GL_ELEMENT_ARRAY_BUFFER, iso_data[i].ind.size () * sizeof (unsigned int), 
		    iso_data[i].ind.data (), GL_STATIC_DRAW);
      iso[i].size = iso_data[i].ind.size ();
    }

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
  vec3 pos = 1.00 * vertexPos;
  gl_Position = MVP * vec4 (pos, 1);

  fragmentColor.r = vertexCol.r;
  fragmentColor.g = 0.;
  fragmentColor.b = 1. - vertexCol.r;
  fragmentColor.a = 1.;

}
)CODE");

  GLuint programID_l = shader 
(
R"CODE(
#version 330 core

out vec4 color;
in vec3 col;

void main()
{
  if(true){
  color.r = 0.;
  color.g = 1.;
  color.b = 0.;
  }else{
  color.r = col.r;
  color.g = col.g;
  color.b = col.b;
  }
  color.a = 1.;
}
)CODE",
R"CODE(
#version 330 core

layout(location = 0) in vec3 vertexPos;

out vec3 col;


uniform mat4 MVP;

void main()
{
  gl_Position =  MVP * vec4 (vertexPos, 1);
  col.x = (1 + vertexPos.x) / 2.0;
  col.y = (1 + vertexPos.y) / 2.0;
  col.z = (1 + vertexPos.z) / 2.0;
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
      glUseProgram (programID_l);
      glUniformMatrix4fv (glGetUniformLocation (programID_l, "MVP"), 1, GL_FALSE, &MVP[0][0]);
      for (int i = 0; i < N; i++)
        {
          glBindVertexArray (iso[i].VertexArrayID);
          glLineWidth (7.0f);
          glDrawElements (GL_LINES, iso[i].size, GL_UNSIGNED_INT, NULL);
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
