const THEME_KEY = 'dmr.theme';
const AUTH_TOKEN_KEY = 'dmr.authToken';
const AUTH_USER_KEY = 'dmr.authUser';

const root = document.documentElement;
const page = document.body.dataset.page || 'dashboard';
let runtimeConfig = { authEnabled: false, registrationEnabled: false, profileEnabled: false, dmrIdsFile: '', serverVersion: '' };
let authState = { token: '', user: null };
let authPromptedOnProfile = false;

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

function loadAuthState() {
  try {
    authState.token = localStorage.getItem(AUTH_TOKEN_KEY) || '';
    const rawUser = localStorage.getItem(AUTH_USER_KEY);
    authState.user = rawUser ? JSON.parse(rawUser) : null;
  } catch (error) {
    console.error('auth localStorage:', error);
    authState = { token: '', user: null };
  }
}

function saveAuthState() {
  if (authState.token) localStorage.setItem(AUTH_TOKEN_KEY, authState.token);
  else localStorage.removeItem(AUTH_TOKEN_KEY);

  if (authState.user) localStorage.setItem(AUTH_USER_KEY, JSON.stringify(authState.user));
  else localStorage.removeItem(AUTH_USER_KEY);
}

function clearAuthState() {
  authState = { token: '', user: null };
  saveAuthState();
}

async function fetchJSON(url, options = {}) {
  const headers = new Headers(options.headers || {});
  if (!headers.has('Cache-Control')) headers.set('Cache-Control', 'no-store');
  if (authState.token && !headers.has('X-Auth-Token')) headers.set('X-Auth-Token', authState.token);
  const response = await fetch(url, { cache: 'no-store', ...options, headers });
  let payload = null;
  try {
    payload = await response.json();
  } catch (error) {
    payload = null;
  }
  if (!response.ok) {
    const message = payload && payload.message ? payload.message : `HTTP ${response.status}`;
    const err = new Error(message);
    err.status = response.status;
    err.payload = payload;
    throw err;
  }
  return payload;
}

async function fetchText(url) {
  const headers = new Headers({ 'Cache-Control': 'no-store' });
  if (authState.token) headers.set('X-Auth-Token', authState.token);
  const response = await fetch(url, { cache: 'no-store', headers });
  if (!response.ok) throw new Error(`HTTP ${response.status}`);
  return response.text();
}

function setText(id, value) {
  const el = document.getElementById(id);
  if (el) el.textContent = value;
}

function setHidden(selector, hidden) {
  document.querySelectorAll(selector).forEach((el) => {
    el.classList.toggle('is-hidden', hidden);
  });
}

function setChipTone(id, tone) {
  const el = document.getElementById(id);
  if (!el) return;
  el.className = 'chip';
  if (tone === 'soft') el.classList.add('chip-soft');
  if (tone === 'warn') el.classList.add('chip-warn');
  if (tone === 'bad') el.classList.add('chip-bad');
}

function createEl(tag, className, text) {
  const el = document.createElement(tag);
  if (className) el.className = className;
  if (text != null) el.textContent = text;
  return el;
}

function buildUrlEncoded(data) {
  const body = new URLSearchParams();
  Object.entries(data).forEach(([key, value]) => body.append(key, String(value ?? '')));
  return body;
}

function showLoginModal() {
  const modal = document.getElementById('login-modal');
  if (!modal) return;
  modal.classList.remove('is-hidden');
  document.body.classList.add('modal-open');
  const input = document.getElementById('login-dmrid');
  if (input) input.focus();
}

function closeLoginModal() {
  const modal = document.getElementById('login-modal');
  if (!modal) return;
  modal.classList.add('is-hidden');
  document.body.classList.remove('modal-open');
}

function setLoginStatus(message, tone = '') {
  const box = document.getElementById('login-status');
  if (!box) return;
  box.textContent = message;
  box.className = 'form-status';
  if (tone === 'ok') box.classList.add('is-ok');
  if (tone === 'warn') box.classList.add('is-warn');
  if (tone === 'bad') box.classList.add('is-bad');
}

function setProfileStatus(message, tone = '') {
  const box = document.getElementById('profile-status');
  if (!box) return;
  box.textContent = message;
  box.className = 'form-status';
  if (tone === 'ok') box.classList.add('is-ok');
  if (tone === 'warn') box.classList.add('is-warn');
  if (tone === 'bad') box.classList.add('is-bad');
}

function ensureAuthChrome() {
  const nav = document.querySelector('.top-actions');
  if (!nav || document.getElementById('auth-slot')) return;

  const slot = createEl('div', 'auth-slot');
  slot.id = 'auth-slot';

  const chip = createEl('span', 'chip chip-soft is-hidden');
  chip.id = 'auth-user-chip';
  slot.append(chip);

  const loginBtn = createEl('button', 'nav-btn is-hidden', 'Login');
  loginBtn.id = 'login-open';
  loginBtn.type = 'button';
  loginBtn.addEventListener('click', showLoginModal);
  slot.append(loginBtn);

  const profileBtn = createEl('button', 'nav-btn is-hidden', 'Profile');
  profileBtn.id = 'profile-nav';
  profileBtn.type = 'button';
  profileBtn.addEventListener('click', () => { location.href = 'profile.html'; });
  slot.append(profileBtn);

  const logoutBtn = createEl('button', 'nav-btn is-hidden', 'Logout');
  logoutBtn.id = 'logout-btn';
  logoutBtn.type = 'button';
  logoutBtn.addEventListener('click', logout);
  slot.append(logoutBtn);

  nav.append(slot);

  const modal = document.createElement('div');
  modal.id = 'login-modal';
  modal.className = 'modal-backdrop is-hidden';
  modal.innerHTML = `
    <div class="modal-card" role="dialog" aria-modal="true" aria-labelledby="login-title">
      <div class="panel-head modal-head">
        <div>
          <div class="panel-title" id="login-title">Login</div>
          <div class="panel-subtitle">Use your DMR-ID and password</div>
        </div>
        <button id="login-close" class="nav-btn modal-close" type="button">Close</button>
      </div>
      <form id="login-form" class="register-form compact-form">
        <label class="field">
          <span>DMR-ID</span>
          <input id="login-dmrid" name="dmrid" type="number" inputmode="numeric" min="1" required />
        </label>
        <label class="field">
          <span>Password</span>
          <input id="login-password" name="password" type="password" maxlength="127" required />
        </label>
        <div class="field-actions field-span-2 auth-actions-row">
          <button id="login-submit" class="nav-btn submit-btn" type="submit">Login</button>
          <div id="login-status" class="form-status">Ready.</div>
        </div>
      </form>
    </div>
  `;
  document.body.append(modal);

  modal.addEventListener('click', (event) => {
    if (event.target === modal) closeLoginModal();
  });

  const closeBtn = document.getElementById('login-close');
  if (closeBtn) closeBtn.addEventListener('click', closeLoginModal);

  const form = document.getElementById('login-form');
  if (form) {
    form.addEventListener('submit', async (event) => {
      event.preventDefault();
      const submitBtn = document.getElementById('login-submit');
      const dmrid = document.getElementById('login-dmrid')?.value || '';
      const password = document.getElementById('login-password')?.value || '';

      if (submitBtn) submitBtn.disabled = true;
      setLoginStatus('Logging in…');

      try {
        const response = await fetchJSON('/api/login', {
          method: 'POST',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
          body: buildUrlEncoded({ dmrid, password })
        });

        authState.token = response.token || '';
        authState.user = {
          dmrid: response.dmrid,
          callsign: response.callsign || '',
          name: response.name || ''
        };
        saveAuthState();
        syncAuthUi();
        renderProfileView();
        setLoginStatus(response.message || 'Login successful.', 'ok');
        form.reset();
        setTimeout(closeLoginModal, 250);
      } catch (error) {
        setLoginStatus(error.message || 'Login failed.', 'bad');
      } finally {
        if (submitBtn) submitBtn.disabled = false;
      }
    });
  }
}

function syncAuthUi() {
  const enabled = !!runtimeConfig.authEnabled;
  const chip = document.getElementById('auth-user-chip');
  const loginBtn = document.getElementById('login-open');
  const profileBtn = document.getElementById('profile-nav');
  const logoutBtn = document.getElementById('logout-btn');

  if (!enabled) {
    chip?.classList.add('is-hidden');
    loginBtn?.classList.add('is-hidden');
    profileBtn?.classList.add('is-hidden');
    logoutBtn?.classList.add('is-hidden');
    return;
  }

  const loggedIn = !!(authState.token && authState.user);
  if (chip) {
    chip.classList.toggle('is-hidden', !loggedIn);
    chip.textContent = loggedIn
      ? `DMR-ID: ${authState.user.dmrid} (${authState.user.name || 'Callsign'})`
      : '';
  }

  loginBtn?.classList.toggle('is-hidden', loggedIn);
  profileBtn?.classList.toggle('is-hidden', !loggedIn);
  logoutBtn?.classList.toggle('is-hidden', !loggedIn);

  if (profileBtn) {
    profileBtn.classList.toggle('is-current', page === 'profile');
  }
}

async function refreshSession() {
  if (!authState.token) {
    syncAuthUi();
    renderProfileView();
    return;
  }

  try {
    const profile = await fetchJSON('/api/profile');
    authState.user = {
      dmrid: profile.dmrid,
      callsign: profile.callsign || '',
      name: profile.name || ''
    };
    saveAuthState();
  } catch (error) {
    if (error.status === 401) clearAuthState();
    else console.error('profile refresh:', error);
  }

  syncAuthUi();
  renderProfileView();
}

async function logout() {
  try {
    if (authState.token) {
      await fetchJSON('/api/logout', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: buildUrlEncoded({})
      });
    }
  } catch (error) {
    console.error('logout:', error);
  }

  clearAuthState();
  syncAuthUi();
  renderProfileView();
  closeLoginModal();
  if (page === 'profile') setProfileStatus('Logged out.', 'warn');
}

async function loadRuntimeConfig() {
  try {
    const config = await fetchJSON('/api/config');
    runtimeConfig = {
      authEnabled: !!config.authEnabled,
      registrationEnabled: !!config.registrationEnabled,
      profileEnabled: !!config.profileEnabled,
      dmrIdsFile: config.dmrIdsFile || '',
      serverVersion: config.serverVersion || ''
    };
  } catch (error) {
    console.error('config:', error);
  }

  setHidden('[data-auth-only]', !runtimeConfig.registrationEnabled);
  setText('server-version', runtimeConfig.serverVersion ? `Server: ${runtimeConfig.serverVersion}` : 'Server version unavailable');

  if (page === 'register') {
    setText('register-auth-state', runtimeConfig.registrationEnabled ? 'Registration enabled' : 'Registration disabled');
    setText('register-storage', runtimeConfig.dmrIdsFile || 'DMRIds.dat');
    setChipTone('register-auth-state', runtimeConfig.registrationEnabled ? '' : 'warn');
    const form = document.getElementById('register-form');
    if (form) {
      Array.from(form.elements).forEach((el) => {
        if ('disabled' in el) el.disabled = !runtimeConfig.registrationEnabled;
      });
    }
  }

  syncAuthUi();
  renderProfileView();
}

function setRegisterStatus(message, tone = 'soft') {
  const box = document.getElementById('register-status');
  if (!box) return;
  box.textContent = message;
  box.className = 'form-status';
  if (tone === 'ok') box.classList.add('is-ok');
  if (tone === 'bad') box.classList.add('is-bad');
  if (tone === 'warn') box.classList.add('is-warn');
}

function initRegistrationForm() {
  const form = document.getElementById('register-form');
  if (!form) return;

  form.addEventListener('submit', async (event) => {
    event.preventDefault();

    if (!runtimeConfig.registrationEnabled) {
      setRegisterStatus('Registration is disabled in dmr.conf.', 'warn');
      return;
    }

    const submitBtn = document.getElementById('register-submit');
    const data = new FormData(form);
    const body = new URLSearchParams();
    for (const [key, value] of data.entries()) body.append(key, String(value));

    if (submitBtn) submitBtn.disabled = true;
    setRegisterStatus('Saving registration…');

    try {
      const response = await fetchJSON('/api/register', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body
      });
      setRegisterStatus(response.message || 'Registration saved.', 'ok');
      form.reset();
    } catch (error) {
      setRegisterStatus(error.message || 'Unable to save registration.', 'bad');
    } finally {
      if (submitBtn) submitBtn.disabled = false;
    }
  });
}

function renderClock() {
  const now = new Date();
  setText('clock-time', now.toLocaleTimeString());
  setText('clock-date', now.toLocaleDateString(undefined, { weekday: 'long', year: 'numeric', month: 'long', day: 'numeric' }));
}

function updateRefresh() {
  const time = new Date().toLocaleTimeString();
  setText('metric-refresh', time);
  setText('metric-refresh-sub', 'Last successful refresh');
  setText('monitor-updated', `Updated ${time}`);
}

function td(text, className = '') {
  const cell = document.createElement('td');
  cell.textContent = text;
  if (className) cell.className = className;
  return cell;
}

function tgLabelFromParts(tg, tgName = '') {
  const number = formatTgValue(tg);
  if (number === '—') return number;
  const name = String(tgName || '').trim();
  return name ? `${number} - ${name}` : number;
}

function tgLabel(row) {
  return tgLabelFromParts(row?.tg, row?.tgName);
}

function tgCell(row, className = '') {
  const label = tgLabel(row);
  const cell = td(label, className || (row?.tgName ? 'emphasis' : ''));
  const country = String(row?.tgCountry || '').trim();
  const name = String(row?.tgName || '').trim();
  if (country) cell.dataset.tgCountry = country;
  if (name) cell.dataset.tgName = name;
  if (country || name) cell.title = [name, country].filter(Boolean).join(' · ');
  return cell;
}

function uniqueNonEmpty(values) {
  return [...new Set(values.filter((value) => value !== null && value !== undefined && value !== ''))];
}

function updateSummaryRows(rows) {
  const summary = document.getElementById('activity-summary');
  if (!summary) return;
  summary.innerHTML = '';
  if (!rows.length) {
    const row = createEl('div', 'summary-row');
    row.append(createEl('span', '', 'No log data yet'));
    row.append(createEl('strong', '', '—'));
    summary.append(row);
    return;
  }
  rows.forEach((item) => {
    const row = createEl('div', 'summary-row');
    row.append(createEl('span', '', item.label));
    row.append(createEl('strong', '', item.value));
    summary.append(row);
  });
}

function badgesFor(row) {
  const wrap = createEl('div', 'badge-stack');
  const entries = [];
  if (row.aprs) entries.push(['APRS', 'badge aprs']);
  if (row.sms) entries.push(['SMS', 'badge sms']);
  if (row.src === 2) entries.push(['OBP', 'badge ob']);
  if (!entries.length) entries.push(['TALK', 'badge talk']);
  entries.forEach(([label, className]) => wrap.append(createEl('span', className, label)));
  return wrap;
}



function formatRadio(row) {
  if (!row) return '—';
  const radio = row.radio ?? '—';
  const callsign = (row.callsign || '').trim();
  if (!callsign) return String(radio);
  return `${radio} (${callsign})`;
}

function statusBadge(status) {
  const wrap = createEl('div', 'badge-stack');
  const map = {
    active: ['ACTIVE', 'badge ob'],
    idle: ['IDLE', 'badge talk'],
    unresolved: ['UNRESOLVED', 'badge sms'],
    disabled: ['DISABLED', 'badge']
  };
  const [label, className] = map[status] || ['UNKNOWN', 'badge'];
  wrap.append(createEl('span', className, label));
  return wrap;
}

function formatSince(seconds) {
  const sec = Number(seconds || 0);
  if (!sec) return 'just now';
  if (sec < 60) return `${sec}s ago`;
  const min = Math.floor(sec / 60);
  if (min < 60) return `${min}m ago`;
  const hr = Math.floor(min / 60);
  if (hr < 24) return `${hr}h ago`;
  const day = Math.floor(hr / 24);
  return `${day}d ago`;
}

async function renderOpenBridge() {
  const tbody = document.querySelector('#tab-openbridge tbody');
  if (!tbody) return;

  try {
    const data = await fetchJSON('/api/openbridge');
    tbody.innerHTML = '';

    if (!data.length) {
      const tr = document.createElement('tr');
      const cell = document.createElement('td');
      cell.colSpan = 11;
      cell.textContent = 'No enabled OpenBridge peers configured';
      tr.append(cell);
      tbody.append(tr);
      setText('ob-metric-total', '0');
      setText('ob-metric-total-sub', 'Enable OpenBridge1 or OpenBridge2 in dmr.conf');
      setText('ob-metric-active', '0');
      setText('ob-metric-active-sub', 'No bridge traffic');
      setText('ob-metric-rx', '—');
      setText('ob-metric-rx-sub', 'Waiting for traffic');
      setText('ob-metric-tx', '—');
      setText('ob-metric-tx-sub', 'Waiting for traffic');
      setText('ob-table-meta', 'No OpenBridge peers loaded');
      setText('ob-hero-status', 'No peers');
      setChipTone('ob-hero-status', 'warn');
      setText('ob-hero-summary', 'Nothing to show until an OpenBridge section is enabled');
      return;
    }

    data.forEach((row) => {
      const tr = document.createElement('tr');
      tr.append(td(row.aliasName || '—', row.aliasName ? 'emphasis' : ''));
      const statusCell = td('');
      statusCell.append(statusBadge(row.status));
      tr.append(statusCell);
      tr.append(td(row.targetHost || '—', row.targetHost ? 'emphasis' : ''));
      tr.append(td(row.networkId || '—'));
      tr.append(td(row.enhanced ? 'YES' : 'NO'));
      tr.append(td(row.permitAll ? 'ALL' : (row.permitTGs || '—')));
      tr.append(td(formatSince(row.secondsSinceRx)));
      tr.append(td(formatSince(row.secondsSinceTx)));
      tbody.append(tr);
    });

    const active = data.filter((row) => row.status === 'active');
    const latestRx = [...data].sort((a, b) => (a.secondsSinceRx ?? 999999) - (b.secondsSinceRx ?? 999999))[0];
    const latestTx = [...data].sort((a, b) => (a.secondsSinceTx ?? 999999) - (b.secondsSinceTx ?? 999999))[0];
    const peerNames = data.map((row) => row.aliasName || row.targetHost || row.name).filter(Boolean);
    const activeNames = active.map((row) => row.aliasName || row.targetHost || row.name).filter(Boolean);

    setText('ob-metric-total', String(data.length));
    setText('ob-metric-total-sub', peerNames.join(', '));
    setText('ob-metric-active', String(active.length));
    setText('ob-metric-active-sub', active.length ? activeNames.join(', ') : 'No peer active in the last 60s');
    setText('ob-metric-rx', latestRx ? (latestRx.aliasName || latestRx.name || '—') : '—');
    setText('ob-metric-rx-sub', latestRx ? `${formatSince(latestRx.secondsSinceRx)} from ${latestRx.targetHost}` : 'Waiting for traffic');
    setText('ob-metric-tx', latestTx ? (latestTx.aliasName || latestTx.name || '—') : '—');
    setText('ob-metric-tx-sub', latestTx ? `${formatSince(latestTx.secondsSinceTx)} to ${latestTx.targetHost}` : 'Waiting for traffic');
    setText('ob-table-meta', `${data.length} peer${data.length === 1 ? '' : 's'} loaded`);
    setText('ob-hero-status', active.length ? 'Bridge active' : 'Bridge idle');
    setChipTone('ob-hero-status', active.length ? '' : 'soft');
    setText('ob-hero-summary', data.map((row) => `${row.aliasName || row.name} → ${row.targetHost}`).join(' · '));
  } catch (error) {
    console.error('openbridge:', error);
    tbody.innerHTML = '';
    const tr = document.createElement('tr');
    const cell = document.createElement('td');
    cell.colSpan = 11;
    cell.textContent = 'Unable to load OpenBridge peers';
    tr.append(cell);
    tbody.append(tr);
    setText('ob-table-meta', 'Request failed');
    setText('ob-hero-status', 'Unavailable');
    setChipTone('ob-hero-status', 'bad');
    setText('ob-hero-summary', 'Check that server.cpp includes /api/openbridge and restart the server');
  }
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
      setText('metric-active', '0');
      setText('metric-active-sub', 'No live activity');
      setText('active-meta', 'No active calls');
      setText('hero-status', 'Idle');
      setChipTone('hero-status', 'soft');
      setText('hero-summary', 'No active transmissions yet');
      document.getElementById('active-panel')?.classList.remove('is-live');
      return;
    }

    data.forEach((row) => {
      const tr = document.createElement('tr');
      tr.append(td(row.date || '—'));
      tr.append(td(formatRadio(row), 'emphasis'));
      const badgeCell = td('');
      badgeCell.append(badgesFor(row));
      tr.append(badgeCell);
      tr.append(tgCell(row));
      tr.append(td(row.slot ?? '—'));
      tr.append(td(row.node ?? '—'));
      tr.append(td(row.time ?? '—', 'numeric'));
      tbody.append(tr);
    });

    const live = data[0];
    const activeTgs = uniqueNonEmpty(data.map((row) => row.tg));
    const activeTgLabels = uniqueNonEmpty(data.map((row) => tgLabel(row)));
    const activeLabel = `${data.length} active call${data.length === 1 ? '' : 's'}`;
    setText('metric-active', String(data.length));
    setText('metric-active-sub', data.length === 1 && live ? `Latest radio ${formatRadio(live)}` : activeLabel);
    setText('active-meta', activeTgLabels.length ? `${activeLabel} · ${activeTgLabels.slice(0, 4).join(', ')}` : activeLabel);
    setText('hero-status', 'Live traffic');
    setChipTone('hero-status', '');
    setText('hero-summary', data.length === 1 && live ? `${formatRadio(live)} on ${tgLabel(live)}` : `${activeLabel} across ${activeTgs.length} talkgroup${activeTgs.length === 1 ? '' : 's'}`);
    document.getElementById('active-panel')?.classList.add('is-live');
  } catch (error) {
    console.error('active:', error);
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
      data.forEach((row) => {
        const tr = document.createElement('tr');
        tr.append(td(row.id ?? '—', 'numeric'));
        tr.append(td(row.date || '—'));
        tr.append(td(formatRadio(row), 'emphasis'));
        const badgeCell = td('');
        badgeCell.append(badgesFor(row));
        tr.append(badgeCell);
        tr.append(tgCell(row));
        tr.append(td(row.slot ?? '—'));
        tr.append(td(row.node ?? '—'));
        tr.append(td(row.time ?? '—', 'numeric'));
        tr.append(td(row.active ?? '—', row.active ? 'bad' : ''));
        tr.append(td(row.online ? 'YES' : 'NO', row.online ? 'ok' : ''));
        tr.append(td(row.connect ?? '—'));
        tbody.append(tr);
      });
    }

    const uniqueTGs = uniqueNonEmpty(data.map((row) => row.tg));
    const uniqueTgLabels = uniqueNonEmpty(data.map((row) => tgLabel(row)));
    const onlineCount = data.filter((row) => row.online).length;
    const openBridgeCount = data.filter((row) => row.src === 2).length;
    const aprsCount = data.filter((row) => row.aprs).length;
    const smsCount = data.filter((row) => row.sms).length;
    const talkCount = Math.max(data.length - openBridgeCount - aprsCount - smsCount, 0);

    setText('metric-talkgroups', String(uniqueTGs.length));
    setText('metric-talkgroups-sub', uniqueTgLabels.length ? `Latest: ${uniqueTgLabels.slice(0, 3).join(', ')}` : 'Across latest 20 records');
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

function parseStatText(text) {
  const result = { sec: null, tick: null, vectors: [], rows: [] };
  const lines = String(text || '').split(/\r?\n/).map((line) => line.trimEnd()).filter(Boolean);
  let currentVector = null;
  let currentNode = null;

  lines.forEach((line) => {
    const secMatch = line.match(/^Sec\s+(\d+)\s+tick\s+(\d+)$/i);
    if (secMatch) {
      result.sec = Number(secMatch[1]);
      result.tick = Number(secMatch[2]);
      return;
    }

    const vectorMatch = line.match(/^Node vector\s+(\d+),\s+radioslot\s+(\d+)$/i);
    if (vectorMatch) {
      currentVector = {
        vectorId: vectorMatch[1],
        radioSlot: vectorMatch[2],
        nodes: []
      };
      result.vectors.push(currentVector);
      currentNode = null;
      return;
    }

    const nodeMatch = line.match(/^\s+ID\s+(\d+)\s+dmrid\s+(\d+)\s+auth\s+(\d+)\s+sec\s+(\d+)$/i);
    if (nodeMatch && currentVector) {
      currentNode = {
        vectorId: currentVector.vectorId,
        radioSlot: currentVector.radioSlot,
        nodeId: nodeMatch[1],
        dmrid: nodeMatch[2],
        auth: nodeMatch[3] === '1',
        hitSec: nodeMatch[4],
        slot1: '—',
        slot2: '—'
      };
      currentVector.nodes.push(currentNode);
      result.rows.push(currentNode);
      return;
    }

    const slotMatch = line.match(/^\s*S([12])\s+TG\s+(\d+)$/i);
    if (slotMatch && currentNode) {
      currentNode[`slot${slotMatch[1]}`] = slotMatch[2];
    }
  });

  return result;
}

function setMonitorMetrics(parsed) {
  const rows = parsed.rows || [];
  const vectors = parsed.vectors || [];
  const talkgroups = new Set();

  rows.forEach((row) => {
    if (row.slot1 && row.slot1 !== '—') talkgroups.add(row.slot1);
    if (row.slot2 && row.slot2 !== '—') talkgroups.add(row.slot2);
  });

  setText('monitor-metric-vectors', String(vectors.length));
  setText('monitor-metric-vectors-sub', vectors.length
    ? vectors.map((vector) => `${vector.vectorId} · RS ${vector.radioSlot}`).join(', ')
    : 'No radio vectors loaded');
  setText('monitor-metric-nodes', String(rows.length));
  setText('monitor-metric-nodes-sub', rows.length
    ? rows.map((row) => row.nodeId).join(', ')
    : 'No connected nodes reported');
  setText('monitor-metric-tgs', String(talkgroups.size));
  setText('monitor-metric-tgs-sub', talkgroups.size
    ? Array.from(talkgroups).join(', ')
    : 'No slot subscriptions yet');
  setText('monitor-metric-tick', parsed.tick != null ? String(parsed.tick) : '—');
  setText('monitor-metric-tick-sub', parsed.sec != null
    ? `Server sec ${parsed.sec}`
    : 'Waiting for counters');

  setText('hero-status', rows.length ? 'Live nodes' : 'Idle');
  setChipTone('hero-status', rows.length ? '' : 'soft');
  setText('hero-summary', rows.length
    ? `${rows.length} node${rows.length === 1 ? '' : 's'} across ${vectors.length} vector${vectors.length === 1 ? '' : 's'}`
    : 'No active nodes reported');
}

function renderStatTable(parsed) {
  const tbody = document.querySelector('#tab-monitor tbody');
  if (!tbody) return;
  tbody.innerHTML = '';

  if (!parsed.rows.length) {
    const tr = document.createElement('tr');
    const cell = document.createElement('td');
    cell.colSpan = 9;
    cell.textContent = 'No connected nodes found in /STAT output';
    tr.append(cell);
    tbody.append(tr);
    return;
  }

  parsed.rows.forEach((row) => {
    const tr = document.createElement('tr');
    tr.append(td(row.vectorId, 'emphasis'));
    tr.append(td(row.radioSlot));
    tr.append(td(row.nodeId, 'numeric'));
    tr.append(td(row.dmrid, 'numeric'));
    tr.append(td(row.auth ? 'YES' : 'NO', row.auth ? 'ok' : 'bad'));
    tr.append(td(row.hitSec, 'numeric'));
    tr.append(td(row.slot1, row.slot1 !== '—' ? 'numeric emphasis' : ''));
    tr.append(td(row.slot2, row.slot2 !== '—' ? 'numeric emphasis' : ''));
    tbody.append(tr);
  });
}

async function renderStat() {
  const statEl = document.getElementById('stat');
  const tbody = document.querySelector('#tab-monitor tbody');
  if (!statEl && !tbody) return;

  try {
    const text = await fetchText('/api/stat');
    if (statEl) statEl.textContent = text || 'No /STAT reply';

    const parsed = parseStatText(text);
    renderStatTable(parsed);
    setMonitorMetrics(parsed);

    setText('monitor-status', text ? 'STAT connected' : 'STAT idle');
    const statusChip = document.getElementById('monitor-status');
    if (statusChip) statusChip.className = text ? 'chip' : 'chip chip-warn';
    setText('stat-meta', parsed.rows.length
      ? `${parsed.rows.length} node${parsed.rows.length === 1 ? '' : 's'} loaded`
      : 'No connected nodes');
  } catch (error) {
    console.error('stat:', error);
    if (statEl) statEl.textContent = 'No /STAT reply';
    if (tbody) {
      tbody.innerHTML = '';
      const tr = document.createElement('tr');
      const cell = document.createElement('td');
      cell.colSpan = 9;
      cell.textContent = 'Unable to load /STAT output';
      tr.append(cell);
      tbody.append(tr);
    }
    setText('monitor-status', 'STAT unavailable');
    const statusChip = document.getElementById('monitor-status');
    if (statusChip) statusChip.className = 'chip chip-bad';
    setText('stat-meta', 'Request failed');
    setText('hero-status', 'Unavailable');
    setChipTone('hero-status', 'bad');
    setText('hero-summary', 'Check the local UDP /STAT responder and try again');
    setText('monitor-metric-vectors', '0');
    setText('monitor-metric-vectors-sub', 'No radio vectors loaded');
    setText('monitor-metric-nodes', '0');
    setText('monitor-metric-nodes-sub', 'Unable to read /STAT');
    setText('monitor-metric-tgs', '0');
    setText('monitor-metric-tgs-sub', 'Unable to read /STAT');
    setText('monitor-metric-tick', '—');
    setText('monitor-metric-tick-sub', 'Request failed');
  }
}


function formatTgValue(value) {
  const text = String(value || '').trim();
  return text ? text : '—';
}

async function renderSystemTgs() {
  const tbody = document.querySelector('#tab-systemstg tbody');
  if (!tbody) return;

  try {
    const data = await fetchJSON('/api/systemstg');
    tbody.innerHTML = '';

    if (!data.length) {
      const tr = document.createElement('tr');
      const cell = document.createElement('td');
      cell.colSpan = 10;
      cell.textContent = 'No hotspot static talkgroups reported yet';
      tr.append(cell);
      tbody.append(tr);
      setText('systemstg-metric-hotspots', '0');
      setText('systemstg-metric-hotspots-sub', 'Waiting for MMDVM config frames');
      setText('systemstg-metric-unique', '0');
      setText('systemstg-metric-unique-sub', 'No static talkgroups active');
      setText('systemstg-metric-ts1', '0');
      setText('systemstg-metric-ts1-sub', 'No TS1 assignments');
      setText('systemstg-metric-ts2', '0');
      setText('systemstg-metric-ts2-sub', 'No TS2 assignments');
      setText('systemstg-meta', 'No hotspot data');
      setText('systemstg-status', 'Idle');
      setChipTone('systemstg-status', 'soft');
      setText('systemstg-summary', 'No static talkgroups announced by hotspots');
      return;
    }

    const uniqueTgs = new Set();
    let ts1Count = 0;
    let ts2Count = 0;

    data.forEach((row) => {
      const tr = document.createElement('tr');
      tr.append(td(row.dmrid || '—', 'numeric emphasis'));
      tr.append(td(row.callsign || row.name || '—', row.callsign || row.name ? 'emphasis' : ''));
      tr.append(td(row.node || '—', 'numeric'));
      tr.append(td(row.auth ? 'YES' : 'NO', row.auth ? 'ok' : 'bad'));
      tr.append(td(row.currentTs1 || '—', row.currentTs1 ? 'numeric' : ''));
      tr.append(td(row.currentTs2 || '—', row.currentTs2 ? 'numeric' : ''));
      tr.append(td(formatTgValue(row.staticTs1), row.staticTs1 ? 'emphasis' : ''));
      tr.append(td(formatTgValue(row.staticTs2), row.staticTs2 ? 'emphasis' : ''));
      tr.append(td(formatSince(row.lastSeenSec), 'numeric'));
      tbody.append(tr);

      const ts1 = String(row.staticTs1 || '').split(',').map((item) => item.trim()).filter(Boolean);
      const ts2 = String(row.staticTs2 || '').split(',').map((item) => item.trim()).filter(Boolean);
      ts1.forEach((tg) => uniqueTgs.add(tg));
      ts2.forEach((tg) => uniqueTgs.add(tg));
      ts1Count += ts1.length;
      ts2Count += ts2.length;
    });

    const latest = [...data].sort((a, b) => (a.lastSeenSec ?? 999999) - (b.lastSeenSec ?? 999999))[0];
    const latestName = latest ? (latest.callsign || latest.name || latest.dmrid || '—') : '—';

    setText('systemstg-metric-hotspots', String(data.length));
    setText('systemstg-metric-hotspots-sub', data.map((row) => row.callsign || row.dmrid).join(', '));
    setText('systemstg-metric-unique', String(uniqueTgs.size));
    setText('systemstg-metric-unique-sub', uniqueTgs.size ? Array.from(uniqueTgs).join(', ') : 'No static talkgroups active');
    setText('systemstg-metric-ts1', String(ts1Count));
    setText('systemstg-metric-ts1-sub', ts1Count ? 'Total TS1 static assignments' : 'No TS1 assignments');
    setText('systemstg-metric-ts2', String(ts2Count));
    setText('systemstg-metric-ts2-sub', ts2Count ? 'Total TS2 static assignments' : 'No TS2 assignments');
    setText('systemstg-meta', `${data.length} hotspot${data.length === 1 ? '' : 's'} loaded`);
    setText('systemstg-status', 'Live');
    setChipTone('systemstg-status', '');
    setText('systemstg-summary', `${latestName} seen ${formatSince(latest?.lastSeenSec)} · ${uniqueTgs.size} unique static talkgroup${uniqueTgs.size === 1 ? '' : 's'}`);
  } catch (error) {
    console.error('systemstg:', error);
    tbody.innerHTML = '';
    const tr = document.createElement('tr');
    const cell = document.createElement('td');
    cell.colSpan = 10;
    cell.textContent = 'Unable to load hotspot static talkgroups';
    tr.append(cell);
    tbody.append(tr);
    setText('systemstg-meta', 'Request failed');
    setText('systemstg-status', 'Unavailable');
    setChipTone('systemstg-status', 'bad');
    setText('systemstg-summary', 'Check that /api/systemstg is available and restart the server');
    setText('systemstg-metric-hotspots', '0');
    setText('systemstg-metric-hotspots-sub', 'Request failed');
    setText('systemstg-metric-unique', '0');
    setText('systemstg-metric-unique-sub', 'Request failed');
    setText('systemstg-metric-ts1', '0');
    setText('systemstg-metric-ts1-sub', 'Request failed');
    setText('systemstg-metric-ts2', '0');
    setText('systemstg-metric-ts2-sub', 'Request failed');
  }
}

function renderProfileView() {
  if (page !== 'profile') return;

  const guest = document.getElementById('profile-guest');
  const details = document.getElementById('profile-details');
  const form = document.getElementById('profile-form');
  const disabled = !runtimeConfig.profileEnabled;
  const loggedIn = !!(authState.token && authState.user);

  if (guest) guest.classList.toggle('is-hidden', disabled || loggedIn);
  if (details) details.classList.toggle('is-hidden', disabled || !loggedIn);
  if (form) form.classList.toggle('is-hidden', disabled || !loggedIn);

  if (disabled) {
    setProfileStatus('Profile editing is disabled because auth is not enabled in dmr.conf.', 'warn');
    return;
  }

  if (!loggedIn) {
    setProfileStatus('Please log in to edit your profile.', 'warn');
    if (!authPromptedOnProfile) {
      authPromptedOnProfile = true;
      showLoginModal();
    }
    return;
  }

  setText('profile-dmrid', String(authState.user.dmrid || '—'));
  setText('profile-callsign', authState.user.callsign || '—');
  const nameInput = document.getElementById('profile-name');
  if (nameInput && document.activeElement !== nameInput) nameInput.value = authState.user.name || '';
  setProfileStatus('Ready. Leave password fields blank to change only the name.');
}

function initProfileForm() {
  const form = document.getElementById('profile-form');
  if (!form) return;

  form.addEventListener('submit', async (event) => {
    event.preventDefault();

    if (!authState.token || !authState.user) {
      setProfileStatus('Please log in first.', 'warn');
      showLoginModal();
      return;
    }

    const submitBtn = document.getElementById('profile-submit');
    const name = document.getElementById('profile-name')?.value || '';
    const currentPassword = document.getElementById('profile-current-password')?.value || '';
    const newPassword = document.getElementById('profile-new-password')?.value || '';
    const confirmPassword = document.getElementById('profile-confirm-password')?.value || '';

    if (newPassword && newPassword !== confirmPassword) {
      setProfileStatus('New passwords do not match.', 'bad');
      return;
    }

    if (submitBtn) submitBtn.disabled = true;
    setProfileStatus('Saving profile…');

    try {
      const response = await fetchJSON('/api/profile', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: buildUrlEncoded({ name, currentPassword, newPassword })
      });

      authState.user = {
        dmrid: response.dmrid,
        callsign: response.callsign || '',
        name: response.name || ''
      };
      saveAuthState();
      syncAuthUi();
      renderProfileView();
      setProfileStatus(response.message || 'Profile updated.', 'ok');
      const currentField = document.getElementById('profile-current-password');
      const newField = document.getElementById('profile-new-password');
      const confirmField = document.getElementById('profile-confirm-password');
      if (currentField) currentField.value = '';
      if (newField) newField.value = '';
      if (confirmField) confirmField.value = '';
    } catch (error) {
      setProfileStatus(error.message || 'Unable to update profile.', 'bad');
    } finally {
      if (submitBtn) submitBtn.disabled = false;
    }
  });
}

async function tick() {
  renderClock();
  updateRefresh();

  const jobs = [];
  if (page === 'dashboard') jobs.push(renderActive(), renderLog());
  if (page === 'monitor') jobs.push(renderStat());
  if (page === 'openbridge') jobs.push(renderOpenBridge());
  if (page === 'systemstg') jobs.push(renderSystemTgs());
  if (page === 'register') jobs.push(renderActive(), renderLog(), renderStat());

  await Promise.allSettled(jobs);
}

loadAuthState();
initThemeToggle();
ensureAuthChrome();
initRegistrationForm();
initProfileForm();
renderClock();
renderProfileView();
loadRuntimeConfig().then(refreshSession);
tick();
setInterval(renderClock, 1000);
setInterval(tick, 3000);
