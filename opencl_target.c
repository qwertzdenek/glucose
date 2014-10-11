/*
The MIT License (MIT)
Copyright (c) 2014 Zdeněk Janeček

** Glucose project**
opencl_target.c
*/

#include <stdio.h>
#include <stdlib.h>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

int init_opencl() {
    int i;
    char* value;
    size_t valueSize;

    // Get platform and device information
    cl_platform_id platform_id = NULL;
    cl_uint ret_num_platforms;

    cl_device_id* devices;
    cl_uint deviceCount;

    clGetPlatformIDs(1, &platform_id, &ret_num_platforms);

    // get all devices
    clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_ALL, 0, NULL, &deviceCount);
    devices = (cl_device_id*) malloc(sizeof(cl_device_id) * deviceCount);
    clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_ALL, deviceCount, devices, NULL);

    for (i = 0; i < deviceCount; i++)
    {
        clGetDeviceInfo(devices[i], CL_DEVICE_NAME, 0, NULL, &valueSize);
        value = (char*) malloc(valueSize);
        clGetDeviceInfo(devices[i], CL_DEVICE_NAME, valueSize, value, NULL);
        printf("%d. Device: %s\n", i+1, value);
        free(value);
    }

    free(devices);

    return 0;
}
