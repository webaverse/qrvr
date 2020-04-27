#ifndef _openvr_test_fnproxy_h_
#define _openvr_test_fnproxy_h_

#include <map>
// #include <mutex>
// #include <semaphore>
#include <functional>

// #include <D3D11_1.h>
// #include <DXGI1_4.h>

#include "device/vr/openvr/test/out.h"
// #include "third_party/openvr/src/headers/openvr.h"
#include "device/vr/openvr/test/serializer.h"

typedef void *HANDLE;

class Mutex {
public:
  HANDLE h;
  Mutex(const char *name = nullptr);
  ~Mutex();
  void lock();
  void unlock();
  bool tryLock();
};

class Semaphore {
public:
  HANDLE h;
  Semaphore(const char *name = nullptr);
  ~Semaphore();
  void lock();
  void unlock();
  bool tryLock();
};

void *allocateShared(const char *szName, size_t s);

extern Mutex checkMutex;
class checkTids {
public:
  checkTids(Mutex &m);
  ~checkTids();
};

#endif