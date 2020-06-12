const http = require('http');
const events = require('events');
const {EventEmitter} = events;
const express = require('express');
const ws = require('ws');
const engine = require('./build/Release/qr.node');

async function start({
  port = 8000,
  key,
} = {}) {
  let initialized = false;
  let live = false;
  let qrEmitter = null;
  let locomotionEmitter = null;
  const presenceWss = new ws.Server({
    noServer: true,
  });
  presenceWss.on('connection', (s, req) => {
    if (!initialized) {
      const resultCode = engine.initVr();

      // Don't continue if there was an error initializing OpenVR
      // See https://github.com/ValveSoftware/openvr/wiki/HmdError for error codes
      if (resultCode === 100) {
        console.error('OpenVR Installation not found', resultCode);
        return process.exit(resultCode);
      } else if (resultCode !== 0) {
        console.error('Error initialising OpenVR', resultCode);
        return process.exit(resultCode);
      }

      qrEmitter = new EventEmitter();
      engine.createQrEngine(qrCode => {
        qrEmitter.emit('qrCode', qrCode);
      });
      locomotionEmitter = new EventEmitter();
      engine.createLocomotionEngine(locomotionInput => {
        locomotionEmitter.emit('locomotionInput', locomotionInput);
      });
      engine.startThread();

      initialized = true;
    }
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
      if (!err) {
        accept(`http://127.0.0.1:${port}/`);
      } else {
        reject(err);
      }
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
