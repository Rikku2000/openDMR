import { localizeText, t } from './i18n.js';

const DATABASES = {
  'radioid-users': {
    label: 'Users [DMR-Database]',
    url: 'https://raw.githubusercontent.com/DMR-Database/dmr-database-appdata/refs/heads/main/users.json',
    fallbackUrls: [
      'https://raw.githubusercontent.com/DMR-Database/dmr-database-appdata/refs/heads/main/user.json'
    ],
    type: 'users',
    parse: (json) => Array.isArray(json)
      ? json
      : (Array.isArray(json?.results) ? json.results : [])
  },
  'radioid-repeaters': {
    label: 'Repeaters [DMR-Database]',
    url: 'https://raw.githubusercontent.com/DMR-Database/dmr-database-appdata/refs/heads/main/rptrs.json',
    type: 'repeaters',
    parse: (json) => Array.isArray(json)
      ? json
      : (Array.isArray(json?.rptrs) ? json.rptrs : [])
  },
  'freeradioid-users': {
    label: 'Users [freeradioid.net]',
    url: 'https://freeradioid.net/api/local_subscriber_ids.json',
    type: 'users',
    parse: (json) => Array.isArray(json?.results) ? json.results : []
  }
};

const DV_PREVIEW_PAGE_SIZE = 10;

const DEVICES = [
  { name: 'Ailunce HD1', value: 'HD1' },
  { name: 'Motorola DGP-6150+', value: 'MOTO' },
  { name: 'Retevis RT52', value: 'RT52' },
  { name: 'Retevis RT3S / RT90 / RT84 / RT82', value: 'RT3S' },
  { name: 'AnyTone', value: 'AnyTone' },
  { name: 'AnyTone (No index)', value: 'AnyToneNX' },
  { name: 'Radioddity GD-77', value: 'GD77' },
  { name: 'TYT MD-380 / MD-2017 / MD-9600', value: 'TYT' },
  { name: 'BOAFENG DM-1701', value: 'BAOFENG' },
  { name: 'DVPi', value: 'DVPI' },
  { name: 'PI-Star', value: 'PISTAR' },
  { name: 'Nextion (stripped)', value: 'NEXTION' },
  { name: 'BlueDV', value: 'BLUEDV' },
  { name: 'Abbree DM-F8', value: 'DM-F8' }
];

const state = {
  databaseKey: 'radioid-users',
  records: [],
  countries: [],
  selectAll: true,
  noCountriesOnly: false,
  selectedCountries: new Set(),
  selectedDevices: new Set(),
  downloadActive: false,
  exportActive: false,
  previewPage: 1
};

function $(id) { return document.getElementById(id); }
function text(id, value) { const el = $(id); if (el) el.textContent = localizeText(value); }
function setDisabled(id, disabled) { const el = $(id); if (el) el.disabled = disabled; }

function setStatus(id, message, tone = '') {
  const el = $(id);
  if (!el) return;
  el.textContent = localizeText(message);
  el.className = 'form-status';
  if (tone === 'ok') el.classList.add('is-ok');
  if (tone === 'warn') el.classList.add('is-warn');
  if (tone === 'bad') el.classList.add('is-bad');
}

function setChip(id, message, tone = 'soft') {
  const el = $(id);
  if (!el) return;
  el.textContent = localizeText(message);
  el.className = 'chip';
  if (tone === 'soft') el.classList.add('chip-soft');
  if (tone === 'warn') el.classList.add('chip-warn');
  if (tone === 'bad') el.classList.add('chip-bad');
}

function sanitize(value) {
  if (value === null || value === undefined) return '';
  return String(value)
    .replace('\u2020', '+')
    .replace(/,/g, ' ')
    .normalize('NFD')
    .replace(/[\u0300-\u036f]/g, '')
    .trim();
}

function capitalizeWords(value) {
  return sanitize(value)
    .toLowerCase()
    .replace(/(?:^|\s|['`\x91\x92.-])[^\x00-\x60^\x7B-\xDF](?!(\s|$))/g, (match) => match.toUpperCase());
}

function csvEscape(value) {
  const str = String(value ?? '');
  return /[",\r\n]/.test(str) ? '"' + str.replace(/"/g, '""') + '"' : str;
}

function optionValues(select) {
  return Array.from(select.selectedOptions).map((option) => option.value);
}

function normalizedCountry(record) {
  return capitalizeWords(record?.country || '');
}

function radioIdFor(record, isRepeater) {
  const raw = isRepeater ? sanitize(record?.id) : sanitize(record?.radio_id || record?.id);
  const id = Number.parseInt(raw, 10);
  if (!Number.isFinite(id) || id < 1 || id > 16777215) return '';
  return String(id);
}

function getRecordName(record, isRepeater) {
  if (isRepeater) {
    const trustee = capitalizeWords(record?.trustee);
    const network = capitalizeWords(record?.ipsc_network);
    const assigned = capitalizeWords(record?.assigned);
    const first = trustee || network;
    return [first, assigned].filter(Boolean).join(' ').trim();
  }

  const fname = capitalizeWords(record?.fname);
  const name = capitalizeWords(record?.name);
  const surname = capitalizeWords(record?.surname);
  const first = fname || name;
  return [first, surname].filter(Boolean).join(' ').trim();
}

function getRemarks(record, isRepeater) {
  return sanitize(isRepeater ? record?.ts_linked : record?.remarks);
}

function getFilteredRecords() {
  if (!state.records.length) return [];
  if (state.selectAll) {
    if (!state.noCountriesOnly) return state.records.slice();
    return state.records.filter((record) => !normalizedCountry(record));
  }
  return state.records.filter((record) => state.selectedCountries.has(normalizedCountry(record)));
}

function countDefinedCountries() {
  return state.countries.filter(Boolean).length;
}

function updatePreviewPagination(totalCount) {
  const totalPages = Math.max(1, Math.ceil(totalCount / DV_PREVIEW_PAGE_SIZE));
  if (state.previewPage > totalPages) state.previewPage = totalPages;
  if (state.previewPage < 1) state.previewPage = 1;

  const prevBtn = $('dv-preview-page-prev');
  const nextBtn = $('dv-preview-page-next');
  const label = $('dv-preview-page-label');

  if (prevBtn) prevBtn.disabled = state.previewPage <= 1 || totalCount === 0;
  if (nextBtn) nextBtn.disabled = state.previewPage >= totalPages || totalCount === 0;
  if (label) label.textContent = localizeText(totalCount ? `Page ${state.previewPage} / ${totalPages}` : 'Page 0 / 0');
}

function populateCountries() {
  const select = $('dv-countries');
  if (!select) return;
  const selectedSnapshot = new Set(state.selectedCountries);
  select.innerHTML = '';

  state.countries.forEach((country) => {
    const count = state.records.filter((record) => normalizedCountry(record) === country).length;
    const option = document.createElement('option');
    option.value = country;
    option.textContent = localizeText(country ? `${country} (${count})` : `Undefined (${count})`);
    option.selected = state.selectAll ? !state.noCountriesOnly : selectedSnapshot.has(country);
    select.append(option);
  });

  if (state.selectAll && !state.noCountriesOnly) state.selectedCountries = new Set(state.countries);
  else if (state.noCountriesOnly) state.selectedCountries = new Set();
  else state.selectedCountries = selectedSnapshot;
}

function populateDevices() {
  const select = $('dv-devices');
  if (!select) return;
  select.innerHTML = '';
  DEVICES.forEach((device) => {
    const option = document.createElement('option');
    option.value = device.value;
    option.textContent = localizeText(device.name);
    option.selected = state.selectedDevices.has(device.value);
    select.append(option);
  });
}

function updatePreview() {
  const tbody = document.querySelector('#dv-preview-table tbody');
  if (!tbody) return;
  const records = getFilteredRecords();
  tbody.innerHTML = '';
  updatePreviewPagination(records.length);

  if (!records.length) {
    const row = document.createElement('tr');
    const cell = document.createElement('td');
    cell.colSpan = 7;
    cell.textContent = localizeText(state.records.length ? 'No contacts match the current filter' : 'Download a dataset to preview contacts');
    row.append(cell);
    tbody.append(row);
    text('dv-preview-meta', state.records.length ? '0 rows match the current selection' : 'No contacts selected');
    return;
  }

  const isRepeater = DATABASES[state.databaseKey].type === 'repeaters';
  const start = (state.previewPage - 1) * DV_PREVIEW_PAGE_SIZE;
  const pageRecords = records.slice(start, start + DV_PREVIEW_PAGE_SIZE);

  pageRecords.forEach((record) => {
    const row = document.createElement('tr');
    const cells = [
      radioIdFor(record, isRepeater) || '—',
      sanitize(record?.callsign) || '—',
      getRecordName(record, isRepeater) || '—',
      capitalizeWords(record?.city) || '—',
      capitalizeWords(record?.state) || '—',
      normalizedCountry(record) || '—',
      getRemarks(record, isRepeater) || '—'
    ];
    cells.forEach((value, index) => {
      const cell = document.createElement('td');
      cell.textContent = localizeText(value);
      if (index === 0) cell.className = 'numeric';
      if (index === 1 || index === 2) cell.classList.add('emphasis');
      row.append(cell);
    });
    tbody.append(row);
  });

  const end = Math.min(start + pageRecords.length, records.length);
  text('dv-preview-meta', `Showing ${start + 1}-${end} of ${records.length} rows`);
}

function updateUi() {
  const filtered = getFilteredRecords();
  const source = DATABASES[state.databaseKey];
  const devicesSelected = state.selectedDevices.size;
  const canChooseCountries = state.records.length > 0;
  const canExport = filtered.length > 0 && devicesSelected > 0 && !state.exportActive;

  setDisabled('dv-countries', !canChooseCountries || state.noCountriesOnly);
  setDisabled('dv-all', !canChooseCountries);
  setDisabled('dv-no-countries', !canChooseCountries);
  setDisabled('dv-download', state.downloadActive);
  setDisabled('dv-devices', !filtered.length);
  setDisabled('dv-export', !canExport);

  text('dv-metric-records', String(state.records.length));
  text('dv-metric-records-sub', state.records.length ? source.label : 'No dataset downloaded');
  text('dv-metric-countries', String(countDefinedCountries()));
  text('dv-metric-countries-sub', state.records.length ? `${state.countries.length} total filter buckets` : 'Available filter regions');
  text('dv-metric-selected', String(filtered.length));
  text('dv-metric-selected-sub', filtered.length ? `${filtered.length} rows ready for export` : 'Adjust filters to change scope');
  text('dv-metric-devices', String(devicesSelected));
  text('dv-metric-devices-sub', devicesSelected ? `${devicesSelected} device${devicesSelected === 1 ? '' : 's'} selected` : 'Choose one or more radios');
  text('dv-panel-meta', state.records.length ? `${source.label} · ${state.records.length} records loaded` : 'Waiting for download');

  setChip('dv-source-chip', state.records.length ? source.label : 'No dataset loaded', state.records.length ? 'soft' : 'warn');
  setChip('dv-selection-chip', `${filtered.length} contact${filtered.length === 1 ? '' : 's'} selected`, filtered.length ? 'soft' : 'warn');
  setChip('dv-status-chip', state.downloadActive ? 'Downloading…' : (state.exportActive ? 'Exporting…' : 'Ready'), state.downloadActive || state.exportActive ? 'warn' : 'soft');

  updatePreview();
}

async function fetchJsonWithProgress(url) {
  const response = await fetch(url, { cache: 'no-store' });
  if (!response.ok) throw new Error(`HTTP ${response.status}`);

  const total = Number.parseInt(response.headers.get('content-length') || '0', 10);
  if (!response.body || !window.TextDecoder) {
    setStatus('dv-download-status', 'Downloading database…', 'warn');
    return response.json();
  }

  const reader = response.body.getReader();
  const decoder = new TextDecoder();
  let received = 0;
  let textBuffer = '';

  while (true) {
    const { done, value } = await reader.read();
    if (done) break;
    received += value.length;
    textBuffer += decoder.decode(value, { stream: true });
    if (total > 0) {
      const percent = Math.trunc((received / total) * 100);
      setStatus('dv-download-status', `Downloading database… ${percent}%`, 'warn');
    } else {
      setStatus('dv-download-status', 'Downloading database…', 'warn');
    }
  }

  textBuffer += decoder.decode();
  return JSON.parse(textBuffer);
}

function toCountryBucketArray(records) {
  const countries = Array.from(new Set(records.map((record) => normalizedCountry(record))));
  return countries.sort((a, b) => {
    if (!a && b) return 1;
    if (a && !b) return -1;
    return a.localeCompare(b);
  });
}

async function downloadDatabase() {
  const source = DATABASES[state.databaseKey];
  state.downloadActive = true;
  state.previewPage = 1;
  state.records = [];
  state.countries = [];
  state.selectAll = true;
  state.noCountriesOnly = false;
  state.selectedCountries = new Set();
  $('dv-all').checked = true;
  $('dv-no-countries').checked = false;
  setStatus('dv-download-status', 'Starting download…', 'warn');
  setStatus('dv-export-status', 'Choose a dataset and at least one device.');
  updateUi();

  try {
    const urls = [source.url, ...(source.fallbackUrls || [])];
    let json = null;
    let lastError = null;

    for (const url of urls) {
      try {
        json = await fetchJsonWithProgress(url);
        break;
      } catch (error) {
        lastError = error;
      }
    }

    if (json === null) throw lastError || new Error('Download failed');

    const parsed = source.parse(json)
      .filter(Boolean)
      .map((record) => ({ ...record, country: normalizedCountry(record) }));

    state.records = parsed;
    state.countries = toCountryBucketArray(parsed);
    populateCountries();

    setStatus('dv-download-status', parsed.length
      ? `Loaded ${parsed.length} record${parsed.length === 1 ? '' : 's'} from ${source.label}.`
      : `No records returned by ${source.label}.`, parsed.length ? 'ok' : 'warn');
  } catch (error) {
    console.error('dvmanager download:', error);
    state.records = [];
    state.countries = [];
    populateCountries();
    setStatus('dv-download-status', `Unable to download ${source.label}. ${error.message}`, 'bad');
  } finally {
    state.downloadActive = false;
    updateUi();
  }
}

function aprsPasscode(callsign) {
  let value = String(callsign || '').toUpperCase();
  const dash = value.indexOf('-');
  if (dash > 1) value = value.slice(0, dash);
  const bytes = value.split('');
  let hash = 0x73e2;
  while (bytes.length) {
    const one = bytes.shift();
    const two = bytes.shift();
    hash ^= one.charCodeAt(0) << 8;
    if (two !== undefined) hash ^= two.charCodeAt(0);
  }
  return String(hash & 0x7fff);
}

function devicePattern(index, device, record) {
  const isRepeater = DATABASES[state.databaseKey].type === 'repeaters';
  const radioId = radioIdFor(record, isRepeater);
  if (!radioId) return '';

  const callsign = sanitize(record?.callsign);
  const city = capitalizeWords(record?.city);
  const stateName = capitalizeWords(record?.state);
  const country = normalizedCountry(record);
  const remarks = getRemarks(record, isRepeater);

  let fname = '';
  let name = '';
  let surname = '';
  if (isRepeater) {
    fname = capitalizeWords(record?.trustee);
    name = capitalizeWords(record?.ipsc_network);
    surname = capitalizeWords(record?.assigned);
  } else {
    fname = capitalizeWords(record?.fname);
    name = capitalizeWords(record?.name);
    surname = capitalizeWords(record?.surname);
  }
  if (!fname) fname = name;
  const wholeName = [fname, surname].filter(Boolean).join(' ').trim();

  switch (device) {
    case 'AnyTone':
      return [index, radioId, callsign, wholeName, city, stateName, country, remarks, '0', '0', ''].map(csvEscape).join(',');
    case 'AnyToneNX':
      return [radioId, callsign, wholeName, city, stateName, country, remarks, '0', '0', ''].map(csvEscape).join(',');
    case 'GD77':
      return [radioId, callsign, wholeName, '', city, stateName, country, ''].map(csvEscape).join(',');
    case 'HD1':
      return ['Private Call', `${callsign} ${fname}`.trim(), city, stateName, country, radioId].map(csvEscape).join(',');
    case 'MOTO':
      return [`${callsign} ${fname}`.trim(), radioId].map(csvEscape).join(',');
    case 'RT3S':
      return [radioId, callsign, wholeName, '', city, stateName, country].map(csvEscape).join(',');
    case 'RT52':
      return [radioId, wholeName, '1', '0', callsign, city, stateName, country, '1'].map(csvEscape).join(',');
    case 'TYT':
      return [radioId, callsign, wholeName, '', city, stateName, country, '', '', '', '', '', ''].map(csvEscape).join(',');
    case 'BAOFENG':
      return [callsign, '2', radioId, '0'].map(csvEscape).join(',');
    case 'DVPI':
      return [radioId, callsign, fname].map(csvEscape).join(',');
    case 'PISTAR':
      return `${radioId}\t${callsign}\t${fname}`;
    case 'NEXTION':
      return [radioId, callsign, wholeName, city, stateName, country].map(csvEscape).join(',');
    case 'BLUEDV':
      return [radioId, callsign, wholeName, city, stateName, country, 'DMR'].map(csvEscape).join(',');
    case 'DM-F8':
      return [index, callsign, 'Private Call', radioId, 'None', '', city, stateName, country, remarks, ''].map(csvEscape).join(',');
    default:
      return '';
  }
}

function headerForDevice(device) {
  switch (device) {
    case 'HD1': return { ext: '.csv', header: 'Call Type,Contacts Alias,City,Province,Country,Call ID\r\n' };
    case 'MOTO': return { ext: '.csv', header: 'Name,radio_id\r\n' };
    case 'RT52': return { ext: '.csv', header: 'Id,Name,Call Type,Call Alert,Call Sign,City,Province,Country,Remarks\r\n' };
    case 'RT3S': return { ext: '.csv', header: 'Radio ID,CallSign,Name,Nickname,City,State,Country\r\n' };
    case 'AnyTone': return { ext: '.csv', header: 'No.,Radio ID,Callsign,Name,City,State,Country,Remarks,Call Type,Call Alert\r\n' };
    case 'AnyToneNX': return { ext: '.csv', header: 'Radio ID,Callsign,Name,City,State,Country,Remarks,Call Type,Call Alert\r\n' };
    case 'GD77': return { ext: '.csv', header: 'Radio ID,Callsign,Name,NickName,City,State,Country,Remarks\r\n' };
    case 'TYT': return { ext: '.csv', header: 'Radio ID,Callsign,Name,NickName,City,State,Country,,,,,,\r\n' };
    case 'BAOFENG': return { ext: '.csv', header: 'Contact Name,Call Type,Call ID,Call Receive Tone\r\n' };
    case 'NEXTION': return { ext: '.csv', header: 'RADIO_ID,CALLSIGN,FIRST_NAME,LAST_NAME,CITY,STATE,COUNTRY\r\n' };
    case 'DM-F8': return { ext: '.csv', header: 'No.,Call Alias,Call Type,Call ID,Call Alert,Repeater Number,City,State / Prov,Country,Remarks,\r\n' };
    case 'PISTAR': return { ext: '.dat', header: '' };
    case 'DVPI':
    case 'BLUEDV':
    default:
      return { ext: '.csv', header: '' };
  }
}

function downloadTextFile(filename, content) {
  const blob = new Blob([content], { type: 'text/plain;charset=utf-8' });
  const link = document.createElement('a');
  const href = URL.createObjectURL(blob);
  link.href = href;
  link.download = filename;
  link.style.display = 'none';
  document.body.append(link);
  link.click();
  setTimeout(() => {
    link.remove();
    URL.revokeObjectURL(href);
  }, 250);
}

async function exportSelectedDevices() {
  const records = getFilteredRecords();
  if (!records.length) {
    setStatus('dv-export-status', 'No records match the current filters.', 'warn');
    return;
  }
  if (!state.selectedDevices.size) {
    setStatus('dv-export-status', 'Select at least one device before exporting.', 'warn');
    return;
  }

  state.exportActive = true;
  updateUi();
  const isRepeater = DATABASES[state.databaseKey].type === 'repeaters';
  const devices = Array.from(state.selectedDevices);

  try {
    for (const [position, device] of devices.entries()) {
      setStatus('dv-export-status', `Generating ${device} (${position + 1}/${devices.length})…`, 'warn');
      const config = headerForDevice(device);
      let content = config.header;
      records.forEach((record, index) => {
        const line = devicePattern(index, device, record);
        if (line) content += `${line}\r\n`;
      });
      const prefix = isRepeater ? 'Repeaters' : 'Contacts';
      downloadTextFile(`${prefix}_${device}${config.ext}`, content);
      await new Promise((resolve) => setTimeout(resolve, 180));
    }
    setStatus('dv-export-status', `Finished ${devices.length} export${devices.length === 1 ? '' : 's'}.`, 'ok');
  } catch (error) {
    console.error('dvmanager export:', error);
    setStatus('dv-export-status', `Export failed. ${error.message}`, 'bad');
  } finally {
    state.exportActive = false;
    updateUi();
  }
}

function handleCountrySelectionChange() {
  state.previewPage = 1;
  state.selectAll = false;
  $('dv-all').checked = false;
  state.selectedCountries = new Set(optionValues($('dv-countries')));
  updateUi();
}

function handleAllToggle() {
  state.previewPage = 1;
  state.selectAll = $('dv-all').checked;
  if (state.selectAll) {
    state.noCountriesOnly = false;
    $('dv-no-countries').checked = false;
    Array.from($('dv-countries').options).forEach((option) => { option.selected = true; });
    state.selectedCountries = new Set(state.countries);
  }
  updateUi();
}

function handleNoCountriesToggle() {
  state.previewPage = 1;
  state.noCountriesOnly = $('dv-no-countries').checked;
  if (state.noCountriesOnly) {
    state.selectAll = false;
    $('dv-all').checked = false;
    Array.from($('dv-countries').options).forEach((option) => { option.selected = false; });
    state.selectedCountries = new Set();
  }
  updateUi();
}

function handleDeviceSelectionChange() {
  state.selectedDevices = new Set(optionValues($('dv-devices')));
  updateUi();
}

function initPreviewControls() {
  const prevBtn = $('dv-preview-page-prev');
  const nextBtn = $('dv-preview-page-next');
  if ((!prevBtn && !nextBtn) || (prevBtn && prevBtn.dataset.bound === '1')) return;

  if (prevBtn) {
    prevBtn.dataset.bound = '1';
    prevBtn.addEventListener('click', () => {
      state.previewPage -= 1;
      updatePreview();
    });
  }

  if (nextBtn) {
    nextBtn.dataset.bound = '1';
    nextBtn.addEventListener('click', () => {
      state.previewPage += 1;
      updatePreview();
    });
  }
}

function init() {
  if (!$('dv-database')) return;
  populateDevices();
  initPreviewControls();

  $('dv-database').addEventListener('change', (event) => {
    state.databaseKey = event.target.value;
    state.previewPage = 1;
    state.records = [];
    state.countries = [];
    state.selectedCountries = new Set();
    state.selectAll = true;
    state.noCountriesOnly = false;
    $('dv-all').checked = true;
    $('dv-no-countries').checked = false;
    populateCountries();
    setStatus('dv-download-status', 'Ready. Download the selected database.');
    setStatus('dv-export-status', 'Choose a dataset and at least one device.');
    updateUi();
  });

  $('dv-countries').addEventListener('change', handleCountrySelectionChange);
  $('dv-all').addEventListener('change', handleAllToggle);
  $('dv-no-countries').addEventListener('change', handleNoCountriesToggle);
  $('dv-devices').addEventListener('change', handleDeviceSelectionChange);
  $('dv-download').addEventListener('click', downloadDatabase);
  $('dv-export').addEventListener('click', exportSelectedDevices);
  $('dv-generate').addEventListener('click', () => {
    const callsign = $('dv-callsign').value.trim();
    if (!callsign) {
      text('dv-passcode', '—');
      return;
    }
    text('dv-passcode', aprsPasscode(callsign));
  });
  $('dv-callsign').addEventListener('keydown', (event) => {
    if (event.key === 'Enter') {
      event.preventDefault();
      $('dv-generate').click();
    }
  });

  document.addEventListener('dmr:languagechange', () => {
    populateCountries();
    populateDevices();
    updateUi();
  });

  setStatus('dv-download-status', 'Ready. Download the selected database.');
  updateUi();
}

init();