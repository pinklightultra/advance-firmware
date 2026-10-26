const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('pt', {
  onFrame: (cb) => {
    ipcRenderer.on('pt:frame', (_event, frame) => cb(frame));
  },
  onStatus: (cb) => {
    ipcRenderer.on('pt:status', (_event, status) => cb(status));
  },
  onLog: (cb) => {
    ipcRenderer.on('pt:log', (_event, line) => cb(line));
  },
  sendController: (mask) => {
    ipcRenderer.send('pt:controller', mask);
  },
  sendKey: (payload) => {
    ipcRenderer.send('pt:key', payload);
  },
  resetDisplay: () => {
    ipcRenderer.send('pt:reset');
  },
  toggleFullscreen: () => {
    ipcRenderer.send('pt:toggle-fullscreen');
  },
  quit: () => {
    ipcRenderer.send('pt:quit');
  },
  restart: () => ipcRenderer.invoke('pt:restart')
});
