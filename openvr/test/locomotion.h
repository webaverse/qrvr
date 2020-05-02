#ifndef _test_locomotion_h_
#define _test_locomotion_h_

#include <stdlib.h>
#include <locale>
#include <codecvt>

#include <nan.h>
#include "openvr.h"

// #include <windows.h>
#include <wrl.h>
#include <d3d11_4.h>

// #include "device/vr/openvr/test/out.h"
// #include "device/vr/openvr/test/compositorproxy.h"

// #include <opencv2/core/core.hpp>
// #include <opencv2/objdetect.hpp>
// #include <opencv2/imgcodecs.hpp>
// #include <opencv2/imgproc.hpp>
// #include <opencv2/features2d.hpp>

#include "./fnproxy.h"

// using namespace v8;

class LocomotionEngine {
public:
  vr::VRActionSetHandle_t pActionSetHandle;
  vr::VRActionHandle_t pActionJoy1Press;
  vr::VRActionHandle_t pActionJoy1Touch;
  vr::VRActionHandle_t pActionJoy1Axis;
  vr::InputAnalogActionData_t pInputJoy1Axis;
  vr::InputDigitalActionData_t pInputJoy1Press;
  vr::InputDigitalActionData_t pInputJoy1Touch;
  bool sceneAppLocomotionEnabled = true;

  Nan::Persistent<v8::Function> fn;
  uv_async_t locomotionAsync;

  Mutex mut;
  Semaphore sem;

public:
  LocomotionEngine(v8::Local<v8::Function> fn);
  void tick();
  static void MainThreadAsync(uv_async_t *handle);
};

#endif