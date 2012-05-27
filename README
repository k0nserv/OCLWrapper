OCLWrapper
==========
OCLWrapper is a simple OpenCL wrapper class that gives easy access to basic functionality in OpenCL.

Example
-------
Rewritten version of http://www.thebigblob.com/using-the-cpp-bindings-for-opencl/

```C++
/*Adding two vectors.*/
#include <iostream>
#include "OCLWrapper.h"


int main() {
  const int size = 500;
  int* l1 = new int[size];
  int* l2 = new int[size];
  /* Result */
  int* re = new int[size];
  OCLWrapper* ocl = new OCLWrapper("C:\\kernel.cl");

  for(int i = 0; i < size; i++ ) {
      l1[i] = i;
      l2[i] = size - i;
  }

  ocl->build("vector_add");
  
  /* Didn't build correctly,
     print build log 
  */
  if(!ocl->isBuilt())
    std::cout << ocl->buildLog() << std::endl;
  
  ocl->addArgument(l1, sizeof(int)*size);
  ocl->addArgument(l2, sizeof(int)*size);
  ocl->addReturn(sizeof(int)*size);

  ocl->execute(size, 1);

  /* Read the result back */
  ocl->readMemory(sizeof(int)*size, 0, re);
  
  for(int i = 0; i < size; i++) {
      std::cout << re[i] << std::endl;
  }

  return 0;
}
```

Kernel source
```C
/* kernel.cl */
__kernel void vector_add(__global const int *A, __global const int *B, __global int *C) {
 
    // Get the index of the current element to be processed
    int i = get_global_id(0);
 
    // Do the operation
    C[i] = A[i] + B[i];
}
```


