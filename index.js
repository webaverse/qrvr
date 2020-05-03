const http = require('http');
const events = require('events');
const {EventEmitter} = events;
const express = require('express');
const ws = require('ws');
const engine = require('./build/Release/qr.node');

async function start({
  port = 8000,
} = {}) {
  engine.initVr();
  const qrEmitter = new EventEmitter();
  engine.createQrEngine(qrCode => {
    qrEmitter.emit('qrCode', qrCode);
  });
  const locomotionEmitter = new EventEmitter();
  engine.createLocomotionEngine(locomotionInput => {
    locomotionEmitter.emit('locomotionInput', locomotionInput);
  });
  engine.startThread();

  const presenceWss = new ws.Server({
    noServer: true,
  });
  let live = false;
  presenceWss.on('connection', (s, req) => {
    if (!live) {
      live = true;
      
      s.on('message', s => {
        const j = JSON.parse(s);
        const {method} = j;
        switch (method) {
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
        s.send(JSON.stringify({
          method: 'qrCode',
          data: qrCode,
        }));
      };
      qrEmitter.on('qrCode', _onQrCode);
      const _onLocomotionInput = locomotionInput => {
        s.send(JSON.stringify({
          method: 'locomotionInput',
          data: locomotionInput,
        }));
      };
      locomotionEmitter.on('locomotionInput', _onLocomotionInput);
      s.once('close', () => {
        live = false;

        qrEmitter.removeListener('qrCode', _onQrCode);
        locomotionEmitter.removeListener('locomotionInput', _onLocomotionInput);
        
        engine.setSceneAppLocomotionEnabled(true);
        engine.setChaperoneTransform(null);
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