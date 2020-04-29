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
vr::HmdMatrix34_t GetViewMatrix() {
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
vr::HmdMatrix34_t GetStageMatrix() {
  return vr::VRSystem()->GetSeatedZeroPoseToStandingAbsoluteTrackingPose();
}
vr::HmdMatrix44_t GetProjectionMatrix() {
  return vr::VRSystem()->GetProjectionMatrix(vr::Eye_Left, 0.3, 100);
}

int frameId = 0;
QrEngine::QrEngine() {
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
    cv::QRCodeDetector qrDecoder;
    
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
      ID3D11ShaderResourceView *pD3D11ShaderResourceView = nullptr;
      vr::EVRCompositorError err = vr::VRCompositor()->GetMirrorTextureD3D11(vr::EVREye::Eye_Left, qrDevice, &(void *)pD3D11ShaderResourceView);
      
      // getOut() << "thread 3 " << err << " " << (void *)pD3D11ShaderResourceView << std::endl;

      if (!err) {
        HRESULT hr;

        ID3D11Resource *res = nullptr;
        pD3D11ShaderResourceView->GetResource(&res);

        ID3D11Texture2D *colorTex = nullptr;
        hr = res->QueryInterface(&colorTex);
        
        // getOut() << "thread 4 " << hr << " " << (void *)colorTex << std::endl;
        
        D3D11_TEXTURE2D_DESC desc;
        colorTex->GetDesc(&desc);
        
        // InfoQueueLog();

        if (!colorReadTex || desc.Width != colorBufferDesc.Width || desc.Height != colorBufferDesc.Height) {
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
            &colorReadTex
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
          colorReadTex,
          colorTex
        );
        
        // InfoQueueLog();

        colorTex->Release();
        res->Release();
        vr::VRCompositor()->ReleaseMirrorTextureD3D11(pD3D11ShaderResourceView);
        
        // getOut() << "thread 8" << std::endl;

        uint32_t eyeWidth = desc.Width;
        uint32_t eyeHeight = desc.Height;
        vr::HmdMatrix34_t viewMatrixHmd = GetViewMatrix();
        // float viewMatrix[16];
        setPoseMatrix(viewMatrixInverse, viewMatrixHmd);
        // getMatrixInverse(viewMatrix, viewMatrixInverse);

        vr::HmdMatrix34_t stageMatrixHmd = GetStageMatrix();
        // float stageMatrix[16];
        setPoseMatrix(stageMatrixInverse, stageMatrixHmd);
        // getMatrixInverse(stageMatrix, stageMatrixInverse);

        vr::HmdMatrix44_t projectionMatrixHmd = GetProjectionMatrix();
        float projectionMatrix[16];
        setPoseMatrix(projectionMatrix, projectionMatrixHmd);
        getMatrixInverse(projectionMatrix, projectionMatrixInverse);

        // getOut() << "thread 9" << std::endl;

        cv::Mat inputImage(colorBufferDesc.Height, colorBufferDesc.Width, CV_8UC4);
        
        // getOut() << "thread 10" << std::endl;
        
        D3D11_MAPPED_SUBRESOURCE resource;
        hr = qrContext->Map(
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

        // getOut() << "thread 11" << std::endl;

        /* char cwdBuf[MAX_PATH];
        if (!GetCurrentDirectory(sizeof(cwdBuf), cwdBuf)) {
          getOut() << "failed to get current directory" << std::endl;
          abort();
        }
        std::string qrPngPath = cwdBuf;
        qrPngPath += R"EOF(\..\..\..\..\..\qr.png)EOF";
        getOut() << "read qr code image 1 " << qrPngPath << std::endl;
        Mat inputImage = imread(qrPngPath, IMREAD_COLOR);
        getOut() << "read qr code image 2" << std::endl; */

        UINT lBmpRowPitch = colorBufferDesc.Width * 4;
        BYTE *sptr = reinterpret_cast<BYTE *>(resource.pData);
        BYTE *dptr = (BYTE *)inputImage.ptr(); // + (lBmpRowPitch * colorBufferDesc.Height) - lBmpRowPitch;
        for (size_t h = 0; h < colorBufferDesc.Height; ++h) {
          memcpy(dptr, sptr, lBmpRowPitch);
          sptr += resource.RowPitch;
          // dptr -= lBmpRowPitch;
          dptr += lBmpRowPitch;
        }
        // memcpy(inputImage.ptr(), subresource.pData, colorBufferDesc.Width * colorBufferDesc.Height * 4);

        // getOut() << "thread 12" << std::endl;

        qrContext->Unmap(colorReadTex, 0);
        
        // getOut() << "thread 13" << std::endl;
        
        cv::Mat inputImage2;
        cv::cvtColor(inputImage, inputImage2, cv::COLOR_RGBA2GRAY);
        
        /* std::string s("frame");
        s += std::to_string(++frameId);
        s += ".png";
        stbi_write_png(s.c_str(), colorBufferDesc.Width, colorBufferDesc.Height, 1, inputImage2.ptr(), colorBufferDesc.Width); */

        cv::Mat bbox, rectifiedImage;
        
        // getOut() << "thread 14" << std::endl;

        std::string data;
        try
        {
          data = qrDecoder.detectAndDecode(inputImage2, bbox, rectifiedImage);
        }
        catch( cv::Exception& e )
        {
            const char* err_msg = e.what();
            getOut() << "exception caught: " << err_msg << std::endl;
        }
        

        // getOut() << "thread 15 " << data.length() << std::endl;

        {
          std::lock_guard<Mutex> lock(mut);

          if (data.length() > 0) {
            /* getOut() << "Decoded QR code: " << data << " " <<
              bbox.at<cv::Point2f>(0).x << " " << bbox.at<cv::Point2f>(0).y << " " <<
              bbox.at<cv::Point2f>(1).x << " " << bbox.at<cv::Point2f>(1).y << " " <<
              bbox.at<cv::Point2f>(2).x << " " << bbox.at<cv::Point2f>(2).y << " " <<
              bbox.at<cv::Point2f>(3).x << " " << bbox.at<cv::Point2f>(3).y << " " <<
              std::endl; */

            qrCodes.resize(1);
            QrCode &qrCode = qrCodes[0];
            qrCode.data = std::move(data);
            
            for (int i = 0; i < 4; i++) {
              const cv::Point2f &p = bbox.at<cv::Point2f>(i);
              float worldPoint[4] = {
                (p.x/(float)eyeWidth) * 2.0f - 1.0f,
                (1.0f-(p.y/(float)eyeHeight)) * 2.0f - 1.0f,
                0.0f,
                1.0f,
              };
              /* getOut() << "points 1: " << data << " " <<
                worldPoint[0] << " " << worldPoint[1] << " " << worldPoint[2] <<
                std::endl; */
              applyVector4Matrix(worldPoint, projectionMatrixInverse);
              /* getOut() << "points 2: " << data << " " <<
                worldPoint[0] << " " << worldPoint[1] << " " << worldPoint[2] <<
                std::endl; */
              perspectiveDivideVector(worldPoint);
              /* getOut() << "points 3: " << data << " " <<
                worldPoint[0] << " " << worldPoint[1] << " " << worldPoint[2] <<
                std::endl; */
              applyVector4Matrix(worldPoint, viewMatrixInverse);
              /* getOut() << "points 4: " << data << " " <<
                worldPoint[0] << " " << worldPoint[1] << " " << worldPoint[2] <<
                std::endl; */
              // applyVector4Matrix(worldPoint, stageMatrixInverse);
              /* getOut() << "points 5: " << data << " " <<
                worldPoint[0] << " " << worldPoint[1] << " " << worldPoint[2] <<
                std::endl; */

              qrCode.points[i*3] = worldPoint[0];
              qrCode.points[i*3+1] = worldPoint[1];
              qrCode.points[i*3+2] = worldPoint[2];
            }
          } else {
            qrCodes.clear();
          }
          sem.unlock();
        }

        // getOut() << "thread 10 " << data.length() << std::endl;

        // running = false;
      }
    }
  }).detach();
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