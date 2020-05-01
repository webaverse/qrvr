#include <nan.h>

#include "matrix.h"
#include "qr.h"

#include <cmath>
#define M_PI 3.14159265358979323846264338327950288

using namespace v8;

float d = 0;

std::unique_ptr<QrEngine> qrEngine;
NAN_METHOD(createQrEngine) {
  qrEngine.reset(new QrEngine());

  char dir[MAX_PATH];
  GetCurrentDirectory(sizeof(dir), dir);
  std::string dirString(dir);
  dirString += "\\openvr\\test\\actions.json";
  vr::EVRInputError err = vr::VRInput()->SetActionManifestPath(dirString.c_str());

  vr::VRActionSetHandle_t pActionSetHandle;
	err = vr::VRInput()->GetActionSetHandle("/actions/main", &pActionSetHandle);

  vr::VRActionHandle_t pActionJoy1Press = 0;
	err = vr::VRInput()->GetActionHandle("/actions/main/in/Joy1Press", &pActionJoy1Press);

  vr::VRActionHandle_t pActionJoy1Touch;
  err = vr::VRInput()->GetActionHandle("/actions/main/in/Joy1Touch", &pActionJoy1Touch);
  
  vr::VRActionHandle_t pActionJoy1Axis;
  err = vr::VRInput()->GetActionHandle("/actions/main/in/Joy1Axis", &pActionJoy1Axis);

  vr::VRChaperoneSetup()->RevertWorkingCopy();
  float m[16] = {
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, -5, 0, 1,
  };
  vr::HmdMatrix34_t m2;
  setPoseMatrix(m2, m);
  vr::VRChaperoneSetup()->SetWorkingStandingZeroPoseToRawTrackingPose(&m2);
  vr::VRChaperoneSetup()->ShowWorkingSetPreview();

  std::thread([=]() -> void {
    uint64_t m_lastFrameIndex = 0;

    for (;;) {
      {
        uint64_t newFrameIndex = 0;
        float lastVSync = 0;
        // while (newFrameIndex < (m_lastFrameIndex + 3)) {
        while (newFrameIndex == m_lastFrameIndex) {
          vr::VRSystem()->GetTimeSinceLastVsync(&lastVSync, &newFrameIndex);
        }
        m_lastFrameIndex = newFrameIndex;
      }
      {
        vr::VRActiveActionSet_t actionSet{};
        actionSet.ulActionSet = pActionSetHandle;
        actionSet.nPriority = vr::k_nActionSetOverlayGlobalPriorityMax;
        vr::EVRInputError err = vr::VRInput()->UpdateActionState(&actionSet, sizeof(actionSet), 1);
        
        vr::InputAnalogActionData_t pInputJoy1Axis;
        err = vr::VRInput()->GetAnalogActionData(pActionJoy1Axis, &pInputJoy1Axis, sizeof(pInputJoy1Axis), vr::k_ulInvalidInputValueHandle);
        
        vr::InputDigitalActionData_t pInputJoy1Press;
        err = vr::VRInput()->GetDigitalActionData(pActionJoy1Press, &pInputJoy1Press, sizeof(pInputJoy1Press), vr::k_ulInvalidInputValueHandle);

        vr::InputDigitalActionData_t pInputJoy1Touch;
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
          0, -v, -d, 1,
        };
        vr::HmdMatrix34_t m2;
        setPoseMatrix(m2, m);
        vr::VRChaperoneSetup()->SetWorkingStandingZeroPoseToRawTrackingPose(&m2);
        vr::VRChaperoneSetup()->ShowWorkingSetPreview();
      }
    }
  }).detach();
}
NAN_METHOD(getQrCodes) {
  Local<Array> array = Nan::New<Array>();

  qrEngine->getQrCodes([&](const std::vector<QrCode> &qrCodes) -> void {
    for (const auto &qrCode : qrCodes) {
      Local<Object> object = Nan::New<Object>();
      
      Local<Array> points = Nan::New<Array>(4);
      for (size_t i = 0; i < 4; i++) {
        Local<Array> point = Nan::New<Array>(3);
        for (size_t j = 0; j < 3; j++) {
          point->Set(Nan::GetCurrentContext(), Nan::New<Number>(j), Nan::New<Number>(qrCode.points[i*3+j]));
        }
        points->Set(Nan::GetCurrentContext(), Nan::New<Number>(i), point);
      }
      object->Set(Nan::GetCurrentContext(), v8::String::NewFromUtf8(Isolate::GetCurrent(), "points").ToLocalChecked(), points);

      Local<String> text = String::NewFromUtf8(Isolate::GetCurrent(), qrCode.data.c_str()).ToLocalChecked();
      object->Set(Nan::GetCurrentContext(), v8::String::NewFromUtf8(Isolate::GetCurrent(), "text").ToLocalChecked(), text);
      
      array->Set(Nan::GetCurrentContext(), array->Length(), object);
    }
  });
  
  info.GetReturnValue().Set(array);
}

void Init2(Local<Object> exports) {
  Nan::SetMethod(exports, "createQrEngine", createQrEngine);
  Nan::SetMethod(exports, "getQrCodes", getQrCodes);
}

NAN_MODULE_INIT(Init) {
  Init2(target);
}
NODE_MODULE(qr, Init)