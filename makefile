OpenBLAS: fconn.cpp
	g++ -L /usr/lib/OpenBLAS/lib \
	-I /usr/lib/OpenBLAS/include -I . \
	-std=c++11 -Wno-deprecated-declarations  fconn.cpp \
	-o fconn.o -lopenblas -llapack -fopenmp -D BUILD1
MKL: mkl_fconn.cpp
	g++ mkl_fconn.cpp -o fconn.o -fopenmp -std=c++11 \
	-L ~/intel/mkl/lib/intel64 \
	-I ~/intel/mkl/include/ -I . \
	-L ~/intel/lib/intel64 -lmkl_blas95_lp64 \
	-lmkl_intel_lp64 -lmkl_lapack95_lp64 -liomp5 -lm -lmkl_intel_thread -lmkl_core -lpthread
CBLAS: fconn.cpp
	 g++ -L /usr/local/atlas -I . -std=c++11 -Wdeprecated-declarations fconn.cpp \
	-o fconn.o -fopenmp -llapack -llapack -lcblas -D BUILD1
CUDA: fconn.cpp
	g++ -L /usr/local/cuda/lib64/ \
	-I /usr/local/cuda/include/ -I . \
	-std=c++11 fconn.cpp -o fconn.o -lnvblas \
	-L /mnt/project1/home1/cs5130287/OpenBLAS/lib/ \
	-I /mnt/project1/home1/cs5130287/OpenBLAS/include \
	-fopenmp -D BUILD2 -lopenblas 


