#include <string>

#include "openvr/test/fnproxy.h"

#include <windows.h>

Mutex::Mutex(const char *name) {
  h = CreateMutex(
    NULL,
    false,
    name
  );
  if (!h) {
    getOut() << "mutex error " << GetLastError() << std::endl;
    // abort();
  }
}
Mutex::~Mutex() {
  // CloseHandle(h);
}
void Mutex::lock() {
  // getOut() << "mutex lock 1" << std::endl;
  auto r = WaitForSingleObject(
    h,
    INFINITE
  );
  // getOut() << "mutex lock 2 " << r << " " << GetLastError() << std::endl;
}
void Mutex::unlock() {
  ReleaseMutex(
    h
  );
}
bool Mutex::tryLock() {
  auto r = WaitForSingleObject(
    h,
    0
  );
  return r == WAIT_OBJECT_0;
}

Semaphore::Semaphore(const char *name) {
  h = CreateSemaphore(
    NULL,
    0,
    1024,
    name
  );
  if (!h) {
    getOut() << "semaphore error " << GetLastError() << std::endl;
    // abort();
  }
}
Semaphore::~Semaphore() {
  // CloseHandle(h);
}
void Semaphore::lock() {
  // getOut() << "sempaphore lock 1" << std::endl;
  auto r = WaitForSingleObject(
    h,
    INFINITE
  );
  // getOut() << "sempaphore lock 2 " << r << " " << GetLastError() << std::endl;
}
void Semaphore::unlock() {
  ReleaseSemaphore(
    h,
    1,
    NULL
  );
}
bool Semaphore::tryLock() {
  // getOut() << "sempaphore lock 1" << std::endl;
  auto r = WaitForSingleObject(
    h,
    0
  );
  return r == WAIT_OBJECT_0;
  // getOut() << "sempaphore lock 2 " << r << " " << GetLastError() << std::endl;
}