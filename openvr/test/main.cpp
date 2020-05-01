#include <nan.h>

#include "matrix.h"
#include "qr.h"

#include <cmath>
#define M_PI 3.14159265358979323846264338327950288

using namespace v8;

NAN_METHOD(initVr) {
  vr::HmdError ;
  vr::VR_Init(&hmdError, vr::EVRApplicationType::VRApplication_Overlay);
}

std::unique_ptr<QrEngine> qrEngine;
NAN_METHOD(createQrEngine) {
  qrEngine.reset(new QrEngine());
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

std::unique_ptr<LocomotionEngine> locomotionEngine;
NAN_METHOD(createLocomotionEngine) {
  locomotionEngine.reset(new LocomotionEngine());
}
NAN_METHOD(setSceneAppLocomotionEnabled) {
  if (args.Length() > 0 && args[0]->IsBoolean()) {
    locomotionEngine->sceneAppLocomotionEnabled = args[0]->BooleanValue();
  }
}
NAN_METHOD(getLocomotionInputs) {
  Local<Array> array = Nan::New<Array>();

  locomotionEngine->getLocomotionInputs([&](float *locomotionInputs) -> void {
    if (locomotionInputs) {
      for (int i = 0; i < 5; i++) {
        array->Set(Nan::GetCurrentContext(), Nan::New<Number>(i), Nan::New<Number>(locomotionInputs[i]));
      }
    }
  });

  info.GetReturnValue().Set(array);
}
NAN_METHOD(setChaperoneTransform) {
  if (args.Length() > 0 && args[0]->IsFloat32Array()) {
    Local<Float32Array> matrixArray = Local<Float32Array>::Cast(args[0]);
    if (matrixArray->Length() == 16) {
      Local<ArrayBuffer> arrayBuffer = matrixArray->Buffer();
      float *m = (float *)arrayBuffer->GetContents()->Data();
      vr::HmdMatrix34_t m2;
      setPoseMatrix(m2, m);
      vr::VRChaperoneSetup()->SetWorkingStandingZeroPoseToRawTrackingPose(&m2);
    } else {
      vr::VRChaperoneSetup()->RevertWorkingCopy();
    }
  } else {
    vr::VRChaperoneSetup()->RevertWorkingCopy();
  }
  vr::VRChaperoneSetup()->ShowWorkingSetPreview();
}

void Init2(Local<Object> exports) {
  Nan::SetMethod(exports, "initVr", initVr);

  Nan::SetMethod(exports, "createQrEngine", createQrEngine);
  Nan::SetMethod(exports, "getQrCodes", getQrCodes);

  Nan::SetMethod(exports, "createLocomotionEngine", createLocomotionEngine);
  Nan::SetMethod(exports, "setSceneAppLocomotionEnabled", setSceneAppLocomotionEnabled);
  Nan::SetMethod(exports, "getLocomotionInputs", getLocomotionInputs);
  Nan::SetMethod(exports, "setChaperoneTransform", setChaperoneTransform);
}

NAN_MODULE_INIT(Init) {
  Init2(target);
}
NODE_MODULE(qr, Init)