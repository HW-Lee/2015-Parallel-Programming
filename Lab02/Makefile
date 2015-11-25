NVFLAGS  := -arch=sm_20 -O3
CXXFLAGS := -fopenmp
LIBS     := -I/usr/include/mpich-x86_64 -L/usr/lib64/mpich/lib -lmpich

all: sobel

clean:
	rm -f sobel
sobel: sobel.cu
	nvcc -o sobel $(NVFLAGS) -Xcompiler="$(CXXFLAGS)" $(LIBS) sobel.cu
