const THEME_KEY = 'dmr.theme';
const root = document.documentElement;

function applyTheme() {
  const t = localStorage.getItem(THEME_KEY) || 'dark';
  root.classList.toggle('light', t === 'light');
}
applyTheme();
document.getElementById('theme').addEventListener('click', () => {
  const t = localStorage.getItem(THEME_KEY) === 'light' ? 'dark' : 'light';
  localStorage.setItem(THEME_KEY, t);
  applyTheme();
});

async function fetchJSON(url) {
  const r = await fetch(url, { cache: 'no-store' });
  if (!r.ok) throw new Error(`HTTP ${r.status}`);
  return r.json();
}
async function fetchText(url) {
  const r = await fetch(url, { cache: 'no-store' });
  if (!r.ok) throw new Error(`HTTP ${r.status}`);
  return r.text();
}
function td(v, cls) {
  const e = document.createElement('td');
  if (cls) e.className = cls;
  e.textContent = v;
  return e;
}

function badge(txt, cls) {
  const s = document.createElement('span');
  s.className = 'badge ' + (cls || '');
  s.textContent = txt;
  return s;
}
function badgesFor(r) {
  const wrap = document.createElement('div');
  if (r.src === 2) wrap.append(badge('OpenBridge', 'ob'));
  else if (r.aprs) wrap.append(badge('APRS', 'aprs'));
  else if (r.sms) wrap.append(badge('SMS', 'sms'));
  else wrap.append(badge('TALK', 'talk'));
  if (!wrap.childNodes.length) wrap.append(document.createTextNode('—'));
  return wrap;
}

async function renderActive() {
  try {
    const data = await fetchJSON('/api/active');
    const tb = document.querySelector('#tab-active tbody');
    tb.innerHTML = '';

    for (const r of data) {
      const tr = document.createElement('tr');
      tr.append(td(r.date || ''));
      tr.append(td(r.radio ?? ''));
	  tr.append(td('').appendChild(badgesFor(r)).parentNode);
      tr.append(td(r.tg ?? ''));
      tr.append(td(r.slot ?? ''));
      tr.append(td(r.node ?? ''));
      tr.append(td(r.time ?? ''));
      tb.append(tr);
    }

    const card = document.querySelector('#tab-active').closest('.card');
    card.classList.toggle('is-active', data.length > 0);

  } catch (e) {
    console.error('active:', e);
  }
}

async function renderLog() {
  try {
    const data = await fetchJSON('/api/log?limit=20');
    const tb = document.querySelector('#tab-log tbody');
    tb.innerHTML = '';
    for (const r of data) {
      const tr = document.createElement('tr');
      tr.append(td(r.id ?? ''));
      tr.append(td(r.date || ''));
      tr.append(td(r.radio ?? ''));
	  tr.append(td('').appendChild(badgesFor(r)).parentNode);
      tr.append(td(r.tg ?? ''));
      tr.append(td(r.slot ?? ''));
      tr.append(td(r.node ?? ''));
      tr.append(td(r.time ?? ''));
      tr.append(td(r.active ?? ''));
	  tr.append(td(r.online ? 'YES' : 'NO'));
      tr.append(td(r.connect ?? ''));
      tb.append(tr);
    }
  } catch (e) {
    console.error('log:', e);
  }
}

async function renderStat() {
  try {
    const s = await fetchText('/api/stat');
    document.getElementById('stat').textContent = s;
  } catch (e) {
    document.getElementById('stat').textContent = 'No /STAT reply';
  }
}

function tick() {
  renderActive();
  renderLog();
  renderStat();
}

setInterval(tick, 3000);
tick();
