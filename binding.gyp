{
  'targets': [
    {
      'target_name': 'qr',
      'sources': [
        './openvr/test/qr.cpp',
        './openvr/test/out.cpp',
        './openvr/test/fnproxy.cpp',
        './openvr/test/matrix.cpp',
        './openvr/test/main.cpp',
      ],
      'include_dirs': [
        "<!(node -e \"console.log(require.resolve('nan').slice(0, -16))\")",
        "../../",
        "../../opencv-bin/include",
        "../../third_party/openvr/src/headers",
      ],
      'library_dirs': [
        '../../opencv-bin/x64/vc16/lib',
        '../../third_party/openvr/src/lib/win64',
      ],
      'libraries': [
        'dxgi.lib',
        'd3d11.lib',
        'd3dcompiler.lib',
        'dxguid.lib',
        'opencv_world420.lib',
        'openvr_api.lib',
      ],
      'copies': [
        {
          'destination': '<(module_root_dir)/build/Release/',
          'files': [
            "../../third_party/openvr/src/bin/win64/openvr_api.dll",
            "../../opencv-bin/x64/vc16/bin/opencv_world420.dll",
          ]
        },
      ],
      'defines': [
        'NOMINMAX',
        # 'OCULUSVR',
        # 'OPENVR',
        # 'LEAPMOTION',
        'WEBRTC_WIN',
      ],
    },
  ],
}
