#include "helpers.h"
#include <stdlib.h>
#include <stdio.h>
#include <cuda_runtime.h>
#include "settings.h"

void printHelp(){
    printf("This is a program for N-body simulation\n");
    printf("Supported arguments :\n");
    printf("\t-s sets the ID of first frame[optional]\n");
    printf("\t-N sets bodys amount\n");
    printf("\t-f sets frames amount\n");
    printf("\t-w sets write step(frequency of file generation)\n");
    printf("\t-t sets threads amount\n");
    printf("\t-I sets Integrator type(1 - Euler, 2 - velocity-Verlet, 4 - Forest-Ruth)[optional, default=1]\n");
    printf("\t-B runs program in bencmarking mode(deactivates processing progress && backups)[optional]\n");
    printf("\t-b writes backups files[optional]\n");
    printf("\t-c sets input catalogue name\n");
    printf("\t-G use GPU instead of CPU for calculations(better works for huge amount of bodys, such as 4096)[optional]\n");
    printf("\t-? or -h prints this help\n");
};

void *protectedMalloc(char const* arrName, size_t size){
    if(size < 1){
        fprintf(stderr, "ERROR: Too small memory amount for array %s\n", arrName);
        exit(1);
    };
    void *ptr = malloc(size);
    if(ptr == NULL){
        fprintf(stderr, "ERROR: Couldn't allocate memory for %s\n", arrName);
        exit(1);
    };
    return ptr;
};

frame * readFrame(char const* frameName, int N_BODYS){
    FILE *inp;
    inp = fopen(frameName, "r");
    if(inp == NULL){
        fprintf(stderr, "ERROR: Can't open file %s\n", frameName);
        exit(1);
    };
    
    frame *tmp = (frame*)protectedMalloc("tmp frame", sizeof(frame));  
	tmp->bodys = (float4*)protectedMalloc("tmp frame bodys", sizeof(float4) * N_BODYS);
	tmp->vels = (float4*)protectedMalloc("tmp frame vels", sizeof(float4) * N_BODYS);                 
                    
    for(int i = 0; i < N_BODYS; i++){
        if(fscanf(inp, "%E %E %E %E %E %E %E",&tmp->bodys[i].w, &tmp->bodys[i].x, &tmp->bodys[i].y, &tmp->bodys[i].z, &tmp->vels[i].x, &tmp->vels[i].y, &tmp->vels[i].z) != 7) {
            fprintf(stderr, "ERROR: Can't read file %s\n", frameName);
            free(tmp->bodys);
            free(tmp->vels);
            free(tmp);
            fclose(inp);
            exit(1);
        };
    };
    
    fclose(inp);
    return tmp;
};

void printFrame(frame const* fr, int N_BODYS){
    for(int i = 0; i < N_BODYS; i++){
        fprintf(stdout, "%f %f %f %f %f %f\n", fr->bodys[i].x, fr->bodys[i].y, fr->bodys[i].z, fr->vels[i].x, fr->vels[i].y, fr->vels[i].z);
    };
};

void writeFrameFull(char const* frameName,const frame* fr, int N_BODYS){
    FILE * out = fopen(frameName, "w");
    if(out == NULL){
        fprintf(stderr, "ERROR: Can't open file %s\n", frameName);
        exit(1);
    };
    for(int i = 0; i < N_BODYS; i++){
        fprintf(out, "%f %f %f %f %f %f %f\n", fr->bodys[i].w, fr->bodys[i].x, fr->bodys[i].y, fr->bodys[i].z, fr->vels[i].x, fr->vels[i].y, fr->vels[i].z);
    };
    fclose(out);
};

void writeFrameShort(char const* frameName,const frame* fr, int N_BODYS){
    FILE * out = fopen(frameName, "w");
    if(out == NULL){
        fprintf(stderr, "ERROR: Can't open file %s\n", frameName);
        exit(1);
    };
    for(int i = 0; i < N_BODYS; i++){
        fprintf(out, "%f %f %f\n", fr->bodys[i].x, fr->bodys[i].y, fr->bodys[i].z);
    };
    fclose(out);
};

void freeFrame(frame* fr){
	free(fr->bodys);
	free(fr->vels);
    if(fr->devBodys != NULL){
        cudaFree(fr->devBodys);
        cudaFree(fr->devVels);
        cudaFree(fr->devAccels);
    } else {
        free(fr->accels);
    };
};

void checkCudaErrors(char const* errMsg){
	cudaError_t err = cudaGetLastError();
	if(err != cudaSuccess){
		fprintf(stderr, "Error: %s\n", cudaGetErrorString(err));
		fprintf(stderr, "%s\n", errMsg);
		exit(1);
	};
};

void* cudaProtectedMalloc(char const* arrName, unsigned int size){
	void * tmp;
	cudaMalloc(&tmp, size);
	checkCudaErrors(arrName);
	return tmp;
};

void cudaProtectedMemcpyD(char const* errMsg, void * devPtr, void * hostPtr, unsigned int size){
	cudaMemcpy(devPtr, hostPtr, size, cudaMemcpyHostToDevice);
	checkCudaErrors(errMsg);
};

void cudaProtectedMemcpyH(char const* errMsg, void * hostPtr, void * devPtr, unsigned int size){
	cudaMemcpy(hostPtr, devPtr, size, cudaMemcpyDeviceToHost);
	checkCudaErrors(errMsg);
};
