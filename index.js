const http = require('http');
const express = require('express');
const ws = require('ws');
const qr = require('./build/Release/qr.node');

const port = 8000;

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
        console.log('detected QR code', );
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
server.listen(port);
console.log(`http://127.0.0.1:${port}/`);