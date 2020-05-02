#ifndef _test_qr_h_
#define _test_qr_h_

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

#include <QRReader.h>
#include <DecodeHints.h>
#include <GenericLuminanceSource.h>
#include <HybridBinarizer.h>
#include <Result.h>
#include <ResultPoint.h>

#include "./fnproxy.h"

// using namespace cv;

class QrCode {
public:
  std::string data;
  float points[3*4];
};

class QrEngine {
public:
  // vr::PVRCompositor *pvrcompositor;
  // vr::IVRSystem *vrsystem;

  ID3D11Device5 *qrDevice = nullptr;
  ID3D11DeviceContext4 *qrContext = nullptr;
  IDXGISwapChain *qrSwapChain = nullptr;
  ID3D11InfoQueue *qrInfoQueue = nullptr;
  
  ZXing::QRCode::Reader reader;
  Nan::Persistent<v8::Function> fn;
  uv_async_t qrAsync;

  Mutex mut;
  // bool running = false;
  ID3D11ShaderResourceView *pD3D11ShaderResourceViewLeft = nullptr;
  ID3D11ShaderResourceView *pD3D11ShaderResourceViewRight = nullptr;
  ID3D11Texture2D *colorReadTexLeft = nullptr;
  ID3D11Texture2D *colorReadTexRight = nullptr;
  D3D11_TEXTURE2D_DESC colorBufferDesc{};
  // ID3D11Texture2D *colorMirrorClientTex = nullptr;
  // ID3D11Texture2D *colorMirrorServerTex = nullptr;
  // ID3D11Fence *fence = nullptr;
  // size_t fenceValue = 0;
  uint64_t m_lastFrameIndex = 0;

  // uint32_t eyeWidth = 0;
  // uint32_t eyeHeight = 0;
  std::vector<QrCode> qrCodes;

public:
  QrEngine(v8::Local<v8::Function> fn);
  QrCode readQrCode(int i, ID3D11Texture2D *colorReadTex, float *viewMatrixInverse, float *projectionMatrixInverse, float eyeWidth, float eyeHeight);
  QrCode getQrCodeDepth(const QrCode &qrCodeLeft, const QrCode &qrCodeRight, float *viewMatrixInverseLeft, float *projectionMatrixInverseLeft, float *viewMatrixInverseRight, float *projectionMatrixInverseRight);
  void getMirrorTexture(ID3D11ShaderResourceView *pD3D11ShaderResourceView, ID3D11Texture2D *&colorReadTex);
  void InfoQueueLog();
  static void MainThreadAsync(uv_async_t *handle);
};

#endif