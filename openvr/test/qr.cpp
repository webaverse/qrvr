#include "qr.h"
#include "matrix.h"

// #define STB_IMAGE_WRITE_IMPLEMENTATION
// #include "stb_image_write.h"

using namespace v8;

void CreateDevice(ID3D11Device5 **device, ID3D11DeviceContext4 **context, IDXGISwapChain **swapChain) {
  int32_t adapterIndex;
  vr::VRSystem()->GetDXGIOutputInfo(&adapterIndex);
  if (adapterIndex == -1) {
    adapterIndex = 0;
  }

  Microsoft::WRL::ComPtr<IDXGIFactory1> dxgi_factory;
  IDXGIAdapter1 *adapter;
  HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), &dxgi_factory);
  if (SUCCEEDED(hr)) {
    // nothing
  } else {
    getOut() << "create dxgi factory failed " << (void *)hr << std::endl;
  }
  dxgi_factory->EnumAdapters1(adapterIndex, &adapter);

  // getOut() << "create device " << adapterIndex << " " << (void *)adapter << std::endl;

  for (unsigned int i = 0;; i++) {
    hr = dxgi_factory->EnumAdapters1(i, &adapter);
    if (SUCCEEDED(hr)) {
      DXGI_ADAPTER_DESC desc;
      adapter->GetDesc(&desc);
      // getOut() << "got adapter desc " << i << (char *)desc.Description << " " << desc.AdapterLuid.HighPart << " " << desc.AdapterLuid.LowPart << std::endl;
    } else {
      break;
    }
  }
  
  // getOut() << "creating device 1" << std::endl;

  // Microsoft::WRL::ComPtr<ID3D11Device> device;
  // Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
  Microsoft::WRL::ComPtr<ID3D11Device> deviceBasic;
  Microsoft::WRL::ComPtr<ID3D11DeviceContext> contextBasic;

  D3D_FEATURE_LEVEL featureLevels[] = {
    D3D_FEATURE_LEVEL_11_1
  };
  hr = D3D11CreateDevice(
    adapter, // pAdapter
    D3D_DRIVER_TYPE_HARDWARE, // DriverType
    NULL, // Software
    0, // D3D11_CREATE_DEVICE_DEBUG, // Flags
    featureLevels, // pFeatureLevels
    ARRAYSIZE(featureLevels), // FeatureLevels
    D3D11_SDK_VERSION, // SDKVersion
    &deviceBasic, // ppDevice
    NULL, // pFeatureLevel
    &contextBasic // ppImmediateContext
  );
  // getOut() << "creating device 2" << std::endl;
  if (SUCCEEDED(hr)) {
    hr = deviceBasic->QueryInterface(__uuidof(ID3D11Device5), (void **)device);
    if (SUCCEEDED(hr)) {
      // nothing
    } else {
      getOut() << "device query failed" << std::endl;
      abort();
    }
    
    Microsoft::WRL::ComPtr<ID3D11DeviceContext3> context3;
    (*device)->GetImmediateContext3(&context3);
    hr = context3->QueryInterface(__uuidof(ID3D11DeviceContext4), (void **)context);
    if (SUCCEEDED(hr)) {
      // nothing
    } else {
      getOut() << "context query failed" << std::endl;
      abort();
    }

    /* hr = contextBasic->QueryInterface(__uuidof(ID3D11DeviceContext1), (void **)&context);
    if (SUCCEEDED(hr)) {
      // nothing
    } else {
      getOut() << "context query failed" << std::endl;
    } */
    
    IDXGIDevice *pDXGIDevice = nullptr;
    hr = (*device)->QueryInterface(__uuidof(IDXGIDevice), (void **)&pDXGIDevice);
    
    IDXGIAdapter *pDXGIAdapter = nullptr;
    hr = pDXGIDevice->GetAdapter(&pDXGIAdapter);
    
    DXGI_ADAPTER_DESC desc;
    hr = pDXGIAdapter->GetDesc(&desc);
    // getOut() << "got desc " << (char *)desc.Description << " " << desc.AdapterLuid.HighPart << " " << desc.AdapterLuid.LowPart << std::endl;
  } else {
    getOut() << "create dx device failed " << (void *)hr << std::endl;
    abort();
  }
}
vr::HmdMatrix34_t GetHmdViewMatrix() {
  vr::TrackedDevicePose_t cachedRenderPoses[vr::k_unMaxTrackedDeviceCount];
  vr::VRCompositor()->GetLastPoses(cachedRenderPoses, ARRAYSIZE(cachedRenderPoses), nullptr, 0);
  for (size_t i = 0; i < ARRAYSIZE(cachedRenderPoses); i++) {
    vr::ETrackedDeviceClass deviceClass = vr::VRSystem()->GetTrackedDeviceClass(i);
    if (deviceClass == vr::TrackedDeviceClass_HMD) {
      return cachedRenderPoses[i].mDeviceToAbsoluteTracking;
    }
  }
  return vr::HmdMatrix34_t{};
}
vr::HmdMatrix34_t GetEyeViewMatrix(vr::EVREye eye) {
  return vr::VRSystem()->GetEyeToHeadTransform(eye);
}
constexpr float zNear = 0.3f;
constexpr float zFar = 100.0f;
vr::HmdMatrix44_t GetProjectionMatrix(vr::EVREye eye) {
  return vr::VRSystem()->GetProjectionMatrix(eye, zNear, zFar);
}

// int frameId = 0;
QrEngine::QrEngine(Local<Function> fn) :
  reader(ZXing::DecodeHints().setTryHarder(true).setTryRotate(true)),
  fn(fn)
{
  // getOut() << "qr cons 1 " << hmdError << std::endl;
  CreateDevice(&qrDevice, &qrContext, &qrSwapChain);
  // getOut() << "qr cons 2" << std::endl;

  uv_loop_t *loop = node::GetCurrentEventLoop(Isolate::GetCurrent());
  uv_async_init(loop, &qrAsync, MainThreadAsync);
  qrAsync.data = this;
  
  HRESULT hr = qrDevice->QueryInterface(__uuidof(ID3D11InfoQueue), (void **)&qrInfoQueue);
  if (SUCCEEDED(hr)) {
    qrInfoQueue->PushEmptyStorageFilter();
  } else {
    // getOut() << "info queue query failed" << std::endl;
    // abort();
  }
  
  vr::EVRCompositorError err = vr::VRCompositor()->GetMirrorTextureD3D11(vr::EVREye::Eye_Left, qrDevice, &(void *)pD3D11ShaderResourceViewLeft);
  if (err) {
    getOut() << "failed to get mirror texture " << err << " " << (void *)pD3D11ShaderResourceViewLeft << std::endl;
    // abort();
  }
  err = vr::VRCompositor()->GetMirrorTextureD3D11(vr::EVREye::Eye_Right, qrDevice, &(void *)pD3D11ShaderResourceViewRight);
  if (err) {
    getOut() << "failed to get mirror texture " << err << " " << (void *)pD3D11ShaderResourceViewRight << std::endl;
    // abort();
  }
}
void QrEngine::getMirrorTexture(ID3D11ShaderResourceView *pD3D11ShaderResourceViewLeft, ID3D11Texture2D *&colorReadTexLeft) {
  HRESULT hr;

  ID3D11Resource *resLeft = nullptr;
  pD3D11ShaderResourceViewLeft->GetResource(&resLeft);

  ID3D11Texture2D *colorTexLeft = nullptr;
  hr = resLeft->QueryInterface(&colorTexLeft);
  if (FAILED(hr)) {
    getOut() << "get color tex failed " << (void *)hr << std::endl;
    InfoQueueLog();
    abort();
  }
  // getOut() << "thread 4 " << hr << " " << (void *)colorTex << std::endl;
  
  D3D11_TEXTURE2D_DESC desc;
  colorTexLeft->GetDesc(&desc);
  
  // InfoQueueLog();

  if (!colorReadTexLeft || desc.Width != colorBufferDesc.Width || desc.Height != colorBufferDesc.Height) {
    getOut() << "thread 5" << std::endl;
    
    colorBufferDesc = desc;

    D3D11_TEXTURE2D_DESC readDesc = desc;
    readDesc.Usage = D3D11_USAGE_STAGING;
    readDesc.BindFlags = 0;
    readDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    readDesc.MiscFlags = 0;
    hr = qrDevice->CreateTexture2D(
      &readDesc,
      NULL,
      &colorReadTexLeft
    );
    if (FAILED(hr)) {
      getOut() << "failed to create color read texture: " << (void *)hr << std::endl;
      // this->pvrcompositor->InfoQueueLog();
      InfoQueueLog();
      abort();
    }
  }
  
  // getOut() << "thread 6" << std::endl;
  
  // sInfoQueueLog();
  
  // getOut() << "thread 7" << std::endl;

  qrContext->CopyResource(
    colorReadTexLeft,
    colorTexLeft
  );
  
  // InfoQueueLog();

  colorTexLeft->Release();
  resLeft->Release();
  // vr::VRCompositor()->ReleaseMirrorTextureD3D11(pD3D11ShaderResourceViewLeft);
}
QrCode QrEngine::readQrCode(int i, ID3D11Texture2D *colorReadTex, float *viewMatrixInverse, float *projectionMatrixInverse, float eyeWidth, float eyeHeight) {
  D3D11_MAPPED_SUBRESOURCE resource;
  HRESULT hr = qrContext->Map(
    colorReadTex,
    0,
    D3D11_MAP_READ,
    0,
    &resource
  );
  if (FAILED(hr)) {
    getOut() << "failed to map read texture: " << (void *)hr << std::endl;
    InfoQueueLog();
    abort();
  }

  std::vector<uint8_t> rgbaImg(colorBufferDesc.Width * colorBufferDesc.Height * 4);

  UINT lBmpRowPitch = colorBufferDesc.Width * 4;
  BYTE *sptr = reinterpret_cast<BYTE *>(resource.pData);
  BYTE *dptr = (BYTE *)rgbaImg.data();
  for (size_t h = 0; h < colorBufferDesc.Height; ++h) {
    memcpy(dptr, sptr, lBmpRowPitch);
    sptr += resource.RowPitch;
    dptr += lBmpRowPitch;
  }

  qrContext->Unmap(colorReadTex, 0);

  /* std::string s("frame");
  s += std::to_string(i);
  s += "-";
  s += std::to_string(++frameId);
  s += ".png";
  stbi_write_png(s.c_str(), colorBufferDesc.Width, colorBufferDesc.Height, 4, rgbaImg.data(), colorBufferDesc.Width * 4); */

  std::shared_ptr<ZXing::GenericLuminanceSource> luminanceSource(
    new ZXing::GenericLuminanceSource(colorBufferDesc.Width, colorBufferDesc.Height, rgbaImg.data(), lBmpRowPitch, 4, 0, 1, 2)
  );
  std::unique_ptr<ZXing::HybridBinarizer> binarizer = std::make_unique<ZXing::HybridBinarizer>(luminanceSource, false);
  ZXing::BinaryBitmap &bitmap = *binarizer;
  ZXing::Result result = reader.decode(bitmap);
  // getOut() << "qr code valid " << i << " " << result.isValid() << std::endl;
  if (result.isValid()) {
    const std::wstring &wText = result.text();
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    std::string data = converter.to_bytes(wText);
    const std::vector<ZXing::ResultPoint> &resultPoints = result.resultPoints();
    QrCode qrCode{};
    qrCode.data = std::move(data);
    for (int i = 0; i < 4; i++) {
      const ZXing::ResultPoint &resultPoint = resultPoints[i];
      qrCode.points[i*3] = (resultPoint.x()/(float)eyeWidth) * 2.0f - 1.0f;
      qrCode.points[i*3+1] = (1.0f - (resultPoint.y()/(float)eyeHeight)) * 2.0f - 1.0f;
      qrCode.points[i*3+2] = 0;
    }
    return qrCode;
  } else {
    return QrCode{};
  }
}
QrCode QrEngine::getQrCodeDepth(const QrCode &qrCodeLeft, const QrCode &qrCodeRight, float *viewMatrixInverseLeft, float *projectionMatrixInverseLeft, float *viewMatrixInverseRight, float *projectionMatrixInverseRight) {
  float worldPointCenterLeft[4] = {
    0,
    0,
    0,
    1,
  };
  applyVector4Matrix(worldPointCenterLeft, viewMatrixInverseLeft);
  float worldPointCenterRight[4] = {
    0,
    0,
    0,
    1,
  };
  applyVector4Matrix(worldPointCenterRight, viewMatrixInverseRight);
  float eyeSeparation = vectorLength(
    worldPointCenterRight[0] - worldPointCenterLeft[0],
    worldPointCenterRight[1] - worldPointCenterLeft[1],
    worldPointCenterRight[2] - worldPointCenterLeft[2]
  );
  
  float viewMatrixLeft[16];
  getMatrixInverse(viewMatrixInverseLeft, viewMatrixLeft);
  
  /* float position[3];
  float quaternion[4];
  float scale[3];
  decomposeMatrix(viewMatrixLeft, position, quaternion, scale); */

  QrCode qrCode;
  qrCode.data = qrCodeLeft.data;
  for (int i = 0; i < 4; i++) {
    float worldPointLeft[4] = {
      qrCodeLeft.points[i*3],
      qrCodeLeft.points[i*3+1],
      0.0f,
      1.0f,
    };
    applyVector4Matrix(worldPointLeft, projectionMatrixInverseLeft);
    perspectiveDivideVector(worldPointLeft);
    float worldPointDot[4];
    memcpy(worldPointDot, worldPointLeft, sizeof(worldPointDot));
    // applyVector4Matrix(worldPointLeft, viewMatrixInverseLeft);

    float worldPointRight[4] = {
      qrCodeRight.points[i*3],
      qrCodeRight.points[i*3+1],
      0.0f,
      1.0f,
    };
    applyVector4Matrix(worldPointRight, projectionMatrixInverseRight);
    perspectiveDivideVector(worldPointRight);
    // applyVector4Matrix(worldPointRight, viewMatrixInverseRight);

    float pointSeparation = vectorLength(
      worldPointRight[0] - worldPointLeft[0],
      worldPointRight[1] - worldPointLeft[1],
      worldPointRight[2] - worldPointLeft[2]
    );
    float z = (zNear * eyeSeparation) / pointSeparation;
    // float z = zNear + zForward;
    float oldZ = worldPointDot[2];
    // worldPointDot[2] = -z;
    applyVector4Matrix(worldPointDot, viewMatrixInverseLeft);

    float depthOffset[3];
    memcpy(depthOffset, worldPointDot, sizeof(depthOffset));
    subVector3(depthOffset, worldPointCenterLeft);
    normalizeVector3(depthOffset);
    multiplyVector3Scalar(depthOffset, z - zNear);
    // applyVector3Quaternion(depthOffset, quaternion);
    addVector3(worldPointDot, depthOffset);

    qrCode.points[i*3] = worldPointDot[0];
    qrCode.points[i*3+1] = worldPointDot[1];
    qrCode.points[i*3+2] = worldPointDot[2];
  }
  return qrCode;
}
void QrEngine::InfoQueueLog() {
  if (qrInfoQueue) {
    ID3D11InfoQueue *infoQueue = qrInfoQueue;
    UINT64 numStoredMessages = infoQueue->GetNumStoredMessagesAllowedByRetrievalFilter();
    for (UINT64 i = 0; i < numStoredMessages; i++) {
      size_t messageSize = 0;
      HRESULT hr = infoQueue->GetMessage(
        i,
        nullptr,
        &messageSize
      );
      if (SUCCEEDED(hr)) {
        D3D11_MESSAGE *message = (D3D11_MESSAGE *)malloc(messageSize);

        hr = infoQueue->GetMessage(
          i,
          message,
          &messageSize
        );
        if (SUCCEEDED(hr)) {
          // if (message->Severity <= D3D11_MESSAGE_SEVERITY_WARNING) {
            getOut() << "info: " << message->Severity << " " << std::string(message->pDescription, message->DescriptionByteLength) << std::endl;
          // }
        } else {
          getOut() << "failed to get info queue message size: " << (void *)hr << std::endl;
        }

        free(message);
      } else {
        getOut() << "failed to get info queue message size: " << (void *)hr << std::endl;
      }
    }
    infoQueue->ClearStoredMessages();
  }
}
void QrEngine::tick() {
  // read mirror texture
  getMirrorTexture(pD3D11ShaderResourceViewLeft, colorReadTexLeft);
  getMirrorTexture(pD3D11ShaderResourceViewRight, colorReadTexRight);
  
  // getOut() << "thread 8" << std::endl;

  vr::HmdMatrix34_t viewMatrixHmd = GetHmdViewMatrix();
  float viewMatrixHmdInverse[16];
  setPoseMatrix(viewMatrixHmdInverse, viewMatrixHmd);

  float viewMatrixEyeInverseLeft[16];
  vr::HmdMatrix34_t viewMatrixEyeLeft = GetEyeViewMatrix(vr::Eye_Left);
  setPoseMatrix(viewMatrixEyeInverseLeft, viewMatrixEyeLeft);
  float viewMatrixInverseLeft[16];
  multiplyMatrices(viewMatrixHmdInverse, viewMatrixEyeInverseLeft, viewMatrixInverseLeft);

  float viewMatrixEyeInverseRight[16];
  vr::HmdMatrix34_t viewMatrixEyeRight = GetEyeViewMatrix(vr::Eye_Right);
  setPoseMatrix(viewMatrixEyeInverseRight, viewMatrixEyeRight);
  float viewMatrixInverseRight[16];
  multiplyMatrices(viewMatrixHmdInverse, viewMatrixEyeInverseRight, viewMatrixInverseRight);

  vr::HmdMatrix44_t projectionMatrixHmdLeft = GetProjectionMatrix(vr::Eye_Left);
  float projectionMatrixLeft[16];
  setPoseMatrix(projectionMatrixLeft, projectionMatrixHmdLeft);
  float projectionMatrixInverseLeft[16];
  getMatrixInverse(projectionMatrixLeft, projectionMatrixInverseLeft);

  vr::HmdMatrix44_t projectionMatrixHmdRight = GetProjectionMatrix(vr::Eye_Right);
  float projectionMatrixRight[16];
  setPoseMatrix(projectionMatrixRight, projectionMatrixHmdRight);
  float projectionMatrixInverseRight[16];
  getMatrixInverse(projectionMatrixRight, projectionMatrixInverseRight);

  float eyeWidth = (float)colorBufferDesc.Width;
  float eyeHeight = (float)colorBufferDesc.Height;
  QrCode qrCodeLeft = readQrCode(0, colorReadTexLeft, viewMatrixInverseLeft, projectionMatrixInverseLeft, eyeWidth, eyeHeight);
  QrCode qrCodeRight = readQrCode(1, colorReadTexRight, viewMatrixInverseRight, projectionMatrixInverseRight, eyeWidth, eyeHeight);
  if (qrCodeLeft.data.length() > 0 && qrCodeLeft.data == qrCodeRight.data) {
    QrCode qrCodeDepth = getQrCodeDepth(qrCodeLeft, qrCodeRight, viewMatrixInverseLeft, projectionMatrixInverseLeft, viewMatrixInverseRight, projectionMatrixInverseRight);

    {
      std::lock_guard<Mutex> lock(mut);

      /* getOut() << "Decoded QR code: " << data << " " <<
        bbox.at<cv::Point2f>(0).x << " " << bbox.at<cv::Point2f>(0).y << " " <<
        bbox.at<cv::Point2f>(1).x << " " << bbox.at<cv::Point2f>(1).y << " " <<
        bbox.at<cv::Point2f>(2).x << " " << bbox.at<cv::Point2f>(2).y << " " <<
        bbox.at<cv::Point2f>(3).x << " " << bbox.at<cv::Point2f>(3).y << " " <<
        std::endl; */

      qrCodes.resize(1);
      QrCode &qrCode = qrCodes[0];
      qrCode.data = std::move(qrCodeDepth.data);
      
      for (int i = 0; i < 4; i++) {
        qrCode.points[i*3] = qrCodeDepth.points[i*3];
        qrCode.points[i*3+1] = qrCodeDepth.points[i*3+1];
        qrCode.points[i*3+2] = qrCodeDepth.points[i*3+2];
      }
    }
  } else {
    std::lock_guard<Mutex> lock(mut);

    qrCodes.clear();
  }
  uv_async_send(&qrAsync);
}
void QrEngine::MainThreadAsync(uv_async_t *handle) {
  QrEngine *self = (QrEngine *)handle->data;

  std::lock_guard<Mutex> lock(self->mut);
  Nan::HandleScope scope;

  Local<Function> localFn = Nan::New(self->fn);
  Local<Array> array = Nan::New<Array>();
  for (const auto &qrCode : self->qrCodes) {
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
  Local<Value> args[] = {
    array,
  };
  localFn->Call(Isolate::GetCurrent()->GetCurrentContext(), Nan::Null(), ARRAYSIZE(args), args);
}