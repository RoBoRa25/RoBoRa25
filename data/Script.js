const cmd_load_ui = 'hello_robora';
const cmd_client_ui = 'webui';
const cmd_ver_ui = '1.0';

const add_host = location.host
//const add_host = 'robora.local'
//const add_host = '192.168.137.160'

const WS_PATHS = [`ws://${add_host}/ws`, `ws://${add_host}/`,];

/**********************
   * UTIL & TEMA
   **********************/
const $ = sel => document.querySelector(sel);
const $$ = sel => Array.from(document.querySelectorAll(sel));

// Tema Light/Dark con persistenza
const themeSwitch = $('#themeSwitch');
let savedTheme = localStorage.getItem('theme') || 'light';
document.documentElement.setAttribute('data-theme', savedTheme);
themeSwitch.checked = savedTheme === 'dark';
themeSwitch.addEventListener('change', () => {
  const t = themeSwitch.checked ? 'dark' : 'light';
  document.documentElement.setAttribute('data-theme', t);
  localStorage.setItem('theme', t);
});

// Router minimal SPA
const sections = {
  home: $('#page-home'),
  configConnessione: $('#page-config-connessione'),
  configMotore: $('#page-config-motore'),
  configTelemetria: $('#page-config-telemetria'),
  info: $('#page-info'),
  robot: $('#page-robot'),
  ota: $('#page-ota'),
  pagewebui: $('#page-webui'),
  model3d: $('#page-model3d'),
  display: $('#page-display'),
};
let currentPage = 'home';
function go(page) {
  Object.values(sections).forEach(s => s.classList.remove('active'));
  sections[page].classList.add('active');
  currentPage = page;
  if (page === 'info') {
    requestInfo();
  }
  if (page === 'robot') {
    startMoveLoopIfNeeded();
  } else {
    stopMoveLoop();
  }
  if (page === 'model3d') {
    start3D();
  } else {
    stop3D();
  }

}
$$("[data-goto]").forEach(btn => btn.addEventListener('click', () => go(btn.dataset.goto)));

/**********************
 * WEBSOCKET MANAGER
 **********************/
const wsLabel = $('#wsLabel');
const wsDot = $('#wsDot');

let ws = null;
let wsUrl = WS_PATHS[0];
let reconnectMs = 500;
let reconnectTimer = null;
let lastSensorPayload = null; // buffer sensori quando non su pagina robot
let configData = null; // Dati di configurazione caricati dal server

function setWsState(state) {
  if (state === 'ok') {
    wsLabel.textContent = 'Connesso';
    wsDot.className = 'dot ok';
  } else if (state === 'connecting') {
    wsLabel.textContent = 'Connessione...';
    wsDot.className = 'dot warn';
  } else {
    wsLabel.textContent = 'Disconnesso';
    wsDot.className = 'dot';
  }
}

function connectWS() {
  setWsState('connecting');
  try {
    ws = new WebSocket(wsUrl);
  } catch (e) {
    wsUrl = WS_PATHS[1];
    ws = new WebSocket(wsUrl);
  }

  ws.addEventListener('open', () => {
    setWsState('ok');
    reconnectMs = 500; // reset backoff
    sendJson({
      CMD: cmd_load_ui,
      client: cmd_client_ui,
      ver: cmd_ver_ui
    });
    // Richiedi i dati di configurazione al server all'apertura della connessione
    sendJson({
      CMD: 'config_req'
    });
  });

  ws.addEventListener('message', (ev) => {
    let msg;
    try {
      msg = JSON.parse(ev.data);
    } catch (e) {
      console.warn('WS non-JSON', ev.data);
      return;
    }
    handleMessage(msg);
  });

  ws.addEventListener('close', () => scheduleReconnect());
  ws.addEventListener('error', () => {
    try {
      ws.close();
    } catch { }
  });
}

function scheduleReconnect() {
  setWsState('down');
  if (reconnectTimer) return;
  reconnectTimer = setTimeout(() => {
    reconnectTimer = null;
    reconnectMs = Math.min(reconnectMs * 1.7, 6000);
    if (reconnectMs > 3000 && wsUrl === WS_PATHS[0]) wsUrl = WS_PATHS[1];
    connectWS();
  }, reconnectMs);
}

function sendJson(obj) {
  if (ws && ws.readyState === 1) {
    console.log('WS >>', obj);
    ws.send(JSON.stringify(obj));
  }
}

/**********************
 * HANDLER MESSAGGI WS
 **********************/
function handleMessage(msg) {
  console.log('WS <<', msg);
  switch (msg.CMD) {
    case 'config_req':
      configData = msg;
      buildConfig();
      updateParamsFromMsg(msg);
      break;
    case 'config_rd':
    case 'config_wr':
      updateParamsFromMsg(msg);
      break;
    case 'sensor':
      update3dGyro(msg);
      if (currentPage === 'robot') updateSensors(msg);
      else lastSensorPayload = msg;
      break;
    case 'info':
      updateInfo(msg);
      break;
    case 'ota':
      handleOtaWs(msg);
      break;
  }
}

/**********************
 * PAGINA CONFIG
 **********************/
function getFieldVal(el) {
  if (!el) return '';
  if (el.type === 'checkbox') return el.checked ? '1' : '0';
  return el.value ?? '';
}

function setFieldVal(el, v) {
  if (!el) return;
  if (el.type === 'checkbox') {
    const s = String(v).toLowerCase();
    el.checked = (v === true) || (v === 1) || (s === '1') || (s === 'true') || (s === 'on');
  } else {
    if (el.tagName === 'SELECT') {
      if (!Array.from(el.options).some(o => o.value == v)) {
        const o = document.createElement('option');
        o.value = v; o.textContent = v;
        el.appendChild(o);
      }
    }
    el.value = v;
  }
}

function buildConfig() {
  if (!configData) return;
  for (const section in configData) {
    if (configData.hasOwnProperty(section)) {
      const params = configData[section].params;
      const labels = configData[section].labels;
      const types = configData[section].types;
      const wrap = document.getElementById(`paramsContainer-${section}`);
      if (!wrap) continue;

      wrap.innerHTML = '';
      params.forEach((name, idx) => {
        const card = document.createElement('div');
        card.className = 'card';
        const row1 = document.createElement('div');
        row1.className = 'row';
        const fieldId = `cfg_${name}`;

        const label = document.createElement('label');
        label.textContent = labels[idx] || name;
        label.setAttribute('for', fieldId);

        let field;
        if (types[idx] === 'select') {
          field = document.createElement('select');
          (configData[section].options[idx] || []).forEach(opt => {
            const o = document.createElement('option');
            o.value = opt;
            o.textContent = opt;
            field.appendChild(o);
          });
        } else {
          field = document.createElement('input');
          field.type = types[idx] || 'text';
        }
        field.id = fieldId;
        field.dataset.paramName = name;

        const btnRead = document.createElement('button'); btnRead.className = 'btn'; btnRead.textContent = 'Leggi';
        btnRead.addEventListener('click', () => sendJson({ CMD: 'config_rd', [name]: '' }));

        const btnWrite = document.createElement('button'); btnWrite.className = 'btn warn'; btnWrite.textContent = 'Scrivi';
        btnWrite.addEventListener('click', () => {
          const val = getFieldVal(field);
          sendJson({ CMD: 'config_wr', [name]: String(val) });
        });

        row1.append(label, field, btnRead, btnWrite);
        card.append(row1);
        wrap.append(card);
      });
    }
  }

  // Aggiunge listener ai pulsanti dinamici dopo la creazione
  $$('.btn.read').forEach(btn => btn.addEventListener('click', (e) => {
    const section = e.target.dataset.section;
    if (configData && configData[section] && configData[section].params) {
      configData[section].params.forEach(paramName => {
        sendJson({
          CMD: 'config_rd',
          [paramName]: ''
        });
      });
    }
  }));

  $$('.btn.write').forEach(btn => btn.addEventListener('click', (e) => {
    const section = e.target.dataset.section;
    if (configData && configData[section] && configData[section].params) {
      configData[section].params.forEach(paramName => {
        const el = document.getElementById(`cfg_${paramName}`);
        sendJson({ CMD: 'config_wr', [paramName]: String(getFieldVal(el)) });
      });
    }
  }));
}

function updateParamsFromMsg(msg) {
  for (const key in msg) {
    if (Object.prototype.hasOwnProperty.call(msg, key) && key !== 'CMD') {
      const el = document.getElementById(`cfg_${key}`);
      if (el) setFieldVal(el, msg[key]);
    }
  }
}

/**********************
 * PAGINA INFO
 **********************/
const INFO_NAMES = ['info1', 'info2', 'info3', 'info4', 'info5', 'info6', 'info7', 'info8'];
function buildInfo() {
  const wrap = $('#infoContainer');
  wrap.innerHTML = '';
  INFO_NAMES.forEach(name => {
    const card = document.createElement('div');
    card.className = 'card';
    const row = document.createElement('div');
    row.className = 'row';
    const label = document.createElement('label');
    label.textContent = name;
    label.setAttribute('for', `inf_${name}`);
    const field = document.createElement('input');
    field.type = 'text';
    field.readOnly = true;
    field.id = `inf_${name}`;
    row.append(label, field);
    card.append(row);
    wrap.append(card);
  });
  $('#refreshInfo').addEventListener('click', requestInfo);
}
function requestInfo() {
  sendJson({ CMD: 'info_req' });
}
function updateInfo(msg) {
  INFO_NAMES.forEach(n => {
    if (n in msg) {
      const el = document.getElementById(`inf_${n}`);
      if (el) el.value = msg[n];
    }
  });
}

/**********************
 * PAGINA ROBOT
 **********************/
const MaxFnButton = 8;
const SENSOR_NAMES = ['sens0', 'sens1', 'sens2', 'sens3', 'sens4', 'sens5', 'sens6', 'sens7'];
function buildSensors() {
  const grid = $('#sensorGrid');
  grid.innerHTML = '';
  SENSOR_NAMES.forEach(n => {
    const wrap = document.createElement('div');
    const label = document.createElement('label');
    label.textContent = n;
    label.setAttribute('for', `sns_${n}`);
    const field = document.createElement('input');
    field.type = 'text';
    field.readOnly = true;
    field.id = `sns_${n}`;
    wrap.append(label, field);
    grid.append(wrap);
  });
}
function updateSensors(msg) {
  SENSOR_NAMES.forEach(n => {
    if (n in msg) {
      const el = document.getElementById(`sns_${n}`);
      if (el) el.value = msg[n];
    }
  });
}

function buildFnButtons() {
  const grid = $('#fnGrid');
  grid.innerHTML = '';
  for (let i = 0; i < MaxFnButton; i++) {
    const b = document.createElement('button');
    b.className = 'fn-btn';
    b.id = `fn${i}`;
    b.textContent = `FN${i}`;
    b.setAttribute('aria-pressed', 'false');
    b.dataset.state = 'off';
    b.dataset.fn = String(i);
    b.addEventListener('click', () => {
      const now = b.dataset.state === 'off' ? 'on' : 'off';
      b.dataset.state = now;
      b.setAttribute('aria-pressed', String(now === 'on'));
      sendJson({
        CMD: 'function',
        [`FN${i}`]: now
      });
    });
    grid.append(b);
  }
}

// Joystick
const LimitJoyVlaue = 127;
const knob = $('#knob');
const joy = $('#joystick');
let joyCenter = { x: 0, y: 0 };
let knobPos = { x: 0, y: 0 };
let joyVec = { x: 0, y: 0 };
let activePointerId = null;
let sendMode = $('#sendMode').value;
let moveTimer = null;

function joyInit() {
  positionKnob(0, 0);
  joy.addEventListener('pointerdown', onPointerDown);
  window.addEventListener('pointermove', onPointerMove);
  window.addEventListener('pointerup', onPointerUp);
  window.addEventListener('pointercancel', onPointerUp);

  document.addEventListener('keydown', onKey);
  document.addEventListener('keyup', onKeyUp);

  $('#sendMode').addEventListener('change', (e) => {
    sendMode = e.target.value;
    if (sendMode === 'interval') startMoveLoopIfNeeded();
    else stopMoveLoop();
  });
}

function joyBounds() {
  const r = joy.getBoundingClientRect();
  const radius = r.width / 2;
  const knobR = knob.offsetWidth / 2;
  return {
    cx: r.left + radius,
    cy: r.top + radius,
    limit: radius - knobR
  };
}

function onPointerDown(e) {
  if (activePointerId !== null) return;
  activePointerId = e.pointerId;
  joy.setPointerCapture(activePointerId);
  computeFromPointer(e);
}
function onPointerMove(e) {
  if (activePointerId === null || e.pointerId !== activePointerId) return;
  computeFromPointer(e);
}
function onPointerUp(e) {
  if (e.pointerId !== activePointerId) return;
  activePointerId = null;
  joy.releasePointerCapture(e.pointerId);
  setJoy(0, 0, true);
}

function computeFromPointer(e) {
  const b = joyBounds();
  let dx = e.clientX - b.cx;
  let dy = e.clientY - b.cy;
  const len = Math.hypot(dx, dy);
  if (len > b.limit) {
    dx = dx * b.limit / len;
    dy = dy * b.limit / len;
  }
  positionKnob(dx, dy);

  const nx = dx / b.limit;
  const ny = -dy / b.limit;
  const x = Math.max(-LimitJoyVlaue, Math.min(LimitJoyVlaue, Math.round(nx * LimitJoyVlaue)));
  const y = Math.max(-LimitJoyVlaue, Math.min(LimitJoyVlaue, Math.round(ny * LimitJoyVlaue)));
  setVec(x, y);
}

function positionKnob(dx, dy) {
  knob.style.transform = `translate(calc(-50% + ${dx}px), calc(-50% + ${dy}px))`;
  knobPos = { x: dx, y: dy };
}

function setVec(x, y) {
  if (joyVec.x === x && joyVec.y === y) return;
  joyVec = { x, y };
  $('#xVal').textContent = x;
  $('#yVal').textContent = y;
  if (sendMode === 'onchange') sendMove();
}

function setJoy(x, y, alsoSend) {
  const b = joyBounds();
  positionKnob(b.limit * (x / LimitJoyVlaue), -b.limit * (y / LimitJoyVlaue));
  joyVec = { x, y };
  $('#xVal').textContent = x;
  $('#yVal').textContent = y;
  if (alsoSend && sendMode === 'onchange') sendMove();
}

function startMoveLoopIfNeeded() {
  if (sendMode !== 'interval') return;
  if (moveTimer) return;
  moveTimer = setInterval(() => sendMove(), 100);
}
function stopMoveLoop() {
  if (moveTimer) {
    clearInterval(moveTimer);
    moveTimer = null;
  }
}

function sendMove() {
  sendJson({ CMD: 'move', x: String(joyVec.x), y: String(joyVec.y) });
}

// Tastiera: frecce
const keyState = { ArrowUp: false, ArrowDown: false, ArrowLeft: false, ArrowRight: false };
function onKey(e) {
  if (!(e.key in keyState)) return;
  if (keyState[e.key]) return;
  keyState[e.key] = true;
  e.preventDefault();
  computeFromKeys();
}
function onKeyUp(e) {
  if (!(e.key in keyState)) return;
  keyState[e.key] = false;
  e.preventDefault();
  computeFromKeys();
}
function computeFromKeys() {
  let x = 0, y = 0;
  if (keyState.ArrowLeft) x -= LimitJoyVlaue;
  if (keyState.ArrowRight) x += LimitJoyVlaue;
  if (keyState.ArrowUp) y += LimitJoyVlaue;
  if (keyState.ArrowDown) y -= LimitJoyVlaue;
  x = Math.max(-LimitJoyVlaue, Math.min(LimitJoyVlaue, x));
  y = Math.max(-LimitJoyVlaue, Math.min(LimitJoyVlaue, y));
  setJoy(x, y, true);
}

/**********************
 * PAGINA OTA 
 **********************/
function buildOTA() {
  const fileInput = $('#fwFile');
  const btnUpload = $('#btnUpload');
  const btnAbort = $('#btnAbort');
  const targetSel = $('#otaTarget');
  const fsTypeSel = $('#fsType');
  const otaBar = $('#otaBar');
  const otaStatus = $('#otaStatus');
  const shaField = $('#sha256');
  const sizeField = $('#filesize');
  const btnHash = $('#btnHash');
  const btnReboot = $('#btnReboot');
  const md5Input = $('#md5');

  let otaXhr = null;
  let endpoint = '/update';

  fileInput.addEventListener('change', () => {
    const f = fileInput.files[0];
    if (!f) {
      btnUpload.disabled = true;
      otaStatus.textContent = 'In attesa file…';
      sizeField.value = '';
      shaField.value = '';
      return;
    }
    btnUpload.disabled = false;
    sizeField.value = formatBytes(f.size);
    otaStatus.textContent = 'File selezionato.';
    shaField.value = '';
  });

  targetSel.addEventListener('change', () => {
    const isFs = targetSel.value === 'fs';
    fsTypeSel.disabled = !isFs;
  });
  targetSel.dispatchEvent(new Event('change'));

  btnHash.addEventListener('click', async () => {
    const f = fileInput.files[0];
    if (!f) return;
    const hash = await sha256Hex(f);
    shaField.value = hash;
    otaStatus.textContent = 'SHA-256 calcolato.';
  });

  btnUpload.addEventListener('click', () => {
    const f = fileInput.files[0];
    if (!f) return;
    const method = $('#otaMethod').value;
    if (method == 'multipart') endpoint = '/update';
    else endpoint = '/ota';
    startUpload(f, endpoint, method, shaField.value, targetSel.value, fsTypeSel.value);
  });

  btnAbort.addEventListener('click', () => { if (otaXhr) otaXhr.abort(); });
  btnReboot.addEventListener('click', () => sendJson({ CMD: 'reboot' }));

  function startUpload(file, endpoint, method, sha256, target, fsType) {
    otaBar.style.width = '0%';
    otaStatus.textContent = 'Inizio caricamento…';
    btnUpload.disabled = true;
    btnAbort.disabled = false;
    btnReboot.disabled = true;

    const xhr = new XMLHttpRequest();
    otaXhr = xhr;
    xhr.open('POST', endpoint, true);
    xhr.responseType = 'text';
    xhr.setRequestHeader('X-File-Name', encodeURIComponent(file.name));
    if (sha256) xhr.setRequestHeader('X-Content-SHA256', sha256);
    if (md5Input.value) xhr.setRequestHeader('X-Content-MD5', md5Input.value.trim());
    if (sha256) xhr.setRequestHeader('X-Content-SHA256', sha256);
    xhr.setRequestHeader('X-Update-Target', target);
    if (target === 'fs') {
      xhr.setRequestHeader('X-FS-Type', fsType);
      xhr.setRequestHeader('X-Update-Partition', 'spiffs');
    }

    xhr.upload.onprogress = (e) => {
      if (e.lengthComputable) {
        const p = Math.round((e.loaded / e.total) * 100);
        otaBar.style.width = p + '%';
        otaStatus.textContent = `Caricamento ${p}%`;
      }
    };
    xhr.onerror = () => finalize('Errore di rete', false);
    xhr.onabort = () => finalize('Annullato', false);
    xhr.onload = () => {
      const ok = xhr.status >= 200 && xhr.status < 300;
      finalize(ok ? `Completato (${xhr.status})` : `Fallito (${xhr.status})`, ok);
    };

    if (method === 'multipart') {
      const fd = new FormData();
      const fieldName = target === 'fs' ? 'filesystem' : 'firmware';
      fd.append(fieldName, file, file.name);
      xhr.send(fd);
    } else {
      xhr.setRequestHeader('Content-Type', 'application/octet-stream');
      xhr.send(file);
    }
  }

  function finalize(msg, ok) {
    otaStatus.textContent = msg + (otaXhr && otaXhr.responseText ? ` • ${otaXhr.responseText}` : '');
    btnAbort.disabled = true;
    btnUpload.disabled = false;
    otaXhr = null;
    if (ok) {
      otaBar.style.width = '100%';
      btnReboot.disabled = false;
    }
  }
}

// Helpers OTA
async function sha256Hex(file) {
  const buf = await file.arrayBuffer();
  const hash = await window.crypto.subtle.digest('SHA-256', buf);
  return Array.from(new Uint8Array(hash)).map(b => b.toString(16).padStart(2, '0')).join('');
}
function formatBytes(n) {
  const u = ['B', 'KB', 'MB', 'GB'];
  let i = 0;
  while (n >= 1024 && i < u.length - 1) {
    n /= 1024;
    i++;
  }
  return (i ? n.toFixed(1) : n.toFixed(0)) + ' ' + u[i];
}

function appendOtaLog(line) {
  const box = document.getElementById('otaLog');
  if (!box) return;
  const t = new Date().toLocaleTimeString();
  const div = document.createElement('div');
  div.textContent = `[${t}] ${line}`;
  box.appendChild(div);
  box.scrollTop = box.scrollHeight;
}
function setOtaUi(pct, msg) {
  if (Number.isFinite(pct)) {
    const p = Math.max(0, Math.min(100, Math.round(pct)));
    const bar = document.getElementById('otaBar');
    if (bar) bar.style.width = p + '%';
  }
  if (msg) {
    const s = document.getElementById('otaStatus');
    if (s) s.textContent = msg;
  }
}
function handleOtaWs(m) {
  const evt = m.event;
  if (evt === 'start') {
    setOtaUi(0, `Inizio OTA (${m.target || '?'})`);
    appendOtaLog(`Start: target=${m.target} total=${m.total || 0} max=${m.max || 0}`);
  } else if (evt === 'reject') {
    setOtaUi(0, `Rifiutato: ${m.reason || ''}`);
    appendOtaLog(`Reject: ${m.reason || ''}`);
  } else if (evt === 'progress') {
    const pct = m.total ? (m.done * 100 / m.total) : (m.percent || 0);
    setOtaUi(pct, `Caricamento ${Math.round(pct)}%`);
  } else if (evt === 'end') {
    setOtaUi(100, m.ok ? `Completato (${m.message || 'OK'})` : `Fallito (${m.message || 'ERR'})`);
    appendOtaLog(`End: ok=${m.ok} msg=${m.message || ''}`);
  } else {
    appendOtaLog(JSON.stringify(m));
  }
}

/**********************
 * PAGINA WEB UI
 **********************/
function hidesensorCard() {
  const sensorCard = document.getElementById('sensorCard');
  if (sensorCard) {
    if (sensorCard.style.display === 'none') {
      sensorCard.style.display = 'grid';
    } else {
      sensorCard.style.display = 'none';
    }
  }
}
function hidefnCard() {
  const fnCard = document.getElementById('fnCard');
  if (fnCard) {
    if (fnCard.style.display === 'none') {
      fnCard.style.display = 'grid';
    } else {
      fnCard.style.display = 'none';
    }
  }
}
function hidemodalita() {
  const modalita = document.getElementById('modalita');
  if (modalita.style.display === 'none') {
    modalita.style.display = '';
  } else {
    modalita.style.display = 'none';
  }
}

function ResetFabbria() {
  sendJson({ CMD: 'reset_memory' });
}

/**********************
 * SEZIONE 3D
 **********************/
// Global 3D state
let scene, camera, renderer, robot, container;
let needsRender = false;
let ro; // ResizeObserver

// Quaternioni per smoothing
const robotTargetQuat = new THREE.Quaternion();
const robotSmoothQuat = new THREE.Quaternion();
const SMOOTH_FACTOR = 0.15; // 0..1: più alto = segue più veloce
const HEMISPHER_LIGHT = 0.6;
const AMBIENT_LIGHT = 0.5;
const DIRECTION_LIGHT = 0.9;

function hexToThreeColor(hex) {
  // accetta "#rrggbb" o "rrggbb"
  const s = String(hex).replace('#', '');
  const n = parseInt(s, 16);
  return new THREE.Color(n);
}

function applyUniformMaterialToRobot(colorHex) {
  if (!robot) return;
  const color = hexToThreeColor(colorHex);
  // luce ambiente per evitare aree nere
  if (!scene.getObjectByName('__ambient')) {
    const amb = new THREE.AmbientLight(0xffffff, AMBIENT_LIGHT);
    amb.name = '__ambient';
    scene.add(amb);
  }

  robot.traverse((child) => {
    if (child.isMesh) {
      const isSkinned = child.isSkinnedMesh === true;
      const hasMorphTargets = !!child.morphTargetInfluences;
      const hasMorphNormals = !!(child.morphTargetDictionary);

      // Se vuoi PBR:
      // const mat = new THREE.MeshStandardMaterial({
      //   color,
      //   metalness: 0.1,
      //   roughness: 0.7,
      //   skinning: isSkinned,
      //   morphTargets: hasMorphTargets,
      //   morphNormals: hasMorphNormals,
      // });

      // Phong è meno esigente alle luci ma con specularità gradevole
      const mat = new THREE.MeshPhongMaterial({
        color,
        shininess: 30,
        skinning: isSkinned,
        morphTargets: hasMorphTargets
      });

      // Se vuoi eliminare eventuali texture scure:
      mat.map = null;
      child.material = mat;
      child.material.needsUpdate = true;

      if (child.geometry && !child.geometry.attributes.normal) {
        child.geometry.computeVertexNormals();
      }
    }
  });

  if (renderer.outputColorSpace !== THREE.SRGBColorSpace) {
    renderer.outputColorSpace = THREE.SRGBColorSpace;
  }

  invalidate3D?.();
}

function setupModelColorUI() {
  const colorInput = document.getElementById('modelColor');
  if (!colorInput) return; // UI non presente su questa pagina
  colorInput.addEventListener('input', () => { const hex = colorInput.value || '#808080'; applyUniformMaterialToRobot(hex); });
}

function init3DOnce() {
  if (renderer) return;
  container = document.getElementById('threeWrap');
  if (!container) return;

  // Scena & camera
  scene = new THREE.Scene();
  camera = new THREE.PerspectiveCamera(60, 1, 0.1, 100);
  camera.position.set(2.2, 1.8, 3.2);

  // Renderer
  renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true });
  renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, 2));
  renderer.setSize(container.clientWidth || 640, container.clientHeight || 400);
  container.appendChild(renderer.domElement);

  // Luci
  const hemi = new THREE.HemisphereLight(0xffffff, 0x444444, HEMISPHER_LIGHT);
  hemi.position.set(0, 1, 0);
  scene.add(hemi);

  if (!scene.getObjectByName('__ambient')) {
    const amb = new THREE.AmbientLight(0xffffff, AMBIENT_LIGHT);
    amb.name = '__ambient';
    scene.add(amb);
  }

  const dir = new THREE.DirectionalLight(0xffffff, DIRECTION_LIGHT);
  dir.position.set(3, 5, 2);
  scene.add(dir);

  // ====== CARICA MODELLO ESTERNO ======
  const loader = new THREE.GLTFLoader();
  loader.load('Robot3d.glb',  // percorso al modello
    (gltf) => { robot = gltf.scene; scene.add(robot); frameObject(robot, camera); invalidate3D(); },
    (xhr) => { console.log((xhr.loaded / xhr.total * 100).toFixed(1) + '% caricato'); },
    (error) => { console.error('Errore caricamento modello 3D:', error); }
  );

  // Resize
  const resize3D = () => {
    const w = container.clientWidth || 640;
    const h = container.clientHeight || 400;
    camera.aspect = w / h;
    camera.updateProjectionMatrix();
    renderer.setSize(w, h, false);
    invalidate3D();
  };
  ro = new ResizeObserver(resize3D);
  ro.observe(container);
  window.addEventListener('orientationchange', resize3D);

  // Gestione perdita/ripristino WebGL
  renderer.domElement.addEventListener('webglcontextlost', (e) => { e.preventDefault(); stop3D(); });
  renderer.domElement.addEventListener('webglcontextrestored', () => { start3D(); });

  // Drag orbitale
  addDragControls(renderer.domElement);
}

// Avvia il loop di render 
function start3D() {
  init3DOnce();
  if (!renderer) return;
  document.addEventListener('visibilitychange', () => { if (!document.hidden) invalidate3D(); });
  renderer.setAnimationLoop(() => {
    if (document.hidden) return;     // pausa se tab non visibile
    if (!needsRender) return;        // render solo quando serve
    renderer.render(scene, camera);
    needsRender = false;
  });
  invalidate3D(); // forza primo frame
}

// Ferma il loop di render
function stop3D() {
  if (renderer) renderer.setAnimationLoop(null);
  if (ro && container) { try { ro.unobserve(container); } catch { } }
}

// Helpers 3D
function invalidate3D() { needsRender = true; } // Chiamata ogni volta che cambi qualcosa nel 3D

// Inquadra un oggetto a prescindere dalla sua dimensione
function frameObject(obj, camera, fitOffset = 1.2) {
  const box = new THREE.Box3().setFromObject(obj);
  const sizeVec = box.getSize(new THREE.Vector3());
  const size = sizeVec.length();
  const center = box.getCenter(new THREE.Vector3());
  if (!isFinite(size) || size === 0) return;

  const halfFovY = (camera.fov * Math.PI / 180) / 2;
  const distance = (size * 0.5) / Math.tan(halfFovY);

  const dir = new THREE.Vector3(0, 0, 1); // scegli la direzione di osservazione di default
  camera.position.copy(center).add(dir.multiplyScalar(distance * fitOffset));
  camera.near = Math.max(0.01, size / 100);
  camera.far = Math.max(camera.near + 1, size * 100);
  camera.updateProjectionMatrix();
  camera.lookAt(center);
}

// Drag orbitale semplice attorno all'origine
function addDragControls(dom) {
  let isDragging = false, lastX = 0, lastY = 0;

  dom.addEventListener('pointerdown', e => {
    isDragging = true; lastX = e.clientX; lastY = e.clientY;
    dom.setPointerCapture?.(e.pointerId);
  });

  dom.addEventListener('pointermove', e => {
    if (!isDragging) return;
    const dx = e.clientX - lastX;
    const dy = e.clientY - lastY;
    lastX = e.clientX; lastY = e.clientY;
    const pivot = new THREE.Vector3(0, 0, 0); // centro 
    const camToPivot = camera.position.clone().sub(pivot);
    const sph = new THREE.Spherical().setFromVector3(camToPivot);
    sph.theta += -dx * 0.005;
    sph.phi = THREE.MathUtils.clamp(sph.phi + -dy * 0.005, 0.01, Math.PI - 0.01);
    camera.position.copy(pivot).add(new THREE.Vector3().setFromSpherical(sph));
    camera.lookAt(pivot);
    invalidate3D();
  });

  dom.addEventListener('pointerup', e => {
    isDragging = false;
    dom.releasePointerCapture?.(e.pointerId);
  });
}

const deg2rad = v => v * Math.PI / 180;
const useDeg = 1;
const EULER_ORDER = 'XYZ';          // Ordine applicazione  ZYX
const DEFAULT_AXIS_MAP = {          // Mappa raw -> (pitch,roll,yaw)
  fromX: 'pitch',  	// raw X -> pitch
  fromY: 'roll',	// raw Y -> roll
  fromZ: 'yaw', 	// raw Z -> yaw 
};

if (typeof robotTargetQuat === 'undefined') window.robotTargetQuat = new THREE.Quaternion();
if (typeof robotSmoothQuat === 'undefined') window.robotSmoothQuat = new THREE.Quaternion();
if (typeof SMOOTH_FACTOR === 'undefined') window.SMOOTH_FACTOR = 0.15;

// Utility: pick (annidati) e toNum (virgola decimale)
function pick(obj, ...paths) {
  for (const p of paths) {
    const v = p.split('.').reduce((o, k) => (o && o[k] != null ? o[k] : undefined), obj);
    if (v != null) return v;
  }
  return undefined;
}

function toNum(v) {
  if (v == null) return NaN;
  const n = parseFloat(String(v).replace(',', '.'));
  return Number.isFinite(n) ? n : NaN;
}

// ===== Rilevamento Euler =====
function tryReadEulerExplicit(msg) {
  // pitch/yaw/roll con sinonimi comuni
  const pitch = toNum(pick(msg, 'pitch', 'Pitch', 'PITCH', 'theta', 'euler.pitch', 'imu.pitch'));
  const roll = toNum(pick(msg, 'roll', 'Roll', 'ROLL', 'phi', 'euler.roll', 'imu.roll'));
  const yaw = toNum(pick(msg, 'yaw', 'Yaw', 'YAW', 'psi', 'euler.yaw', 'imu.yaw'));
  if ([pitch, roll, yaw].some(Number.isFinite)) {
    const arr = [pitch, roll, yaw].map(v => Number.isFinite(v) ? v : 0);
    const maxAbs = Math.max(...arr.map(v => Math.abs(v)));
    const x = useDeg ? deg2rad(arr[0]) : arr[0]; // pitch -> X
    const y = useDeg ? deg2rad(arr[1]) : arr[1]; // roll  -> Y 
    const z = useDeg ? deg2rad(arr[2]) : arr[2]; // yaw   -> Z
    return new THREE.Euler(x, y, z, EULER_ORDER);
  }
  return null;
}

function tryReadEulerRaw(msg) {
  // raw assi: Sens0/1/2, x/y/z, angX/Y/Z, euler.{x,y,z}
  const rx = toNum(pick(msg, 'Sens0', 'sens0', 'x', 'X', 'angX', 'euler.x', 'imu.x'));
  const ry = toNum(pick(msg, 'Sens1', 'sens1', 'y', 'Y', 'angY', 'euler.y', 'imu.y'));
  const rz = toNum(pick(msg, 'Sens2', 'sens2', 'z', 'Z', 'angZ', 'euler.z', 'imu.z'));
  if (![rx, ry, rz].every(v => Number.isNaN(v))) {
    const vals = [rx, ry, rz].map(v => Number.isFinite(v) ? v : 0);
    const maxAbs = Math.max(...vals.map(v => Math.abs(v)));
    const xRaw = useDeg ? deg2rad(vals[0]) : vals[0];
    const yRaw = useDeg ? deg2rad(vals[1]) : vals[1];
    const zRaw = useDeg ? deg2rad(vals[2]) : vals[2];
    const map = DEFAULT_AXIS_MAP;// Mappa raw -> pitch/roll/yaw (configurabile in DEFAULT_AXIS_MAP)
    let pitch = 0, yaw = 0, roll = 0;
    // assegna in base alla mappa
    const assign = (rawVal, meaning) => {
      if (meaning === 'pitch') pitch = rawVal;
      else if (meaning === 'yaw') yaw = rawVal;
      else if (meaning === 'roll') roll = rawVal;
    };
    assign(xRaw, map.fromX);
    assign(yRaw, map.fromY);
    assign(zRaw, map.fromZ);
    return new THREE.Euler(pitch, yaw, roll, EULER_ORDER); // (x,y,z) = (pitch,yaw,roll)
  }
  return null;
}

function update3dGyro(msg) {
  if (!robot) return;

  //Euler esplicito (pitch/yaw/roll) oppure Euler raw (sens0/1/2, x/y/z)
  const euler = tryReadEulerExplicit(msg) || tryReadEulerRaw(msg);
  if (!euler) return; // niente dati validi
  robotTargetQuat.setFromEuler(euler);

  // Smoothing (slerp)
  robotSmoothQuat.copy(robot.quaternion).slerp(robotTargetQuat, SMOOTH_FACTOR);
  robot.quaternion.copy(robotSmoothQuat);

  // Render on demand
  if (typeof invalidate3D === 'function') invalidate3D();
}

document.getElementById('btnReset3D')?.addEventListener('click', () => {
  robot?.rotation.set(0, 0, 0);
  frameObject(robot, camera);
  invalidate3D();
});

//utility se servono altrove
//window.frameObject = frameObject;
//window.update3dGyro = update3dGyro;

/**********************
 * DISPLAY over WebSocket
 **********************/
(function initDisplayOverWs() {
  const $ = sel => document.querySelector(sel);

  const previewCanvas = $('#disp_preview');
  const imageInput = $('#disp_imageInput');
  const invertChk = $('#disp_invert');
  const uploadBtn = $('#disp_uploadBtn');

  const ta = $('#disp_textarea');
  const sizeSel = $('#disp_size');
  const invertText = $('#disp_invertText');
  const truncateChk = $('#disp_truncate');
  const scrollSel = $('#disp_scroll');
  const delayInput = $('#disp_delay');
  const loopChk = $('#disp_loop');
  const sendTextBtn = $('#disp_sendText');

  const statusDiv = $('#disp_status');

  if (!previewCanvas || !imageInput || !uploadBtn || !ta || !sizeSel || !sendTextBtn) return;

  const ctx = previewCanvas.getContext('2d');
  const W = 128, H = 64;

  function setStatus(msg, ok = null) {
    statusDiv.textContent = msg || '';
    statusDiv.style.color = ok === null ? 'inherit' : (ok ? 'var(--good)' : 'var(--bad)');
  }

  // helper: bytes -> HEX string
  function bytesToHex(u8) {
    let out = '';
    for (let i = 0; i < u8.length; i++) {
      const h = u8[i].toString(16).padStart(2, '0');
      out += h;
    }
    return out.toUpperCase();
  }

  async function updatePreview() {
    const file = imageInput.files && imageInput.files[0];
    if (!file) { ctx.clearRect(0, 0, W, H); setStatus(''); return; }
    try {
      const bmp = await createImageBitmap(file);
      const tmp = document.createElement('canvas'); tmp.width = W; tmp.height = H;
      const tctx = tmp.getContext('2d');
      tctx.drawImage(bmp, 0, 0, W, H);

      const img = tctx.getImageData(0, 0, W, H);
      const inv = invertChk.checked;
      for (let i = 0; i < img.data.length; i += 4) {
        const brightness = (img.data[i] + img.data[i + 1] + img.data[i + 2]) / 3;
        let color = brightness > 127 ? 255 : 0;
        if (inv) color = color === 0 ? 255 : 0;
        img.data[i] = img.data[i + 1] = img.data[i + 2] = color;
      }
      ctx.putImageData(img, 0, 0);
      setStatus('Anteprima aggiornata');
    } catch (err) {
      console.error(err); setStatus('Errore anteprima: ' + err.message, false);
    }
  }

  async function buildMonochromeBytesFromFile(file, invert) {
    const tmp = document.createElement('canvas'); tmp.width = W; tmp.height = H;
    const tctx = tmp.getContext('2d');
    const bmp = await createImageBitmap(file);
    tctx.drawImage(bmp, 0, 0, W, H);
    const img = tctx.getImageData(0, 0, W, H);
    const bytes = new Uint8Array((W * H) / 8);
    for (let y = 0; y < H; y++) {
      for (let x = 0; x < W; x++) {
        const px = (y * W + x) * 4;
        const br = (img.data[px] + img.data[px + 1] + img.data[px + 2]) / 3;
        let isWhite = br > 127;
        if (invert) isWhite = !isWhite;
        if (isWhite) {
          const byteIndex = ((y * W) + x) >> 3;
          const bitIndex = x & 7;
          bytes[byteIndex] |= (1 << (7 - bitIndex));
        }
      }
    }
    return bytes;
  }

  async function uploadImage(bytes) {
    const url = `http://${add_host}/upload_image`;
    const blob = new Blob([bytes], { type: 'application/octet-stream' });
    const form = new FormData();
    form.append('image_file', blob, 'image.raw');
    const res = await fetch(url, { method: 'POST', body: form });
    if (!res.ok) {
      const txt = await res.text().catch(() => '');
      throw new Error(`HTTP ${res.status}: ${txt || 'errore sconosciuto'}`);
    }
  }

  async function processAndUploadImage() {
    const file = imageInput.files && imageInput.files[0];
    if (!file) {
      setStatus('Seleziona un file prima di procedere.', false);
      return;
    }
    try {
      setStatus('Elaborazione e invio in corso...');
      const bytes = await buildMonochromeBytesFromFile(file, invertChk.checked);
      await uploadImage(bytes);
      setStatus('Immagine inviata con successo!', true);
    } catch (err) {
      console.error('Upload fallito:', err);
      setStatus(`Errore: ${err.message}`, false);
      alert('Si è verificato un errore. Controlla la console per i dettagli.');
    }
  }

  async function onUploadImage() {
    const file = imageInput.files && imageInput.files[0];
    if (!file) { setStatus('Seleziona un file.', false); return; }

    try {
      setStatus('Elaborazione…');
      const bytes = await buildMonochromeBytesFromFile(file, invertChk.checked);
      const hex = bytesToHex(bytes);
      const msg = { cmd: 'uploadimage', newimage: hex };
      sendJson(msg);
      setStatus('Comando inviato (uploadimage). Attendi rendering sul device…', true);
    } catch (err) {
      console.error(err); setStatus('Errore: ' + err.message, false);
    }
  }

  function onSendText() {
    const raw = (ta.value || '').replace(/\r\n/g, '\n');
    const lines = raw.split('\n').filter(s => s.length > 0).slice(0, 16); // fino a 16
    if (lines.length === 0) {
      setStatus('Inserisci almeno una riga.', false);
      return;
    }

    const size = parseInt(sizeSel.value || '1', 10) || 1;
    const invert = invertText.checked ? 1 : 0;
    const truncate = truncateChk.checked ? 1 : 0;
    const scroll = parseInt(scrollSel.value || '1', 10) || 0;
    const delay = Math.max(1, parseInt(delayInput.value || '500', 10) || 500);
    const loop = loopChk.checked ? 1 : 0;
    const msg = { CMD: 'displaymsg', size, invert, truncate, scroll, delay, loop, strings: lines };
    sendJson(msg);
    setStatus(`Comando inviato (displaymsg) – righe:${lines.length}, size:${size}${scroll ? ', scroll ON' : ''}`, true);
  }

  // wiring
  imageInput.addEventListener('change', updatePreview);
  invertChk.addEventListener('change', updatePreview);
  uploadBtn.addEventListener('click', processAndUploadImage);
  sendTextBtn.addEventListener('click', onSendText);
})();


function boot() {
  buildInfo();
  buildSensors();
  buildFnButtons();
  joyInit();
  buildOTA();
  connectWS();
  setupModelColorUI();
}
window.addEventListener('load', () => {
  boot();
  if (lastSensorPayload) updateSensors(lastSensorPayload);
});