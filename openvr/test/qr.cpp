#include "qr.h"
#include "matrix.h"

// #define STB_IMAGE_WRITE_IMPLEMENTATION
// #include "stb_image_write.h"

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

int frameId = 0;
QrEngine::QrEngine() :
  reader(ZXing::DecodeHints().setTryHarder(true).setTryRotate(true))
{
  // getOut() << "qr cons 0" << std::endl;
  vr::HmdError hmdError;
  vr::VR_Init(&hmdError, vr::EVRApplicationType::VRApplication_Overlay);
  // getOut() << "qr cons 1 " << hmdError << std::endl;
  CreateDevice(&qrDevice, &qrContext, &qrSwapChain);
  // getOut() << "qr cons 2" << std::endl;
  
  HRESULT hr = qrDevice->QueryInterface(__uuidof(ID3D11InfoQueue), (void **)&qrInfoQueue);
  if (SUCCEEDED(hr)) {
    qrInfoQueue->PushEmptyStorageFilter();
  } else {
    // getOut() << "info queue query failed" << std::endl;
    // abort();
  }

  std::thread([this]() -> void {
    for (;;) {
      // getOut() << "thread 1" << std::endl;
      
      /* vr::TrackedDevicePose_t rRenderPoses[vr::k_unMaxTrackedDeviceCount];
      if (openvr_->GetCompositor()->CanRenderScene() == false)
            return; */
      {
        uint64_t newFrameIndex = 0;
        float lastVSync = 0;
        while (newFrameIndex == m_lastFrameIndex) {
          vr::VRSystem()->GetTimeSinceLastVsync(&lastVSync, &newFrameIndex);
        }

        /* if (m_lastFrameIndex + 1 < newFrameIndex)
              m_framesSkipped++; */

        m_lastFrameIndex = newFrameIndex;
      }
      
      // getOut() << "thread 2" << std::endl;

      // read mirror texture
      ID3D11ShaderResourceView *pD3D11ShaderResourceViewLeft = nullptr;
      vr::EVRCompositorError err = vr::VRCompositor()->GetMirrorTextureD3D11(vr::EVREye::Eye_Left, qrDevice, &(void *)pD3D11ShaderResourceViewLeft);
      if (err) {
        getOut() << "failed to get mirror texture " << err << " " << (void *)pD3D11ShaderResourceViewLeft << std::endl;
        // abort();
      }
      ID3D11ShaderResourceView *pD3D11ShaderResourceViewRight = nullptr;
      err = vr::VRCompositor()->GetMirrorTextureD3D11(vr::EVREye::Eye_Right, qrDevice, &(void *)pD3D11ShaderResourceViewRight);
      if (err) {
        getOut() << "failed to get mirror texture " << err << " " << (void *)pD3D11ShaderResourceViewRight << std::endl;
        // abort();
      }
      
      // getOut() << "thread 3 " << err << " " << (void *)pD3D11ShaderResourceView << std::endl;

      HRESULT hr;

      ID3D11Resource *resLeft = nullptr;
      pD3D11ShaderResourceViewLeft->GetResource(&resLeft);
      ID3D11Resource *resRight = nullptr;
      pD3D11ShaderResourceViewRight->GetResource(&resRight);

      ID3D11Texture2D *colorTexLeft = nullptr;
      hr = resLeft->QueryInterface(&colorTexLeft);
      ID3D11Texture2D *colorTexRight = nullptr;
      hr = resLeft->QueryInterface(&colorTexRight);
      
      // getOut() << "thread 4 " << hr << " " << (void *)colorTex << std::endl;
      
      D3D11_TEXTURE2D_DESC desc;
      colorTexLeft->GetDesc(&desc);
      
      // InfoQueueLog();

      if (!colorReadTexLeft || desc.Width != colorBufferDesc.Width || desc.Height != colorBufferDesc.Height) {
        // getOut() << "thread 5" << std::endl;
        
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
          // getOut() << "failed to create color read texture: " << (void *)hr << std::endl;
          // this->pvrcompositor->InfoQueueLog();
          InfoQueueLog();
          abort();
        }
        hr = qrDevice->CreateTexture2D(
          &readDesc,
          NULL,
          &colorReadTexRight
        );
        if (FAILED(hr)) {
          // getOut() << "failed to create color read texture: " << (void *)hr << std::endl;
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
      qrContext->CopyResource(
        colorReadTexRight,
        colorTexRight
      );
      
      // InfoQueueLog();

      colorTexLeft->Release();
      colorTexRight->Release();
      resLeft->Release();
      resRight->Release();
      vr::VRCompositor()->ReleaseMirrorTextureD3D11(pD3D11ShaderResourceViewLeft);
      vr::VRCompositor()->ReleaseMirrorTextureD3D11(pD3D11ShaderResourceViewRight);
      
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

      float eyeWidth = (float)desc.Width;
      float eyeHeight = (float)desc.Height;
      QrCode qrCodeLeft = readQrCode(colorReadTexLeft, viewMatrixInverseLeft, projectionMatrixInverseLeft, eyeWidth, eyeHeight);
      QrCode qrCodeRight = readQrCode(colorReadTexRight, viewMatrixInverseRight, projectionMatrixInverseRight, eyeWidth, eyeHeight);
      if (qrCodeLeft.data.length() > 0 && qrCodeRight.data.length() > 0 && qrCodeLeft.data == qrCodeRight.data) {
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
      sem.unlock();
    }
  }).detach();
}
QrCode QrEngine::readQrCode(ID3D11Texture2D *colorReadTex, float *viewMatrixInverse, float *projectionMatrixInverse, float eyeWidth, float eyeHeight) {
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

  std::shared_ptr<ZXing::GenericLuminanceSource> luminanceSource(
    new ZXing::GenericLuminanceSource(colorBufferDesc.Width, colorBufferDesc.Height, rgbaImg.data(), lBmpRowPitch, 4, 0, 1, 2)
  );
  std::unique_ptr<ZXing::HybridBinarizer> binarizer = std::make_unique<ZXing::HybridBinarizer>(luminanceSource, false);
  ZXing::BinaryBitmap &bitmap = *binarizer;
  ZXing::Result result = reader.decode(bitmap);
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
    applyVector4Matrix(worldPointLeft, viewMatrixInverseLeft);

    float worldPointRight[4] = {
      qrCodeRight.points[i*3],
      qrCodeRight.points[i*3+1],
      0.0f,
      1.0f,
    };
    applyVector4Matrix(worldPointRight, projectionMatrixInverseRight);
    perspectiveDivideVector(worldPointRight);
    applyVector4Matrix(worldPointRight, viewMatrixInverseRight);

    float pointSeparation = vectorLength(
      worldPointRight[0] - worldPointLeft[0],
      worldPointRight[1] - worldPointLeft[1],
      worldPointRight[2] - worldPointLeft[2]
    );
    float zForward = (zNear * eyeSeparation) / pointSeparation;
    float z = zNear + zForward;
    worldPointDot[2] = -z;
    applyVector4Matrix(worldPointDot, viewMatrixInverseLeft);
    
    if (i == 0) {
      getOut() << "get separation " << eyeSeparation << " " << pointSeparation << std::endl;
    }

    qrCode.points[i*3] = worldPointDot[0];
    qrCode.points[i*3+1] = worldPointDot[1];
    qrCode.points[i*3+2] = worldPointDot[2];
  }
  return qrCode;
}
void QrEngine::getQrCodes(std::function<void(const std::vector<QrCode> &)> cb) {
  sem.lock();

  std::lock_guard<Mutex> lock(mut);
  cb(qrCodes);
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