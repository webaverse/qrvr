const http = require('http');
const events = require('events');
const {EventEmitter} = events;
const express = require('express');
const ws = require('ws');
const engine = require('./build/Release/qr.node');

async function initQr(qrEmitter, locomotionEmitter) {
  const resultCode = engine.initVr();

  // Reject if there was an error initializing OpenVR
  // See https://github.com/ValveSoftware/openvr/wiki/HmdError for error codes
  if (resultCode !== 0) {
    console.error(resultCode === 100 ? 'OpenVR Installation not found' : 'Error initialising OpenVR', resultCode);
    throw resultCode;
  }

  engine.createQrEngine(qrCode => qrEmitter.emit('qrCode', qrCode));

  engine.createLocomotionEngine(locomotionInput =>
    locomotionEmitter.emit('locomotionInput', locomotionInput)
  );
  engine.startThread();
  return 0;
}

async function start({
  port = 8000,
  key,
} = {}) {
  let live = false;
  let qrEmitter = new EventEmitter();
  let locomotionEmitter = new EventEmitter();

  const presenceWss = new ws.Server({
    noServer: true,
  });

  presenceWss.on('connection', (s, req) => {
    if (!live) {
      live = true;

      let authed = typeof key !== 'string';
      s.on('message', s => {
        const j = JSON.parse(s);
        const {method} = j;
        switch (method) {
          case 'init': {
            if (!authed) {
              authed = j.key === key;
            }
            break;
          }
          case 'setSceneAppLocomotionEnabled': {
            const {data} = j;
            engine.setSceneAppLocomotionEnabled(data);
            break;
          }
          case 'setChaperoneTransform': {
            let {data} = j;
            if (data) {
              data = Float32Array.from(data);
            }
            engine.setChaperoneTransform(data);
            break;
          }
          default: {
            console.warn('unknown method', method);
            break;
          }
        }
      });
      const _onQrCode = qrCode => {
        if (authed) {
          s.send(JSON.stringify({
            method: 'qrCode',
            data: qrCode,
          }));
        }
      };
      qrEmitter.on('qrCode', _onQrCode);
      const _onLocomotionInput = locomotionInput => {
        if (authed) {
          s.send(JSON.stringify({
            method: 'locomotionInput',
            data: locomotionInput,
          }));
        }
      };
      locomotionEmitter.on('locomotionInput', _onLocomotionInput);
      s.once('close', () => {
        live = false;

        qrEmitter.removeListener('qrCode', _onQrCode);
        locomotionEmitter.removeListener('locomotionInput', _onLocomotionInput);

        if (authed) {
          engine.setSceneAppLocomotionEnabled(true);
          engine.setChaperoneTransform(null);
        }
      });
    } else {
      s.close();
    }
  });
  const app = express();
  app.use(express.static(__dirname));
  const server = http.createServer(app);
  server.on('upgrade', (req, socket, head) => {
    presenceWss.handleUpgrade(req, socket, head, s => {
      presenceWss.emit('connection', s, req);
    });
  });
  return new Promise((accept, reject) => {
    server.listen(port, err => {
      if (err) return reject(err);

      initQr(qrEmitter, locomotionEmitter).then(
        () => accept(`http://127.0.0.1:${port}/`),
        error => reject(error)
      );
    });
  });
};

if (require.main === module) {
  start().then(s => {
    console.log('started', s);
  });
} else {
  module.exports = {
    start,
  };
}
