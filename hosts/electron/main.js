const { app, BrowserWindow, ipcMain } = require('electron');
const path = require('path');
const fs = require('fs');
const { spawn } = require('child_process');

let mainWindow = null;
let ptProcess = null;
let isQuitting = false;

function log(msg) {
  try {
    const line = `[${new Date().toISOString()}] ${msg}\n`;
    fs.appendFileSync('C:\\Users\\justi\\picoTracker-launch.log', line);
  } catch (_) {}
}

process.on('uncaughtException', (err) => {
  log(`UNCAUGHT EXCEPTION: ${err ? err.stack || err : 'unknown'}`);
});

process.on('unhandledRejection', (err) => {
  log(`UNHANDLED REJECTION: ${err}`);
});

function getBackendDir() {
  if (app.isPackaged) {
    return path.join(process.resourcesPath, 'backend');
  }
  return path.join(__dirname, 'backend');
}

function sendStatus(patch) {
  if (mainWindow && !mainWindow.isDestroyed()) {
    mainWindow.webContents.send('pt:status', patch);
  }
}

function sendLog(line) {
  if (mainWindow && !mainWindow.isDestroyed()) {
    mainWindow.webContents.send('pt:log', line);
  }
}

function stopBackendProcess() {
  return new Promise((resolve) => {
    if (!ptProcess) {
      resolve();
      return;
    }
    const proc = ptProcess;
    ptProcess = null;
    let resolved = false;
    const finish = () => {
      if (!resolved) {
        resolved = true;
        resolve();
      }
    };
    proc.once('exit', finish);
    try {
      if (proc.stdin && !proc.stdin.destroyed) {
        proc.stdin.write(Buffer.from([0x51])); // 'Q'
        proc.stdin.end();
      }
    } catch (_) {}
    setTimeout(() => {
      if (!resolved) {
        try {
          proc.kill();
        } catch (_) {}
        finish();
      }
    }, 2000);
  });
}

function startBackendProcess() {
  const backendDir = getBackendDir();
  const exePath = path.join(backendDir, 'picoTrackerAdvance.exe');
  log(`startBackendProcess: backendDir=${backendDir}, exePath=${exePath}`);

  if (!fs.existsSync(exePath)) {
    log(`ERROR: ${exePath} not found!`);
    sendStatus({ state: 'error', message: `picoTrackerAdvance.exe not found at ${exePath}` });
    return;
  }

  const args = ['--electron-bridge'];

  sendStatus({
    state: 'booting',
    fps: '60 FPS',
    audio: 'SDL2 Audio 44.1kHz',
    engine: 'Advance v3'
  });

  log(`Spawning ${exePath} in ${backendDir}`);
  const proc = spawn(exePath, args, {
    cwd: backendDir,
    stdio: ['pipe', 'pipe', 'pipe'],
    windowsHide: true
  });
  ptProcess = proc;

  proc.on('error', (err) => {
    log(`BACKEND SPAWN ERROR: ${err ? err.stack || err : 'unknown'}`);
  });

  let frameBuf = Buffer.alloc(0);

  proc.stdout.on('data', (chunk) => {
    frameBuf = Buffer.concat([frameBuf, chunk]);
    let latestFrame = null;

    while (frameBuf.length >= 8) {
      if (
        frameBuf[0] !== 0x50 || // 'P'
        frameBuf[1] !== 0x54 || // 'T'
        frameBuf[2] !== 0x46 || // 'F'
        frameBuf[3] !== 0x52    // 'R'
      ) {
        const idx = frameBuf.indexOf('PTFR', 1, 'ascii');
        if (idx === -1) {
          frameBuf = Buffer.alloc(0);
          break;
        }
        frameBuf = frameBuf.subarray(idx);
        if (frameBuf.length < 8) break;
      }

      const width = frameBuf.readUInt16LE(4);
      const height = frameBuf.readUInt16LE(6);
      const byteLength = width * height * 4;
      const totalPacketLength = 8 + byteLength;

      if (frameBuf.length < totalPacketLength) {
        break;
      }

      const pixels = Uint8ClampedArray.prototype.slice.call(
        frameBuf,
        8,
        totalPacketLength
      );
      latestFrame = { width, height, pixels };
      frameBuf = frameBuf.subarray(totalPacketLength);
    }

    if (latestFrame && mainWindow && !mainWindow.isDestroyed()) {
      mainWindow.webContents.send('pt:frame', latestFrame);
      sendStatus({ state: 'running' });
    }
  });

  let stderrRemainder = '';
  proc.stderr.on('data', (chunk) => {
    stderrRemainder += chunk.toString('utf8');
    const lines = stderrRemainder.split(/\r?\n/);
    stderrRemainder = lines.pop() || '';

    for (const line of lines) {
      const trimmed = line.trim();
      if (!trimmed) continue;
      sendLog(trimmed);
      log(`BACKEND LOG: ${trimmed}`);
      if (trimmed.includes('Opened joystick') || trimmed.includes('XInput')) {
        sendStatus({ gamepad: trimmed });
      }
    }
  });

  proc.on('exit', (code) => {
    log(`BACKEND EXITED WITH CODE: ${code}`);
    if (ptProcess === proc) {
      ptProcess = null;
      if (!isQuitting) {
        sendStatus({
          state: 'stopped',
          message: `EXIT (${code})`
        });
      }
    }
  });
}

function writeStdin(buf) {
  if (ptProcess && ptProcess.stdin && !ptProcess.stdin.destroyed) {
    try {
      ptProcess.stdin.write(buf);
    } catch (_) {}
  }
}

function createWindow() {
  log('createWindow starting');
  mainWindow = new BrowserWindow({
    width: 1280,
    height: 720,
    minWidth: 960,
    minHeight: 540,
    fullscreen: true,
    title: 'picoTracker Advance - ROG Ally Handheld Edition',
    backgroundColor: '#090a0f',
    autoHideMenuBar: true,
    show: true,
    webPreferences: {
      preload: path.join(__dirname, 'preload.js'),
      contextIsolation: true,
      nodeIntegration: false
    }
  });

  mainWindow.loadFile(path.join(__dirname, 'index.html'));

  mainWindow.once('ready-to-show', () => {
    log('ready-to-show fired');
    mainWindow.setFullScreen(true);
    mainWindow.show();
    mainWindow.focus();
  });

  mainWindow.webContents.once('did-finish-load', () => {
    log('did-finish-load fired');
    startBackendProcess();
  });

  mainWindow.webContents.on('did-fail-load', (e, code, desc) => {
    log(`did-fail-load: ${code} - ${desc}`);
  });

  mainWindow.on('closed', () => {
    log('mainWindow closed');
    mainWindow = null;
  });
}

ipcMain.on('pt:controller', (_event, mask) => {
  const buf = Buffer.alloc(3);
  buf[0] = 0x43; // 'C'
  buf.writeUInt16LE(mask & 0xffff, 1);
  writeStdin(buf);
});

ipcMain.on('pt:key', (_event, { down, scancode, keycode, mod }) => {
  const buf = Buffer.alloc(10);
  buf[0] = 0x53; // 'S'
  buf[1] = down ? 1 : 0;
  buf.writeUInt16LE(scancode & 0xffff, 2);
  buf.writeUInt32LE(keycode >>> 0, 4);
  buf.writeUInt16LE(mod & 0xffff, 8);
  writeStdin(buf);
});

ipcMain.on('pt:reset', () => {
  writeStdin(Buffer.from([0x52])); // 'R'
});

ipcMain.on('pt:toggle-fullscreen', () => {
  if (mainWindow && !mainWindow.isDestroyed()) {
    mainWindow.setFullScreen(!mainWindow.isFullScreen());
  }
});

ipcMain.on('pt:quit', async () => {
  isQuitting = true;
  await stopBackendProcess();
  app.quit();
});

ipcMain.handle('pt:restart', async () => {
  await stopBackendProcess();
  startBackendProcess();
  return { ok: true };
});

app.whenReady().then(() => {
  log('app.whenReady fired');
  createWindow();
});

app.on('before-quit', async (e) => {
  if (ptProcess && !isQuitting) {
    e.preventDefault();
    isQuitting = true;
    await stopBackendProcess();
    app.quit();
  }
});

app.on('window-all-closed', async () => {
  log('window-all-closed fired');
  isQuitting = true;
  await stopBackendProcess();
  app.quit();
});
