{
  'targets': [
    {
      'target_name': 'qr',
      'sources': [
        './openvr/test/qr.cpp',
        './openvr/test/locomotion.cpp',
        './openvr/test/out.cpp',
        './openvr/test/fnproxy.cpp',
        './openvr/test/matrix.cpp',
        './openvr/test/main.cpp',
        './third_party/zxing-cpp/core/src/*.cpp',
        './third_party/zxing-cpp/core/src/qrcode/*.cpp',
        './third_party/zxing-cpp/core/src/oned/*.cpp',
        './third_party/zxing-cpp/core/src/oned/rss/*.cpp',
        './third_party/zxing-cpp/core/src/datamatrix/*.cpp',
        './third_party/zxing-cpp/core/src/aztec/*.cpp',
        './third_party/zxing-cpp/core/src/maxicode/*.cpp',
        './third_party/zxing-cpp/core/src/pdf417/*.cpp',
        './third_party/zxing-cpp/core/src/textcodec/*.cpp',
      ],
      'include_dirs': [
        "<!(node -e \"console.log(require.resolve('nan').slice(0, -16))\")",
        ".",
        "./third_party/openvr/src/headers",
        "./third_party/zxing-cpp/core/src",
        "./third_party/zxing-cpp/core/src/qrcode",
      ],
      'library_dirs': [
        './third_party/opencv/x64/vc16/lib',
        './third_party/openvr/src/lib/win64',
      ],
      'libraries': [
        'dxgi.lib',
        'd3d11.lib',
        'd3dcompiler.lib',
        'dxguid.lib',
        'openvr_api.lib',
      ],
      'copies': [
        {
          'destination': '<(module_root_dir)/build/Release/',
          'files': [
            "./third_party/openvr/src/bin/win64/openvr_api.dll",
          ]
        },
      ],
      'defines': [
        'NOMINMAX',
        'WEBRTC_WIN',
      ],
    },
  ],
}
