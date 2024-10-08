// Note: Only this file may declare the following two macros

#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <Metal/Metal.hpp>
#include <MetalKit/MetalKit.hpp>
#include <simd/simd.h>

#include "engine.hpp"

const char* LIB_NAME = "ferrum";
const char* LIB_TYPE = "metallib";
const char* FERRUM_LIB = "FERRUM_LIB";

const char* str(const NS::String* s);
MTL::Device* getDevice();
MTL::Library* initLibrary(MTL::Device* device, const char* path);


// constructor for Ferrum::MetalEngine
Ferrum::MetalEngine::MetalEngine(const char* path) :
    emptyAction([](std::vector<MTL::Buffer*>&, int) {}) {
  DBG("Getting Metal device");
  device = getDevice();
  DBG("Initializing library...");
  library = initLibrary(device, path);
  if (library == nullptr) {
    std::cerr << "Error: Failed to initialize Metal library" << std::endl;
    return;
  }
  DBG("Creating command queue...");

  commandQueue = device->newCommandQueue();

  DBG("Retrieving function names...");
  NS::Array* functions = library->functionNames();
  fnCount = functions->count();
  if (fnCount == 0) {
    std::cerr << "Error: No functions found in library" << std::endl;
    return;
  }
  DBG("Retrieved ", fnCount, " functions");
  DBG("Collecting function pipline states...");
  kernelFunctions = new MTL::Function*[fnCount];
  computePipelineStates = new MTL::ComputePipelineState*[fnCount];
  NS::Error* pError = nullptr;
  for (int i = 0; i < fnCount; i++) {
    NS::String* fnName = static_cast<NS::String*>(functions->object(i));
    kernelFunctions[i] = library->newFunction(fnName);
    if (kernelFunctions[i] == nullptr) {
      std::cerr << "Error: Failed to create function: " << str(fnName) << std::endl;
    }
    MTL::ComputePipelineState* pipelineState = device->newComputePipelineState(kernelFunctions[i], &pError);
    if (pError != nullptr) {
      std::cerr << "Error: on function '" << str(fnName) << "': " << str(pError->localizedDescription()) << std::endl;
    } else if (pipelineState == nullptr) {
      std::cerr << "Error: Failed to create pipeline state for: " << str(fnName) << std::endl;
    } else {
      DBG("Created pipeline state for: ", str(fnName));
      auto idIt = functionMap->find(str(fnName));
      if (idIt == functionMap->end()) {
        std::cerr << "Error: Unknown function: " << str(fnName) << std::endl;
      } else {
        computePipelineStates[static_cast<int>(idIt->second)] = pipelineState;
      }
    }
  }
  DBG("Initialization complete");
}


Ferrum::MetalEngine::~MetalEngine() {
  if (computePipelineStates != nullptr) {
    for (int i = 0; i < fnCount; i++) {
      if (computePipelineStates[i] != nullptr) {
        computePipelineStates[i]->release();
      }
      if (kernelFunctions[i] != nullptr) {
        kernelFunctions[i]->release();
      }
    }
    delete[] computePipelineStates;
    delete[] kernelFunctions;
  }
  if (commandQueue != nullptr) {
    commandQueue->release();
  }
  if (library != nullptr) {
    library->release();
  }
}


// convert C string to NSString
inline NS::String* nsStr(const char* s) {
  return NS::String::string(s, NS::UTF8StringEncoding);
}

// convert NSString to C string
const char* str(const NS::String* s) {
  return (s != nullptr) ? s->utf8String() : "<null>";
}

// library names for the metal shader library and the ferrum library
NS::String* metalExt = nsStr(LIB_TYPE);
NS::String* ferrumName = nsStr(LIB_NAME);

NS::String* getEnv(const char* name) {
  const char* value = std::getenv(name);
  return value != nullptr ? nsStr(value) : nullptr;
}

const NS::String* getLibPath(const char* path) {
  NS::String* resource = (path == nullptr) ? ferrumName : nsStr(path);
  DBG("Library Resource Name: ", str(resource));
  // check in the bundle
  NS::BundleEx* mainBundle = NS::BundleEx::mainBundle();
  DBG("Called for a main bundle");
  if (mainBundle != nullptr && mainBundle->bundlePath() != nullptr) {
    DBG("Bundle has a path");
    const NS::String* resource_path = mainBundle->pathForResource(resource, metalExt);
    if (resource_path != nullptr) {
      return resource_path;
    }
  }

  DBG("Not in bundle");
  // not in the bundle, so check if the path points to the file
  bool isDir = false;
  NS::FileManager* fileManager = NS::FileManager::defaultManager();

  DBG("Got a file manager");
  if (path != nullptr) {
    if (fileManager != nullptr && fileManager->fileExistsAtPath(resource, &isDir)) {
      // if it's not a directory, then it's the file
      if (!isDir) {
        return resource;
      }
      // it's a directory, so check if the file is in that directory
      const NS::String* lib_path = NS::StringEx::asStringEx(resource)
                                       ->stringByAppendingPathComponent(ferrumName)
                                       ->stringByAppendingPathExtension(metalExt);
      if (fileManager->fileExistsAtPath(lib_path, &isDir)) {
        if (!isDir) {
          return lib_path;
        }
      }
    }
    return nullptr;
  }

  DBG("Looking in environment");
  NS::StringEx* resource_path = NS::StringEx::asStringEx(getEnv(FERRUM_LIB));
  if (resource_path != nullptr) {
    DBG("Resource path: ", str(resource_path));
    resource_path = NS::StringEx::asStringEx(resource_path)
                        ->stringByAppendingPathComponent(ferrumName)
                        ->stringByAppendingPathExtension(metalExt);
  } else {
    DBG("No resource path. Using ./lib");
    resource_path = NS::StringEx::asStringEx(nsStr("./lib"))
                        ->stringByAppendingPathComponent(ferrumName)
                        ->stringByAppendingPathExtension(metalExt);
    DBG("Default resource path: ", str(resource_path));
  }
  if (fileManager != nullptr && fileManager->fileExistsAtPath(resource_path, &isDir)) {
    DBG("Resource exists!");
    if (!isDir) {
      return resource_path;
    }
  }
  DBG("Failed!");
  return nullptr;
}


// Finds a Metal device. This is not straightforward as there is no default device when
// there is no UI.
MTL::Device* getDevice() {
  NS::Array* devices = MTL::CopyAllDevices();
  int length = devices != nullptr ? devices->count() : 0;
  if (length == 0) {
    std::cerr << "Metal is not supported on this device" << std::endl;
    return nullptr;
  }
  MTL::Device* device = devices->object<MTL::Device>(0);
  DBG("Running on device: ", device->name()->utf8String());
  return device;
}

extern "C" char binary_ferrum_bin_start[];
extern "C" unsigned long long binary_ferrum_bin_size;


// Loads the Metal library from the dylib
MTL::Library* loadFromDylib(MTL::Device* device) {
  size_t size = (size_t)binary_ferrum_bin_size;
  if (size == 0) {
    std::cerr << "Error: Failed to find library" << std::endl;
    return nullptr;
  }
  dispatch_data_t libraryData = dispatch_data_create(binary_ferrum_bin_start, size, nullptr, DISPATCH_DATA_DESTRUCTOR_DEFAULT);

  NS::Error* pError = nullptr;
  MTL::Library* library = device->newLibrary(libraryData, &pError);

  if (pError != nullptr) {
    std::cerr << "Bad data at: " << std::hex << binary_ferrum_bin_start << " to " << std::hex << (char*)binary_ferrum_bin_start + size << std::endl;
    std::cerr << "Error: Loading library: " << str(pError->localizedDescription()) << std::endl;
    return nullptr;
  } else if (library == nullptr) {
    std::cerr << "Error: Failed to create library" << std::endl;
    return nullptr;
  }

  return library;
}


MTL::Library* loadFromPath(MTL::Device* device, const char* path) {
  DBG("Getting library path...");
  const NS::String* libPath = getLibPath(path);
  if (libPath == nullptr) {
    std::cerr << "Error: Failed to find library" << std::endl;
    return nullptr;
  }
  DBG("Found: ", str(libPath));
  DBG("Creating URL...");
  NS::URL* url = NS::URL::fileURLWithPath(libPath);
  if (url == nullptr) {
    std::cerr << "Error: Failed to create URL for: " << str(libPath) << std::endl;
    return nullptr;
  }
  DBG("Creating library...");

  NS::Error* pError = nullptr;
  MTL::Library* library = device->newLibrary(url, &pError);

  if (pError != nullptr) {
    std::cerr << "Error: " << str(pError->localizedDescription()) << std::endl;
    return nullptr;
  } else if (library == nullptr) {
    std::cerr << "Error: Failed to create library from: " << url->fileSystemRepresentation() << std::endl;
    return nullptr;
  }
  DBG("Successfully loaded library: ", str(libPath));

  return library;
}

// Initializes a Metal library with a device, reading the library from a file
MTL::Library* initLibrary(MTL::Device* device, const char* path = nullptr) {
  MTL::Library* library;
  if (path == nullptr) {
    library = loadFromDylib(device);
  }
  if (library != nullptr) {
    DBG("Loaded Metal library from dylib");
    return library;
  } else {
    DBG("Loaded Metal library from path");
    return loadFromPath(device, path);
  }
}


template<typename CreateBuffers, typename SetBuffers, typename CopyResults>
float* Ferrum::MetalEngine::call_metal(Ferrum::FunctionID id,
                                       float* result, int len, int offset, int stride,
                                       CreateBuffers createBuffers, SetBuffers setBuffers,
                                       CopyResults copyResults) {
  MTL::ComputePipelineState* pipelineState = computePipelineStates[static_cast<int>(id)];
  if (pipelineState == nullptr) {
    std::cerr << "Error: Failed to find pipeline state for '" << id << "'" << std::endl;
    return nullptr;
  }

  auto buffers = createBuffers();

  for (auto& buffer : buffers) {
    if (buffer == nullptr) {
      std::cerr << "Error: Failed to create buffer" << std::endl;
      return nullptr;
    }
  } 

  MTL::CommandBuffer* commandBuffer = commandQueue->commandBuffer();
  if (commandBuffer == nullptr) {
    std::cerr << "Error: Failed to create command buffer" << std::endl;
    return nullptr;
  }

  MTL::ComputeCommandEncoder* encoder = commandBuffer->computeCommandEncoder();
  if (encoder == nullptr) {
    std::cerr << "Error: Failed to create command encoder" << std::endl;
    return nullptr;
  }

  encoder->setComputePipelineState(pipelineState);

  setBuffers(encoder, buffers);

  MTL::Size gridSize = MTL::Size(1, 1, 1);
  MTL::Size threadGroupSize = MTL::Size(len, 1, 1);

  encoder->dispatchThreadgroups(gridSize, threadGroupSize);

  encoder->endEncoding();
  commandBuffer->commit();
  commandBuffer->waitUntilCompleted();

  float* bresult = reinterpret_cast<float*>(buffers.back()->contents());
  memcpy(result, bresult, sizeof(float) * len);
  // bring over more buffers if there is more than one result
  copyResults(buffers, len);
  for (auto& buffer : buffers) {
    buffer->release();
  }
  return result;
}

// general vector functions
float* Ferrum::MetalEngine::vect_bB(Ferrum::FunctionID id, const float* a, int lena, int offset_a, int stride_a,
                                    float* result, int len, int offset, int stride) {
  return call_metal(id, result, len, offset, stride,
      [&]() {
        MTL::Buffer* bufferA = device->newBuffer(a, sizeof(float) * lena, MTL::StorageModeShared);
        MTL::Buffer* bufferR = device->newBuffer(sizeof(float) * len, MTL::StorageModeShared);
        return std::vector<MTL::Buffer*>{bufferA, bufferR};
      },
      [&](MTL::ComputeCommandEncoder* encoder, std::vector<MTL::Buffer*>& buffers) {
        encoder->setBuffer(buffers[0], 0, 0);
        encoder->setBytes(&offset_a, sizeof(offset_a), 1);
        encoder->setBytes(&stride_a, sizeof(stride_a), 2);
        encoder->setBuffer(buffers[1], 0, 3);
        encoder->setBytes(&offset, sizeof(offset), 4);
        encoder->setBytes(&stride, sizeof(stride), 5);
      },
      emptyAction);
}

float* Ferrum::MetalEngine::vect_bfB(Ferrum::FunctionID id, const float* a, int lena, int offset_a, int stride_a,
                                     float sa,
                                     float* result, int len, int offset, int stride) {
  return call_metal(id, result, len, offset, stride,
      [&]() {
        MTL::Buffer* bufferA = device->newBuffer(a, sizeof(float) * lena, MTL::StorageModeShared);
        MTL::Buffer* bufferR = device->newBuffer(sizeof(float) * len, MTL::StorageModeShared);
        return std::vector<MTL::Buffer*>{bufferA, bufferR};
      },
      [&](MTL::ComputeCommandEncoder* encoder, std::vector<MTL::Buffer*>& buffers) {
        encoder->setBuffer(buffers[0], 0, 0);
        encoder->setBytes(&offset_a, sizeof(offset_a), 1);
        encoder->setBytes(&stride_a, sizeof(stride_a), 2);
        encoder->setBytes(&sa, sizeof(sa), 3);
        encoder->setBuffer(buffers[1], 0, 4);
        encoder->setBytes(&offset, sizeof(offset), 5);
        encoder->setBytes(&stride, sizeof(stride), 6);
      },
      emptyAction);
}

float* Ferrum::MetalEngine::vect_fbB(Ferrum::FunctionID id, float sa,
                                     const float* a, int lena, int offset_a, int stride_a,
                                     float* result, int len, int offset, int stride) {
  return call_metal(id, result, len, offset, stride,
      [&]() {
        MTL::Buffer* bufferA = device->newBuffer(a, sizeof(float) * lena, MTL::StorageModeShared);
        MTL::Buffer* bufferR = device->newBuffer(sizeof(float) * len, MTL::StorageModeShared);
        return std::vector<MTL::Buffer*>{bufferA, bufferR};
      },
      [&](MTL::ComputeCommandEncoder* encoder, std::vector<MTL::Buffer*>& buffers) {
        encoder->setBytes(&sa, sizeof(sa), 0);
        encoder->setBuffer(buffers[0], 0, 1);
        encoder->setBytes(&offset_a, sizeof(offset_a), 2);
        encoder->setBytes(&stride_a, sizeof(stride_a), 3);
        encoder->setBuffer(buffers[1], 0, 4);
        encoder->setBytes(&offset, sizeof(offset), 5);
        encoder->setBytes(&stride, sizeof(stride), 6);
      },
      emptyAction);
}

float* Ferrum::MetalEngine::vect_bbB(Ferrum::FunctionID id, const float* a, int lena, int offset_a, int stride_a,
                                     const float* b, int lenb, int offset_b, int stride_b,
                                     float* result, int len, int offset, int stride) {
  return call_metal(id, result, len, offset, stride,
      [&]() {
        MTL::Buffer* bufferA = device->newBuffer(a, sizeof(float) * lena, MTL::StorageModeShared);
        MTL::Buffer* bufferB = device->newBuffer(b, sizeof(float) * lena, MTL::StorageModeShared);
        MTL::Buffer* bufferR = device->newBuffer(sizeof(float) * len, MTL::StorageModeShared);
        return std::vector<MTL::Buffer*>{bufferA, bufferB, bufferR};
      },
      [&](MTL::ComputeCommandEncoder* encoder, std::vector<MTL::Buffer*>& buffers) {
        encoder->setBuffer(buffers[0], 0, 0);
        encoder->setBytes(&offset_a, sizeof(offset_a), 1);
        encoder->setBytes(&stride_a, sizeof(stride_a), 2);
        encoder->setBuffer(buffers[1], 0, 3);
        encoder->setBytes(&offset_b, sizeof(offset_b), 4);
        encoder->setBytes(&stride_b, sizeof(stride_b), 5);
        encoder->setBuffer(buffers[2], 0, 6);
        encoder->setBytes(&offset, sizeof(offset), 7);
        encoder->setBytes(&stride, sizeof(stride), 8);
      },
      emptyAction);
}

float* Ferrum::MetalEngine::vect_bBB(Ferrum::FunctionID id, const float* a, int lena, int offset_a, int stride_a,
                                     float* b, int lenb, int offset_b, int stride_b,
                                     float* result, int len, int offset, int stride) {
  return call_metal(id, result, len, offset, stride,
      [&]() {
        MTL::Buffer* bufferA = device->newBuffer(a, sizeof(float) * lena, MTL::StorageModeShared);
        MTL::Buffer* bufferB = device->newBuffer(b, sizeof(float) * lena, MTL::StorageModeShared);
        MTL::Buffer* bufferR = device->newBuffer(sizeof(float) * len, MTL::StorageModeShared);
        return std::vector<MTL::Buffer*>{bufferA, bufferB, bufferR};
      },
      [&](MTL::ComputeCommandEncoder* encoder, std::vector<MTL::Buffer*>& buffers) {
        encoder->setBuffer(buffers[0], 0, 0);
        encoder->setBytes(&offset_a, sizeof(offset_a), 1);
        encoder->setBytes(&stride_a, sizeof(stride_a), 2);
        encoder->setBuffer(buffers[1], 0, 3);
        encoder->setBytes(&offset_b, sizeof(offset_b), 4);
        encoder->setBytes(&stride_b, sizeof(stride_b), 5);
        encoder->setBuffer(buffers[2], 0, 6);
        encoder->setBytes(&offset, sizeof(offset), 7);
        encoder->setBytes(&stride, sizeof(stride), 8);
      },
      [&](std::vector<MTL::Buffer*>& buffers, int len) {
        float* b_result = reinterpret_cast<float*>(buffers[1]->contents());
        memcpy(b, b_result, sizeof(float) * len);
      });
}

float* Ferrum::MetalEngine::vect_bffffB(Ferrum::FunctionID id, const float* a, int lena, int offset_a, int stride_a,
                                        float sa, float sha,
                                        float sb, float shb,
                                        float* result, int len, int offset, int stride) {
  return call_metal(id, result, len, offset, stride,
      [&]() {
        MTL::Buffer* bufferA = device->newBuffer(a, sizeof(float) * lena, MTL::StorageModeShared);
        MTL::Buffer* bufferR = device->newBuffer(sizeof(float) * len, MTL::StorageModeShared);
        return std::vector<MTL::Buffer*>{bufferA, bufferR};
      },
      [&](MTL::ComputeCommandEncoder* encoder, std::vector<MTL::Buffer*>& buffers) {
        encoder->setBuffer(buffers[0], 0, 0);
        encoder->setBytes(&offset_a, sizeof(offset_a), 1);
        encoder->setBytes(&stride_a, sizeof(stride_a), 2);
        encoder->setBytes(&sa, sizeof(sa), 3);
        encoder->setBytes(&sha, sizeof(sha), 4);
        encoder->setBytes(&sb, sizeof(sb), 5);
        encoder->setBytes(&shb, sizeof(shb), 6);
        encoder->setBuffer(buffers[1], 0, 7);
        encoder->setBytes(&offset, sizeof(offset), 8);
        encoder->setBytes(&stride, sizeof(stride), 9);
      },
      emptyAction);
}

float* Ferrum::MetalEngine::vect_bbffffB(Ferrum::FunctionID id, const float* a, int lena, int offset_a, int stride_a,
                                         const float* b, int lenb, int offset_b, int stride_b,
                                         float sa, float sha,
                                         float sb, float shb,
                                         float* result, int len, int offset, int stride) {
  return call_metal(id, result, len, offset, stride,
      [&]() {
        MTL::Buffer* bufferA = device->newBuffer(a, sizeof(float) * lena, MTL::StorageModeShared);
        MTL::Buffer* bufferB = device->newBuffer(b, sizeof(float) * lena, MTL::StorageModeShared);
        MTL::Buffer* bufferR = device->newBuffer(sizeof(float) * len, MTL::StorageModeShared);
        return std::vector<MTL::Buffer*>{bufferA, bufferB, bufferR};
      },
      [&](MTL::ComputeCommandEncoder* encoder, std::vector<MTL::Buffer*>& buffers) {
        encoder->setBuffer(buffers[0], 0, 0);
        encoder->setBytes(&offset_a, sizeof(offset_a), 1);
        encoder->setBytes(&stride_a, sizeof(stride_a), 2);
        encoder->setBuffer(buffers[1], 0, 3);
        encoder->setBytes(&offset_b, sizeof(offset_b), 4);
        encoder->setBytes(&stride_b, sizeof(stride_b), 5);
        encoder->setBytes(&sa, sizeof(sa), 6);
        encoder->setBytes(&sha, sizeof(sha), 7);
        encoder->setBytes(&sb, sizeof(sb), 8);
        encoder->setBytes(&shb, sizeof(shb), 9);
        encoder->setBuffer(buffers[2], 0, 10);
        encoder->setBytes(&offset, sizeof(offset), 11);
        encoder->setBytes(&stride, sizeof(stride), 12);
      },
      emptyAction);
}

// general matrix functions
float* Ferrum::MetalEngine::ge_bB(Ferrum::FunctionID id, int sd, int fd,
                                  const float* a, int lena, int offset_a, int stride_a,
                                  float* result, int len, int offset, int stride) {
  return call_metal(id, result, len, offset, stride,
      [&]() {
        MTL::Buffer* bufferA = device->newBuffer(a, sizeof(float) * lena, MTL::StorageModeShared);
        MTL::Buffer* bufferR = device->newBuffer(sizeof(float) * len, MTL::StorageModeShared);
        return std::vector<MTL::Buffer*>{bufferA, bufferR};
      },
      [&](MTL::ComputeCommandEncoder* encoder, std::vector<MTL::Buffer*>& buffers) {
        encoder->setBytes(&sd, sizeof(sd), 0);
        encoder->setBytes(&fd, sizeof(fd), 1);
        encoder->setBuffer(buffers[0], 0, 2);
        encoder->setBytes(&offset_a, sizeof(offset_a), 3);
        encoder->setBytes(&stride_a, sizeof(stride_a), 4);
        encoder->setBuffer(buffers[1], 0, 5);
        encoder->setBytes(&offset, sizeof(offset), 6);
        encoder->setBytes(&stride, sizeof(stride), 7);
      },
      emptyAction);
}

float* Ferrum::MetalEngine::ge_bfB(Ferrum::FunctionID id, int sd, int fd,
                                   const float* a, int lena, int offset_a, int stride_a,
                                   float sa,
                                   float* result, int len, int offset, int stride) {
  return call_metal(id, result, len, offset, stride,
      [&]() {
        MTL::Buffer* bufferA = device->newBuffer(a, sizeof(float) * lena, MTL::StorageModeShared);
        MTL::Buffer* bufferR = device->newBuffer(sizeof(float) * len, MTL::StorageModeShared);
        return std::vector<MTL::Buffer*>{bufferA, bufferR};
      },
      [&](MTL::ComputeCommandEncoder* encoder, std::vector<MTL::Buffer*>& buffers) {
        encoder->setBytes(&sd, sizeof(sd), 0);
        encoder->setBytes(&fd, sizeof(fd), 1);
        encoder->setBuffer(buffers[0], 0, 2);
        encoder->setBytes(&offset_a, sizeof(offset_a), 3);
        encoder->setBytes(&stride_a, sizeof(stride_a), 4);
        encoder->setBytes(&sa, sizeof(sa), 5);
        encoder->setBuffer(buffers[1], 0, 6);
        encoder->setBytes(&offset, sizeof(offset), 7);
        encoder->setBytes(&stride, sizeof(stride), 8);
      },
      emptyAction);
}

float* Ferrum::MetalEngine::ge_fbB(Ferrum::FunctionID id, int sd, int fd, float sa,
                                   const float* a, int lena, int offset_a, int stride_a,
                                   float* result, int len, int offset, int stride) {
  return call_metal(id, result, len, offset, stride,
      [&]() {
        MTL::Buffer* bufferA = device->newBuffer(a, sizeof(float) * lena, MTL::StorageModeShared);
        MTL::Buffer* bufferR = device->newBuffer(sizeof(float) * len, MTL::StorageModeShared);
        return std::vector<MTL::Buffer*>{bufferA, bufferR};
      },
      [&](MTL::ComputeCommandEncoder* encoder, std::vector<MTL::Buffer*>& buffers) {
        encoder->setBytes(&sd, sizeof(sd), 0);
        encoder->setBytes(&fd, sizeof(fd), 1);
        encoder->setBytes(&sa, sizeof(sa), 2);
        encoder->setBuffer(buffers[0], 0, 3);
        encoder->setBytes(&offset_a, sizeof(offset_a), 4);
        encoder->setBytes(&stride_a, sizeof(stride_a), 5);
        encoder->setBuffer(buffers[1], 0, 6);
        encoder->setBytes(&offset, sizeof(offset), 7);
        encoder->setBytes(&stride, sizeof(stride), 8);
      },
      emptyAction);
}

float* Ferrum::MetalEngine::ge_bbB(Ferrum::FunctionID id, int sd, int fd,
                                   const float* a, int lena, int offset_a, int stride_a,
                                   const float* b, int lenb, int offset_b, int stride_b,
                                   float* result, int len, int offset, int stride) {
  return call_metal(id, result, len, offset, stride,
      [&]() {
        MTL::Buffer* bufferA = device->newBuffer(a, sizeof(float) * lena, MTL::StorageModeShared);
        MTL::Buffer* bufferB = device->newBuffer(b, sizeof(float) * lena, MTL::StorageModeShared);
        MTL::Buffer* bufferR = device->newBuffer(sizeof(float) * len, MTL::StorageModeShared);
        return std::vector<MTL::Buffer*>{bufferA, bufferB, bufferR};
      },
      [&](MTL::ComputeCommandEncoder* encoder, std::vector<MTL::Buffer*>& buffers) {
        encoder->setBytes(&sd, sizeof(sd), 0);
        encoder->setBytes(&fd, sizeof(fd), 1);
        encoder->setBuffer(buffers[0], 0, 2);
        encoder->setBytes(&offset_a, sizeof(offset_a), 3);
        encoder->setBytes(&stride_a, sizeof(stride_a), 4);
        encoder->setBuffer(buffers[1], 0, 5);
        encoder->setBytes(&offset_b, sizeof(offset_b), 6);
        encoder->setBytes(&stride_b, sizeof(stride_b), 7);
        encoder->setBuffer(buffers[2], 0, 8);
        encoder->setBytes(&offset, sizeof(offset), 9);
        encoder->setBytes(&stride, sizeof(stride), 10);
      },
      emptyAction);
}

float* Ferrum::MetalEngine::ge_bBB(Ferrum::FunctionID id, int sd, int fd,
                                   const float* a, int lena, int offset_a, int stride_a,
                                   float* b, int lenb, int offset_b, int stride_b,
                                   float* result, int len, int offset, int stride) {
  return call_metal(id, result, len, offset, stride,
      [&]() {
        MTL::Buffer* bufferA = device->newBuffer(a, sizeof(float) * lena, MTL::StorageModeShared);
        MTL::Buffer* bufferB = device->newBuffer(b, sizeof(float) * lena, MTL::StorageModeShared);
        MTL::Buffer* bufferR = device->newBuffer(sizeof(float) * len, MTL::StorageModeShared);
        return std::vector<MTL::Buffer*>{bufferA, bufferB, bufferR};
      },
      [&](MTL::ComputeCommandEncoder* encoder, std::vector<MTL::Buffer*>& buffers) {
        encoder->setBytes(&sd, sizeof(sd), 0);
        encoder->setBytes(&fd, sizeof(fd), 1);
        encoder->setBuffer(buffers[0], 0, 2);
        encoder->setBytes(&offset_a, sizeof(offset_a), 3);
        encoder->setBytes(&stride_a, sizeof(stride_a), 4);
        encoder->setBuffer(buffers[1], 0, 5);
        encoder->setBytes(&offset_b, sizeof(offset_b), 6);
        encoder->setBytes(&stride_b, sizeof(stride_b), 7);
        encoder->setBuffer(buffers[2], 0, 8);
        encoder->setBytes(&offset, sizeof(offset), 9);
        encoder->setBytes(&stride, sizeof(stride), 10);
      },
      [&](std::vector<MTL::Buffer*>& buffers, int len) {
        float* b_result = reinterpret_cast<float*>(buffers[1]->contents());
        memcpy(b, b_result, sizeof(float) * len);
      });
}

float* Ferrum::MetalEngine::ge_bffffB(Ferrum::FunctionID id, int sd, int fd,
                                      const float* a, int lena, int offset_a, int stride_a,
                                      float sa, float sha,
                                      float sb, float shb,
                                      float* result, int len, int offset, int stride) {
  return call_metal(id, result, len, offset, stride,
      [&]() {
        MTL::Buffer* bufferA = device->newBuffer(a, sizeof(float) * lena, MTL::StorageModeShared);
        MTL::Buffer* bufferR = device->newBuffer(sizeof(float) * len, MTL::StorageModeShared);
        return std::vector<MTL::Buffer*>{bufferA, bufferR};
      },
      [&](MTL::ComputeCommandEncoder* encoder, std::vector<MTL::Buffer*>& buffers) {
        encoder->setBytes(&sd, sizeof(sd), 0);
        encoder->setBytes(&fd, sizeof(fd), 1);
        encoder->setBuffer(buffers[0], 0, 2);
        encoder->setBytes(&offset_a, sizeof(offset_a), 3);
        encoder->setBytes(&stride_a, sizeof(stride_a), 4);
        encoder->setBytes(&sa, sizeof(sa), 5);
        encoder->setBytes(&sha, sizeof(sha), 6);
        encoder->setBytes(&sb, sizeof(sb), 7);
        encoder->setBytes(&shb, sizeof(shb), 8);
        encoder->setBuffer(buffers[1], 0, 9);
        encoder->setBytes(&offset, sizeof(offset), 10);
        encoder->setBytes(&stride, sizeof(stride), 11);
      },
      emptyAction);
}

float* Ferrum::MetalEngine::ge_bbffffB(Ferrum::FunctionID id, int sd, int fd,
                                       const float* a, int lena, int offset_a, int stride_a,
                                       const float* b, int lenb, int offset_b, int stride_b,
                                       float sa, float sha,
                                       float sb, float shb,
                                       float* result, int len, int offset, int stride) {
  return call_metal(id, result, len, offset, stride,
      [&]() {
        MTL::Buffer* bufferA = device->newBuffer(a, sizeof(float) * lena, MTL::StorageModeShared);
        MTL::Buffer* bufferB = device->newBuffer(b, sizeof(float) * lena, MTL::StorageModeShared);
        MTL::Buffer* bufferR = device->newBuffer(sizeof(float) * len, MTL::StorageModeShared);
        return std::vector<MTL::Buffer*>{bufferA, bufferB, bufferR};
      },
      [&](MTL::ComputeCommandEncoder* encoder, std::vector<MTL::Buffer*>& buffers) {
        encoder->setBytes(&sd, sizeof(sd), 0);
        encoder->setBytes(&fd, sizeof(fd), 1);
        encoder->setBuffer(buffers[0], 0, 2);
        encoder->setBytes(&offset_a, sizeof(offset_a), 3);
        encoder->setBytes(&stride_a, sizeof(stride_a), 4);
        encoder->setBuffer(buffers[1], 0, 5);
        encoder->setBytes(&offset_b, sizeof(offset_b), 6);
        encoder->setBytes(&stride_b, sizeof(stride_b), 7);
        encoder->setBytes(&sa, sizeof(sa), 8);
        encoder->setBytes(&sha, sizeof(sha), 9);
        encoder->setBytes(&sb, sizeof(sb), 10);
        encoder->setBytes(&shb, sizeof(shb), 11);
        encoder->setBuffer(buffers[2], 0, 12);
        encoder->setBytes(&offset, sizeof(offset), 13);
        encoder->setBytes(&stride, sizeof(stride), 14);
      },
      emptyAction);
}

// general uplo functions
float* Ferrum::MetalEngine::uplo_bB(Ferrum::FunctionID id, int sd, int unit, int bottom,
                                    const float* a, int lena, int offset_a, int stride_a,
                                    float* result, int len, int offset, int stride) {
  return call_metal(id, result, len, offset, stride,
      [&]() {
        MTL::Buffer* bufferA = device->newBuffer(a, sizeof(float) * lena, MTL::StorageModeShared);
        MTL::Buffer* bufferR = device->newBuffer(sizeof(float) * len, MTL::StorageModeShared);
        return std::vector<MTL::Buffer*>{bufferA, bufferR};
      },
      [&](MTL::ComputeCommandEncoder* encoder, std::vector<MTL::Buffer*>& buffers) {
        encoder->setBytes(&sd, sizeof(sd), 0);
        encoder->setBytes(&unit, sizeof(unit), 1);
        encoder->setBytes(&bottom, sizeof(bottom), 2);
        encoder->setBuffer(buffers[0], 0, 3);
        encoder->setBytes(&offset_a, sizeof(offset_a), 4);
        encoder->setBytes(&stride_a, sizeof(stride_a), 5);
        encoder->setBuffer(buffers[1], 0, 6);
        encoder->setBytes(&offset, sizeof(offset), 7);
        encoder->setBytes(&stride, sizeof(stride), 8);
      },
      emptyAction);
}

float* Ferrum::MetalEngine::uplo_bfB(Ferrum::FunctionID id, int sd, int unit, int bottom,
                                     const float* a, int lena, int offset_a, int stride_a,
                                     float sa,
                                     float* result, int len, int offset, int stride) {
  return call_metal(id, result, len, offset, stride,
      [&]() {
        MTL::Buffer* bufferA = device->newBuffer(a, sizeof(float) * lena, MTL::StorageModeShared);
        MTL::Buffer* bufferR = device->newBuffer(sizeof(float) * len, MTL::StorageModeShared);
        return std::vector<MTL::Buffer*>{bufferA, bufferR};
      },
      [&](MTL::ComputeCommandEncoder* encoder, std::vector<MTL::Buffer*>& buffers) {
        encoder->setBytes(&sd, sizeof(sd), 0);
        encoder->setBytes(&unit, sizeof(unit), 1);
        encoder->setBytes(&bottom, sizeof(bottom), 2);
        encoder->setBuffer(buffers[0], 0, 3);
        encoder->setBytes(&offset_a, sizeof(offset_a), 4);
        encoder->setBytes(&stride_a, sizeof(stride_a), 5);
        encoder->setBytes(&sa, sizeof(sa), 6);
        encoder->setBuffer(buffers[1], 0, 7);
        encoder->setBytes(&offset, sizeof(offset), 8);
        encoder->setBytes(&stride, sizeof(stride), 9);
      },
      emptyAction);
}

float* Ferrum::MetalEngine::uplo_fbB(Ferrum::FunctionID id, int sd, int unit, int bottom,
                                     const float* a, int lena, int offset_a, int stride_a,
				     float sa,
                                     float* result, int len, int offset, int stride) {
  return call_metal(id, result, len, offset, stride,
      [&]() {
        MTL::Buffer* bufferA = device->newBuffer(a, sizeof(float) * lena, MTL::StorageModeShared);
        MTL::Buffer* bufferR = device->newBuffer(sizeof(float) * len, MTL::StorageModeShared);
        return std::vector<MTL::Buffer*>{bufferA, bufferR};
      },
      [&](MTL::ComputeCommandEncoder* encoder, std::vector<MTL::Buffer*>& buffers) {
        encoder->setBytes(&sd, sizeof(sd), 0);
        encoder->setBytes(&unit, sizeof(unit), 1);
        encoder->setBytes(&bottom, sizeof(bottom), 2);
        encoder->setBuffer(buffers[0], 0, 3);
        encoder->setBytes(&offset_a, sizeof(offset_a), 4);
        encoder->setBytes(&stride_a, sizeof(stride_a), 5);
        encoder->setBytes(&sa, sizeof(sa), 6);
        encoder->setBuffer(buffers[1], 0, 7);
        encoder->setBytes(&offset, sizeof(offset), 8);
        encoder->setBytes(&stride, sizeof(stride), 9);
      },
      emptyAction);
}

float* Ferrum::MetalEngine::uplo_bbB(Ferrum::FunctionID id, int sd, int unit, int bottom,
                                     const float* a, int lena, int offset_a, int stride_a,
                                     const float* b, int lenb, int offset_b, int stride_b,
                                     float* result, int len, int offset, int stride) {
  return call_metal(id, result, len, offset, stride,
      [&]() {
        MTL::Buffer* bufferA = device->newBuffer(a, sizeof(float) * lena, MTL::StorageModeShared);
        MTL::Buffer* bufferB = device->newBuffer(b, sizeof(float) * lena, MTL::StorageModeShared);
        MTL::Buffer* bufferR = device->newBuffer(sizeof(float) * len, MTL::StorageModeShared);
        return std::vector<MTL::Buffer*>{bufferA, bufferB, bufferR};
      },
      [&](MTL::ComputeCommandEncoder* encoder, std::vector<MTL::Buffer*>& buffers) {
        encoder->setBytes(&sd, sizeof(sd), 0);
        encoder->setBytes(&unit, sizeof(unit), 1);
        encoder->setBytes(&bottom, sizeof(bottom), 2);
        encoder->setBuffer(buffers[0], 0, 3);
        encoder->setBytes(&offset_a, sizeof(offset_a), 4);
        encoder->setBytes(&stride_a, sizeof(stride_a), 5);
        encoder->setBuffer(buffers[1], 0, 6);
        encoder->setBytes(&offset_b, sizeof(offset_b), 7);
        encoder->setBytes(&stride_b, sizeof(stride_b), 8);
        encoder->setBuffer(buffers[2], 0, 9);
        encoder->setBytes(&offset, sizeof(offset), 10);
        encoder->setBytes(&stride, sizeof(stride), 11);
      },
      emptyAction);
}

float* Ferrum::MetalEngine::uplo_bBB(Ferrum::FunctionID id, int sd, int unit, int bottom,
                                     const float* a, int lena, int offset_a, int stride_a,
                                     float* b, int lenb, int offset_b, int stride_b,
                                     float* result, int len, int offset, int stride) {
  return call_metal(id, result, len, offset, stride,
      [&]() {
        MTL::Buffer* bufferA = device->newBuffer(a, sizeof(float) * lena, MTL::StorageModeShared);
        MTL::Buffer* bufferB = device->newBuffer(b, sizeof(float) * lena, MTL::StorageModeShared);
        MTL::Buffer* bufferR = device->newBuffer(sizeof(float) * len, MTL::StorageModeShared);
        return std::vector<MTL::Buffer*>{bufferA, bufferB, bufferR};
      },
      [&](MTL::ComputeCommandEncoder* encoder, std::vector<MTL::Buffer*>& buffers) {
        encoder->setBytes(&sd, sizeof(sd), 0);
        encoder->setBytes(&unit, sizeof(unit), 1);
        encoder->setBytes(&bottom, sizeof(bottom), 2);
        encoder->setBuffer(buffers[0], 0, 3);
        encoder->setBytes(&offset_a, sizeof(offset_a), 4);
        encoder->setBytes(&stride_a, sizeof(stride_a), 5);
        encoder->setBuffer(buffers[1], 0, 6);
        encoder->setBytes(&offset_b, sizeof(offset_b), 7);
        encoder->setBytes(&stride_b, sizeof(stride_b), 8);
        encoder->setBuffer(buffers[2], 0, 9);
        encoder->setBytes(&offset, sizeof(offset), 10);
        encoder->setBytes(&stride, sizeof(stride), 11);
      },
      [&](std::vector<MTL::Buffer*>& buffers, int len) {
        float* b_result = reinterpret_cast<float*>(buffers[1]->contents());
        memcpy(b, b_result, sizeof(float) * len);
      });
}

float* Ferrum::MetalEngine::uplo_bffffB(Ferrum::FunctionID id, int sd, int unit, int bottom,
                                        const float* a, int lena, int offset_a, int stride_a,
                                        float sa, float sha,
                                        float sb, float shb,
                                        float* result, int len, int offset, int stride) {
  return call_metal(id, result, len, offset, stride,
      [&]() {
        MTL::Buffer* bufferA = device->newBuffer(a, sizeof(float) * lena, MTL::StorageModeShared);
        MTL::Buffer* bufferR = device->newBuffer(sizeof(float) * len, MTL::StorageModeShared);
        return std::vector<MTL::Buffer*>{bufferA, bufferR};
      },
      [&](MTL::ComputeCommandEncoder* encoder, std::vector<MTL::Buffer*>& buffers) {
        encoder->setBytes(&sd, sizeof(sd), 0);
        encoder->setBytes(&unit, sizeof(unit), 1);
        encoder->setBytes(&bottom, sizeof(bottom), 2);
        encoder->setBuffer(buffers[0], 0, 3);
        encoder->setBytes(&offset_a, sizeof(offset_a), 4);
        encoder->setBytes(&stride_a, sizeof(stride_a), 5);
        encoder->setBytes(&sa, sizeof(sa), 6);
        encoder->setBytes(&sha, sizeof(sha), 7);
        encoder->setBytes(&sb, sizeof(sb), 8);
        encoder->setBytes(&shb, sizeof(shb), 9);
        encoder->setBuffer(buffers[1], 0, 10);
        encoder->setBytes(&offset, sizeof(offset), 11);
        encoder->setBytes(&stride, sizeof(stride), 12);
      },
      emptyAction);
}

float* Ferrum::MetalEngine::uplo_bbffffB(Ferrum::FunctionID id, int sd, int unit, int bottom,
                                         const float* a, int lena, int offset_a, int stride_a,
                                         const float* b, int lenb, int offset_b, int stride_b,
                                         float sa, float sha,
                                         float sb, float shb,
                                         float* result, int len, int offset, int stride) {
  return call_metal(id, result, len, offset, stride,
      [&]() {
        MTL::Buffer* bufferA = device->newBuffer(a, sizeof(float) * lena, MTL::StorageModeShared);
        MTL::Buffer* bufferB = device->newBuffer(b, sizeof(float) * lena, MTL::StorageModeShared);
        MTL::Buffer* bufferR = device->newBuffer(sizeof(float) * len, MTL::StorageModeShared);
        return std::vector<MTL::Buffer*>{bufferA, bufferB, bufferR};
      },
      [&](MTL::ComputeCommandEncoder* encoder, std::vector<MTL::Buffer*>& buffers) {
        encoder->setBytes(&sd, sizeof(sd), 0);
        encoder->setBytes(&unit, sizeof(unit), 1);
        encoder->setBytes(&bottom, sizeof(bottom), 2);
        encoder->setBuffer(buffers[0], 0, 3);
        encoder->setBytes(&offset_a, sizeof(offset_a), 4);
        encoder->setBytes(&stride_a, sizeof(stride_a), 5);
        encoder->setBuffer(buffers[1], 0, 6);
        encoder->setBytes(&offset_b, sizeof(offset_b), 7);
        encoder->setBytes(&stride_b, sizeof(stride_b), 8);
        encoder->setBytes(&sa, sizeof(sa), 9);
        encoder->setBytes(&sha, sizeof(sha), 10);
        encoder->setBytes(&sb, sizeof(sb), 11);
        encoder->setBytes(&shb, sizeof(shb), 12);
        encoder->setBuffer(buffers[2], 0, 13);
        encoder->setBytes(&offset, sizeof(offset), 14);
        encoder->setBytes(&stride, sizeof(stride), 15);
      },
      emptyAction);
}


