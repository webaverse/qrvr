#include "matrix.h"
#include "qr.h"
#include "locomotion.h"

using namespace v8;

NAN_METHOD(initVr) {
  vr::HmdError hmdError;
  vr::VR_Init(&hmdError, vr::EVRApplicationType::VRApplication_Overlay);
}

std::unique_ptr<QrEngine> qrEngine;
NAN_METHOD(createQrEngine) {
  if (info.Length() > 0 && info[0]->IsFunction()) {
    Local<Function> fn = Local<Function>::Cast(info[0]);
    qrEngine.reset(new QrEngine(fn));
  }
}

std::unique_ptr<LocomotionEngine> locomotionEngine;
NAN_METHOD(createLocomotionEngine) {
  if (info.Length() > 0 && info[0]->IsFunction()) {
    Local<Function> fn = Local<Function>::Cast(info[0]);
    locomotionEngine.reset(new LocomotionEngine(fn));
  }
}
NAN_METHOD(setSceneAppLocomotionEnabled) {
  if (info.Length() > 0 && info[0]->IsBoolean()) {
    locomotionEngine->sceneAppLocomotionEnabled = info[0]->BooleanValue(Isolate::GetCurrent());
  }
}
NAN_METHOD(setChaperoneTransform) {
  if (info.Length() > 0 && info[0]->IsFloat32Array()) {
    Local<Float32Array> matrixArray = Local<Float32Array>::Cast(info[0]);
    if (matrixArray->Length() == 16) {
      Local<ArrayBuffer> arrayBuffer = matrixArray->Buffer();
      float *m = (float *)arrayBuffer->GetContents().Data();
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

  Nan::SetMethod(exports, "createLocomotionEngine", createLocomotionEngine);
  Nan::SetMethod(exports, "setSceneAppLocomotionEnabled", setSceneAppLocomotionEnabled);
  Nan::SetMethod(exports, "setChaperoneTransform", setChaperoneTransform);
}

NAN_MODULE_INIT(Init) {
  Init2(target);
}
NODE_MODULE(qr, Init)