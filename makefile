

main.x: main.cc gensphere.cc shader.cc bmp.cc
	g++ -std=c++11 -I$(HOME)/3d/sphere/usr/include -fopenmp -g -o main.x main.cc gensphere.cc shader.cc bmp.cc -L$(HOME)/3d/sphere/usr/lib64 -lglfw -lGLEW -lGL -Wl,-rpath,$(HOME)/3d/sphere/usr/lib64 -lssl -lcrypto

clean:
	\rm -f *.o *.x
