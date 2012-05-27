/*
File: OCLWrapper.cpp
Class: OCLWrapper
Author: Hugo Tunius
Web: http://hugotunius.se
Date: May 2012

Description:
The class contained in this file is a naive wrapper for basic opencl functions, it's in no way a complete
library and is only tested under very special cases of OpenCL usage.

The purpose of the wrapper is to create a simple way to manage memory, read, execute and build the kernel source code.

Lisence:
Copyright (C) 2012 Hugo Tunius

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), 
to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <iostream>
#include "OCLWrapper.h"


OCLWrapper::OCLWrapper (char* file) {
    cl_int result;
    platform_ids = NULL;
    device_id = NULL;
    arguments = vector<cl_mem > ();

    failed = false;
    printDebug = true;
    ran = false;
    built = false;

	readSource(file);

    /* Setting up opencl */
    clGetPlatformIDs(1, &platform_ids, &num_platforms); //Get avalible platform
    failed = !(num_platforms > 0);
    clGetDeviceIDs(platform_ids, CL_DEVICE_TYPE_DEFAULT, 1,
                   &device_id, &num_devices); //Get avalible devices on platform
    failed = !(num_devices > 0);

    context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &result); //Retrive context from the devices
    handleError(result, __LINE__);
    command_queue = clCreateCommandQueue(context, device_id, 0, &result); //Retrive a command queue from the context
    handleError(result, __LINE__);
}

OCLWrapper::~OCLWrapper () {
    /* Clean up memory */
    freeMemory();
    delete[] source;
}

void OCLWrapper::freeMemory () {
    for (int i = 0; i < arguments.size(); i++) {
        clReleaseMemObject(arguments.at(i));
    }

    for (int i = 0; i < return_values.size(); i++) {
        clReleaseMemObject(return_values.at(i));
    }

    arguments.clear();
    return_values.clear();
    ran = false;
}

void OCLWrapper::addArgument (void* t, uint length) {
    cl_int result;
    cl_mem mem = clCreateBuffer(context, CL_MEM_READ_ONLY,
                                length, NULL, &result);
    clEnqueueWriteBuffer(command_queue, mem, CL_TRUE, 0,
                         length, t, 0, NULL, NULL);
    arguments.push_back(mem);
}

void OCLWrapper::addReturn (uint length) {
    cl_int result;
    cl_mem mem = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                                length, NULL, &result);
    handleError(result, __LINE__);
    return_values.push_back(mem);
}

bool OCLWrapper::build (char* kernelName) {
    bool toReturn = true;
    const size_t a[] = {srclength};
    const char* c[] = {source};
    cl_int result = 0;

    program = clCreateProgramWithSource(context, 1, c, a, &result);
    toReturn = toReturn && (result == CL_SUCCESS);
    result = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
    toReturn = toReturn && (result == CL_SUCCESS);
    kernel = clCreateKernel(program, kernelName, &result);
    toReturn = toReturn && (result == CL_SUCCESS);

	built  = toReturn;
	failed = !toReturn;

    return toReturn;
}

void OCLWrapper::execute (size_t work_size, size_t local_size) {
    if (built) {
        int j;
        cl_int ret;
        for (int i = 0; i < arguments.size(); i++) {
            cl_mem current = arguments.at(i);
            ret = clSetKernelArg(kernel, i, sizeof (current), (void *) &current);
            j = i;
        }
        j++;
        for (int i = 0; i < return_values.size(); i++) {
            int arg = j + i;
            cl_mem current = return_values.at(i);
            ret = clSetKernelArg(kernel, arg, sizeof (current), (void*) &current);
        }

        ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, &work_size,
                                     &local_size, 0, NULL, NULL);
        handleError(ret, __LINE__);
        ran = ret == CL_SUCCESS;
    }
}

int OCLWrapper::readMemory (uint size, uint arg, void* o) {
    if (arg > return_values.size()) {
        return -1;
    }

    cl_mem mem = return_values.at(arg);
    return clEnqueueReadBuffer(command_queue, mem, CL_TRUE, 0,
                               size, o, 0, NULL, NULL);
}

char* OCLWrapper::buildLog () {
    cl_build_status status = 0;
    size_t size;

    clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_STATUS, NULL, &status, &size);
    clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, NULL, NULL, &size);
    if (status == CL_BUILD_ERROR && size > 0) {
        char* c = new char[size];
        clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, size, c, &size);
        return c;
    }
    else {
        return "Kernel built successfully";
    }
}

/* Getters */
char* OCLWrapper::getSource () {
    return this->source;
}

bool OCLWrapper::isBuilt () {
    return built;
}

bool OCLWrapper::isDone () {
    return ran;
}

/* Setters */
void OCLWrapper::setDebug (bool f) {
    printDebug = f;
}

void OCLWrapper::setArgument (void* t, uint length, uint arg) {
    if (arg < arguments.size()) {
        clReleaseMemObject(arguments.at(arg)); //Release the memory first
        cl_int result;
        cl_mem mem = clCreateBuffer(context, CL_MEM_READ_ONLY,
                                    length, NULL, &result);
        clEnqueueWriteBuffer(command_queue, mem, CL_TRUE, 0,
                             length, t, 0, NULL, NULL);
        arguments[arg] = mem;
    }
}

/* Private utility Methods */
void OCLWrapper::handleError (cl_int result, int line) {
    if (printDebug) {
        std::cerr << "Result at line " << line - 1 << " in " << __FILE__ << " ";
        std::cerr << errors[result] << std::endl;
    }
}

void OCLWrapper::readSource(char* file) {
	/* Reading the source */
	int length = 0;
	ifstream in;

	in.open(file, ios::binary);
	in.seekg(0, ios::end);
	length = static_cast<int> (in.tellg());
	in.seekg(0, ios::beg);
	if (length > 0) {
		source = new char [length];
		in.read(source, length);
		in.close();
		}
	else {
		source = "No file found";
		failed = true;
		};
	srclength = length;

}





