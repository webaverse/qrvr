const http = require('http');
const express = require('express');
const ws = require('ws');
const qr = require('./build/Release/qr.node');

async function start({
  port = 8000,
} = {}) {
  qr.createQrEngine();

  const presenceWss = new ws.Server({
    noServer: true,
  });
  let live = false;
  presenceWss.on('connection', (s, req) => {
    if (!live) {
      live = true;

      const _recurse = () => {
        if (live) {
          const qrCodes = qr.getQrCodes();
          // console.log('detected QR code', qrCodes);
          s.send(JSON.stringify(qrCodes));
        }
        setTimeout(_recurse);
      };
      _recurse();
      s.on('close', () => {
        live = false;
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