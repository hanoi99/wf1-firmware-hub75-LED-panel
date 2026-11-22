// Fuehrt data/app.js UNVERAENDERT gegen die echte Firmware-Antwort (build/web_contract) aus.
// Fake-DOM statt Browser: findet Format-/Namensfehler zwischen UI und Firmware.
const { spawn } = require('child_process');
const fs = require('fs');
const path = require('path');
const vm = require('vm');
const root = path.join(__dirname, '..', '..', '..');
const html = fs.readFileSync(path.join(root, 'data', 'index.html'), 'utf8');
const appJs = fs.readFileSync(path.join(root, 'data', 'app.js'), 'utf8');

// --- 1. statisch: jede $('#id') im JS muss im HTML existieren -------------------------------
const ids = new Set([...html.matchAll(/id="([\w-]+)"/g)].map((m) => m[1]));
const missingIds = [...new Set([...appJs.matchAll(/\$\('#([\w-]+)/g)].map((m) => m[1]))].filter((i) => !ids.has(i));

// --- 2. Firmware als Kindprozess --------------------------------------------------------------
const child = spawn(path.join(root, 'build', 'web_contract'), [], { stdio: ['pipe', 'pipe', 'inherit'] });
let buf = ''; const waiters = [];
child.stdout.on('data', (d) => {
  buf += d;
  let i;
  while ((i = buf.indexOf('\n')) >= 0) {
    const line = buf.slice(0, i); buf = buf.slice(i + 1);
    if (/^\d{3}\t/.test(line) && waiters.length) waiters.shift()(line);
  }
});
const rawCall = (method, uri, body) => new Promise((res) => { waiters.push(res); child.stdin.write(`${method}\t${uri}\t${body || ''}\n`); });
async function fetchShim(url, opt) {
  const method = (opt && opt.method) || 'GET';
  const body = opt && opt.body ? String(opt.body) : '';
  const line = await rawCall(method, url, body);
  const [code, , ...rest] = line.split('\t');
  const text = rest.join('\t');
  return { ok: +code >= 200 && +code < 300, status: +code, json: async () => JSON.parse(text) };
}

// --- 3. Fake-DOM -----------------------------------------------------------------------------
const cache = {}; const log = [];
function makeEl(tag) {
  const cls = new Set();
  const el = {
    tag, children: [], dataset: {}, style: {}, value: '', textContent: '', checked: false, hidden: false, className: '',
    classList: { add: (c) => cls.add(c), remove: (c) => cls.delete(c), contains: (c) => cls.has(c), toggle: (c, f) => { (f === undefined ? !cls.has(c) : f) ? cls.add(c) : cls.delete(c); } },
    append(...n) { el.children.push(...n); }, replaceChildren(...n) { el.children = n; }, add(o) { el.children.push(o); },
    addEventListener(t, f) { el['on' + t] = f; }, closest() { return el; }, querySelector() { return makeEl('q'); },
  };
  Object.defineProperty(el, 'innerHTML', { set() { el.children = [makeEl('s'), makeEl('s'), makeEl('s')]; }, get() { return ''; } });
  return el;
}
const sel = (s) => (cache[s] = cache[s] || makeEl(s));
const messages = [];
const sandbox = {
  document: { querySelector: sel, querySelectorAll: () => [], createElement: makeEl, createTextNode: (t) => ({ text: t }), activeElement: null },
  Option: function (text, value) { return { text, value }; },
  fetch: fetchShim, URLSearchParams, TextEncoder, setInterval: () => 0, setTimeout, console,
};
vm.createContext(sandbox);
const get = (expr) => vm.runInContext(expr, sandbox);
const fails = [];
const check = (cond, msg) => { if (!cond) fails.push(msg); else console.log('  ok  ' + msg); };

(async () => {
  console.log('Statische Pruefung');
  check(missingIds.length === 0, 'alle IDs aus app.js existieren in index.html' + (missingIds.length ? ' (fehlt: ' + missingIds.join(', ') + ')' : ''));

  console.log('Laden');
  vm.runInContext(appJs, sandbox);
  await new Promise((r) => setTimeout(r, 400));
  const conn = sel('#connection');
  check(conn.classList.contains('online'), 'Verbindungsanzeige "online" (nicht "Nicht erreichbar")');
  const status = get('status');
  check(status && status.scenes.length === 13, 'Status geladen, 13 Szenen');
  check(sel('#scene-groups').children.length > 0, 'Szenenliste aufgebaut');
  const cards = sel('#scene-groups').children.filter((c) => c.className === 'scene-grid').reduce((n, g) => n + g.children.length, 0);
  check(cards === 13, 'alle 13 Szenenkarten erzeugt (' + cards + ')');
  check(sel('#active-title').textContent === 'Lagerfeuer', 'aktive Szene "Lagerfeuer" angezeigt');
  check(sel('#brightness').value === 96 || sel('#brightness').value == 96, 'Helligkeit gesetzt');
  check(/Bytes/.test(sel('#counter').textContent) && !/undefined/.test(sel('#counter').textContent), 'Text-Zaehler ohne "undefined": ' + sel('#counter').textContent);
  check(/Heap/.test(sel('#sys').textContent) && !/NaN|undefined/.test(sel('#sys').textContent), 'Systemzeile: ' + sel('#sys').textContent);
  check(sel('#options').children.length >= 3, 'Optionen der Feuer-Szene (' + sel('#options').children.length + ')');

  console.log('Bedienung');
  const click = async (selector, target) => { await sel(selector).onclick({ target }); await new Promise((r) => setTimeout(r, 150)); };
  const lastMsg = () => sel('#message-status').textContent;
  // Szene waehlen (Klick auf Karte im Container)
  const card = { dataset: { scene: 'clock' }, closest: () => card };
  await sel('#scene-groups').onclick({ target: card }); await new Promise((r) => setTimeout(r, 200));
  check(get('status.scene') === 'clock' && !/Fehler/.test(lastMsg()), 'Szene "clock" aktiviert: ' + lastMsg());
  // Text
  sel('#message').value = 'Grüße & Äpfel + ß';
  await sel('#text-send').onclick(); await new Promise((r) => setTimeout(r, 200));
  check(get('status.scene') === 'text' && get('status.text') === 'Grüße & Äpfel + ß', 'Text gesendet, Szene "text", Umlaute/&/+ unversehrt: ' + lastMsg());
  // Option aendern (erste Option der Text-Szene = Schriftart select)
  const optEl = sel('#options').children[0].children[1];
  optEl.value = '3'; await optEl.onchange(); await new Promise((r) => setTimeout(r, 150));
  await rawCall('GET', '/api/status').then((l) => { const j = JSON.parse(l.split('\t').slice(2).join('\t')); check(j.scenes[0].opts[0].v === 3, 'Option Schriftart=3 gespeichert: ' + lastMsg()); });
  // Helligkeit
  sel('#brightness').value = '120'; await sel('#brightness').onchange({ target: { value: '120' } }); await new Promise((r) => setTimeout(r, 150));
  await get('loadAll()'); check(get('status.brightness') === 120, 'Helligkeit 120 gespeichert: ' + lastMsg());
  // Demo (leere ID-Liste = alle Szenen -> alle Haken gesetzt)
  check(get("status.demo.ids") === '' , 'Demo-Liste initial leer (= alle)');
  sel('#demo-on').checked = true; await sel('#demo-on').onchange({ target: { checked: true } }); await new Promise((r) => setTimeout(r, 200));
  check(get('status.demo.on') === true && !/Fehler/.test(lastMsg()), 'Demo-Modus eingeschaltet: ' + lastMsg() + ' ids="' + get('status.demo.ids') + '"');
  sel('#demo-sec').value = '45'; await sel('#demo-sec').onchange(); await new Promise((r) => setTimeout(r, 200));
  check(get('status.demo.sec') === 45, 'Demo-Intervall 45 s gespeichert');
  // Poll
  await get('poll()'); check(conn.classList.contains('online'), 'Polling (/api/state) funktioniert');
  // WLAN
  await sel('#wifi-scan').onclick(); await new Promise((r) => setTimeout(r, 1500));
  check(!/Fehler/.test(lastMsg()), 'WLAN-Suche: ' + lastMsg());
  check(sel('#wifi-list').children.length === 2, 'WLAN-Liste nach Suche (2 Netze)');
  sel('#wifi-ssid').value = 'Heimnetz'; sel('#wifi-pass').value = 'geheim1234';
  await sel('#wifi-save').onclick(); await new Promise((r) => setTimeout(r, 150));
  check(/verbunden/i.test(lastMsg()), 'WLAN speichern: ' + lastMsg());

  const fg = await rawCall('POST', '/api/wifi/forget', ''); check(fg.startsWith('200'), 'WLAN vergessen');
  const wj = JSON.parse((await rawCall('GET', '/api/wifi')).split('\t').slice(2).join('\t')); check(wj.ssid === '', 'gespeicherte SSID ist leer');
  // Display-Tuning
  check(get('status.disp.hz') > 0 && get('status.disp.spd') === 8 && get('status.disp.lat') === 12, 'Status enthaelt disp-Block: ' + sel('#disp-info').textContent);
  sel('#disp-spd').value = '20'; sel('#disp-lat').value = '12'; sel('#disp-clk').value = '0'; sel('#disp-db').value = '0'; sel('#disp-hz').value = '120';
  await sel('#disp-save').onclick(); await new Promise((r) => setTimeout(r, 200));
  check(/neu/i.test(lastMsg()), 'Display-Tuning speichern: ' + lastMsg());
  const bad = await rawCall('POST', '/api/display', 'spd=12'); check(bad.startsWith('400'), 'ungueltiger Takt wird abgelehnt');

  child.kill();
  console.log(fails.length ? '\nFEHLGESCHLAGEN:\n - ' + fails.join('\n - ') : '\nUI-KONTRAKTTEST OK');
  process.exit(fails.length ? 1 : 0);
})().catch((e) => { console.error('Testfehler', e); child.kill(); process.exit(2); });
