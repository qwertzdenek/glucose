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

int init_opencl()
{
    int i, j;
    char buf[100];
    size_t valueSize;

    // Get platform and device information
    cl_platform_id *platforms;
    cl_uint platformCount;

    cl_device_id *devices;
    cl_uint deviceCount;

    clGetPlatformIDs(0, NULL, &platformCount);
    platforms = (cl_platform_id *) malloc(sizeof(cl_platform_id) * platformCount);
    clGetPlatformIDs(platformCount, platforms, NULL);

    printf("have %d platforms\n", platformCount);

    for (i = 0; i < platformCount; i++)
    {
        // get all devices
        clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, 0, NULL, &deviceCount);
        devices = (cl_device_id*) malloc(sizeof(cl_device_id) * deviceCount);
        clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, deviceCount, devices, NULL);

        for (j = 0; j < deviceCount; j++)
        {
            clGetDeviceInfo(devices[j], CL_DEVICE_NAME, sizeof(buf), buf, &valueSize);
            buf[valueSize - 1] = ' ';
            clGetDeviceInfo(devices[j], CL_DEVICE_OPENCL_C_VERSION, sizeof(buf), (char *) buf + valueSize, NULL);

            printf("platform=%d, device=%d: %s\n", i, j, buf);
        }

        free(devices);
    }

    free(platforms);

    return 0;
}
