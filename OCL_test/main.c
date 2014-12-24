#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#include "mwc64x_rng.h"

#define DATA_D 16

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

int main()
{
    int i, j;
    char string_one[128];
    char string_two[128];
    char string[256];

    const char *source = NULL;

    cl_int err;

    cl_platform_id *platforms;
    cl_uint platformCount;

    cl_device_id *devices;
    cl_uint deviceCount;

    cl_context context;
    cl_context_properties properties[3];

    cl_command_queue command_queue;
    cl_program program;
    cl_kernel kernel;
    cl_mem input, output;
    size_t global[2]={DATA_D, DATA_D};

    int *inputData = NULL;
    float *outputData = NULL;
    float res[DATA_D];
    uint64_t state;

    float avg = 0.0f;

    inputData = (int *) malloc(DATA_D * DATA_D * sizeof(int));
    outputData = (float *) malloc(DATA_D * DATA_D * sizeof(float));

    // generate data
    MWC64X_Seed(&state, DATA_D * DATA_D, time(NULL));
    for (i = 0; i < DATA_D * DATA_D; i++)
    {
        inputData[i] = (int) MWC64X_Next(&state, DATA_D * DATA_D);
        outputData[i] = 0.0f;
    }

    source = read_source_file("sine_vec.cl");

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
 //       puts("platform number: ");
 //      fgets((char *) string, 7, stdin);
 //       i = strtol(string, NULL, 10);
 i = 0;
    } while (i >= platformCount);

    // get all devices
    clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, 0, NULL, &deviceCount);
    devices = (cl_device_id*) malloc(sizeof(cl_device_id) * deviceCount);
    clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, deviceCount, devices, NULL);

    do
    {
   //     puts("device number: ");
   //     fgets((char *) string, 7, stdin);
   //     j = strtol(string, NULL, 10);
        j=0;
    } while (j >= deviceCount);

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
    kernel = clCreateKernel(program, "sine_gpu", &err);

    // create buffers for the input and ouput
    input = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(int) * DATA_D * DATA_D, inputData, NULL);
    output = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(float) * DATA_D * DATA_D, NULL, NULL);

//    clEnqueueWriteBuffer(command_queue, input, CL_TRUE, 0, sizeof(int) * DATA_D * DATA_D, inputData, 0, NULL, NULL);

    // set the argument list for the kernel command
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &input);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &output);

    clEnqueueNDRangeKernel(command_queue, kernel, 2, NULL, global, NULL, 0, NULL, NULL);
    clFinish(command_queue);
    
    clEnqueueReadBuffer(command_queue, output, CL_TRUE, 0, sizeof(float) * DATA_D * DATA_D, outputData, 0, NULL, NULL);

    for (i = 0; i < DATA_D; i++)
    {
        avg = 0.0f;
        for (j = 0; j < DATA_D; j++)
        {
            avg += outputData[i * DATA_D + j];
        }
        res[i] = avg / DATA_D;
    }

    for (i = 0; i < DATA_D; i++)
    {
        printf(" %f\n", res[i]);
    }

    clReleaseMemObject(input);
    clReleaseMemObject(output);
    clReleaseProgram(program);
    clReleaseKernel(kernel);
    clReleaseCommandQueue(command_queue);
    clReleaseContext(context);
    free(devices);
    free(platforms);
    free((void *) source);
    free(inputData);
    free(outputData);

    return 0;
}
