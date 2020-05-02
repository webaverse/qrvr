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
  
  err = vr::VRInput()->GetActionHandle("/actions/main/in/Joy2Press", &pActionJoy2Press);
  err = vr::VRInput()->GetActionHandle("/actions/main/in/Joy2Touch", &pActionJoy2Touch);
  err = vr::VRInput()->GetActionHandle("/actions/main/in/Joy2Axis", &pActionJoy2Axis);
  
  err = vr::VRInput()->GetActionHandle("/actions/main/in/Joy3Press", &pActionJoy3Press);
  err = vr::VRInput()->GetActionHandle("/actions/main/in/Joy3Touch", &pActionJoy3Touch);
  err = vr::VRInput()->GetActionHandle("/actions/main/in/Joy3Axis", &pActionJoy3Axis);
  
  err = vr::VRInput()->GetActionHandle("/actions/main/in/Joy4Press", &pActionJoy4Press);
  err = vr::VRInput()->GetActionHandle("/actions/main/in/Joy4Touch", &pActionJoy4Touch);
  err = vr::VRInput()->GetActionHandle("/actions/main/in/Joy4Axis", &pActionJoy4Axis);
  
  uv_loop_t *loop = node::GetCurrentEventLoop(Isolate::GetCurrent());
  uv_async_init(loop, &locomotionAsync, MainThreadAsync);
  locomotionAsync.data = this;
}
void LocomotionEngine::setSceneAppLocomotionEnabled(bool enabled) {
  sceneAppLocomotionEnabled = enabled;
}
void LocomotionEngine::tick() {
  vr::VRActiveActionSet_t actionSet{};
  actionSet.ulActionSet = pActionSetHandle;
  if (!sceneAppLocomotionEnabled) {
    actionSet.nPriority = vr::k_nActionSetOverlayGlobalPriorityMax;
  }
  vr::EVRInputError err = vr::VRInput()->UpdateActionState(&actionSet, sizeof(actionSet), 1);
  
  err = vr::VRInput()->GetAnalogActionData(pActionJoy1Axis, &pInputJoy1Axis, sizeof(pInputJoy1Axis), vr::k_ulInvalidInputValueHandle);
  err = vr::VRInput()->GetDigitalActionData(pActionJoy1Press, &pInputJoy1Press, sizeof(pInputJoy1Press), vr::k_ulInvalidInputValueHandle);
  err = vr::VRInput()->GetDigitalActionData(pActionJoy1Touch, &pInputJoy1Touch, sizeof(pInputJoy1Touch ), vr::k_ulInvalidInputValueHandle);
  
  err = vr::VRInput()->GetAnalogActionData(pActionJoy2Axis, &pInputJoy2Axis, sizeof(pInputJoy2Axis), vr::k_ulInvalidInputValueHandle);
  err = vr::VRInput()->GetDigitalActionData(pActionJoy2Press, &pInputJoy2Press, sizeof(pInputJoy2Press), vr::k_ulInvalidInputValueHandle);
  err = vr::VRInput()->GetDigitalActionData(pActionJoy2Touch, &pInputJoy2Touch, sizeof(pInputJoy2Touch), vr::k_ulInvalidInputValueHandle);
  
  err = vr::VRInput()->GetAnalogActionData(pActionJoy3Axis, &pInputJoy3Axis, sizeof(pInputJoy3Axis), vr::k_ulInvalidInputValueHandle);
  err = vr::VRInput()->GetDigitalActionData(pActionJoy3Press, &pInputJoy3Press, sizeof(pInputJoy3Press), vr::k_ulInvalidInputValueHandle);
  err = vr::VRInput()->GetDigitalActionData(pActionJoy3Touch, &pInputJoy3Touch, sizeof(pInputJoy3Touch ), vr::k_ulInvalidInputValueHandle);
  
  err = vr::VRInput()->GetAnalogActionData(pActionJoy4Axis, &pInputJoy4Axis, sizeof(pInputJoy4Axis), vr::k_ulInvalidInputValueHandle);
  err = vr::VRInput()->GetDigitalActionData(pActionJoy4Press, &pInputJoy4Press, sizeof(pInputJoy4Press), vr::k_ulInvalidInputValueHandle);
  err = vr::VRInput()->GetDigitalActionData(pActionJoy4Touch, &pInputJoy4Touch, sizeof(pInputJoy4Touch), vr::k_ulInvalidInputValueHandle);
  
  if (!sceneAppLocomotionEnabled) {
    uv_async_send(&locomotionAsync);
  }
}
void LocomotionEngine::MainThreadAsync(uv_async_t *handle) {
  LocomotionEngine *self = (LocomotionEngine *)handle->data;

  std::lock_guard<Mutex> lock(self->mut);
  Nan::HandleScope scope;

  Local<Function> localFn = Nan::New(self->fn);
  if (!self->sceneAppLocomotionEnabled) {
    Local<Array> array = Nan::New<Array>();

    int index = 0;
    array->Set(Nan::GetCurrentContext(), Nan::New<Number>(index++), Nan::New<Number>(self->pInputJoy1Axis.x));
    array->Set(Nan::GetCurrentContext(), Nan::New<Number>(index++), Nan::New<Number>(self->pInputJoy1Axis.y));
    array->Set(Nan::GetCurrentContext(), Nan::New<Number>(index++), Nan::New<Number>(self->pInputJoy1Axis.z));
    array->Set(Nan::GetCurrentContext(), Nan::New<Number>(index++), Nan::New<Number>(self->pInputJoy1Press.bState ? 1.0f : 0.0f));
    array->Set(Nan::GetCurrentContext(), Nan::New<Number>(index++), Nan::New<Number>(self->pInputJoy1Touch.bState ? 1.0f : 0.0f));
    
    array->Set(Nan::GetCurrentContext(), Nan::New<Number>(index++), Nan::New<Number>(self->pInputJoy2Axis.x));
    array->Set(Nan::GetCurrentContext(), Nan::New<Number>(index++), Nan::New<Number>(self->pInputJoy2Axis.y));
    array->Set(Nan::GetCurrentContext(), Nan::New<Number>(index++), Nan::New<Number>(self->pInputJoy2Axis.z));
    array->Set(Nan::GetCurrentContext(), Nan::New<Number>(index++), Nan::New<Number>(self->pInputJoy2Press.bState ? 1.0f : 0.0f));
    array->Set(Nan::GetCurrentContext(), Nan::New<Number>(index++), Nan::New<Number>(self->pInputJoy2Touch.bState ? 1.0f : 0.0f));
    
    array->Set(Nan::GetCurrentContext(), Nan::New<Number>(index++), Nan::New<Number>(self->pInputJoy3Axis.x));
    array->Set(Nan::GetCurrentContext(), Nan::New<Number>(index++), Nan::New<Number>(self->pInputJoy3Axis.y));
    array->Set(Nan::GetCurrentContext(), Nan::New<Number>(index++), Nan::New<Number>(self->pInputJoy3Axis.z));
    array->Set(Nan::GetCurrentContext(), Nan::New<Number>(index++), Nan::New<Number>(self->pInputJoy3Press.bState ? 1.0f : 0.0f));
    array->Set(Nan::GetCurrentContext(), Nan::New<Number>(index++), Nan::New<Number>(self->pInputJoy3Touch.bState ? 1.0f : 0.0f));
    
    array->Set(Nan::GetCurrentContext(), Nan::New<Number>(index++), Nan::New<Number>(self->pInputJoy4Axis.x));
    array->Set(Nan::GetCurrentContext(), Nan::New<Number>(index++), Nan::New<Number>(self->pInputJoy4Axis.y));
    array->Set(Nan::GetCurrentContext(), Nan::New<Number>(index++), Nan::New<Number>(self->pInputJoy4Axis.z));
    array->Set(Nan::GetCurrentContext(), Nan::New<Number>(index++), Nan::New<Number>(self->pInputJoy4Press.bState ? 1.0f : 0.0f));
    array->Set(Nan::GetCurrentContext(), Nan::New<Number>(index++), Nan::New<Number>(self->pInputJoy4Touch.bState ? 1.0f : 0.0f));
    
    Local<Value> args[] = {
      array,
    };
    localFn->Call(Isolate::GetCurrent()->GetCurrentContext(), Nan::Null(), ARRAYSIZE(args), args);
  } else {
    localFn->Call(Isolate::GetCurrent()->GetCurrentContext(), Nan::Null(), 0, nullptr);
  }
}