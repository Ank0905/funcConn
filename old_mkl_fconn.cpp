//extern "C" {


//#include <mkl_cblas.h>

//}
#include "mkl.h"
#include "mkl_cblas.h"
#include <omp.h>
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/stat.h>
#include "image/image.hpp"
#include <vector>
#include <fstream>
#include <time.h>
#include <map>
#include <cstdlib>
#define min(x,y) (((x) < (y)) ? (x) : (y))
#define max(x,y) (((x) > (y)) ? (x) : (y))

struct Valid{
int x,y,z;
};

std::string type(".csv"),ofname,ipfilename;
bool input = false,roi1 = false,roi2 = false,output=false,gzip = false;
bool mask = false;
std::string roifname,maskfilename;
int thresh=0;
bool seedcmp = false,all=false,flops = false;
int seedx ,seedy,seedz;
int ROI_MAX;
long no_of_oper=0;
double time_taken =0.0;




void showhelpinfo();
void correl();
void seed_correl();
void all_pair_corr();
void avg_corr_roi();
void avg_roi_time_corr();
void getattributes(int argc,char *argv[]);

int main (int argc,char *argv[])
{
   omp_set_num_threads(16);
  /*if the program is ran witout options ,it will show the usage and exit*/
  if(argc < 2)
  {
	showhelpinfo();
	exit(1);
  }
  
  std::cout<<" 1"<<std::endl;

  getattributes(argc,argv);
  
  if(input==false||output==false ||(roi2 == false && roi1 == false && all == false && seedcmp == false))
  {
	showhelpinfo();
	exit(1);
  }
std::cout<<" 2"<<std::endl;
  if( ( roi1 == true&&roi2 == true&& all == true ) || ( roi1 == true&&roi2 == true&& seedcmp == true ) || ( all == true && seedcmp == true ))
  {
	showhelpinfo();
	exit(1);
  }
  std::cout<<" 3"<<std::endl;
	///######################### Creating the project folder #######################################################
   
	mode_t nMode = 0733; // UNIX style permissions
	int nError = 0;
	#if defined(_WIN32)
	  nError = _mkdir(ofname.c_str()); // can be used on Windows
	#else 
	  nError = mkdir(ofname.c_str(),nMode); // can be used on non-Windows
	#endif
	if (nError != 0) {
	  std::cout<<"Problem in creating directory "<<std::endl;
	  exit(0);
	}
  if(roi1==false&&roi2==false){
	std::string sPath = "/voxel";
	sPath = ofname + sPath;
	mode_t nMode = 0733; // UNIX style permissions
	int nError = 0;
	#if defined(_WIN32)
	  nError = _mkdir(sPath.c_str()); // can be used on Windows
	#else 
	  nError = mkdir(sPath.c_str(),nMode); // can be used on non-Windows
	#endif
	if (nError != 0) {
	  std::cout<<"Problem in creating directory "<<std::endl;
	  exit(0);
	}
  }
  else{
	std::string sPath = "/roi";
  sPath = ofname + sPath;
  mode_t nMode = 0733; // UNIX style permissions
  int nError = 0;
	#if defined(_WIN32)
	  nError = _mkdir(sPath.c_str()); // can be used on Windows
	#else 
	  nError = mkdir(sPath.c_str(),nMode); // can be used on non-Windows
	#endif
	if (nError != 0) {
	  std::cout<<"Problem in creating directory "<<std::endl;
	  exit(0);
	}
  }

	//############################## END ###########################################################################
  correl();
  return 0;
}


void seed_correl(){

	//DEFINING VARS
  image::io::nifti nifti_parser,nifti_parser2;
  image::basic_image<double,4> image_data;
  image::basic_image<double,4> image_data_cpy;
  image::basic_image<int,3> mask_image;
  std::vector<Valid> valid;

  image_data_cpy = image_data;

	//LOADING THE IMAGE
  if(nifti_parser.load_from_file(ipfilename));
	  nifti_parser >> image_data;

	// LOADING THE MASK
  if(mask&&nifti_parser2.load_from_file(maskfilename))
  	   nifti_parser2 >> mask_image;
  
  image::geometry<4> g = image_data.geometry();
  image::geometry<4> g_copy;
  image::geometry<3> g_mask;

	//CHECK IF THE GEOMETRY OF THE MASK AND IMAGE IS SAME
  if(mask){
  	g_mask = mask_image.geometry();
  	if(g_mask[0]!=g[0]||g_mask[1]!=g[1]||g_mask[2]!=g[2]){
  		std::cout<<":::: ERROR INVALID MASK ::::"<<std::endl;
  		return;
  	}
  }

  std::string cmd = "gzip ";
  cmd+=ipfilename;
  system(cmd.c_str());

	//############################# DEFINING MORE VARIABLES ####################################

  double *EX = (double *) malloc( sizeof(double) * g[0] * g[1]* g[2] ) ;
  double *EX_sq = (double *) malloc( sizeof(double) * g[0] * g[1]* g[2] ) ;
  std::string filename2("/voxelmap");
  filename2 = ofname + filename2;
  const char *s1 = filename2.c_str();
  std::ofstream f(s1,std::ios::binary);

	// #########################CALCULATING THE MEAN AND STD DEVIATION OF DATA AND CREATING A VALID VOXEL MAPPINGS ################################
  
  clock_t tStart;

  for (int z = 0; z < g[2]; ++z){
	for (int y = 0; y < g[1]; ++y)
	  {
		for (int x = 0; x < g[0]; ++x)
		{

		  tStart = clock();
		  double temp  = 0,temp2 = 0;
		  for (int t = 0; t < g[3]; ++t)
		  {
			temp += (double)image_data[((t*g[2] + z)*g[1]+y)*g[0]+x]/g[3] ;
			temp2 += (double)image_data[((t*g[2] + z)*g[1]+y)*g[0]+x]*(double)image_data[((t*g[2] + z)*g[1]+y)*g[0]+x]/g[3];
		  }

		  time_taken+=(double)(clock()-tStart)/CLOCKS_PER_SEC;

		  if(!mask){
			  if(temp > max(20,thresh)){
				Valid tv ;
				tv.x = x;
				tv.y = y;
				tv.z = z;
				int temp = valid.size();
				f.write(reinterpret_cast<char *>(&temp),sizeof(int));
				valid.push_back(tv);
			  }else{
				int temp = -1;
				f.write(reinterpret_cast<char *>(&temp),sizeof(int));
				// direc.insert( ( ( z*g[1] + y )*g[0] + x ) ,-1);
			  }
			}else{
				if(mask_image[(z*g[1]+y)*g[0]+x]==1){
				Valid tv ;
				tv.x = x;
				tv.y = y;
				tv.z = z;
				int temp = valid.size();
				f.write(reinterpret_cast<char *>(&temp),sizeof(int));
				valid.push_back(tv);
			  }else{
				int temp = -1;
				f.write(reinterpret_cast<char *>(&temp),sizeof(int));
				
			  }


			}
		  
		  EX[ (x*g[1] +y)*g[2] + z] = temp;
		  EX_sq[ (x*g[1] +y)*g[2] + z] = sqrt(temp2- temp*temp);

		}
	  } 

  }

  f.close();


	// ########################################### MAKING A MATRIX OF NORMALIZED DATA###########################################
  double * Valid_matrix = (double * )malloc( sizeof(double)*valid.size()*g[3]);

  tStart = clock();
  # pragma omp parallel for
  for (int i = 0; i < valid.size(); ++i)
  {
	for (int t = 0; t < g[3]; ++t){
		double currentdev = EX_sq[(valid[i].x*g[1] +valid[i].y)*g[2] + valid[i].z];
		double tempNormdata;
		if (currentdev < 0.005){
			tempNormdata = 0;
			image_data[((t*g[2] + valid[i].z)*g[1]+valid[i].y)*g[0]+valid[i].x] = tempNormdata;
			Valid_matrix[i*g[3]+t] = tempNormdata;
		}
		else{
			tempNormdata = (double)(image_data[((t*g[2] + valid[i].z)*g[1]+valid[i].y)*g[0]+valid[i].x] - EX[(valid[i].x*g[1] +valid[i].y)*g[2] + valid[i].z])/currentdev;
	  		image_data[((t*g[2] + valid[i].z)*g[1]+valid[i].y)*g[0]+valid[i].x] = tempNormdata;
	  		Valid_matrix[i*g[3]+t] = tempNormdata;
		}

	}
  }
  time_taken+=(double)(clock()-tStart)/CLOCKS_PER_SEC;
	// #############INTIALIZE#############################################################################
   // clock_t tStart = clock();
  // free(EXY);
    free(EX_sq);
    
  	std::vector<Valid> seeds;
	double * seed = (double * )malloc( sizeof(double)*g[3]);
	double * Corr = (double * )malloc( sizeof(double)*valid.size());
	
	
	Valid tv;
	tv.x = seedx;
	tv.y = seedy;
	tv.z = seedz;
	seeds.push_back(tv);

	int tempi = 0;		

	for (int t = 0; t < g[3]; ++t){

		if(t == 0)
			seed[t]= (image_data[((t*g[2] + seeds[tempi].z)*g[1]+seeds[tempi].y)*g[0]+seeds[tempi].x]);
		else
			seed[t]+= (image_data[((t*g[2] + seeds[tempi].z)*g[1]+seeds[tempi].y)*g[0]+seeds[tempi].x]);

	}


	g_copy.dim[0] = g[0];
	g_copy.dim[1] = g[1];
	g_copy.dim[2] = g[2];
	g_copy.dim[3] = 1;
	image_data_cpy.resize(g_copy);

	#pragma omp parallel for collapse(3)
	for (int x = 0; x < g[0]; ++x)
	  for (int y = 0; y < g[1]; ++y)
		 for (int z = 0; z < g[2]; ++z)
			image_data_cpy[((z)*g[1]+y)*g[0]+x] = 0;

	// #############################FINDING MATRIX FOR CORRELATION OF A SEED#####################################

	// clock_t tStart = clock();

	if(EX[(seedx*g[1] +seedy)*g[2] + seedz] == 0){
	  
	   for (int i = 0; i < valid.size(); ++i){
			for (int t = 0; t < g[3]; ++t){
		
		image_data_cpy[((valid[i].z)*g[1]+valid[i].y)*g[0]+valid[i].x] = 0;

			}
	  
		}
	}
	else{
	// vector matrix multiplication
	// free(EXY);
	free(EX);
	int m = valid.size();
	int dime = g[3];
	float timeseries = 1/(float)dime;
	// std::cout<<m<<std::endl;
	cblas_dgemv(CblasRowMajor,CblasNoTrans,m,dime,timeseries,Valid_matrix,dime,seed,1,0.0,Corr,1);
	 
	for (int i = 0; i < valid.size(); ++i)
		image_data_cpy[((valid[i].z)*g[1]+valid[i].y)*g[0]+valid[i].x] = Corr[i];

  

	}

	//image::io::nifti nifti_parser2;
	nifti_parser2.load_from_image(image_data_cpy);
	std::string filename("output.nii");
	filename = ofname + filename;
	nifti_parser2.save_to_file(filename.c_str());
	// std::cout<< ((double)(clock() - tStart)/CLOCKS_PER_SEC)<<",";
}

void all_pair_corr(){
 
	//DEFINING VARIABLES  
  image::io::nifti nifti_parser,nifti_parser2;
  image::basic_image<double,4> image_data;
  image::basic_image<double,4> image_data_cpy;
  image::basic_image<int,3> mask_image;
  std::vector<Valid> valid;

  image_data_cpy = image_data;

	//LOADING THE IMAGE
  if(nifti_parser.load_from_file(ipfilename));
	  nifti_parser >> image_data;

	// LOADING THE MASK
  if(mask&&nifti_parser2.load_from_file(maskfilename))
  	   nifti_parser2 >> mask_image;
  
  image::geometry<4> g = image_data.geometry();
  image::geometry<4> g_copy;
  image::geometry<3> g_mask;

	//CHECK IF THE GEOMETRY OF THE MASK AND IMAGE IS SAME
  if(mask){
  	g_mask = mask_image.geometry();
  	if(g_mask[0]!=g[0]||g_mask[1]!=g[1]||g_mask[2]!=g[2]){
  		std::cout<<":::: ERROR INVALID MASK ::::"<<std::endl;
  		return;
  	}
  }

  std::string cmd = "gzip ";
  cmd+=ipfilename;
  system(cmd.c_str());

	//############### DEFINING VARIABLES ####################################

  double *EX = (double *) malloc( sizeof(double) * g[0] * g[1]* g[2] ) ;
  double *EX_sq = (double *) malloc( sizeof(double) * g[0] * g[1]* g[2] ) ;
  std::string filename2("/voxelmap");
  filename2 = ofname + filename2;
  const char *s1 = filename2.c_str();
  std::ofstream f(s1,std::ios::binary);

	//#CALCULATING THE MEAN AND STD DEVIATION OF DATA AND CREATING A VALID VOXEL MAPPINGS ################################
  clock_t tStart ;
  for (int z = 0; z < g[2]; ++z){
	for (int y = 0; y < g[1]; ++y)
	  {
		for (int x = 0; x < g[0]; ++x)
		{
		  tStart = clock();
		  double temp  = 0,temp2 = 0;
		  for (int t = 0; t < g[3]; ++t)
		  {
			temp += (double)image_data[((t*g[2] + z)*g[1]+y)*g[0]+x]/g[3] ;
			temp2 += (double)image_data[((t*g[2] + z)*g[1]+y)*g[0]+x]*(double)image_data[((t*g[2] + z)*g[1]+y)*g[0]+x]/g[3];
		  }
		  time_taken+=(double)(clock()-tStart)/CLOCKS_PER_SEC;

		  if(!mask){
			  if(temp > max(20,thresh)){
				Valid tv ;
				tv.x = x;
				tv.y = y;
				tv.z = z;
				int temp = valid.size();
				f.write(reinterpret_cast<char *>(&temp),sizeof(int));
				valid.push_back(tv);
			  }else{
				int temp = -1;
				f.write(reinterpret_cast<char *>(&temp),sizeof(int));
				// direc.insert( ( ( z*g[1] + y )*g[0] + x ) ,-1);
			  }
			}else{
				if(mask_image[(z*g[1]+y)*g[0]+x]==1){
				Valid tv ;
				tv.x = x;
				tv.y = y;
				tv.z = z;
				int temp = valid.size();
				f.write(reinterpret_cast<char *>(&temp),sizeof(int));
				valid.push_back(tv);
			  }else{
				int temp = -1;
				f.write(reinterpret_cast<char *>(&temp),sizeof(int));
				
			  }


			}
		  
		  EX[ (x*g[1] +y)*g[2] + z] = temp;
		  EX_sq[ (x*g[1] +y)*g[2] + z] = sqrt(temp2- temp*temp);

		}
	  } 

  }

  f.close();

	//################## MAKING A MATRIX OF NORMALIZED DATA###########################################
  double * Valid_matrix = (double * )malloc( sizeof(double)*valid.size()*g[3]);
  tStart = clock();
  # pragma omp parallel for
  for (int i = 0; i < valid.size(); ++i)
  {
	for (int t = 0; t < g[3]; ++t){
		double currentdev = EX_sq[(valid[i].x*g[1] +valid[i].y)*g[2] + valid[i].z];
		double tempNormdata;
		if (currentdev == 0){
			tempNormdata = 0;
			image_data[((t*g[2] + valid[i].z)*g[1]+valid[i].y)*g[0]+valid[i].x] = tempNormdata;
			Valid_matrix[i*g[3]+t] = tempNormdata;
		}
		else{
			tempNormdata = (double)(image_data[((t*g[2] + valid[i].z)*g[1]+valid[i].y)*g[0]+valid[i].x] - EX[(valid[i].x*g[1] +valid[i].y)*g[2] + valid[i].z])/currentdev;
	  		image_data[((t*g[2] + valid[i].z)*g[1]+valid[i].y)*g[0]+valid[i].x] = tempNormdata;
	  		Valid_matrix[i*g[3]+t] = tempNormdata;
		}

	}
  }
  time_taken+=(double)(clock()-tStart)/CLOCKS_PER_SEC;
	free(EX_sq);
	// clock_t tStart = clock();
	// free(EXY);


	// #######FINDING MATRIX FOR CORRELATION OF COMBINATION########################################
	free(EX);
	int dime = g[3];
	int n =  8192;
	double * res = (double * )malloc( sizeof(double)*n*n);
	double * Valid_matrix_chunk= (double * )malloc( sizeof(double)*g[3]*n);
	double * Valid_matrix_chunk_trans= (double * )malloc( sizeof(double)*n*g[3]);
  if(res==NULL){
	  free(Valid_matrix);
	  free(Valid_matrix_chunk);
	  free(res);
	  std::cout<<"res Allocating failed"<<std::endl;
	  exit(0);
	}

  if(Valid_matrix_chunk==NULL){
	  free(Valid_matrix);
	  free(Valid_matrix_chunk);
	  free(res);
	  std::cout<<"valid Allocating failed"<<std::endl;
	  exit(0);
	}
   
	double timeTk =0;
	//clock_t tStart = clock();
  for(int starti = 0;starti<valid.size();starti+=n)
	for(int startj = starti;startj<valid.size();startj+=n)
	{


	  // tStart = clock();
	 
	  // std::cout<<starti<<","<<startj<<std::endl;
	  int n2 = min(n,valid.size()-starti);
	  int n3 = min(n,valid.size()-startj);
	  int n_toget=max(n2,n3);

	  #pragma omp parallel for collapse(2)
	  for (int i = 0; i < n_toget; ++i)
	  {
		for (int j = 0; j < g[3]; ++j)
		{
		  //######### AS BOTH THE MATRICES ARE OF DIFFERENT SIZES SO SELECT ACCORDINGLY######################################### 
		  if(i<n2)
		  Valid_matrix_chunk[i*g[3] + j]=Valid_matrix[(starti+i)*g[3] + (j)];
		  if(i<n3)
		  Valid_matrix_chunk_trans[j*n + i]=Valid_matrix[(startj+i)*g[3] + (j)];
		}
	  }


	  double timeseries = 1/float(g[3]);

	  tStart = clock();
	  cblas_dgemm(CblasRowMajor,CblasNoTrans,CblasNoTrans,n2,n3,dime,timeseries,Valid_matrix_chunk,dime,Valid_matrix_chunk_trans,n3,0.0,res,n3);
	  time_taken+=(double)(clock()-tStart)/CLOCKS_PER_SEC;
	  //std::cout<<"Time taken:  for CORRELATION "<< ((double)(clock() - tStart)/CLOCKS_PER_SEC)<<std::endl;
	  //timeTk += (double)(clock() - tStart)/CLOCKS_PER_SEC ;
	  # pragma omp parallel for
	  for (int resi = 0; resi < n2; ++resi)
	  {
		// double *arr = (double *)malloc(sizeof(double)*n3);
		std::string filename("/voxel/voxel_");
		filename = ofname + filename;
		filename+= std::to_string(starti+resi);
		filename+=type;
		//direc.insert( ( ( valid[starti + resi].z*g[1] + valid[starti + resi].y )*g[0] + valid[starti + resi]x ) ,starti+resi);
		const char *s1 = filename.c_str();
		//std::ofstream f(s1,std::ios::binary|std::ios::app);
		std::ofstream f(s1,std::ios::binary|std::ios::app);
		for (int resj = 0; resj < n3; ++resj){
		   // f<< res[resi*n+resj]<<'\n';
		   double tmp = res[resi*n+resj];
		  f.write(reinterpret_cast<char *>(&tmp),sizeof(double));

		}
		  //f.write(reinterpret_cast<char *>(&arr),sizeof(double)*n3);
		  //fprintf(f, "%f ",res[resi*n+resj]);

		f.close();
		// free(arr);

	  }
	if(starti!=startj){
		#pragma omp parallel for
		for (int resj = 0; resj < n3; ++resj)
		{
		  //double *arr = (double *)malloc(sizeof(double)*n3);
		  std::string filename("/voxel/voxel_");
		  filename = ofname + filename;
		  filename+= std::to_string(startj+resj);
		  const char *s1 = filename.c_str();
		  filename+=type;
		  std::ofstream f(s1,std::ios::binary|std::ios::app);
		  for (int resi = 0; resi < n2; ++resi){
		  	double tmp = res[resi*n+resj];
		  	f.write(reinterpret_cast<char *>(&tmp),sizeof(double));
		  }
			// f << res[resi*n+resj]<<'\n';
			//f.write(reinterpret_cast<char *>(&arr),sizeof(double)*n3);
			// f.write((char*)&res[resi*n+resj],sizeof(double));
			// fprintf(f, "%f ",res[resi*n+resj]);
		  f.close();
		  //free(arr);
		}  
	  }

	}


	no_of_oper = 5*(g[0]*g[1]*g[2]*g[3])+(g[0]*g[1]*g[2]) + valid.size()*valid.size()*2*g[3];
	//std::cout<<"Time taken:  for CORRELATION "<< timeTk<<std::endl;
}


void avg_roi_time_corr(){
	//DEFINING VARIABLES  
  image::io::nifti nifti_parser,nifti_parser2;
  image::basic_image<double,4> image_data;
  image::basic_image<double,4> image_data_cpy;
  image::basic_image<int,3> roi_image;
  std::vector<Valid> valid;

  image_data_cpy = image_data;

	//LOADING THE IMAGE
  if(nifti_parser.load_from_file(ipfilename));
	  nifti_parser >> image_data;

	std::cout<<ipfilename<<std::endl;

	// LOADING THE ROI
  if(nifti_parser2.load_from_file(roifname))
  	   nifti_parser2 >> roi_image;

	std::cout<<roifname<<std::endl;  
  image::geometry<4> g = image_data.geometry();
  //image::geometry<4> g_copy;
  image::geometry<3> g_roi;

	//CHECK IF THE GEOMETRY OF THE MASK AND IMAGE IS SAME
   	g_roi = roi_image.geometry();
  	if(g_roi[0]!=g[0]||g_roi[1]!=g[1]||g_roi[2]!=g[2]){
  		std::cout<<":::: ERROR INVALID MASK ::::"<<std::endl;
  		return;
  	}
  image::basic_image<int,3> mask_image;
	if(mask&&nifti_parser.load_from_file(maskfilename))
	  	   nifti_parser >> mask_image;
	image::geometry<3> g_mask;


		//CHECK IF THE GEOMETRY OF THE MASK AND IMAGE IS SAME
	  if(mask){
	  	g_mask = mask_image.geometry();
	  	if(g_mask[0]!=g[0]||g_mask[1]!=g[1]||g_mask[2]!=g[2]){
	  		std::cout<<":::: ERROR INVALID MASK ::::"<<std::endl;
	  		return;
	  	}
	  }


  // std::string cmd = "gzip ";
  // cmd+=ipfilename;
  // system(cmd.c_str());

	//############### DEFINING VARIABLES ####################################

  double *EX = (double *) malloc( sizeof(double) * g[0] * g[1]* g[2] ) ;
  double *EX_sq = (double *) malloc( sizeof(double) * g[0] * g[1]* g[2] ) ;
  
	//#CALCULATING THE MEAN AND STD DEVIATION OF DATA AND CREATING A VALID VOXEL MAPPINGS ################################
  clock_t tStart;
  for (int z = 0; z < g[2]; ++z){
	for (int y = 0; y < g[1]; ++y)
	  {
		for (int x = 0; x < g[0]; ++x)
		{

		  double temp  = 0,temp2 = 0;
		  tStart = clock();
		  for (int t = 0; t < g[3]; ++t)
		  {
			temp += (double)image_data[((t*g[2] + z)*g[1]+y)*g[0]+x]/g[3] ;
			temp2 += image_data[((t*g[2] + z)*g[1]+y)*g[0]+x]*image_data[((t*g[2] + z)*g[1]+y)*g[0]+x]/g[3];
		  }
		  time_taken += (double)(clock()-tStart)/CLOCKS_PER_SEC;
			
		  // else{
				// int temp = -1;
				
				
		  // }
				  
		  EX[ (x*g[1] +y)*g[2] + z] = temp;
		  EX_sq[ (x*g[1] +y)*g[2] + z] = sqrt(temp2- temp*temp);

		  if((mask&&mask_image[(z*g[1]+y)*g[0]+x]==1)||(!mask&&EX_sq[ (x*g[1] +y)*g[2] + z] > 1e-2)){
				Valid tv ;
				tv.x = x;
				tv.y = y;
				tv.z = z;
				// int temp = valid.size();
				valid.push_back(tv);
		  }

		}
	  } 

  }



	//################## MAKING A MATRIX OF NORMALIZED DATA###########################################
  int valid_size = valid.size();
  double * Valid_matrix = (double * )malloc( sizeof(double)*valid_size*g[3]);
  // double * roi_result = (double * )malloc( sizeof(double)*ROI_MAX*valid_size);
  double * roi_avg = (double * ) calloc(ROI_MAX*g[3], sizeof(double));
  double * roi_var = (double * ) calloc(ROI_MAX, sizeof(double));
  double * roi_tot = (double * ) calloc(ROI_MAX, sizeof(double));
  double * roi_mean = (double * ) calloc(ROI_MAX, sizeof(double));	
  
  // std::cout<<"val "<<roi_avg[0]<<std::endl;
  tStart = clock();
  #pragma omp parallel for shared(time_taken,roi_tot,roi_avg)
  for (int i = 0; i < valid.size(); ++i)
  {	
  	
  	int roi_no = roi_image[(valid[i].z*g[1] +valid[i].y)*g[0] + valid[i].x];
  	if(roi_no!=0)
  		roi_tot[roi_no-1]++;
	double currentdev = EX_sq[(valid[i].x*g[1] +valid[i].y)*g[2] + valid[i].z];
	for (int t = 0; t < g[3]; ++t){
		
		double tempNormdata;
		if (currentdev == 0){
			tempNormdata = 0;
			image_data[((t*g[2] + valid[i].z)*g[1]+valid[i].y)*g[0]+valid[i].x] = tempNormdata;
			Valid_matrix[t*valid_size + i] = tempNormdata;

		}
		else{
			tempNormdata = (double)(image_data[((t*g[2] + valid[i].z)*g[1]+valid[i].y)*g[0]+valid[i].x] - EX[(valid[i].x*g[1] +valid[i].y)*g[2] + valid[i].z])/currentdev;
	  		if(roi_no!=0)
  				roi_avg[(roi_no-1)*g[3]+t ] += tempNormdata;
	  		image_data[((t*g[2] + valid[i].z)*g[1]+valid[i].y)*g[0]+valid[i].x] = tempNormdata;
	  		Valid_matrix[t*valid_size + i] = tempNormdata;
	  		
		}

	}

	

  }

  time_taken += (double)(clock()-tStart)/CLOCKS_PER_SEC;
  tStart = clock();

  // #pragma omp parallel for
  for (int i = 0; i < ROI_MAX; ++i)
  {	
  		double temp  = 0,temp2 = 0;
  		// std::cout<<i<<":"<<roi_tot[i]<<std::endl;
  		for (int t = 0; t < g[3]; ++t){
  			roi_avg[i*g[3]+t]/=roi_tot[i];
  	// 		temp += (double)roi_avg[i*g[3]+t]/g[3] ;
			// temp2 += roi_avg[i*g[3]+t]*roi_avg[i*g[3]+t]/g[3];
  		}

  		// roi_mean[i]= temp;
  		// roi_var[i] = sqrt(temp2- temp*temp);

  }
  time_taken += (double)(clock()-tStart)/CLOCKS_PER_SEC;
  
  // #pragma omp parallel for
  // for (int i = 0; i < ROI_MAX; ++i){	
  // 		if(roi_var[i]!=0){
  // 			for (int t = 0; t < g[3]; ++t){
  // 				roi_avg[i*g[3]+t]=(roi_avg[i*g[3]+t]-roi_mean[i])/roi_var[i];
  // 				}
  // 		}else{
  // 			for (int t = 0; t < g[3]; ++t){
  // 				roi_avg[i*g[3]+t] = 0;
  // 			}

  // 		}	
  // }


  free(EX_sq);
  // free(roi_var);
  // free(roi_tot);
	// clock_t tStart = clock();
	// free(EXY);


	// #######FINDING MATRIX FOR CORRELATION OF COMBINATION########################################
	free(EX);
	int dime = g[3];
	int n =  8192;
	// double * roi_chunk = (double * )malloc( sizeof(double)*ROI_MAX*n);
	double * res = (double * )malloc( sizeof(double)*ROI_MAX*valid_size);
	// double * Valid_matrix_chunk= (double * )malloc( sizeof(double)*g[3]*n);
	// double * Valid_matrix_chunk_trans= (double * )malloc( sizeof(double)*n*g[3]);
 //  if(res==NULL){
	//   free(Valid_matrix);
	//   free(Valid_matrix_chunk);
	//   free(res);
	//   std::cout<<"res Allocating failed"<<std::endl;
	//   exit(0);
	// }

 //  if(Valid_matrix_chunk==NULL){
	//   free(Valid_matrix);
	//   free(Valid_matrix_chunk);
	//   free(res);
	//   std::cout<<"valid Allocating failed"<<std::endl;
	//   exit(0);
	// }

	// for(int starti = 0;starti<valid.size();starti+=n)
	// for(int startj = starti;startj<valid.size();startj+=n)
	// {


	  // tStart = clock();
	 
	  // std::cout<<starti<<","<<startj<<std::endl;
	 //  int n2 = ROI_MAX;
	 //  int n3 = min(n,valid.size()-startj);
	 //  int n_toget=max(n2,n3);

	 //  #pragma omp parallel for collapse(2)
	 //  for (int i = 0; i < n_toget; ++i)
	 //  {
		// for (int j = 0; j < g[3]; ++j)
		// {
		//   //######### AS BOTH THE MATRICES ARE OF DIFFERENT SIZES SO SELECT ACCORDINGLY######################################### 
		//   if(i<n2)
		//   roi_chunk[i*g[3] + j]=roi_avg[(starti+i)*g[3] + (j)];
		//   if(i<n3)
		//   Valid_matrix_chunk_trans[j*n + i]=Valid_matrix[(startj+i)*g[3] + (j)];
		// }
	 //  }

		image::geometry<4> g_copy;
		g_copy.dim[0] = g[0];
		g_copy.dim[1] = g[1];
		g_copy.dim[2] = g[2];
		g_copy.dim[3] = ROI_MAX;
		image_data_cpy.resize(g_copy);

	#pragma omp parallel for collapse(3)
		for (int x = 0; x < g[0]; ++x)
		  for (int y = 0; y < g[1]; ++y)
			 for (int z = 0; z < g[2]; ++z)
			 	for(int r = 0;r<ROI_MAX;r++)
					image_data_cpy[((r*g[2]+z)*g[1]+y)*g[0]+x] = 0;


	  double timeseries = 1/float(g[3]);

	  tStart = clock();
	  cblas_dgemm(CblasRowMajor,CblasNoTrans,CblasNoTrans,ROI_MAX,valid_size,dime,timeseries,roi_avg,dime,Valid_matrix,valid_size,0.0,res,valid_size);
	  time_taken += (double)(clock()-tStart)/CLOCKS_PER_SEC;
	   //std::cout<<"Time taken:  for CORRELATION "<< ((double)(clock() - tStart)/CLOCKS_PER_SEC)<<std::endl;
	  //timeTk += (double)(clock() - tStart)/CLOCKS_PER_SEC ;
		

	
	  // for(int i_roi = 0;i_roi<ROI_MAX;i_roi++)
		 //  for(int j_roi = 0;j_roi<n3;j_roi++){
		 //  	if(startj==0){

		 //  		// roi_result[(i_roi)*valid_size + (starti*n+j_roi) ]=res[i_roi*n+j_roi];
		 //  		image_data_cpy[((i_roi*g[2]+valid[(starti*n+j_roi)].z)*g[1]+valid[(starti*n+j_roi)].y)*g[0]+valid[(starti*n+j_roi)].x] = res[i_roi*n+j_roi];

		 //  	}else{
		 //  		// roi_result[(i_roi)*valid_size + (starti*n+j_roi) ]+=res[i_roi*n+j_roi];
		 //  		image_data_cpy[((i_roi*g[2]+valid[(starti*n+j_roi)].z)*g[1]+valid[(starti*n+j_roi)].y)*g[0]+valid[(starti*n+j_roi)].x] += res[i_roi*n+j_roi];
		 //  	}

		 //  }
	
	// }
	//std::cout<<"Time taken:  for CORRELATION "<< timeTk<<std::endl;

	// make result 
	
	// #pragma omp parallel for collapse(3)
	// for (int x = 0; x < g[0]; ++x)
	//   for (int y = 0; y < g[1]; ++y)
	// 	 for (int z = 0; z < g[2]; ++z)
	// 	 	for(int r = 0;r<ROI_MAX;r++)
	// 			image_data_cpy[((r*g[2]+z)*g[1]+y)*g[0]+x] = 0;

	#pragma omp parallel for
	for(int r = 0;r<ROI_MAX;r++)
		for (int i = 0; i < valid_size; ++i)
		 	image_data_cpy[((r*g[2]+valid[i].z)*g[1]+valid[i].y)*g[0]+valid[i].x] = res[r*valid_size +i];

 	// save the result

	//image::io::nifti nifti_parser2;
	std::string cmd = "gzip ";
	cmd+=ipfilename;
  	system(cmd.c_str());

	nifti_parser2.load_from_image(image_data_cpy);
	std::string filename("avg_roi_time_series.nii");
	filename = ofname+"/"+ filename;
	nifti_parser2.save_to_file(filename.c_str());
	no_of_oper = 2*ROI_MAX*g[3]*valid_size+5*(g[0]*g[1]*g[2]*g[3])+(g[0]*g[1]*g[2]);
}

void avg_corr_roi(){

	//DEFINING VARIABLES  
  image::io::nifti nifti_parser,nifti_parser2;
  image::basic_image<double,4> image_data;
  image::basic_image<double,4> image_data_cpy;
  image::basic_image<int,3> roi_image;
  std::vector<Valid> valid;

  image_data_cpy = image_data;

	//LOADING THE IMAGE
  if(nifti_parser.load_from_file(ipfilename));
	  nifti_parser >> image_data;

	// LOADING THE ROI
  if(nifti_parser2.load_from_file(roifname))
  	   nifti_parser2 >> roi_image;
  
  image::geometry<4> g = image_data.geometry();
  image::geometry<4> g_copy;
  image::geometry<3> g_roi;

	//CHECK IF THE GEOMETRY OF THE MASK AND IMAGE IS SAME
   	g_roi = roi_image.geometry();
  	if(g_roi[0]!=g[0]||g_roi[1]!=g[1]||g_roi[2]!=g[2]){
  		std::cout<<":::: ERROR INVALID MASK ::::"<<std::endl;
  		return;
  	}

   image::basic_image<int,3> mask_image;
	if(mask&&nifti_parser.load_from_file(maskfilename))
	  	   nifti_parser >> mask_image;
	image::geometry<3> g_mask;


		//CHECK IF THE GEOMETRY OF THE MASK AND IMAGE IS SAME
	  if(mask){
	  	g_mask = mask_image.geometry();
	  	if(g_mask[0]!=g[0]||g_mask[1]!=g[1]||g_mask[2]!=g[2]){
	  		std::cout<<":::: ERROR INVALID MASK ::::"<<std::endl;
	  		return;
	  	}
	  }

  

  std::string cmd = "gzip ";
  cmd+=ipfilename;
  system(cmd.c_str());

	//############### DEFINING VARIABLES ####################################

  double *EX = (double *) malloc( sizeof(double) * g[0] * g[1]* g[2] ) ;
  double *EX_sq = (double *) malloc( sizeof(double) * g[0] * g[1]* g[2] ) ;
  
	//#CALCULATING THE MEAN AND STD DEVIATION OF DATA AND CREATING A VALID VOXEL MAPPINGS ################################
 std::cout<<":::: mean std start ::::"<<std::endl;
  for (int z = 0; z < g[2]; ++z){
	for (int y = 0; y < g[1]; ++y)
	  {
		for (int x = 0; x < g[0]; ++x)
		{

		  double temp  = 0,temp2 = 0;
		  for (int t = 0; t < g[3]; ++t)
		  {
			temp += (double)image_data[((t*g[2] + z)*g[1]+y)*g[0]+x]/g[3] ;
			temp2 += (double)image_data[((t*g[2] + z)*g[1]+y)*g[0]+x]*(double)image_data[((t*g[2] + z)*g[1]+y)*g[0]+x]/g[3];
		  }
		  
			
				  
		  EX[ (x*g[1] +y)*g[2] + z] = temp;
		  EX_sq[ (x*g[1] +y)*g[2] + z] = sqrt(temp2- temp*temp);

		  if((mask&&mask_image[(z*g[1]+y)*g[0]+x]==1)||(!mask&&EX_sq[ (x*g[1] +y)*g[2] + z] > 1e-2)){
				Valid tv ;
				tv.x = x;
				tv.y = y;
				tv.z = z;
				// int temp = valid.size();
				valid.push_back(tv);
		  }

		}
	  } 

  }

 std::cout<<":::: mean std end ::::"<<std::endl;

	//################## MAKING A MATRIX OF NORMALIZED DATA###########################################
  int valid_size = valid.size();
  double * Valid_matrix = (double * )malloc( sizeof(double)*valid_size*g[3]);
  double * roi_result = (double * )malloc( sizeof(double)*ROI_MAX*valid_size);
  double * roi_result_temp = (double * )malloc( sizeof(double)*ROI_MAX*g[3]);
  double * roi_coeff = (double * )calloc( ROI_MAX*valid_size, sizeof(double));
  double * roi_avg = (double * )calloc(ROI_MAX*g[3], sizeof(double));
  double * roi_var = (double * ) calloc(ROI_MAX, sizeof(double));
  double * roi_tot = (double * ) calloc(ROI_MAX, sizeof(double));
	
 std::cout<<":::: init start ::::"<<std::endl;
 # pragma omp parallel for shared(roi_avg,roi_tot)
  for (int i = 0; i < valid.size(); ++i)
  {	
  	int roi_no = roi_image[(valid[i].z*g[1] +valid[i].y)*g[2] + valid[i].x];
  	if(roi_no!=0)
  	  	roi_tot[roi_no-1]++;
  	double currentdev = EX_sq[(valid[i].x*g[1] +valid[i].y)*g[2] + valid[i].z];
	for (int t = 0; t < g[3]; ++t){
		
		double tempNormdata;
		if (currentdev == 0){
			tempNormdata = 0;
			image_data[((t*g[2] + valid[i].z)*g[1]+valid[i].y)*g[0]+valid[i].x] = tempNormdata;
			Valid_matrix[i*g[3]+t] = tempNormdata;

		}
		else{
			tempNormdata = (double)(image_data[((t*g[2] + valid[i].z)*g[1]+valid[i].y)*g[0]+valid[i].x] - EX[(valid[i].x*g[1] +valid[i].y)*g[2] + valid[i].z])/currentdev;
	  		if(roi_no!=0)
	  			roi_avg[(roi_no-1)*g[3]+t ] += (double)(image_data[((t*g[2] + valid[i].z)*g[1]+valid[i].y)*g[0]+valid[i].x] - EX[(valid[i].x*g[1] +valid[i].y)*g[2] + valid[i].z]);
	  		image_data[((t*g[2] + valid[i].z)*g[1]+valid[i].y)*g[0]+valid[i].x] = tempNormdata;
	  		Valid_matrix[i*g[3]+t] = tempNormdata;
	  		
		}

	}
	if(roi_no!=0)
		roi_coeff[(roi_no-1)*valid_size+i]=EX_sq[(valid[i].x*g[1] +valid[i].y)*g[2] + valid[i].z];

  }

  # pragma omp parallel for
  for (int i = 0; i < ROI_MAX; ++i)
  {	
  		double temp  = 0,temp2 = 0;
  		for (int t = 0; t < g[3]; ++t){
  			roi_avg[i*g[3]+t]/=roi_tot[i];
  			temp += (double)roi_avg[i*g[3]+t]/g[3] ;
			temp2 += (double)roi_avg[i*g[3]+t]*(double)roi_avg[i*g[3]+t]/g[3];
  		}

  		roi_var[i] = sqrt(temp2- temp*temp);

  }


  // # pragma omp parallel for
  for (int i = 0; i < valid.size(); ++i)
  {	
  	int roi_no = roi_image[(valid[i].z*g[1] +valid[i].y)*g[2] + valid[i].x];
  	if(roi_no==0)
  		continue;
  	roi_coeff[(roi_no-1)*valid_size+i]/=roi_var[roi_no];
  	roi_coeff[(roi_no-1)*valid_size+i]/=roi_tot[roi_no];
  }

   std::cout<<":::: init end ::::"<<std::endl;

  free(EX_sq);
  // free(roi_var);
  // free(roi_tot);
  // free(roi_avg);
	// clock_t tStart = clock();
	// free(EXY);


	// #######FINDING MATRIX FOR CORRELATION OF COMBINATION########################################
	free(EX);
	int dime = g[3];
	float timeseries = 1/(float)dime;
	 std::cout<<":::: blas start ::::"<<std::endl;
	cblas_dgemm(CblasRowMajor,CblasNoTrans,CblasNoTrans,ROI_MAX,dime,valid_size,1.0,roi_coeff,valid_size,Valid_matrix,dime,0.0,roi_result_temp,dime);
	cblas_dgemm(CblasRowMajor,CblasNoTrans,CblasTrans,ROI_MAX,valid_size,dime,timeseries,roi_result_temp,dime,Valid_matrix,dime,0.0,roi_result,valid_size);
	 std::cout<<":::: blas end ::::"<<std::endl;
	// int n =  8192;
	// double * roi_chunk = (double * )malloc( sizeof(double)*ROI_MAX*n );
	// double * roi_chunk2 = (double * )malloc( sizeof(double)*ROI_MAX*n );
	// double * roi_chunk_res = (double * )malloc( sizeof(double)*ROI_MAX*n );
	// double * res = (double * )malloc( sizeof(double)*n*n);
	// double * Valid_matrix_chunk= (double * )malloc( sizeof(double)*g[3]*n);
	// double * Valid_matrix_chunk_trans= (double * )malloc( sizeof(double)*n*g[3]);
 //  if(res==NULL){
	//   free(Valid_matrix);
	//   free(Valid_matrix_chunk);
	//   free(res);
	//   std::cout<<"res Allocating failed"<<std::endl;
	//   exit(0);
	// }

 //  if(Valid_matrix_chunk==NULL){
	//   free(Valid_matrix);
	//   free(Valid_matrix_chunk);
	//   free(res);
	//   std::cout<<"valid Allocating failed"<<std::endl;
	//   exit(0);
	// }
   
	// double timeTk =0;

	// //clock_t tStart = clock();


 //  for(int starti = 0;starti<valid.size();starti+=n)
	// for(int startj = starti;startj<valid.size();startj+=n)
	// {


	//   // tStart = clock();
	 
	//   // std::cout<<starti<<","<<startj<<std::endl;
	//   int n2 = min(n,valid.size()-starti);
	//   int n3 = min(n,valid.size()-startj);
	//   int n_toget=max(n2,n3);

	//   #pragma omp parallel for collapse(2)
	//   for (int i = 0; i < n_toget; ++i)
	//   {
	// 	for (int j = 0; j < g[3]; ++j)
	// 	{
	// 	  //######### AS BOTH THE MATRICES ARE OF DIFFERENT SIZES SO SELECT ACCORDINGLY######################################### 
	// 	  if(i<n2)
	// 	  Valid_matrix_chunk[i*g[3] + j]=Valid_matrix[(starti+i)*g[3] + (j)];
	// 	  if(i<n3)
	// 	  Valid_matrix_chunk_trans[j*n + i]=Valid_matrix[(startj+i)*g[3] + (j)];
	// 	}
	//   }
	//   #pragma omp parallel for collapse(2)
	//   for (int i = 0; i < ROI_MAX; ++i)
	//   {
	//   	for (int j = 0; j < n2; ++j)
	//   	{
	//   		roi_chunk[i*n+j] = roi_coeff[i*valid_size+starti*n+j]; 
	//   	}
	//   }


	//   double timeseries = 1/float(g[3]);

	//   cblas_dgemm(CblasRowMajor,CblasNoTrans,CblasNoTrans,n2,n3,dime,timeseries,Valid_matrix_chunk,dime,Valid_matrix_chunk_trans,n3,0.0,res,n3);
	  
	//   cblas_dgemm(CblasRowMajor,CblasNoTrans,CblasNoTrans,ROI_MAX,n3,n2,1.0,roi_chunk,n3,res,n3,0.0,roi_chunk_res,n3); 


	//   for(int i_roi = 0;i_roi<ROI_MAX;i_roi++)
	// 	  for(int j_roi = 0;j_roi<n3;j_roi++){
	// 	  	if(startj==0){

	// 	  		roi_result[(i_roi)*valid_size + (starti*n+j_roi) ]=roi_chunk_res[i_roi*n+j_roi];
	// 	  		// image_data_cpy[((i_roi*g[2]+valid[(starti*n+j_roi)].z)*g[1]+valid[(starti*n+j_roi)].y)*g[0]+valid[(starti*n+j_roi)].x] = roi_chunk_res[i_roi*n+j_roi];

	// 	  	}else{
	// 	  		roi_result[(i_roi)*valid_size + (starti*n+j_roi) ]+=roi_chunk_res[i_roi*n+j_roi];
	// 	  		// image_data_cpy[((i_roi*g[2]+valid[(starti*n+j_roi)].z)*g[1]+valid[(starti*n+j_roi)].y)*g[0]+valid[(starti*n+j_roi)].x] += roi_chunk_res[i_roi*n+j_roi];
	// 	  	}

	// 	  }
	// 	if(starti!=startj){
	// 		#pragma omp parallel for collapse(2)
	// 		for (int i = 0; i < ROI_MAX; ++i)
	// 		  {
	// 		  	for (int j = 0; j < n2; ++j)
	// 		  	{
	// 		  		roi_chunk2[i*n+j] = roi_coeff[i*valid_size+startj*n+j]; 
	// 		  	}
	// 		  }
	// 		cblas_dgemm(CblasRowMajor,CblasNoTrans,CblasNoTrans,ROI_MAX,n3,n2,1.0,roi_chunk2,n3,res,n3,0.0,roi_chunk_res,n3); 
	// 		for(int i_roi = 0;i_roi<ROI_MAX;i_roi++)
	// 		  for(int j_roi = 0;j_roi<n3;j_roi++){
	// 		  	if(startj==0){
	// 		  		// image_data_cpy[((i_roi*g[2]+valid[(startj*n+j_roi)].z)*g[1]+valid[(startj*n+j_roi)].y)*g[0]+valid[(startj*n+j_roi)].x] = roi_chunk_res[i_roi*n+j_roi];
	// 		  		roi_result[(startj*n+i_roi)*valid_size + (j_roi) ]=roi_chunk_res[i_roi*n+j_roi];

	// 		  	}else{
	// 		  		// image_data_cpy[((i_roi*g[2]+valid[(startj*n+j_roi)].z)*g[1]+valid[(startj*n+j_roi)].y)*g[0]+valid[(startj*n+j_roi)].x] += roi_chunk_res[i_roi*n+j_roi];
	// 		  		roi_result[(startj*n+i_roi)*valid_size + (j_roi) ]+=roi_chunk_res[i_roi*n+j_roi];
	// 		  	}

	// 		  }

	// 	}
 // 	}

	// make result 
	g_copy.dim[0] = g[0];
	g_copy.dim[1] = g[1];
	g_copy.dim[2] = g[2];
	g_copy.dim[3] = ROI_MAX;
	image_data_cpy.resize(g_copy);

 std::cout<<":::: output start ::::"<<std::endl;
	#pragma omp parallel for collapse(3)
	for (int x = 0; x < g[0]; ++x)
	  for (int y = 0; y < g[1]; ++y)
		 for (int z = 0; z < g[2]; ++z)
		 	for(int r = 0;r<ROI_MAX;r++)
				image_data_cpy[((r*g[2]+z)*g[1]+y)*g[0]+x] = 0;

	#pragma omp parallel for
	for(int r = 0;r<ROI_MAX;r++)
		for (int i = 0; i < valid_size; ++i)
		 	image_data_cpy[((r*g[2]+valid[i].z)*g[1]+valid[i].y)*g[0]+valid[i].x] = roi_result[r*valid_size +i];

 	// save the result

	//image::io::nifti nifti_parser2;
	cmd = "gzip ";
	cmd+=ipfilename;
	system(cmd.c_str());
	nifti_parser2.load_from_image(image_data_cpy);
	std::string filename("avg_roi_avg_time_series.nii");
	filename = ofname +"/"+ filename;
	nifti_parser2.save_to_file(filename.c_str());
	 std::cout<<":::: output end  ::::"<<std::endl;
}

void correl()
{
  if(seedcmp==true){
  		seed_correl();
  }else if(roi1==true){
  		avg_roi_time_corr();
  		if(flops)
  			std::cout<<"FLOPS: "<<(double)(no_of_oper)/time_taken<<std::endl;
  }else if(roi2){
  		avg_corr_roi();
  }else if(all==true){
  		all_pair_corr();
  		if(flops)
  			std::cout<<"FLOPS: "<<(double)(no_of_oper)/time_taken<<std::endl;
  }
}


// #########################################get options #############################################################################
void getattributes(int argc,char *argv[])
{
  char tmp;
  std::string inp(*argv);
  std::istringstream iss(inp);
  std::string s;
  std::vector< std::string > inp_arg;
  while ( getline( iss, s, ' ' ) ) {
    inp_arg.push_back(s);
  }

  int i =1;

  while(i<argc)
  {
  	char tmp = argv[i][1];
    std::cout<<tmp<<std::endl;
	switch(tmp)
	{
	  /*option h show the help infomation*/
	  case 'h':
		showhelpinfo();
		exit(0);
		break;
	  /*option u pres ent the username*/
	  case 'i':
		input= true;
		// std::cout<<optarg<<std::endl;
		// strcpy(ipfilename,optarg);
		i++;
		ipfilename = argv[i];
		if(ipfilename.find(".gz")!=std::string::npos){
			gzip = true;
			std::string command = "gunzip ";
			command += ipfilename;
			system(command.c_str());
			ipfilename = ipfilename.substr(0,(ipfilename.length()-3));
		}

		// if(ipfilename.find(".dcm")!=std::string::npos)
		// 	dcm = true;

		break;
	  /*option p present the password*/ 
	  case 'r':
		roi1= true;
		i++;
		roifname = argv[i];
		if(roifname.find(".gz")!=std::string::npos){
			gzip = true;
			std::string command = "gunzip ";
			command += roifname;
			system(command.c_str());
			roifname = roifname.substr(0,(roifname.length()-3));
		}
		i++;
		ROI_MAX = std::stoi(argv[i]);
		
		break;
	  case 'R':
		roi2= true;
		i++;
		roifname = argv[i];
		if(roifname.find(".gz")!=std::string::npos){
			gzip = true;
			std::string command = "gunzip ";
			command += roifname;
			system(command.c_str());
			roifname = roifname.substr(0,(roifname.length()-3));
		}
		i++;
		ROI_MAX = std::stoi(argv[i]);
		break;
	  case 'o':
		output = true;
		//strcpy(ofname,optarg);
		i++;
		ofname = argv[i];
		break;
	  case 't':
		// std::cout<<"thresh"<<std::endl;
		// std::cout<<optarg<<std::endl;
	  	i++;
		thresh = std::stoi(argv[i]);
	  break;
	  case 's':
		 seedcmp = true;
		 i++;
		 seedx = std::stoi(argv[i]);
		 i++;
		 seedy = std::stoi(argv[i]);
		 i++;
		 seedz = std::stoi(argv[i]);
		break;
	  case 'a': all = true;
				// std::cout<<all<<std::endl;
				break;
	  /*invail input will get the heil infomation*/
	  case 'm': mask = true;
	  			i++;
	  			maskfilename = argv[i];
				if(maskfilename.find(".gz")!=std::string::npos){
					gzip = true;
					std::string command = "gunzip ";
					command += maskfilename;
					system(command.c_str());
					maskfilename = maskfilename.substr(0,(maskfilename.length()-3));
				}
	  			break;
	  case 'f': flops = true;
	  			
	  			break;

	  default:
				showhelpinfo();
	  break;
	}

	i++;
  }
  // return 0;
}

/*################################funcion that show the help information################################################################*/
void showhelpinfo()
{
 std::cout<<"Usage :\n fconn -i <fmri.nii/nii.gz> -o <project name>  -[r <roi_filename> <N>/R <roi_filename> <N> /s <x> <y> <z>/a] -[f] -[t] -[h] \n";
  
  std::cout<<"Compulsory arguments(You have to specify the following)\n";
  std::cout<<"\t -i\t\t filename of the input volume \n";
  std::cout<<"\t -o \t\t project name  \n ";
  std::cout<<" one of the arguments must be present \n";
  std::cout<<"\t -r <roi_filename> <N>\t\t filename of the volume containg the desired ROI \n";
  std::cout<<"\t -R <roi_filename> <N> \t\t filename of the volume containg the desired ROI \n";
  std::cout<<"\t -s x y z  \t\t for seed to all voxel mode(no argument) \n";
  std::cout<<"\t\t x \t\t x-coordinate for seed (compulosry in -s mode if -r options not there)\n";
  std::cout<<"\t\t y \t\t y-coordinate for seed (compulosry in -s mode if -r options not there)\n";
  std::cout<<"\t\t z \t\t z-coordinate for seed (compulosry in -s mode if -r options not there)\n";
  std::cout<<"\t -a \t\t for all voxels to all voxels mode(no argument) \n";
  std::cout<<"\t -f \t\t report FLOPS \n";
  std::cout<<"Optional arguments(You may optionally specify the following)\n";
  std::cout<<"\t -t \t an upper threshold  \n ";
  std::cout<<"\t -h \t display this message  \n ";
  // 
}
