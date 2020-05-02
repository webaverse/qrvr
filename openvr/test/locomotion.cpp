#include "locomotion.h"
#include "matrix.h"

#include <cmath>
#define M_PI 3.14159265358979323846264338327950288

using namespace v8;

LocomotionEngine::LocomotionEngine(Local<Function> fn) :
  fn(fn)
{
  char dir[MAX_PATH];
  GetCurrentDirectory(sizeof(dir), dir);
  std::string dirString(dir);
  dirString += "\\openvr\\test\\actions.json";
  vr::EVRInputError err = vr::VRInput()->SetActionManifestPath(dirString.c_str());

  err = vr::VRInput()->GetActionSetHandle("/actions/main", &pActionSetHandle);
  err = vr::VRInput()->GetActionHandle("/actions/main/in/Joy1Press", &pActionJoy1Press);
  err = vr::VRInput()->GetActionHandle("/actions/main/in/Joy1Touch", &pActionJoy1Touch);
  err = vr::VRInput()->GetActionHandle("/actions/main/in/Joy1Axis", &pActionJoy1Axis);
  
  uv_loop_t *loop = node::GetCurrentEventLoop(Isolate::GetCurrent());
  uv_async_init(loop, &locomotionAsync, MainThreadAsync);
  locomotionAsync.data = this;
}
void LocomotionEngine::tick() {
  {
    vr::VRActiveActionSet_t actionSet{};
    actionSet.ulActionSet = pActionSetHandle;
    actionSet.nPriority = vr::k_nActionSetOverlayGlobalPriorityMax;
    vr::EVRInputError err = vr::VRInput()->UpdateActionState(&actionSet, sizeof(actionSet), 1);

    err = vr::VRInput()->GetAnalogActionData(pActionJoy1Axis, &pInputJoy1Axis, sizeof(pInputJoy1Axis), vr::k_ulInvalidInputValueHandle);
    err = vr::VRInput()->GetDigitalActionData(pActionJoy1Press, &pInputJoy1Press, sizeof(pInputJoy1Press), vr::k_ulInvalidInputValueHandle);
    err = vr::VRInput()->GetDigitalActionData(pActionJoy1Touch, &pInputJoy1Touch, sizeof(pInputJoy1Touch ), vr::k_ulInvalidInputValueHandle);
  }
  {      
    std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()
    );
    
    float v = 5.0f * (1.0f + std::sin((float)(ms.count() % 2000) / 2000.0f * (float)M_PI * 2.0f) / 2.0f);
    // d += 0.1;
    float m[16] = {
      1, 0, 0, 0,
      0, 1, 0, 0,
      0, 0, 1, 0,
      0, -v, 0, 1,
    };
    vr::HmdMatrix34_t m2;
    setPoseMatrix(m2, m);
    vr::VRChaperoneSetup()->SetWorkingStandingZeroPoseToRawTrackingPose(&m2);
    vr::VRChaperoneSetup()->ShowWorkingSetPreview();
  }
  uv_async_send(&locomotionAsync);
}
void LocomotionEngine::MainThreadAsync(uv_async_t *handle) {
  LocomotionEngine *self = (LocomotionEngine *)handle->data;

  std::lock_guard<Mutex> lock(self->mut);
  Nan::HandleScope scope;

  Local<Function> localFn = Nan::New(self->fn);
  if (!self->sceneAppLocomotionEnabled) {
    Local<Array> array = Nan::New<Array>();
    array->Set(Nan::GetCurrentContext(), Nan::New<Number>(0), Nan::New<Number>(self->pInputJoy1Axis.x));
    array->Set(Nan::GetCurrentContext(), Nan::New<Number>(1), Nan::New<Number>(self->pInputJoy1Axis.y));
    array->Set(Nan::GetCurrentContext(), Nan::New<Number>(2), Nan::New<Number>(self->pInputJoy1Axis.z));
    array->Set(Nan::GetCurrentContext(), Nan::New<Number>(3), Nan::New<Number>(self->pInputJoy1Press.bState ? 1.0f : 0.0f));
    array->Set(Nan::GetCurrentContext(), Nan::New<Number>(4), Nan::New<Number>(self->pInputJoy1Touch.bState ? 1.0f : 0.0f));
    Local<Value> args[] = {
      array,
    };
    localFn->Call(Isolate::GetCurrent()->GetCurrentContext(), Nan::Null(), ARRAYSIZE(args), args);
  } else {
    localFn->Call(Isolate::GetCurrent()->GetCurrentContext(), Nan::Null(), 0, nullptr);
  }
}