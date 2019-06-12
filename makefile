


CXXFLAGS=-fopenmp -std=c++11 -g -I$(HOME)/3d/usr/include -L$(HOME)/3d/usr/lib64 -Wl,-rpath,$(HOME)/3d/usr/lib64 -leccodes -lGLEW -lGL -lglfw -lpng -lreadline -lncurses -ltinfo -lssl -lcrypto
CXXFLAGS += -I$(HOME)/install/eccodes--2.13.0_FIXOMMCODES/include -L$(HOME)/install/eccodes--2.13.0_FIXOMMCODES/lib -Wl,-rpath,$(HOME)/install/eccodes--2.13.0_FIXOMMCODES/lib


main.x: main.o gensphere.o shader.o bmp.o load.o gensphere.h
	g++ $(CXXFLAGS) -o main.x main.o gensphere.o shader.o bmp.o load.o $(CXXFLAGS)

%.o: %.cc gensphere.h
	g++ $(CXXFLAGS) -o $@ -c $<


clean:
	\rm -f *.o *.x

test_saddle0: ./main.x
	./main.x 30 saddle0
