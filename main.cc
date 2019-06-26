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


static bool inst = true;
static bool uselist = false;

class isoline_data_t
{
public:
  std::vector<unsigned int> ind;
  std::vector<float> xyz;
  std::vector<float> drw;
  void push (const float & x, const float & y, const float & z, const float d = 1.)
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

void process (const jlonlat_t & jlonlat0, const float * r, const float r0,
              bool * seen, const geom_t & geom, const float * xyz, 
              isoline_data_t * iso, const std::vector<neigh_t> & neighlist, bool edge, bool remove_open)
{
  static int II = 0;
  bool dbg = II == 762;


  int ind_start = iso->size ();         // Number of points so far

  neigh_t neigh0 = uselist ?  neighlist[geom.jglo (jlonlat0)] : geom.getNeighbours (jlonlat0);

  jlonlat_t jlonlat = jlonlat0;         // Current position
  neigh_t neigh = neigh0;               // Current neighbours
  int pos1 = 0;                         // Current edge
  neigh_t::rot_t rot1 = neigh_t::P;     // Current direction of rotation 

  int count = 0;                        // Number of points we added for the current line


  bool * seen0 = NULL, * seen1 = NULL;
  bool reset = false;
  bool closed = false;

  while (1)
    {
      int jglo0 = geom.jglo (jlonlat);

      if (count > 2 && ! reset)
        {
          *seen0 = false;
          *seen1 = false;
	  reset = true;
	}

      bool close = false;
      bool stop = false;
      if (neigh.done (&seen[neigh_t::NMAX*jglo0]))  // All edges of current point have been explored
        {
          if (count > 1)                // More than one point have been added; try to close the current line
            {
              int jglo0 = geom.jglo (jlonlat0);
              if (jlonlat == jlonlat0)  // We came back to the first point
                {
                  if (inst)
                    iso->pop ();
                  close = true;
                }
	    }
          stop = true;
        }


      if (count > 1)
      if ((iso->xyz[3*ind_start+0] == iso->xyz[3*(ind_start+count-1)+0])
       && (iso->xyz[3*ind_start+1] == iso->xyz[3*(ind_start+count-1)+1])
       && (iso->xyz[3*ind_start+2] == iso->xyz[3*(ind_start+count-1)+2]))
        {
	  stop = true;
	  closed = true;
	}

      if (close)               // Close the current line
        {
          iso->ind.push_back (iso->ind.back ());
          iso->ind.push_back (ind_start);
          if (inst)
            {
              iso->push (iso->xyz[3*ind_start+0], 
                         iso->xyz[3*ind_start+1], 
        		 iso->xyz[3*ind_start+2]);
            }
	  stop = true;
	  closed = true;
        }

      if (stop)
        break;                       // Exit main loop; this is the only place where it is possible to exit

      int pos2 = neigh.next (pos1, rot1);             // Next edge, following current direction of rotation
                                                      // We consider two edges: current edge (pos1), and next edge

      if (seen[neigh_t::NMAX*jglo0+pos1])             // Current edge has been explored; skip
        goto next;

      {
        jlonlat_t jlonlat1 = neigh.jlonlat[pos1];
        jlonlat_t jlonlat2 = neigh.jlonlat[pos2];
        int jglo1 = geom.jglo (jlonlat1);   
        int jglo2 = geom.jglo (jlonlat2);

	// Points are black
        bool b0 = r[jglo0] >= r0;   
        bool b1 = r[jglo1] >= r0;
        bool b2 = r[jglo2] >= r0;
        
	// Points are white
        bool w0 = ! b0; //r[jglo0] <= r0;
        bool w1 = ! b1; //r[jglo1] <= r0;
        bool w2 = ! b2; //r[jglo2] <= r0;
        
	int pos0 = geom.opposite (neigh, pos1);           // If current edge points to NE 
	                                                  // then pointed grid-point points back to SW

        seen[neigh_t::NMAX*jglo0+pos1] = true;            // Mark these edges as visited
        seen[neigh_t::NMAX*jglo1+pos0] = true;

        auto push = [=, &count, &seen0, &seen1]()                        
	{ 
	  if (seen0 == NULL) seen0 = &seen[neigh_t::NMAX*jglo0+pos1]; 
          if (seen1 == NULL) seen1 = &seen[neigh_t::NMAX*jglo1+pos0];

          int jgloa, jglob;
          if (jglo0 < jglo1)
            {
              jgloa = jglo0; jglob = jglo1;
	    }
	  else
            {
              jgloa = jglo1; jglob = jglo0;
	    }
          float a = (r0 - r[jgloa]) / (r[jglob] - r[jgloa]);        // Weight

	  // Coordinates of point
	  //
          float X = (1 - a) * xyz[3*jgloa+0] + a * xyz[3*jglob+0];  
          float Y = (1 - a) * xyz[3*jgloa+1] + a * xyz[3*jglob+1];
          float Z = (1 - a) * xyz[3*jgloa+2] + a * xyz[3*jglob+2];

	  // Normalize
	  float R = sqrt (X * X + Y * Y + Z * Z);
	  X /= R; Y /= R; Z /= R;

	  iso->push (X, Y, Z);
          
	  // We need at least two points in order to start drawing lines
          if (count > 0)
            {
              iso->ind.push_back (ind_start + count - 1);
              iso->ind.push_back (ind_start + count + 0);
            }

          if(dbg)
            if (count > 0)
              {
                float lat = asin (Z);
                float lon = atan2 (Y, X);
                if (count == 1)
                printf (" %8d %9.4f %9.4f %9.4f %9.4f %9.4f   %d\n", 
                        ind_start, iso->xyz[3*ind_start+0], iso->xyz[3*ind_start+1],  iso->xyz[3*ind_start+2], 
                        180. * atan2 (iso->xyz[3*ind_start+1], iso->xyz[3*ind_start+0]) / M_PI, 
                        180. * asin (iso->xyz[3*ind_start+2]) / M_PI,
                        geom.jglo (jlonlat0));

                printf (" %8d %9.4f %9.4f %9.4f %9.4f %9.4f %s %4d %4d %1d %1d %1d\n", 
                        ind_start + count, X, Y, Z, 180. * lon / M_PI, 
                        180. * lat / M_PI, neigh_t::strrot (rot1).c_str (),
			jglo0, jglo1, b0, b1, b2);
              }

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
            neigh_t neigh2 = uselist ? neighlist[jglo2] : geom.getNeighbours (neigh.jlonlat[pos2]);
            // Look for position which corresponds to point #1 seen from point #2
	    for (int pos = 0; ; ) 
              {
                if (neigh2.jlonlat[pos] == jlonlat1)
                  {
                    pos1 = pos;                       // Found
                    break;
		  }
                pos = neigh2.next (pos, neigh_t::P);  
		if (pos == 0)
                  {
                    pos1 = 0;
		    break;
		  }
	      }
	    jlonlat = neigh.jlonlat[pos2];
            neigh = neigh2;
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
	    jlonlat = neigh.jlonlat[pos2]; 
	    rot1 = neigh_t::inv (rot1); 
	    pos1 = geom.opposite (neigh, pos2); 
            neigh = uselist ? neighlist[jglo2] : geom.getNeighbours (jlonlat);
	    continue;
          }
        else if (w0 && b1 && w2) // D
          {
            push (); 
	    jlonlat = neigh.jlonlat[pos1]; 
	    rot1 = neigh_t::inv (rot1); 
	    pos1 = geom.opposite (neigh, pos1); 
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


if (seen0) *seen0 = true;
if (seen1) *seen1 = true;


// A single point was added; not enough to make a line
// A two point segments is not considered useful
// -> Remove
bool keep = count > 2;

if (! keep)
  {
    int size = iso->size () - ind_start;
    for (int i = 0; i < size; i++)
      iso->pop ();
    for (int i = 0; i < 2*(size-1); i++)
      iso->ind.pop_back ();
  }

if (keep)
  {
    if (inst)
      {
        iso->push (0, 0, 0, 0);
      }
  }

if(remove_open)
if ((! closed) && (! edge) && keep)
  {
    for (int i = ind_start; i < ind_start+count; i++)
      {
        iso->drw[i] = 0.;
      }
  }

if (II != 762)
  {
    for (int i = ind_start; i < ind_start+count; i++)
      {
        iso->drw[i] = 0.;
      }
  }


if (keep)
  II++;

if (dbg)
if (keep)
{


  printf ("> count = %d, II = %d, edge = %d, closed = %d\n", count, II, edge, closed);

  if(1){

  std::cout << " r0 = " << r0 << std::endl;

  neigh0.prn (geom, jlonlat0);

  int jglo0 = geom.jglo (jlonlat0);
  std::cout << " done = " << neigh0.done (&seen[neigh_t::NMAX*jglo0]) << std::endl;
  printf ("%30.20f\n", r[jglo0]);

  for (int i = 0; i < neigh_t::NMAX; i++)
    {
      std::cout << i << " " << neigh0.jlonlat[i].ok () << " " << seen[neigh_t::NMAX*jglo0+i]; 
      if (neigh0.jlonlat[i].ok ()) printf ("%30.20f", r[geom.jglo (neigh0.jlonlat[i])]); printf ("\n"); 
    }

  printf ("<");
  neigh.prn (geom, jlonlat);

  int jglo = geom.jglo (jlonlat);
  std::cout << " done = " << neigh.done (&seen[neigh_t::NMAX*jglo]) << std::endl;
  printf ("%30.20f\n", r[jglo]);

  for (int i = 0; i < neigh_t::NMAX; i++)
    {
      std::cout << i << "  " << neigh.jlonlat[i].ok () << " " << seen[neigh_t::NMAX*jglo+i]; 
      if (neigh.jlonlat[i].ok ()) printf ("%30.20f", r[geom.jglo (neigh.jlonlat[i])]); printf ("\n"); 
    }


  }
}

  return; 

  int jglo0 = geom.jglo (jlonlat0);
  if (! neigh.done (&seen[neigh_t::NMAX*jglo0]))  // All edges of current point have not been explored
    process (jlonlat0, r, r0, seen, geom, xyz, iso, neighlist, edge, remove_open);

}


static bool verbose = true;
static float lonc = 33.0f; // 0.0f;
static float latc = -1.0f; // 0.0f;
static float R = 6.0f;
static float fov = 0.8f; // 20.0f;
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


void checkSphere (const geom_t & geom)
{
#pragma omp parallel for
  for (int jlat = 1; jlat <= geom.Nj; jlat++)
  for (int jlon = 1; jlon <= geom.pl[jlat-1]; jlon++)
    {
      jlonlat_t jlonlat0 = jlonlat_t (jlon, jlat);
      neigh_t neigh0 = geom.getNeighbours (jlonlat0);
      for (int i = 0; i < neigh_t::NMAX; i++)
        {
          const jlonlat_t & jlonlat1 = neigh0.jlonlat[i];
          if (! jlonlat1.ok ())
            continue;
          neigh_t neigh1 = geom.getNeighbours (jlonlat1);
          for (int j = 0; j < neigh_t::NMAX; j++)
             if (neigh1.jlonlat[j] == jlonlat0)
               goto cont;
          neigh0.prn (geom, jlonlat0);
          neigh1.prn (geom, jlonlat1);
          abort ();
cont:
          continue;
        }
    }
}


int main (int argc, char * argv[])
{
  float * xyz;
  int np; 
  unsigned int nt;
  unsigned int * ind;
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
      gensphere_grib (&geom, &np, &xyz, &nt, &ind, &F, argv[1]);
      if (argc > 2)
        gensphere (&geom, &np, &xyz, &nt, &ind, &F, argv[2]);
    }
  else
    {
      geom.Nj = atoi (argv[1]);
      std::string type = "gradx";
      if (argc > 2)
        type = argv[2];
      gensphere (&geom, &np, &xyz, &nt, &ind, &F, type);
    }


//checkSphere (geom);

  int size = 0;
  for (int jlat = 1; jlat <= geom.Nj-1; jlat++)
    size += geom.pl[jlat-1];

  float maxval = *std::max_element (F, F + size);
  float minval = *std::min_element (F, F + size);

  std::cout << minval << " " << maxval << std::endl;

  r = (unsigned char *)malloc (sizeof (unsigned char) * size);
  for (int i = 0; i < size; i++)
    r[i] = 255 * (F[i] - minval) / (maxval - minval);


  std::vector<neigh_t> neighlist;
  if (uselist)
  neighlist = geom.getNeighbours ();

  const int N = 8;
  isoline_data_t iso_data[N];

#pragma omp parallel for
  for (int i = 0; i < N; i++)
    {
//if (i != 0) continue;
  if (i != N-8) continue;
//if (i != N-1) continue;
      bool * seen = (bool *)malloc (sizeof (bool) * neigh_t::NMAX * np);
      float F0 = minval + (i + 1) * (maxval - minval) / (N + 1);

      for (int i = 0; i < neigh_t::NMAX * np; i++)
        seen[i] = false;
     
      // Process edges of the domain first
     
      // Top
      for (int jlat = 1; jlat <= 1; jlat++)
        for (int jlon = 1; jlon <= geom.pl[jlat-1]; jlon++)
          process (jlonlat_t (jlon, jlat), F, F0, seen, geom, xyz, 
                   &iso_data[i], neighlist, true, remove_open);
     
      // Bottom
      for (int jlat = geom.Nj; jlat <= geom.Nj; jlat++)
        for (int jlon = 1; jlon <= geom.pl[jlat-1]; jlon++)
          process (jlonlat_t (jlon, jlat), F, F0, seen, geom, xyz, 
                   &iso_data[i], neighlist, true, remove_open);
     
      // Process center of the domain
      for (int jlat = 2; jlat < geom.Nj; jlat++)
        for (int jlon = 1; jlon <= geom.pl[jlat-1]; jlon++)
          process (jlonlat_t (jlon, jlat), F, F0, seen, geom, xyz, 
                   &iso_data[i], neighlist, false, remove_open);

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
      if (inst)
        glVertexAttribDivisor (0, 1);

      glEnableVertexAttribArray (1); 
      glVertexAttribPointer (1, 3, GL_FLOAT, GL_FALSE, 0, (const void *)(3 * sizeof (float))); 
      if (inst)
        glVertexAttribDivisor (1, 1);

      glEnableVertexAttribArray (2); 
      glVertexAttribPointer (2, 3, GL_FLOAT, GL_FALSE, 0, (const void *)(6 * sizeof (float))); 
      if (inst)
        glVertexAttribDivisor (2, 1);


      glGenBuffers (1, &iso[i].normalbuffer);
      glBindBuffer (GL_ARRAY_BUFFER, iso[i].normalbuffer);
      glBufferData (GL_ARRAY_BUFFER, iso_data[i].drw.size () * sizeof (float), 
                    iso_data[i].drw.data (), GL_STATIC_DRAW);

      glEnableVertexAttribArray (3); 
      glVertexAttribPointer (3, 1, GL_FLOAT, GL_FALSE, 0, NULL); 
      if (inst)
        glVertexAttribDivisor (3, 1);

      glEnableVertexAttribArray (4); 
      glVertexAttribPointer (4, 1, GL_FLOAT, GL_FALSE, 0, (const void *)(sizeof (float))); 
      if (inst)
        glVertexAttribDivisor (4, 1);



      if (inst)
        {

        }
      else
        {
          glGenBuffers (1, &iso[i].elementbuffer);
          glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, iso[i].elementbuffer);
          glBufferData (GL_ELEMENT_ARRAY_BUFFER, iso_data[i].ind.size () * sizeof (unsigned int), 
		        iso_data[i].ind.data (), GL_STATIC_DRAW);
        }

      std::cout << i << " np = " << iso_data[i].size () << std::endl;

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
      if (inst)
        {
          glUseProgram (programID_l_inst);
          glUniformMatrix4fv (glGetUniformLocation (programID_l_inst, "MVP"), 
			      1, GL_FALSE, &MVP[0][0]);
	}
      else
        {
          glUseProgram (programID_l);
          glUniformMatrix4fv (glGetUniformLocation (programID_l, "MVP"), 
			      1, GL_FALSE, &MVP[0][0]);
	}
      for (int i = 0; i < N; i++)
        {
          glBindVertexArray (iso[i].VertexArrayID);
	  if (inst)
            {
              glDrawArraysInstanced (GL_LINE_STRIP, 0, 2, iso[i].size_inst);
	    }
          else
            {
              glDrawElements (GL_LINES, iso[i].size, GL_UNSIGNED_INT, NULL);
	    }
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
