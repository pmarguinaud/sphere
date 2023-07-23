

main.x: main.cc gensphere.cc shader.cc bmp.cc grib.cc
	g++ -std=c++11 -I$(HOME)/glgrib/usr/include -I$(HOME)/3d/sphere/usr/include -fopenmp -g -o main.x main.cc gensphere.cc shader.cc bmp.cc grib.cc -L$(HOME)/3d/sphere/usr/lib64 -lglfw -lGLEW -lGL -Wl,-rpath,$(HOME)/3d/sphere/usr/lib64  -leccodes

clean:
	\rm -f *.o *.x
