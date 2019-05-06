#include <memory>
#include <iostream>


class opengl_buffer
{
public:
  ~opengl_buffer ()
  {
    std::cout << "~opengl_buffer" << std::endl;
  }
  int vbo;
};

typedef std::shared_ptr<opengl_buffer> opengl_buffer_ptr;
opengl_buffer_ptr new_opengl_buffer_ptr ()
{
  return std::make_shared<opengl_buffer>();
}

int main ()
{
  {
  opengl_buffer_ptr sptr = new_opengl_buffer_ptr ();
  std::cout << sptr.use_count () << std::endl;

  opengl_buffer_ptr sptr1 = sptr;
  std::cout << sptr.use_count () << std::endl;
  sptr->vbo = 1;
  }
  std::cout << "----" << std::endl;

  return 0;
}
