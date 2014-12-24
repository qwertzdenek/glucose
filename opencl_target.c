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

cl_mem buf_seg_vals; // H->GPU
cl_mem buf_seg_vals_res; // GPU
cl_mem buf_lenghts; // H->GPU
cl_mem buf_res; // GPU->H

size_t two_dim[2] = {0, 0};
size_t one_dim[1] = {0};

int act_metric;

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

int init_opencl(int num_values, mvalue_ptr *values, int metric_type)
{
    int i, j;
    char string_one[128];
    char string_two[128];
    char string[256];

    const char *source = NULL;

    act_metric = metric_type;

    cl_int err;

    cl_uint platformCount;
    cl_uint deviceCount;
    cl_context_properties properties[3];

    int max = 0;

    // load values to dynamic memory
    for (i = 0; i < num_values; i++)
        max = fmaxl(max, values[i].cvals);

    mvalue *seg_vals = (mvalue *) malloc(sizeof(mvalue) * max * num_values);
    memset(seg_vals, 0, sizeof(mvalue) * max * num_values); // initialize

    for (i = 0; i < num_values; i++)
        memcpy(seg_vals + i * max, values[i].vals, sizeof(mvalue) * values[i].cvals);

    // create lenghts array
    int *lenghts = (int *) malloc(sizeof(int) * num_values);
    memset(lenghts, 0, sizeof(int) * num_values); // initialize

    for (i = 0; i < num_values; i++)
        lenghts[i] = values[i].cvals;

    source = read_source_file("fitness.cl");

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

    // ASK user
    do
    {
//        puts("platform number: ");
//        fgets((char *) string, 7, stdin);
//        i = strtol(string, NULL, 10);
        i = 0;
    }
    while (i >= platformCount);

    // get all devices
    clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, 0, NULL, &deviceCount);
    devices = (cl_device_id*) malloc(sizeof(cl_device_id) * deviceCount);
    clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, deviceCount, devices, NULL);

    do
    {
//        puts("device number: ");
//        fgets((char *) string, 7, stdin);
//        j = strtol(string, NULL, 10);
        j = 0;
    }
    while (j >= deviceCount);

    // context properties list - must be terminated with 0
    properties[0]= CL_CONTEXT_PLATFORM; // specifies the platform to use
    properties[1]= (cl_context_properties) platforms[i];
    properties[2]= 0;

    // create context
    context = clCreateContext(properties,deviceCount,devices,NULL,NULL,&err);
    if (err != CL_SUCCESS)
    {
        printf("chyba ve vytváření kontextu %d\n", err);
    }

    // create command queue
    command_queue = clCreateCommandQueueWithProperties(context, devices[j], 0, &err);
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

    buf_seg_vals = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(mvalue) * max * num_values, seg_vals, NULL);
    buf_lenghts = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(int) * num_values, lenghts, NULL);
    buf_seg_vals_res = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(mvalue) * max * num_values, NULL, NULL);
    buf_res = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(float) * num_values, NULL, NULL);

    free(seg_vals);
    free(lenghts);

    // set the argument list for the kernel command
    clSetKernelArg(kernel_equation, 0, sizeof(int), &num_values);
    clSetKernelArg(kernel_equation, 1, sizeof(cl_mem), &buf_seg_vals);
    clSetKernelArg(kernel_equation, 2, sizeof(cl_mem), &buf_lenghts);
    clSetKernelArg(kernel_equation, 4, sizeof(cl_mem), &buf_seg_vals_res);
    clSetKernelArg(kernel_equation, 5, sizeof(char), &act_metric);

    clSetKernelArg(kernel_avg, 0, sizeof(int), &max);
    clSetKernelArg(kernel_avg, 1, sizeof(cl_mem), &buf_seg_vals_res);
    clSetKernelArg(kernel_avg, 2, sizeof(cl_mem), &buf_lenghts);
    clSetKernelArg(kernel_avg, 3, sizeof(cl_mem), &buf_res);

    one_dim[0] = num_values;
    two_dim[0] = max;
    two_dim[1] = max;

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

void cl_compute_fitness(int num_members, member *members)
{
    int i;
    int num_values = one_dim[0];
    float *res = (float *) malloc(sizeof(float) * num_values);
    float (*eval)(int, float *);

    switch (act_metric)
    {
    case METRIC_ABS:
    case METRIC_SQ:
        eval = avg;
        break;
    case METRIC_MAX:
        eval = max;
        break;
    default:
        return;
    }

    for (i = 0; i < num_members; i++)
    {
        clSetKernelArg(kernel_equation, 3, sizeof(member), &members[i]);
        clEnqueueNDRangeKernel(command_queue, kernel_equation, 2, NULL, two_dim, NULL, 0, NULL, NULL);
        clEnqueueNDRangeKernel(command_queue, kernel_avg, 1, NULL, one_dim, NULL, 0, NULL, NULL);
        clFinish(command_queue);
        clEnqueueReadBuffer(command_queue, buf_res, CL_TRUE, 0, sizeof(float) * num_values, res, 0, NULL, NULL);

        members[i].fitness = eval(num_values, res);
    }

    free(res);

    // write members
//    clEnqueueWriteBuffer(command_queue, input, CL_TRUE, 0, sizeof(int) * DATA_D * DATA_D, inputData, 0, NULL, NULL);
    // set kernel argument
//    clEnqueueNDRangeKernel(command_queue, kernel, 2, NULL, global, NULL, 0, NULL, NULL);
//    clFinish(command_queue);

//    clEnqueueReadBuffer(command_queue, output, CL_TRUE, 0, sizeof(float) * DATA_D * DATA_D, outputData, 0, NULL, NULL);
}

void cl_cleanup()
{
    // clean CL objects
    clReleaseMemObject(buf_seg_vals);
    clReleaseMemObject(buf_seg_vals_res);
    clReleaseMemObject(buf_lenghts);
    clReleaseMemObject(buf_res);
    clReleaseProgram(program);
    clReleaseKernel(kernel_equation);
    clReleaseKernel(kernel_avg);
    clReleaseCommandQueue(command_queue);
    clReleaseContext(context);

    // clean host memory
    free(devices);
    free(platforms);
}
