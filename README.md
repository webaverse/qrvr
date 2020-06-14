# qrvr

SteamVR QR code reader for node.js.

Note: OpenVR is required for qrvr to work. See the error codes below for more details.

```
npm install qrvr
```

```
const qrvr = require('qrvr');
qrvr.start().then(
  s => console.log('started', s),
  errorCode => {
    // Handle error code, as per https://github.com/ValveSoftware/openvr/wiki/HmdError
    console.log(errorCode);
  }
);
```

<img width="958" alt="card" src="https://user-images.githubusercontent.com/6926057/80404207-d516db80-888e-11ea-8373-7fd9d819fbed.PNG">

## Development instructions

The core qrvr code is in [`./openvr/test/main.cpp`](./openvr/test/main.cpp).

Note you need **Python 2.7**, **VS 2017**, and **Node v13** to build the package successfullly. You can use something like [nvm-windows](https://github.com/coreybutler/nvm-windows) to manage multiple Node versions.

Once you have all the dependencies, run:

```
nvm use 13 # e.g. if you use nvm to manage Node versions
npm install --python=python2.7 --msvs_version=2017
```

in the root.

This will produce the final binary `qr.node` inside the `./build/Release` directory.

## Errors

qrvr requires [OpenVR](https://github.com/ValveSoftware/openvr), which may fail initialisation. If it does, running `qrvr.start()` will reject with a non-0 exit code from https://github.com/ValveSoftware/openvr/wiki/HmdError. The most common ones are likely:

- `HmdError_Unknown`: 1
- `HmdError_Init_InstallationNotFound`: 100
- `HmdError_Init_InstallationCorrupt`: 101
- `HmdError_Init_VRClientDLLNotFound`: 102

Please check the linked wiki for an exhaustive list and explanations.
