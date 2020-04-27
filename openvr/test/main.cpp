#include <nan.h>

#include "qr.h"

using namespace v8;

std::unique_ptr<QrEngine> qrEngine;
NAN_METHOD(createQrEngine) {
  qrEngine.reset(new QrEngine());
}
NAN_METHOD(getQrCodes) {
  Local<Array> array = Nan::New<Array>();
  // exports->Set(Nan::GetCurrentContext(), v8::String::NewFromUtf8(Isolate::GetCurrent(), "test").ToLocalChecked(), cache);

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
  /* Local<Object> cache = Nan::New<Object>();
  exports->Set(Nan::GetCurrentContext(), v8::String::NewFromUtf8(Isolate::GetCurrent(), "test").ToLocalChecked(), cache); */
  
  Nan::SetMethod(exports, "createQrEngine", createQrEngine);
  Nan::SetMethod(exports, "getQrCodes", getQrCodes);
}

NAN_MODULE_INIT(Init) {
  Init2(target);
}
NODE_MODULE(qr, Init)