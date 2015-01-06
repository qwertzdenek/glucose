/*
The MIT License (MIT)
Copyright (c) 2014 Zdeněk Janeček

** Glucose project**
opencl_target.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

#include "opencl_target.h"

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

cl_platform_id *platforms;
cl_device_id *devices;
cl_context context;
cl_command_queue command_queue;
cl_program program;
cl_kernel kernel_equation;
cl_kernel kernel_avg;

cl_mem buf_seg_vals;
cl_mem buf_lenghts;

cl_mem buf_seg_vals_res; // (segmenty) * (max) * (populace) -> float
cl_mem buf_res; // (segmenty) * (populace) -> float
cl_mem buf_members; // (populace) -> member

size_t three_dim[3] = {0, 0, 0};
size_t one_dim[1] = {0};

int population;
int segments;
int max_seg_vals;
int act_metric;

float *res_avg; // results from the GPU (population size)

static char* read_source_file(const char *filename)
{
    long int
    size = 0,
    res  = 0;

    char *src = NULL;

    FILE *file = fopen(filename, "rb");

    if (!file)  return NULL;

    if (fseek(file, 0, SEEK_END))
    {
        fclose(file);
        return NULL;
    }

    size = ftell(file);
    if (size == 0)
    {
        fclose(file);
        return NULL;
    }

    rewind(file);

    src = (char *)calloc(size + 1, sizeof(char));
    if (!src)
    {
        src = NULL;
        fclose(file);
        return src;
    }

    res = fread(src, 1, sizeof(char) * size, file);
    if (res != sizeof(char) * size)
    {
        fclose(file);
        free(src);

        return src;
    }

    src[size] = '\0'; /* NULL terminated */
    fclose(file);

    return src;
}

int init_opencl(int num_values, mvalue_ptr *values, int size_members, int metric_type)
{
    int i, j;
    char string_one[128];
    char string_two[128];
    char string[256];
    int platform_index = 0;
    int device_index = 0;

    const char *source = NULL;

    population = size_members;
    segments = num_values;
    act_metric = metric_type;

    cl_int err;

    cl_uint platformCount;
    cl_uint deviceCount;
    cl_context_properties properties[3];

    // Probe platforms
    clGetPlatformIDs(0, NULL, &platformCount);
    platforms = (cl_platform_id *) malloc(sizeof(cl_platform_id) * platformCount);
    clGetPlatformIDs(platformCount, platforms, NULL);

    for (i = 0; i < platformCount; i++)
    {
        printf("platform %d\n", i);

        // get all devices
        clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, 0, NULL, &deviceCount);
        devices = (cl_device_id*) malloc(sizeof(cl_device_id) * deviceCount);
        clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, deviceCount, devices, NULL);

        for (j = 0; j < deviceCount; j++)
        {
            clGetDeviceInfo(devices[j], CL_DEVICE_NAME, 128, string_one, NULL);
            clGetDeviceInfo(devices[j], CL_DEVICE_OPENCL_C_VERSION, 128, string_two, NULL);

            sprintf(string, "%s (version %s)", string_one, string_two);

            printf("  device %d: %s\n", j, string);
        }

        free(devices);
    }

    if (platformCount == 0)
    {
        fprintf(stderr, "OpenCL platform not found\n");
        return OPENCL_ERROR;
    }

    // ASK user
    do
    {
//        puts("platform number: ");
//        fgets((char *) string, 7, stdin);
//        i = strtol(string, NULL, 10);
i = 0;
    }
    while (i >= platformCount);

    platform_index = i;

    // get all devices
    clGetDeviceIDs(platforms[platform_index], CL_DEVICE_TYPE_ALL, 0, NULL, &deviceCount);
    devices = (cl_device_id*) malloc(sizeof(cl_device_id) * deviceCount);
    clGetDeviceIDs(platforms[platform_index], CL_DEVICE_TYPE_ALL, deviceCount, devices, NULL);

    do
    {
//        puts("device number: ");
//        fgets((char *) string, 7, stdin);
//        j = strtol(string, NULL, 10);
j = 0;
    }
    while (j >= deviceCount);

    device_index = j;

    // load values to dynamic memory
    for (i = 0; i < segments; i++)
        max_seg_vals = max_seg_vals > values[i].cvals ? max_seg_vals : values[i].cvals;

    mvalue *seg_vals = (mvalue *) malloc(sizeof(mvalue) * max_seg_vals * segments);
    memset(seg_vals, 0, sizeof(mvalue) * max_seg_vals * segments); // initialize

    for (i = 0; i < segments; i++)
        memcpy(seg_vals + i * max_seg_vals, values[i].vals, sizeof(mvalue) * values[i].cvals);

    // create lenghts array
    int *lenghts = (int *) malloc(sizeof(int) * segments);

    for (i = 0; i < segments; i++)
        lenghts[i] = values[i].cvals;

    // read kernels
    source = read_source_file("fitness.cl");

    // context properties list - must be terminated with 0
    properties[0]= CL_CONTEXT_PLATFORM; // specifies the platform to use
    properties[1]= (cl_context_properties) platforms[platform_index];
    properties[2]= 0;

    // create context
    context = clCreateContext(properties,deviceCount,devices,NULL,NULL,&err);
    if (err != CL_SUCCESS)
    {
        printf("chyba ve vytváření kontextu %d\n", err);
    }

    // create command queue
    command_queue = clCreateCommandQueueWithProperties(context, devices[device_index], 0, &err);
    if (err != CL_SUCCESS)
    {
        printf("chyba ve vytváření fronty úloh %d\n", err);
    }

    program = clCreateProgramWithSource(context, 1, &source, 0, &err);

    if (clBuildProgram(program, 0, NULL, NULL, NULL, NULL) != CL_SUCCESS)
    {
        printf("Error building program\n");
        clReleaseCommandQueue(command_queue);
        clReleaseContext(context);
        free(devices);
        free(platforms);
        return 1;
    }

    // specify which kernel from the program to execute
    kernel_equation = clCreateKernel(program, "solve_equation", &err);
    kernel_avg = clCreateKernel(program, "solve_avg", &err);

    free((void *) source);

    buf_seg_vals = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(mvalue) * max_seg_vals * segments, seg_vals, NULL);
    buf_lenghts = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(int) * segments, lenghts, NULL);

    buf_seg_vals_res = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(float) * max_seg_vals * segments * population, NULL, NULL);
    buf_res = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(float) * population, NULL, NULL);
    buf_members = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(member) * population, NULL, NULL);

    free(seg_vals);
    free(lenghts);

    // set the argument list for the kernel command
    clSetKernelArg(kernel_equation, 0, sizeof(int), &segments);
    clSetKernelArg(kernel_equation, 1, sizeof(cl_mem), &buf_seg_vals);
    clSetKernelArg(kernel_equation, 2, sizeof(cl_mem), &buf_lenghts);
    clSetKernelArg(kernel_equation, 3, sizeof(int), &population);
    clSetKernelArg(kernel_equation, 4, sizeof(cl_mem), &buf_members);
    clSetKernelArg(kernel_equation, 5, sizeof(cl_mem), &buf_seg_vals_res);
    clSetKernelArg(kernel_equation, 6, sizeof(char), &act_metric);

    clSetKernelArg(kernel_avg, 0, sizeof(int), &max_seg_vals);
    clSetKernelArg(kernel_avg, 1, sizeof(int), &segments);
    clSetKernelArg(kernel_avg, 2, sizeof(cl_mem), &buf_seg_vals_res);
    clSetKernelArg(kernel_avg, 3, sizeof(cl_mem), &buf_lenghts);
    clSetKernelArg(kernel_avg, 4, sizeof(cl_mem), &buf_res);
    clSetKernelArg(kernel_avg, 5, sizeof(char), &act_metric);

    three_dim[0] = max_seg_vals;
    three_dim[1] = segments;
    three_dim[2] = population;

    one_dim[0] = population;

    // results from the solve_avg kernel
    res_avg = (float *) malloc(sizeof(float) * segments * population);

    return 0;
}

float avg(int num_array, float *array)
{
    float average = 0;
    int i;
    for (i = 0; i < num_array; i++)
    {
        average += array[i];
    }
    return average / num_array;
}

float max(int num_array, float *array)
{
    float maximum = FLT_MIN;
    int i;
    for (i = 0; i < num_array; i++)
    {
        maximum = fmaxf(maximum, array[i]);
    }
    return maximum;
}

void cl_compute_fitness(member *members)
{
    int i;

    clEnqueueWriteBuffer(command_queue, buf_members, CL_TRUE, 0, sizeof(member) * population, members, 0, NULL, NULL);

    clEnqueueNDRangeKernel(command_queue, kernel_equation, 3, NULL, three_dim, NULL, 0, NULL, NULL);
    clEnqueueNDRangeKernel(command_queue, kernel_avg, 1, NULL, one_dim, NULL, 0, NULL, NULL);

    clFinish(command_queue);
    clEnqueueReadBuffer(command_queue, buf_res, CL_TRUE, 0, sizeof(float) * population, res_avg, 0, NULL, NULL);

    for (i = 0; i < population; i++)
    {
        members[i].fitness = res_avg[i];
    }
}

void cl_cleanup()
{
    // clean CL objects
    clReleaseMemObject(buf_seg_vals);
    clReleaseMemObject(buf_seg_vals_res);
    clReleaseMemObject(buf_lenghts);
    clReleaseMemObject(buf_res);
    clReleaseMemObject(buf_members);
    clReleaseProgram(program);
    clReleaseKernel(kernel_equation);
    clReleaseKernel(kernel_avg);
    clReleaseCommandQueue(command_queue);
    clReleaseContext(context);

    // clean host memory
    free(res_avg);
    free(devices);
    free(platforms);
}
