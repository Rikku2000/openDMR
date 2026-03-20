const THEME_KEY = 'dmr.theme';
const root = document.documentElement;
const page = document.body.dataset.page || 'dashboard';

function applyTheme() {
  const theme = localStorage.getItem(THEME_KEY) || 'dark';
  root.classList.toggle('light', theme === 'light');
}

function initThemeToggle() {
  applyTheme();
  const themeBtn = document.getElementById('theme');
  if (!themeBtn) return;
  themeBtn.addEventListener('click', () => {
    const current = localStorage.getItem(THEME_KEY) || 'dark';
    const next = current === 'light' ? 'dark' : 'light';
    localStorage.setItem(THEME_KEY, next);
    applyTheme();
  });
}

async function fetchJSON(url) {
  const response = await fetch(url, { cache: 'no-store' });
  if (!response.ok) throw new Error(`HTTP ${response.status}`);
  return response.json();
}

async function fetchText(url) {
  const response = await fetch(url, { cache: 'no-store' });
  if (!response.ok) throw new Error(`HTTP ${response.status}`);
  return response.text();
}

function setText(id, value) {
  const el = document.getElementById(id);
  if (el) el.textContent = value;
}

function td(value, className = '') {
  const cell = document.createElement('td');
  if (className) cell.className = className;
  cell.textContent = value;
  return cell;
}

function badge(text, cls) {
  const span = document.createElement('span');
  span.className = `badge ${cls || ''}`.trim();
  span.textContent = text;
  return span;
}

function badgesFor(record) {
  const wrap = document.createElement('div');
  wrap.className = 'badge-stack';

  if (record.src === 2) wrap.append(badge('OpenBridge', 'ob'));
  else if (record.aprs) wrap.append(badge('APRS', 'aprs'));
  else if (record.sms) wrap.append(badge('SMS', 'sms'));
  else wrap.append(badge('Talk', 'talk'));

  return wrap;
}

function localNowParts() {
  const now = new Date();
  const time = now.toLocaleTimeString([], {
    hour: '2-digit',
    minute: '2-digit',
    second: '2-digit'
  });
  const date = now.toLocaleDateString([], {
    weekday: 'short',
    year: 'numeric',
    month: 'short',
    day: 'numeric'
  });
  return { time, date };
}

function renderClock() {
  const { time, date } = localNowParts();
  document.querySelectorAll('#clock-time').forEach((el) => { el.textContent = time; });
  document.querySelectorAll('#clock-date').forEach((el) => { el.textContent = date; });
}

function uniqueNonEmpty(values) {
  return [...new Set(values.filter((value) => value !== '' && value != null))];
}

function updateSummaryRows(rows) {
  const host = document.getElementById('activity-summary');
  if (!host) return;
  host.innerHTML = '';

  if (!rows.length) {
    const empty = document.createElement('div');
    empty.className = 'summary-row';
    empty.innerHTML = '<span>No log data yet</span><strong>—</strong>';
    host.append(empty);
    return;
  }

  for (const row of rows) {
    const div = document.createElement('div');
    div.className = 'summary-row';
    div.innerHTML = `<span>${row.label}</span><strong>${row.value}</strong>`;
    host.append(div);
  }
}

function updateHero(activeCount, lastDate) {
  const heroStatus = document.getElementById('hero-status');
  const heroSummary = document.getElementById('hero-summary');
  const activePanel = document.getElementById('active-panel');
  const activeMeta = document.getElementById('active-meta');

  if (!heroStatus || !heroSummary) return;

  if (activeCount > 0) {
    heroStatus.textContent = 'Live traffic detected';
    heroStatus.className = 'chip chip-bad';
    heroSummary.textContent = `${activeCount} active transmission${activeCount === 1 ? '' : 's'} right now`;
    activePanel?.classList.add('is-live');
    if (activeMeta) activeMeta.textContent = `${activeCount} active now`;
  } else {
    heroStatus.textContent = 'Network idle';
    heroStatus.className = 'chip chip-soft';
    heroSummary.textContent = lastDate ? `Last seen activity: ${lastDate}` : 'No active transmissions at the moment';
    activePanel?.classList.remove('is-live');
    if (activeMeta) activeMeta.textContent = 'No active calls';
  }
}

function updateRefresh() {
  const { time } = localNowParts();
  setText('metric-refresh', time);
  setText('metric-refresh-sub', 'Synced just now');
  setText('monitor-updated', `Updated ${time}`);
}

async function renderActive() {
  const tbody = document.querySelector('#tab-active tbody');
  if (!tbody) return;

  try {
    const data = await fetchJSON('/api/active');
    tbody.innerHTML = '';

    if (!data.length) {
      const tr = document.createElement('tr');
      const cell = document.createElement('td');
      cell.colSpan = 7;
      cell.textContent = 'No active transmissions';
      tr.append(cell);
      tbody.append(tr);
    } else {
      for (const row of data) {
        const tr = document.createElement('tr');
        tr.append(td(row.date || '—'));
        tr.append(td(row.radio ?? '—', 'emphasis'));
        const badgeCell = td('');
        badgeCell.append(badgesFor(row));
        tr.append(badgeCell);
        tr.append(td(row.tg ?? '—'));
        tr.append(td(row.slot ?? '—'));
        tr.append(td(row.node ?? '—'));
        tr.append(td(row.time ?? '—', 'numeric'));
        tbody.append(tr);
      }
    }

    setText('metric-active', String(data.length));
    setText('metric-active-sub', data.length ? 'Live transmissions on-air now' : 'No live activity');
    updateHero(data.length, data[0]?.date || '');
  } catch (error) {
    console.error('active:', error);
    tbody.innerHTML = '';
    const tr = document.createElement('tr');
    const cell = document.createElement('td');
    cell.colSpan = 7;
    cell.textContent = 'Unable to load active calls';
    tr.append(cell);
    tbody.append(tr);
    updateHero(0, '');
  }
}

async function renderLog() {
  const tbody = document.querySelector('#tab-log tbody');
  if (!tbody) return;

  try {
    const data = await fetchJSON('/api/log?limit=20');
    tbody.innerHTML = '';

    if (!data.length) {
      const tr = document.createElement('tr');
      const cell = document.createElement('td');
      cell.colSpan = 11;
      cell.textContent = 'No recent log entries';
      tr.append(cell);
      tbody.append(tr);
    } else {
      for (const row of data) {
        const tr = document.createElement('tr');
        tr.append(td(row.id ?? '—', 'numeric'));
        tr.append(td(row.date || '—'));
        tr.append(td(row.radio ?? '—', 'emphasis'));
        const badgeCell = td('');
        badgeCell.append(badgesFor(row));
        tr.append(badgeCell);
        tr.append(td(row.tg ?? '—'));
        tr.append(td(row.slot ?? '—'));
        tr.append(td(row.node ?? '—'));
        tr.append(td(row.time ?? '—', 'numeric'));
        tr.append(td(row.active ?? '—', row.active ? 'bad' : ''));
        tr.append(td(row.online ? 'YES' : 'NO', row.online ? 'ok' : ''));
        tr.append(td(row.connect ?? '—'));
        tbody.append(tr);
      }
    }

    const uniqueTGs = uniqueNonEmpty(data.map((row) => row.tg));
    const onlineCount = data.filter((row) => row.online).length;
    const openBridgeCount = data.filter((row) => row.src === 2).length;
    const aprsCount = data.filter((row) => row.aprs).length;
    const smsCount = data.filter((row) => row.sms).length;
    const talkCount = Math.max(data.length - openBridgeCount - aprsCount - smsCount, 0);

    setText('metric-talkgroups', String(uniqueTGs.length));
    setText('metric-talkgroups-sub', uniqueTGs.length ? `Latest: ${uniqueTGs.slice(0, 3).join(', ')}` : 'Across latest 20 records');
    setText('metric-online', String(onlineCount));
    setText('metric-online-sub', data.length ? `Out of ${data.length} recent records` : 'Seen in recent activity');
    setText('log-meta', data.length ? `${data.length} rows loaded` : 'No recent activity');

    updateSummaryRows([
      { label: 'OpenBridge events', value: String(openBridgeCount) },
      { label: 'APRS events', value: String(aprsCount) },
      { label: 'SMS events', value: String(smsCount) },
      { label: 'Voice / talk events', value: String(talkCount) }
    ]);
  } catch (error) {
    console.error('log:', error);
    tbody.innerHTML = '';
    const tr = document.createElement('tr');
    const cell = document.createElement('td');
    cell.colSpan = 11;
    cell.textContent = 'Unable to load recent log';
    tr.append(cell);
    tbody.append(tr);
    updateSummaryRows([]);
  }
}

async function renderStat() {
  const statEl = document.getElementById('stat');
  if (!statEl) return;

  try {
    const text = await fetchText('/api/stat');
    statEl.textContent = text || 'No /STAT reply';

    const lines = text.split('\n').filter(Boolean);
    setText('monitor-status', text ? 'STAT connected' : 'STAT idle');
    const statusChip = document.getElementById('monitor-status');
    if (statusChip) statusChip.className = text ? 'chip' : 'chip chip-warn';
    setText('stat-meta', lines.length ? `${lines.length} lines received` : 'Empty reply');
  } catch (error) {
    console.error('stat:', error);
    statEl.textContent = 'No /STAT reply';
    setText('monitor-status', 'STAT unavailable');
    const statusChip = document.getElementById('monitor-status');
    if (statusChip) statusChip.className = 'chip chip-bad';
    setText('stat-meta', 'Request failed');
  }
}

async function tick() {
  renderClock();
  updateRefresh();

  const jobs = [];
  if (page === 'dashboard') {
    jobs.push(renderActive(), renderLog());
  }
  if (page === 'monitor') {
    jobs.push(renderStat());
  }

  await Promise.allSettled(jobs);
}

initThemeToggle();
renderClock();
tick();
setInterval(renderClock, 1000);
setInterval(tick, 3000);
