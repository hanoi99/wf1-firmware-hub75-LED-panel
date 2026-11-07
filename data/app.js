const $ = (s) => document.querySelector(s);
const say = (m) => { $('#message-status').textContent = m; };
const CATS = { text: 'Text', info: 'Information', effect: 'Effekte' };
const ICONS = { text: 'Aa', clock: '◷', world: '🌍', iss: '🛰', fire: '🔥', rain: '▥', stars: '✦', warp: '➤', plasma: '◍', ascii: '#', mandel: '❂' };

let status = null;      // letzter /api/status
let sceneSig = '';

async function api(path, values) {
  const opt = values ? { method: 'POST', headers: { 'Content-Type': 'application/x-www-form-urlencoded' }, body: new URLSearchParams(values) } : { cache: 'no-store' };
  const r = await fetch(path, opt);
  const j = await r.json().catch(() => ({}));
  if (!r.ok || j.ok === false) throw new Error(j.error || 'Fehler ' + r.status);
  return j;
}
const fail = (e) => say('Fehler: ' + e.message);
const active = () => status && status.scenes.find((s) => s.id === status.scene);

function renderScenes() {
  const box = $('#scene-groups');
  box.replaceChildren();
  for (const cat of ['text', 'info', 'effect']) {
    const list = status.scenes.filter((s) => s.cat === cat);
    if (!list.length) continue;
    const t = document.createElement('p'); t.className = 'group-title'; t.textContent = CATS[cat];
    const grid = document.createElement('div'); grid.className = 'scene-grid';
    for (const s of list) {
      const b = document.createElement('button');
      b.type = 'button'; b.className = 'scene-card' + (s.avail ? '' : ' off'); b.dataset.scene = s.id;
      b.innerHTML = '<span class="scene-icon"></span><strong></strong><small></small>';
      b.children[0].textContent = ICONS[s.id] || '✧';
      b.children[1].textContent = s.title;
      b.children[2].textContent = s.avail ? '' : 'nicht verfügbar';
      grid.append(b);
    }
    box.append(t, grid);
  }
}

function control(sceneId, o) {
  const wrap = document.createElement('div');
  const label = document.createElement('label'); label.textContent = o.l;
  let el, out;
  const send = async (v) => { try { await api('/api/option', { scene: sceneId, key: o.k, value: v }); o.v = Number(v); say('Gespeichert'); } catch (e) { fail(e); } };
  if (o.t === 'select') {
    el = document.createElement('select');
    o.c.split('|').forEach((txt, i) => el.add(new Option(txt, i)));
    el.value = o.v; el.onchange = () => send(el.value);
  } else if (o.t === 'range') {
    el = document.createElement('input'); el.type = 'range'; el.min = o.min; el.max = o.max; el.value = o.v;
    out = document.createElement('output'); out.textContent = ' ' + o.v; label.append(out);
    el.oninput = () => { out.textContent = ' ' + el.value; };
    el.onchange = () => send(el.value);
  } else if (o.t === 'toggle') {
    label.className = 'chip'; label.textContent = '';
    el = document.createElement('input'); el.type = 'checkbox'; el.checked = !!o.v;
    el.onchange = () => send(el.checked ? 1 : 0);
    label.append(el, document.createTextNode(o.l)); wrap.append(label); return wrap;
  } else {
    el = document.createElement('input'); el.type = 'color'; el.value = '#' + (o.v >>> 0).toString(16).padStart(6, '0');
    el.onchange = () => send(parseInt(el.value.slice(1), 16));
  }
  el.dataset.key = o.k;
  wrap.append(label, el);
  return wrap;
}

function renderSettings() {
  const s = active();
  const panel = $('#settings-panel');
  if (!s || (!s.opts.length && s.id !== 'text')) { panel.hidden = true; return; }
  panel.hidden = false;
  $('#settings-title').textContent = s.title;
  $('#text-box').hidden = s.id !== 'text';
  const box = $('#options');
  box.replaceChildren(...s.opts.map((o) => control(s.id, o)));
}

function renderDemo() {
  const d = status.demo;
  $('#demo-on').checked = d.on;
  if (document.activeElement !== $('#demo-sec')) { $('#demo-sec').value = d.sec; $('#demo-sec-out').textContent = d.sec; }
  const ids = d.ids.split(',');
  const box = $('#demo-ids');
  box.replaceChildren(...status.scenes.map((s) => {
    const l = document.createElement('label'); l.className = 'chip';
    const c = document.createElement('input'); c.type = 'checkbox'; c.value = s.id; c.checked = ids.includes(s.id);
    l.append(c, document.createTextNode(s.title)); return l;
  }));
}

function renderActive() {
  document.querySelectorAll('[data-scene]').forEach((b) => b.classList.toggle('active', b.dataset.scene === status.scene));
  const a = active();
  $('#active-title').textContent = a ? a.title : '–';
  $('#live').textContent = status.demo.on ? 'DEMO' : 'LIVE';
  $('#live').classList.toggle('demo', status.demo.on);
}

function fmtUp(s) { return s < 3600 ? Math.floor(s / 60) + ' min' : Math.floor(s / 3600) + ' h'; }
function renderSys() {
  const y = status.sys;
  $('#version').textContent = status.version;
  $('#sys').textContent = `Heap ${(y.heap / 1024).toFixed(0)} KB · WLAN ${y.rssi ? y.rssi + ' dBm' : (y.ap ? 'Setup-AP' : 'offline')} · ${fmtUp(y.up)}`;
}

async function loadAll() {
  try {
    status = await api('/api/status');
    $('#connection').classList.add('online');
    $('#connection span:last-child').textContent = status.sys.ip;
    sceneSig = status.scene;
    renderScenes(); renderActive(); renderSettings(); renderDemo(); renderSys();
    if (document.activeElement !== $('#message')) { $('#message').value = status.text; updateCounter(); }
    $('#brightness').max = status.maxBrightness;
    if (document.activeElement !== $('#brightness')) { $('#brightness').value = status.brightness; $('#brightness-value').textContent = status.brightness; }
  } catch (e) {
    $('#connection').classList.remove('online');
    $('#connection span:last-child').textContent = 'Nicht erreichbar';
  }
}

async function poll() {
  try {
    const st = await api('/api/state');
    $('#connection').classList.add('online');
    if (st.scene !== sceneSig || st.demo !== status.demo.on) { await loadAll(); return; }
    const a = active(); if (a) a.note = st.note;
    status.sys.heap = st.heap; status.sys.rssi = st.rssi; status.sys.up = st.up; renderSys();
    const n = st.note ? ' · ' + st.note : '';
    $('#active-title').textContent = (a ? a.title : '–') + n;
  } catch (e) { $('#connection').classList.remove('online'); $('#connection span:last-child').textContent = 'Nicht erreichbar'; }
}

function updateCounter() { $('#counter').textContent = `(${new TextEncoder().encode($('#message').value).length}/${status ? status.maxText : 240} Bytes)`; }
$('#message').addEventListener('input', updateCounter);

$('#scene-groups').addEventListener('click', async (e) => {
  const b = e.target.closest('[data-scene]'); if (!b) return;
  try { await api('/api/scene', { id: b.dataset.scene }); say('Szene aktiviert'); await loadAll(); } catch (er) { fail(er); }
});

$('#text-send').addEventListener('click', async () => {
  try { await api('/api/text', { text: $('#message').value }); await api('/api/scene', { id: 'text' }); say('Text angezeigt'); await loadAll(); } catch (e) { fail(e); }
});

$('#brightness').addEventListener('input', (e) => { $('#brightness-value').textContent = e.target.value; });
$('#brightness').addEventListener('change', async (e) => { try { await api('/api/brightness', { value: e.target.value }); say('Helligkeit gespeichert'); } catch (er) { fail(er); } });

async function sendDemo(extra) {
  const ids = [...document.querySelectorAll('#demo-ids input:checked')].map((c) => c.value).join(',');
  try { await api('/api/demo', Object.assign({ sec: $('#demo-sec').value, ids }, extra)); say('Demo gespeichert'); await loadAll(); } catch (e) { fail(e); }
}
$('#demo-on').addEventListener('change', (e) => sendDemo({ on: e.target.checked ? 1 : 0 }));
$('#demo-sec').addEventListener('input', (e) => { $('#demo-sec-out').textContent = e.target.value; });
$('#demo-sec').addEventListener('change', () => sendDemo({}));
$('#demo-ids').addEventListener('change', () => sendDemo({}));

async function wifiRefresh() {
  try {
    const w = await api('/api/wifi');
    $('#wifi-info').textContent = w.connected ? `Verbunden mit „${w.ssid}“ (${w.ip})` : (w.ap ? `Nicht verbunden – Setup-AP aktiv (${w.ip})` : 'Nicht verbunden');
    if (document.activeElement !== $('#wifi-ssid') && !$('#wifi-ssid').value) $('#wifi-ssid').value = w.ssid;
    $('#wifi-list').replaceChildren(...w.networks.map((n) => new Option(`${n.rssi} dBm${n.open ? ' (offen)' : ''}`, n.ssid)));
    return w;
  } catch (e) { return null; }
}
$('#wifi-scan').addEventListener('click', async () => {
  try { await api('/api/wifi/scan', {}); say('Suche …'); for (let i = 0; i < 12; i++) { await new Promise((r) => setTimeout(r, 1000)); const w = await wifiRefresh(); if (w && !w.scanning) break; } say('Suche beendet'); } catch (e) { fail(e); }
});
$('#wifi-save').addEventListener('click', async () => {
  try { await api('/api/wifi', { ssid: $('#wifi-ssid').value, pass: $('#wifi-pass').value }); $('#wifi-pass').value = ''; say('WLAN wird verbunden …'); setTimeout(wifiRefresh, 6000); } catch (e) { fail(e); }
});

loadAll().then(wifiRefresh);
setInterval(poll, 4000);
