const LANG_KEY = 'dmr.lang';
const DEFAULT_LANG = 'en';
const LANGUAGE_LABELS = { en: 'English', zh: '中文', de: 'Deutsch' };
const LOCALE_MAP = { en: 'en-US', zh: 'zh-CN', de: 'de-DE' };

const EXACT_TRANSLATIONS = {
  zh: {
  "Dashboard": "仪表盘",
  "Monitor": "监控",
  "Hotspot": "热点",
  "Map": "地图",
  "Manager": "管理器",
  "Register": "注册",
  "The Ultimate Gateway for Digital Operators Seeking Worldwide Connections Without the Static of Analog Radio.": "为数字通信爱好者打造的终极网关，让你畅联全球，不再受模拟电台噪声干扰。",
  "Waiting for live data": "等待实时数据",
  "No active transmissions yet": "尚无活动发射",
  "Connecting": "连接中",
  "Waiting for data": "等待数据",
  "Local time": "本地时间",
  "Loading date": "正在加载日期",
  "Auto refresh": "自动刷新",
  "Every 3 seconds": "每 3 秒",
  "Active calls": "活动呼叫",
  "No live activity": "暂无实时活动",
  "Recent talkgroups": "最近通话组",
  "Across latest 20 records": "基于最近 20 条记录",
  "Online radios": "在线电台",
  "Seen in recent activity": "已在最近活动中检测到",
  "Last update": "上次更新",
  "Waiting for first sync": "等待首次同步",
  "Currently Active": "当前活跃",
  "Live transmissions on the network right now": "当前网络中的实时发射",
  "No active calls": "当前无活动呼叫",
  "Date": "日期",
  "Radio": "电台",
  "Mode": "模式",
  "Slot": "时隙",
  "Node": "节点",
  "Secs": "秒",
  "Traffic snapshot": "流量快照",
  "Quick breakdown from the recent log": "最近日志的快速统计",
  "No log data yet": "暂无日志数据",
  "Recent PTT / Talkgroups": "最近 PTT / 通话组",
  "Latest 20 records from your activity log": "你的活动日志中的最新 20 条记录",
  "Awaiting log sync": "等待日志同步",
  "Active": "活跃",
  "Online": "在线",
  "Conn": "连接",
  "Server version unavailable": "服务器版本不可用",
  "programmed by": "程序开发：",
  "Github": "GitHub",
  "Toggle theme": "切换主题",
  "Primary navigation": "主导航",
  "Key metrics": "关键指标",
  "Recent PTT": "最近 PTT",
  "Server version": "服务器版本",
  "Node vectors": "节点向量",
  "No radio vectors loaded": "未加载电台向量",
  "Connected nodes": "已连接节点",
  "Waiting for /STAT": "等待 /STAT",
  "Active talkgroups": "活跃通话组",
  "No slot subscriptions yet": "暂无时隙订阅",
  "Server tick": "服务器 Tick",
  "Waiting for counters": "等待计数器",
  "Server Monitor": "服务器监控",
  "No reply yet": "尚无响应",
  "Node vector": "节点 vector",
  "Radio slot": "无线电时隙",
  "Node ID": "节点 ID",
  "Auth": "认证",
  "Seen sec": "已见秒数",
  "Slot 1 TG": "时隙 1 TG",
  "Slot 2 TG": "时隙 2 TG",
  "Raw /STAT output": "原始 /STAT 输出",
  "Waiting for /STAT reply…": "等待 /STAT 响应…",
  "Monitor metrics": "监控指标",
  "Server monitor table": "服务器监控表",
  "Loading peers": "正在加载对等端",
  "Waiting for /api/openbridge": "等待 /api/openbridge",
  "Bridge status": "桥接状态",
  "Auto refresh every 3 seconds": "每 3 秒自动刷新",
  "Configured peers": "已配置对等端",
  "No OpenBridge peers loaded": "未加载 OpenBridge 对等端",
  "Active peers": "活跃对等端",
  "No recent RX/TX": "最近无 RX/TX",
  "Last RX": "最近 RX",
  "Waiting for traffic": "等待流量",
  "Last TX": "最近 TX",
  "OpenBridge peers": "OpenBridge 对等端",
  "Each peer shows the remote server and current bridge state": "每个对等端都会显示远程服务器和当前桥接状态",
  "Awaiting bridge sync": "等待桥接同步",
  "Bridge": "桥接",
  "Status": "状态",
  "Remote server": "远程服务器",
  "Network ID": "网络 ID",
  "Enhanced": "增强",
  "Talkgroups": "通话组",
  "OpenBridge metrics": "OpenBridge 指标",
  "No static talkgroups received yet": "尚未接收到静态通话组",
  "Hotspots": "热点数",
  "Waiting for MMDVM config frames": "等待 MMDVM 配置帧",
  "Unique static TGs": "唯一静态 TG",
  "No static talkgroups active": "当前无静态通话组",
  "TS1 assignments": "TS1 分配",
  "No TS1 assignments": "无 TS1 分配",
  "TS2 assignments": "TS2 分配",
  "No TS2 assignments": "无 TS2 分配",
  "Hotspot static talkgroups": "热点静态通话组",
  "No hotspot data yet": "暂无热点数据",
  "Previous": "上一页",
  "Page 1 / 1": "第 1 页 / 共 1 页",
  "Next": "下一页",
  "Callsign": "呼号",
  "Current TS1": "当前 TS1",
  "Current TS2": "当前 TS2",
  "Static TS1": "静态 TS1",
  "Static TS2": "静态 TS2",
  "Last seen": "最后出现",
  "System talkgroup metrics": "系统通话组指标",
  "Hotspot pagination controls": "热点分页控件",
  "Hotspot static talkgroups table": "热点静态通话组表",
  "Waiting for map data": "等待地图数据",
  "No APRS stations or hotspot locations yet": "尚无 APRS 台站或热点位置",
  "APRS stations": "APRS 台站",
  "No APRS positions parsed yet": "尚未解析到 APRS 位置",
  "Hotspots with GPS": "带 GPS 的热点",
  "No hotspot LAT/LON received yet": "尚未收到热点经纬度",
  "Markers": "标记",
  "Waiting for positions": "等待位置数据",
  "Latest seen": "最近出现",
  "APRS Stations": "APRS 台站",
  "Map idle": "地图空闲",
  "Station": "台站",
  "Position table": "位置表",
  "Latest merged positions from APRS-IS and hotspot config frames": "来自 APRS-IS 与热点配置帧的最新合并位置",
  "No positions loaded": "未加载位置数据",
  "Callsign search": "呼号搜索",
  "Go to match": "跳转到匹配项",
  "Clear": "清除",
  "Type": "类型",
  "Display": "显示名称",
  "Latitude": "纬度",
  "Longitude": "经度",
  "Details": "详情",
  "APRS and map metrics": "APRS 和地图指标",
  "Map legend": "地图图例",
  "APRS map": "APRS 地图",
  "APRS table controls": "APRS 表格控件",
  "APRS pagination controls": "APRS 分页控件",
  "APRS and hotspot position table": "APRS 与热点位置表",
  "Search callsign, display, DMR ID, or node": "搜索呼号、显示名称、DMR ID 或节点",
  "Ready": "就绪",
  "No dataset loaded": "未加载数据集",
  "0 contacts selected": "已选择 0 个联系人",
  "Browser export": "浏览器导出",
  "Files are generated locally in your browser": "文件会在你的浏览器中本地生成",
  "Records loaded": "已加载记录",
  "No dataset downloaded": "尚未下载数据集",
  "Countries": "国家",
  "Available filter regions": "可用筛选区域",
  "Selected contacts": "已选联系人",
  "Adjust filters to change scope": "调整筛选条件以更改范围",
  "Export targets": "导出目标",
  "Choose one or more radios": "选择一个或多个电台",
  "Data source and filters": "数据源与筛选器",
  "Choose a public DMR database, then refine the export scope": "选择一个公开 DMR 数据库，然后细化导出范围",
  "Waiting for download": "等待下载",
  "Database": "数据库",
  "Users [github.com]": "用户 [github.com]",
  "Repeaters [github.com]": "中继台 [github.com]",
  "Users [freeradioid.net]": "用户 [freeradioid.net]",
  "Selection": "选择",
  "Select all countries": "选择所有国家",
  "Only undefined country": "仅未定义国家",
  "Download database": "下载数据库",
  "Ready.": "就绪.",
  "Selection preview": "选择预览",
  "Browse the current export set 10 rows at a time": "每次浏览当前导出集的 10 行",
  "No contacts selected": "未选择联系人",
  "Page 0 / 0": "第 0 页 / 共 0 页",
  "Name": "名称",
  "City": "城市",
  "State": "州/省",
  "Country": "国家",
  "Remarks": "备注",
  "Download a dataset to preview contacts": "下载数据集以预览联系人",
  "Export devices": "导出设备",
  "Generate radio-ready contact files": "生成可直接用于电台的联系人文件",
  "Devices": "设备",
  "Export selected devices": "导出所选设备",
  "Choose a dataset and at least one device.": "请选择一个数据集并至少选择一个设备。",
  "APRS passcode": "APRS 密码",
  "Generate an APRS-IS passcode from a callsign": "根据呼号生成 APRS-IS 密码",
  "Passcode": "密码",
  "Generate passcode": "生成密码",
  "DV Manager metrics": "DV 管理器指标",
  "Selection preview pagination controls": "选择预览分页控件",
  "Enter your callsign": "输入你的呼号",
  "Profile sync": "资料同步",
  "Session refresh on load": "页面加载时刷新会话",
  "User profile": "用户资料",
  "Update your display name and password": "更新你的显示名称和密码",
  "Please log in to edit your profile.": "请先登录以编辑你的资料。",
  "Current password": "当前密码",
  "New password": "新密码",
  "Confirm new password": "确认新密码",
  "Save profile": "保存资料",
  "Your name": "你的姓名",
  "Required only when changing password": "仅在修改密码时必填",
  "Leave blank to keep current password": "留空则保持当前密码",
  "Repeat new password": "再次输入新密码",
  "New user registration": "新用户注册",
  "Enter DMR-ID, callsign, name and password": "输入 DMR-ID、呼号、姓名和密码",
  "Password": "密码",
  "Save registration": "保存注册信息",
  "Choose a password": "设置一个密码",
  "English": "English",
  "Deutsch": "Deutsch",
  "中文": "中文",
  "Language": "语言",
  "Login": "登录",
  "Profile": "个人资料",
  "Logout": "退出登录",
  "Close": "关闭",
  "Use your DMR-ID and password": "使用你的 DMR-ID 和密码",
  "Logging in…": "登录中…",
  "Login successful.": "登录成功。",
  "Login failed.": "登录失败。",
  "Logged out.": "已退出登录。",
  "Registration enabled": "已启用注册",
  "Registration disabled": "已禁用注册",
  "Registration is disabled in dmr.conf.": "dmr.conf 中已禁用注册。",
  "Saving registration…": "正在保存注册信息…",
  "Registration saved.": "注册信息已保存。",
  "Unable to save registration.": "无法保存注册信息。",
  "Last successful refresh": "上次成功刷新",
  "No enabled OpenBridge peers configured": "未配置已启用的 OpenBridge 对等端",
  "Enable OpenBridge1 or OpenBridge2 in dmr.conf": "请在 dmr.conf 中启用 OpenBridge1 或 OpenBridge2",
  "No bridge traffic": "无桥接流量",
  "No peers": "无对等端",
  "Nothing to show until an OpenBridge section is enabled": "启用 OpenBridge 配置段后才会显示内容",
  "No peer active in the last 60s": "最近 60 秒内没有活跃对等端",
  "Bridge active": "桥接活跃",
  "Bridge idle": "桥接空闲",
  "Unable to load OpenBridge peers": "无法加载 OpenBridge 对等端",
  "Request failed": "请求失败",
  "Unavailable": "不可用",
  "Idle": "空闲",
  "Check that server.cpp includes /api/openbridge and restart the server": "请确认 server.cpp 包含 /api/openbridge，并重启服务器",
  "No active transmissions": "当前无活动发射",
  "Live traffic": "实时流量",
  "No recent log entries": "暂无最近日志记录",
  "Across rolling 20 records": "基于滚动的 20 条记录",
  "No recent activity": "暂无最近活动",
  "OpenBridge events": "OpenBridge 事件",
  "APRS events": "APRS 事件",
  "SMS events": "短信事件",
  "Voice / talk events": "语音 / 通话事件",
  "Unable to load recent log": "无法加载最近日志",
  "No connected nodes reported": "未报告已连接节点",
  "Live nodes": "活跃节点",
  "No active nodes reported": "未报告活跃节点",
  "No connected nodes found in /STAT output": "在 /STAT 输出中未找到已连接节点",
  "No /STAT reply": "无 /STAT 响应",
  "STAT connected": "STAT 已连接",
  "STAT idle": "STAT 空闲",
  "No connected nodes": "无已连接节点",
  "Unable to load /STAT output": "无法加载 /STAT 输出",
  "STAT unavailable": "STAT 不可用",
  "Check the local UDP /STAT responder and try again": "请检查本地 UDP /STAT 响应器后重试",
  "Unable to read /STAT": "无法读取 /STAT",
  "No hotspot static talkgroups reported yet": "尚未上报热点静态通话组",
  "No hotspot data": "暂无热点数据",
  "No static talkgroups announced by hotspots": "热点尚未公布静态通话组",
  "Total TS1 static assignments": "TS1 静态分配总数",
  "Total TS2 static assignments": "TS2 静态分配总数",
  "Unable to load hotspot static talkgroups": "无法加载热点静态通话组",
  "Check that /api/systemstg is available and restart the server": "请确认 /api/systemstg 可用，并重启服务器",
  "Lat/Lon": "纬度/经度",
  "Map live": "地图在线",
  "Stations plus hotspot markers": "台站与热点标记",
  "No packets yet": "尚无数据包",
  "Unable to load APRS map data": "无法加载 APRS 地图数据",
  "Check that /api/aprs is available and APRS is enabled": "请确认 /api/aprs 可用且 APRS 已启用",
  "Profile editing is disabled because auth is not enabled in dmr.conf.": "由于 dmr.conf 未启用认证，因此无法编辑个人资料。",
  "Ready. Leave password fields blank to change only the name.": "就绪。留空密码字段即可只修改名称。",
  "Please log in first.": "请先登录。",
  "New passwords do not match.": "两次输入的新密码不一致。",
  "Saving profile…": "正在保存个人资料…",
  "Profile updated.": "个人资料已更新。",
  "Unable to update profile.": "无法更新个人资料。",
  "Live APRS station packet": "实时 APRS 台站数据包",
  "Hotspot via openDMR": "通过 openDMR 的热点",
  "Center map on this callsign": "将地图定位到此呼号",
  "Unknown": "未知",
  "APRS station": "APRS 台站",
  "YES": "是",
  "NO": "否",
  "ALL": "全部",
  "Downloading…": "下载中…",
  "Exporting…": "导出中…",
  "Users [DMR-Database]": "用户 [DMR-Database]",
  "Repeaters [DMR-Database]": "中继台 [DMR-Database]",
  "No contacts match the current filter": "没有联系人匹配当前筛选条件",
  "0 rows match the current selection": "当前选择匹配 0 行",
  "Download failed": "下载失败",
  "No records match the current filters.": "没有记录匹配当前筛选条件。",
  "Select at least one device before exporting.": "导出前请至少选择一个设备。",
  "Ready. Download the selected database.": "就绪。请下载所选数据库。",
  "Undefined": "未定义",
  "ACTIVE": "活跃",
  "IDLE": "空闲",
  "UNRESOLVED": "未解析",
  "DISABLED": "已禁用",
  "UNKNOWN": "未知",
  "TALK": "语音",
  "SMS": "短信",
  "OBP": "OBP",
  "just now": "刚刚",
  "No APRS entries match the current search": "没有 APRS 条目匹配当前搜索"
  }, de: {
    "Dashboard": "Dashboard",
    "Monitor": "Monitor",
    "Hotspot": "Hotspot",
    "Map": "Karte",
    "Manager": "Manager",
    "Register": "Registrieren",
    "The Ultimate Gateway for Digital Operators Seeking Worldwide Connections Without the Static of Analog Radio.": "Das ultimative Gateway für Digitalfunker, die weltweite Verbindungen ohne das Rauschen des Analogfunks suchen.",
    "Waiting for live data": "Warte auf Live-Daten",
    "No active transmissions yet": "Noch keine aktiven Übertragungen",
    "Connecting": "Verbinde",
    "Waiting for data": "Warte auf Daten",
    "Local time": "Ortszeit",
    "Loading date": "Datum wird geladen",
    "Auto refresh": "Automatische Aktualisierung",
    "Every 3 seconds": "Alle 3 Sekunden",
    "Active calls": "Aktive Verbindungen",
    "No live activity": "Keine Live-Aktivität",
    "Recent talkgroups": "Letzte Talkgroups",
    "Across latest 20 records": "Basierend auf den letzten 20 Einträgen",
    "Online radios": "Online-Funkgeräte",
    "Seen in recent activity": "In letzter Aktivität gesehen",
    "Last update": "Letzte Aktualisierung",
    "Waiting for first sync": "Warte auf erste Synchronisierung",
    "Currently Active": "Derzeit aktiv",
    "Live transmissions on the network right now": "Aktuelle Live-Übertragungen im Netzwerk",
    "No active calls": "Keine aktiven Verbindungen",
    "Date": "Datum",
    "Radio": "Funkgerät",
    "Mode": "Modus",
    "Slot": "Zeitschlitz",
    "Node": "Knoten",
    "Secs": "Sek.",
    "Traffic snapshot": "Verkehrsübersicht",
    "Quick breakdown from the recent log": "Schnellübersicht aus dem letzten Protokoll",
    "No log data yet": "Noch keine Protokolldaten",
    "Recent PTT / Talkgroups": "Letzte PTT / Talkgroups",
    "Latest 20 records from your activity log": "Die letzten 20 Einträge aus deinem Aktivitätsprotokoll",
    "Awaiting log sync": "Warte auf Protokoll-Synchronisierung",
    "Active": "Aktiv",
    "Online": "Online",
    "Conn": "Verb.",
    "Server version unavailable": "Serverversion nicht verfügbar",
    "programmed by": "programmiert von",
    "Github": "GitHub",
    "Toggle theme": "Design umschalten",
    "Primary navigation": "Hauptnavigation",
    "Key metrics": "Kennzahlen",
    "Recent PTT": "Letzte PTT",
    "Server version": "Serverversion",
    "Node vectors": "Knotenvektoren",
    "No radio vectors loaded": "Keine Funkgeräte-Vektoren geladen",
    "Connected nodes": "Verbundene Knoten",
    "Waiting for /STAT": "Warte auf /STAT",
    "Active talkgroups": "Aktive Talkgroups",
    "No slot subscriptions yet": "Noch keine Slot-Abonnements",
    "Server tick": "Server-Tick",
    "Waiting for counters": "Warte auf Zähler",
    "Server Monitor": "Server-Monitor",
    "No reply yet": "Noch keine Antwort",
    "Node vector": "Knotenvektor",
    "Radio slot": "Funk-Slot",
    "Node ID": "Knoten-ID",
    "Auth": "Authentifizierung",
    "Seen sec": "Gesehen Sek.",
    "Slot 1 TG": "Slot 1 TG",
    "Slot 2 TG": "Slot 2 TG",
    "Raw /STAT output": "Rohes /STAT-Ergebnis",
    "Waiting for /STAT reply…": "Warte auf /STAT-Antwort…",
    "Monitor metrics": "Monitor-Kennzahlen",
    "Server monitor table": "Server-Monitor-Tabelle",
    "Loading peers": "Peers werden geladen",
    "Waiting for /api/openbridge": "Warte auf /api/openbridge",
    "Bridge status": "Bridge-Status",
    "Auto refresh every 3 seconds": "Automatische Aktualisierung alle 3 Sekunden",
    "Configured peers": "Konfigurierte Peers",
    "No OpenBridge peers loaded": "Keine OpenBridge-Peers geladen",
    "Active peers": "Aktive Peers",
    "No recent RX/TX": "Kein aktuelles RX/TX",
    "Last RX": "Letztes RX",
    "Waiting for traffic": "Warte auf Verkehr",
    "Last TX": "Letztes TX",
    "OpenBridge peers": "OpenBridge-Peers",
    "Each peer shows the remote server and current bridge state": "Jeder Peer zeigt den entfernten Server und den aktuellen Bridge-Status",
    "Awaiting bridge sync": "Warte auf Bridge-Synchronisierung",
    "Bridge": "Bridge",
    "Status": "Status",
    "Remote server": "Entfernter Server",
    "Network ID": "Netzwerk-ID",
    "Enhanced": "Erweitert",
    "Talkgroups": "Talkgroups",
    "OpenBridge metrics": "OpenBridge-Kennzahlen",
    "No static talkgroups received yet": "Noch keine statischen Talkgroups empfangen",
    "Hotspots": "Hotspots",
    "Waiting for MMDVM config frames": "Warte auf MMDVM-Konfigurationsframes",
    "Unique static TGs": "Eindeutige statische TGs",
    "No static talkgroups active": "Keine statischen Talkgroups aktiv",
    "TS1 assignments": "TS1-Zuweisungen",
    "No TS1 assignments": "Keine TS1-Zuweisungen",
    "TS2 assignments": "TS2-Zuweisungen",
    "No TS2 assignments": "Keine TS2-Zuweisungen",
    "Hotspot static talkgroups": "Statische Hotspot-Talkgroups",
    "No hotspot data yet": "Noch keine Hotspot-Daten",
    "Previous": "Zurück",
    "Page 1 / 1": "Seite 1 / 1",
    "Next": "Weiter",
    "Callsign": "Rufzeichen",
    "Current TS1": "Aktuelles TS1",
    "Current TS2": "Aktuelles TS2",
    "Static TS1": "Statisches TS1",
    "Static TS2": "Statisches TS2",
    "Last seen": "Zuletzt gesehen",
    "System talkgroup metrics": "System-Talkgroup-Kennzahlen",
    "Hotspot pagination controls": "Hotspot-Seitennavigation",
    "Hotspot static talkgroups table": "Tabelle statischer Hotspot-Talkgroups",
    "Waiting for map data": "Warte auf Kartendaten",
    "No APRS stations or hotspot locations yet": "Noch keine APRS-Stationen oder Hotspot-Standorte",
    "APRS stations": "APRS-Stationen",
    "No APRS positions parsed yet": "Noch keine APRS-Positionen verarbeitet",
    "Hotspots with GPS": "Hotspots mit GPS",
    "No hotspot LAT/LON received yet": "Noch keine Hotspot-LAT/LON empfangen",
    "Markers": "Marker",
    "Waiting for positions": "Warte auf Positionen",
    "Latest seen": "Zuletzt gesehen",
    "APRS Stations": "APRS-Stationen",
    "Map idle": "Karte inaktiv",
    "Station": "Station",
    "Position table": "Positionstabelle",
    "Latest merged positions from APRS-IS and hotspot config frames": "Neueste zusammengeführte Positionen aus APRS-IS und Hotspot-Konfigurationsframes",
    "No positions loaded": "Keine Positionen geladen",
    "Callsign search": "Rufzeichensuche",
    "Go to match": "Zum Treffer",
    "Clear": "Löschen",
    "Type": "Typ",
    "Display": "Anzeigename",
    "Latitude": "Breitengrad",
    "Longitude": "Längengrad",
    "Details": "Details",
    "APRS and map metrics": "APRS- und Karten-Kennzahlen",
    "Map legend": "Kartenlegende",
    "APRS map": "APRS-Karte",
    "APRS table controls": "APRS-Tabellensteuerung",
    "APRS pagination controls": "APRS-Seitennavigation",
    "APRS and hotspot position table": "APRS- und Hotspot-Positionstabelle",
    "Search callsign, display, DMR ID, or node": "Rufzeichen, Anzeigename, DMR-ID oder Knoten suchen",
    "Ready": "Bereit",
    "No dataset loaded": "Kein Datensatz geladen",
    "0 contacts selected": "0 Kontakte ausgewählt",
    "Browser export": "Browser-Export",
    "Files are generated locally in your browser": "Dateien werden lokal in deinem Browser erzeugt",
    "Records loaded": "Einträge geladen",
    "No dataset downloaded": "Kein Datensatz heruntergeladen",
    "Countries": "Länder",
    "Available filter regions": "Verfügbare Filterregionen",
    "Selected contacts": "Ausgewählte Kontakte",
    "Adjust filters to change scope": "Passe die Filter an, um den Umfang zu ändern",
    "Export targets": "Exportziele",
    "Choose one or more radios": "Wähle ein oder mehrere Funkgeräte",
    "Data source and filters": "Datenquelle und Filter",
    "Choose a public DMR database, then refine the export scope": "Wähle eine öffentliche DMR-Datenbank und verfeinere dann den Exportumfang",
    "Waiting for download": "Warte auf Download",
    "Database": "Datenbank",
    "Users [github.com]": "Benutzer [github.com]",
    "Repeaters [github.com]": "Relais [github.com]",
    "Users [freeradioid.net]": "Benutzer [freeradioid.net]",
    "Selection": "Auswahl",
    "Select all countries": "Alle Länder auswählen",
    "Only undefined country": "Nur undefiniertes Land",
    "Download database": "Datenbank herunterladen",
    "Ready.": "Bereit.",
    "Selection preview": "Auswahlvorschau",
    "Browse the current export set 10 rows at a time": "Das aktuelle Exportset jeweils 10 Zeilen auf einmal durchsuchen",
    "No contacts selected": "Keine Kontakte ausgewählt",
    "Page 0 / 0": "Seite 0 / 0",
    "Name": "Name",
    "City": "Stadt",
    "State": "Bundesland",
    "Country": "Land",
    "Remarks": "Bemerkungen",
    "Download a dataset to preview contacts": "Lade einen Datensatz herunter, um Kontakte anzuzeigen",
    "Export devices": "Geräte exportieren",
    "Generate radio-ready contact files": "Kontaktdateien für Funkgeräte erzeugen",
    "Devices": "Geräte",
    "Export selected devices": "Ausgewählte Geräte exportieren",
    "Choose a dataset and at least one device.": "Wähle einen Datensatz und mindestens ein Gerät.",
    "APRS passcode": "APRS-Passcode",
    "Generate an APRS-IS passcode from a callsign": "Einen APRS-IS-Passcode aus einem Rufzeichen erzeugen",
    "Passcode": "Passcode",
    "Generate passcode": "Passcode erzeugen",
    "DV Manager metrics": "DV-Manager-Kennzahlen",
    "Selection preview pagination controls": "Seitennavigation der Auswahlvorschau",
    "Enter your callsign": "Gib dein Rufzeichen ein",
    "Profile sync": "Profil-Synchronisierung",
    "Session refresh on load": "Sitzung beim Laden aktualisieren",
    "User profile": "Benutzerprofil",
    "Update your display name and password": "Anzeigenamen und Passwort aktualisieren",
    "Please log in to edit your profile.": "Bitte melde dich an, um dein Profil zu bearbeiten.",
    "Current password": "Aktuelles Passwort",
    "New password": "Neues Passwort",
    "Confirm new password": "Neues Passwort bestätigen",
    "Save profile": "Profil speichern",
    "Your name": "Dein Name",
    "Required only when changing password": "Nur beim Ändern des Passworts erforderlich",
    "Leave blank to keep current password": "Leer lassen, um das aktuelle Passwort beizubehalten",
    "Repeat new password": "Neues Passwort wiederholen",
    "New user registration": "Neue Benutzerregistrierung",
    "Enter DMR-ID, callsign, name and password": "DMR-ID, Rufzeichen, Name und Passwort eingeben",
    "Password": "Passwort",
    "Save registration": "Registrierung speichern",
    "Choose a password": "Wähle ein Passwort",
    "English": "English",
    "Deutsch": "Deutsch",
    "中文": "中文",
    "Language": "Sprache",
    "Login": "Anmelden",
    "Profile": "Profil",
    "Logout": "Abmelden",
    "Close": "Schließen",
    "Use your DMR-ID and password": "Verwende deine DMR-ID und dein Passwort",
    "Logging in…": "Anmeldung läuft…",
    "Login successful.": "Anmeldung erfolgreich.",
    "Login failed.": "Anmeldung fehlgeschlagen.",
    "Logged out.": "Abgemeldet.",
    "Registration enabled": "Registrierung aktiviert",
    "Registration disabled": "Registrierung deaktiviert",
    "Registration is disabled in dmr.conf.": "Die Registrierung ist in dmr.conf deaktiviert.",
    "Saving registration…": "Registrierung wird gespeichert…",
    "Registration saved.": "Registrierung gespeichert.",
    "Unable to save registration.": "Registrierung konnte nicht gespeichert werden.",
    "Last successful refresh": "Letzte erfolgreiche Aktualisierung",
    "No enabled OpenBridge peers configured": "Keine aktivierten OpenBridge-Peers konfiguriert",
    "Enable OpenBridge1 or OpenBridge2 in dmr.conf": "Aktiviere OpenBridge1 oder OpenBridge2 in dmr.conf",
    "No bridge traffic": "Kein Bridge-Verkehr",
    "No peers": "Keine Peers",
    "Nothing to show until an OpenBridge section is enabled": "Nichts anzuzeigen, bis ein OpenBridge-Abschnitt aktiviert ist",
    "No peer active in the last 60s": "In den letzten 60 s war kein Peer aktiv",
    "Bridge active": "Bridge aktiv",
    "Bridge idle": "Bridge inaktiv",
    "Unable to load OpenBridge peers": "OpenBridge-Peers konnten nicht geladen werden",
    "Request failed": "Anfrage fehlgeschlagen",
    "Unavailable": "Nicht verfügbar",
    "Idle": "Inaktiv",
    "Check that server.cpp includes /api/openbridge and restart the server": "Prüfe, ob server.cpp /api/openbridge enthält, und starte den Server neu",
    "No active transmissions": "Keine aktiven Übertragungen",
    "Live traffic": "Live-Verkehr",
    "No recent log entries": "Keine aktuellen Protokolleinträge",
    "Across rolling 20 records": "Basierend auf den laufenden letzten 20 Einträgen",
    "No recent activity": "Keine aktuelle Aktivität",
    "OpenBridge events": "OpenBridge-Ereignisse",
    "APRS events": "APRS-Ereignisse",
    "SMS events": "SMS-Ereignisse",
    "Voice / talk events": "Sprach- / Sprechereignisse",
    "Unable to load recent log": "Letztes Protokoll konnte nicht geladen werden",
    "No connected nodes reported": "Keine verbundenen Knoten gemeldet",
    "Live nodes": "Live-Knoten",
    "No active nodes reported": "Keine aktiven Knoten gemeldet",
    "No connected nodes found in /STAT output": "Keine verbundenen Knoten in der /STAT-Ausgabe gefunden",
    "No /STAT reply": "Keine /STAT-Antwort",
    "STAT connected": "STAT verbunden",
    "STAT idle": "STAT inaktiv",
    "No connected nodes": "Keine verbundenen Knoten",
    "Unable to load /STAT output": "/STAT-Ausgabe konnte nicht geladen werden",
    "STAT unavailable": "STAT nicht verfügbar",
    "Check the local UDP /STAT responder and try again": "Prüfe den lokalen UDP-/STAT-Responder und versuche es erneut",
    "Unable to read /STAT": "/STAT konnte nicht gelesen werden",
    "No hotspot static talkgroups reported yet": "Noch keine statischen Hotspot-Talkgroups gemeldet",
    "No hotspot data": "Keine Hotspot-Daten",
    "No static talkgroups announced by hotspots": "Keine statischen Talkgroups von Hotspots angekündigt",
    "Total TS1 static assignments": "Gesamte statische TS1-Zuweisungen",
    "Total TS2 static assignments": "Gesamte statische TS2-Zuweisungen",
    "Unable to load hotspot static talkgroups": "Statische Hotspot-Talkgroups konnten nicht geladen werden",
    "Check that /api/systemstg is available and restart the server": "Prüfe, ob /api/systemstg verfügbar ist, und starte den Server neu",
    "Lat/Lon": "Breite/Länge",
    "Map live": "Karte live",
    "Stations plus hotspot markers": "Stationen plus Hotspot-Marker",
    "No packets yet": "Noch keine Pakete",
    "Unable to load APRS map data": "APRS-Kartendaten konnten nicht geladen werden",
    "Check that /api/aprs is available and APRS is enabled": "Prüfe, ob /api/aprs verfügbar ist und APRS aktiviert ist",
    "Profile editing is disabled because auth is not enabled in dmr.conf.": "Die Profilbearbeitung ist deaktiviert, weil die Authentifizierung in dmr.conf nicht aktiviert ist.",
    "Ready. Leave password fields blank to change only the name.": "Bereit. Lass die Passwortfelder leer, um nur den Namen zu ändern.",
    "Please log in first.": "Bitte melde dich zuerst an.",
    "New passwords do not match.": "Die neuen Passwörter stimmen nicht überein.",
    "Saving profile…": "Profil wird gespeichert…",
    "Profile updated.": "Profil aktualisiert.",
    "Unable to update profile.": "Profil konnte nicht aktualisiert werden.",
    "Live APRS station packet": "Live-APRS-Station-Paket",
    "Hotspot via openDMR": "Hotspot über openDMR",
    "Center map on this callsign": "Karte auf dieses Rufzeichen zentrieren",
    "Unknown": "Unbekannt",
    "APRS station": "APRS-Station",
    "YES": "JA",
    "NO": "NEIN",
    "ALL": "ALLE",
    "Downloading…": "Wird heruntergeladen…",
    "Exporting…": "Wird exportiert…",
    "Users [DMR-Database]": "Benutzer [DMR-Database]",
    "Repeaters [DMR-Database]": "Relais [DMR-Database]",
    "No contacts match the current filter": "Keine Kontakte entsprechen dem aktuellen Filter",
    "0 rows match the current selection": "0 Zeilen entsprechen der aktuellen Auswahl",
    "Download failed": "Download fehlgeschlagen",
    "No records match the current filters.": "Keine Datensätze entsprechen den aktuellen Filtern.",
    "Select at least one device before exporting.": "Wähle vor dem Export mindestens ein Gerät aus.",
    "Ready. Download the selected database.": "Bereit. Lade die ausgewählte Datenbank herunter.",
    "Undefined": "Undefiniert",
    "ACTIVE": "AKTIV",
    "IDLE": "INAKTIV",
    "UNRESOLVED": "UNGELÖST",
    "DISABLED": "DEAKTIVIERT",
    "UNKNOWN": "UNBEKANNT",
    "TALK": "SPRECHEN",
    "SMS": "SMS",
    "OBP": "OBP",
    "just now": "gerade eben",
    "No APRS entries match the current search": "Keine APRS-Einträge entsprechen der aktuellen Suche"
  }
};

const TEXT_NODE_KEY = '__i18nOriginalText';

export function getSupportedLanguages() {
  return ['en', 'zh', 'de'];
}

export function getLanguageLabel(lang) {
  return LANGUAGE_LABELS[lang] || lang;
}

export function getLanguage() {
  const stored = localStorage.getItem(LANG_KEY);
  return getSupportedLanguages().includes(stored) ? stored : DEFAULT_LANG;
}

export function getLocale(lang = getLanguage()) {
  return LOCALE_MAP[lang] || LOCALE_MAP[DEFAULT_LANG];
}

function translateExact(text, lang = getLanguage()) {
  if (!text || lang === 'en') return text;
  return EXACT_TRANSLATIONS[lang]?.[text] || text;
}

function interpolate(template, vars = {}) {
  return String(template).replace(/\{(\w+)\}/g, (_, key) => String(vars[key] ?? ''));
}

export function t(key, vars = {}) {
  return interpolate(translateExact(key), vars);
}

function localizePattern(value, lang = getLanguage()) {
  if (lang === 'en') return value;
  const str = String(value ?? '');
  let m = null;

  if (lang === 'de') {
    if ((m = str.match(/^Server: (.+)$/))) return `Server: ${m[1]}`;
    if ((m = str.match(/^Updated (.+)$/))) return `Aktualisiert ${m[1]}`;
    if ((m = str.match(/^DMR-ID: (.+)$/))) return `DMR-ID: ${m[1]}`;
    if ((m = str.match(/^Page (\d+) \/ (\d+)$/))) return `Seite ${m[1]} / ${m[2]}`;
    if ((m = str.match(/^Showing (\d+)-(\d+) of (\d+) rows$/))) return `Zeige ${m[1]}-${m[2]} von ${m[3]} Zeilen`;
    if ((m = str.match(/^Showing (\d+)-(\d+) of (\d+) hotspots?$/))) return `Zeige ${m[1]}-${m[2]} von ${m[3]} Hotspots`;
    if ((m = str.match(/^Showing (\d+)-(\d+) of (\d+) position rows?(?: · (\d+) matches? for “(.+)”)?$/))) {
      return m[4]
        ? `Zeige ${m[1]}-${m[2]} von ${m[3]} Positionszeilen · ${m[4]} Treffer für „${m[5]}“`
        : `Zeige ${m[1]}-${m[2]} von ${m[3]} Positionszeilen`;
    }
    if ((m = str.match(/^0 matches for “(.+)”$/))) return `0 Treffer für „${m[1]}“`;
    if ((m = str.match(/^Undefined \((\d+)\)$/))) return `Undefiniert (${m[1]})`;
    if ((m = str.match(/^(\d+) total filter buckets$/))) return `${m[1]} Filtergruppen insgesamt`;
    if ((m = str.match(/^(\d+) rows ready for export$/))) return `${m[1]} Zeilen exportbereit`;
    if ((m = str.match(/^(\d+) device(?:s)? selected$/))) return `${m[1]} Gerät(e) ausgewählt`;
    if ((m = str.match(/^(.+) · (\d+) records loaded$/))) return `${m[1]} · ${m[2]} Einträge geladen`;
    if ((m = str.match(/^(\d+) contact(?:s)? selected$/))) return `${m[1]} Kontakt(e) ausgewählt`;
    if ((m = str.match(/^Downloading database… (\d+)%$/))) return `Datenbank wird heruntergeladen… ${m[1]}%`;
    if ((m = str.match(/^Loaded (\d+) record(?:s)? from (.+)\.$/))) return `${m[1]} Einträge aus ${m[2]} geladen.`;
    if ((m = str.match(/^No records returned by (.+)\.$/))) return `Keine Einträge von ${m[1]} zurückgegeben.`;
    if ((m = str.match(/^Unable to download (.+)\. (.+)$/))) return `${m[1]} konnte nicht heruntergeladen werden. ${m[2]}`;
    if ((m = str.match(/^Generating (.+) \((\d+)\/(\d+)\)…$/))) return `${m[1]} wird erzeugt (${m[2]}/${m[3]})…`;
    if ((m = str.match(/^Finished (\d+) export(?:s)?\.$/))) return `${m[1]} Export(e) abgeschlossen.`;
    if ((m = str.match(/^Export failed\. (.+)$/))) return `Export fehlgeschlagen. ${m[1]}`;
    if ((m = str.match(/^(\d+) peer(?:s)? loaded$/))) return `${m[1]} Peer(s) geladen`;
    if ((m = str.match(/^Latest radio (.+)$/))) return `Letztes Funkgerät ${m[1]}`;
    if ((m = str.match(/^(\d+) active call(?:s)?$/))) return `${m[1]} aktive Verbindung(en)`;
    if ((m = str.match(/^(\d+) active call(?:s)? across (\d+) talkgroup(?:s)?$/))) return `${m[1]} aktive Verbindung(en) über ${m[2]} Talkgroup(s)`;
    if ((m = str.match(/^(.+? \(.+\)) on (.+)$/))) return `${m[1]} auf ${m[2]}`;
    if ((m = str.match(/^Latest: (.+)$/))) return `Zuletzt: ${m[1]}`;
    if ((m = str.match(/^Out of (\d+) recent records$/))) return `Aus ${m[1]} letzten Einträgen`;
    if ((m = str.match(/^(\d+) rows loaded$/))) return `${m[1]} Zeilen geladen`;
    if ((m = str.match(/^Server sec (\d+)$/))) return `Server-Sek. ${m[1]}`;
    if ((m = str.match(/^(\d+) node(?:s)? across (\d+) vector(?:s)?$/))) return `${m[1]} Knoten über ${m[2]} Vektor(en)`;
    if ((m = str.match(/^(\d+) node(?:s)? loaded$/))) return `${m[1]} Knoten geladen`;
    if ((m = str.match(/^(.+) seen (.+) · (\d+) unique static talkgroup(?:s)?$/))) return `${m[1]} gesehen ${m[2]} · ${m[3]} eindeutige statische Talkgroups`;
    if ((m = str.match(/^(\d+) APRS station(?:s)? · (\d+) hotspot(?:s)?$/))) return `${m[1]} APRS-Station(en) · ${m[2]} Hotspot(s)`;
    if ((m = str.match(/^(\d+) marker(?:s)? on map$/))) return `${m[1]} Marker auf der Karte`;
    if ((m = str.match(/^(\d+)m ago$/))) return `vor ${m[1]} Min.`;
    if ((m = str.match(/^(\d+)h ago$/))) return `vor ${m[1]} Std.`;
    if ((m = str.match(/^(\d+)d ago$/))) return `vor ${m[1]} T.`;
    if ((m = str.match(/^(\d+)s ago$/))) return `vor ${m[1]} Sek.`;
    if ((m = str.match(/^(.+) from (.+)$/))) return `${m[1]} von ${m[2]}`;
    if ((m = str.match(/^(.+) to (.+)$/))) return `${m[1]} an ${m[2]}`;
    return translateExact(str, lang);
  }

  if ((m = str.match(/^Server: (.+)$/))) return `服务器：${m[1]}`;
  if ((m = str.match(/^Updated (.+)$/))) return `已更新 ${m[1]}`;
  if ((m = str.match(/^DMR-ID: (.+)$/))) return `DMR-ID：${m[1]}`;
  if ((m = str.match(/^Page (\d+) \/ (\d+)$/))) return `第 ${m[1]} 页 / 共 ${m[2]} 页`;
  if ((m = str.match(/^Showing (\d+)-(\d+) of (\d+) rows$/))) return `显示第 ${m[1]}-${m[2]} 条，共 ${m[3]} 行`;
  if ((m = str.match(/^Showing (\d+)-(\d+) of (\d+) hotspots?$/))) return `显示第 ${m[1]}-${m[2]} 条，共 ${m[3]} 个热点`;
  if ((m = str.match(/^Showing (\d+)-(\d+) of (\d+) position rows?(?: · (\d+) matches? for “(.+)”)?$/))) {
    return m[4]
      ? `显示第 ${m[1]}-${m[2]} 条，共 ${m[3]} 条位置记录 · “${m[5]}” 匹配 ${m[4]} 条`
      : `显示第 ${m[1]}-${m[2]} 条，共 ${m[3]} 条位置记录`;
  }
  if ((m = str.match(/^0 matches for “(.+)”$/))) return `“${m[1]}” 匹配 0 条`;
  if ((m = str.match(/^Undefined \((\d+)\)$/))) return `未定义 (${m[1]})`;
  if ((m = str.match(/^(\d+) total filter buckets$/))) return `共 ${m[1]} 个筛选分组`;
  if ((m = str.match(/^(\d+) rows ready for export$/))) return `${m[1]} 行可导出`;
  if ((m = str.match(/^(\d+) device(?:s)? selected$/))) return `已选择 ${m[1]} 个设备`;
  if ((m = str.match(/^(.+) · (\d+) records loaded$/))) return `${m[1]} · 已加载 ${m[2]} 条记录`;
  if ((m = str.match(/^(\d+) contact(?:s)? selected$/))) return `已选择 ${m[1]} 个联系人`;
  if ((m = str.match(/^Downloading database… (\d+)%$/))) return `正在下载数据库… ${m[1]}%`;
  if ((m = str.match(/^Loaded (\d+) record(?:s)? from (.+)\.$/))) return `已从 ${m[2]} 加载 ${m[1]} 条记录。`;
  if ((m = str.match(/^No records returned by (.+)\.$/))) return `${m[1]} 未返回任何记录。`;
  if ((m = str.match(/^Unable to download (.+)\. (.+)$/))) return `无法下载 ${m[1]}。${m[2]}`;
  if ((m = str.match(/^Generating (.+) \((\d+)\/(\d+)\)…$/))) return `正在生成 ${m[1]}（${m[2]}/${m[3]}）…`;
  if ((m = str.match(/^Finished (\d+) export(?:s)?\.$/))) return `已完成 ${m[1]} 个导出任务。`;
  if ((m = str.match(/^Export failed\. (.+)$/))) return `导出失败。${m[1]}`;
  if ((m = str.match(/^(\d+) peer(?:s)? loaded$/))) return `已加载 ${m[1]} 个对等端`;
  if ((m = str.match(/^Latest radio (.+)$/))) return `最新电台 ${m[1]}`;
  if ((m = str.match(/^(\d+) active call(?:s)?$/))) return `${m[1]} 个活动呼叫`;
  if ((m = str.match(/^(\d+) active call(?:s)? across (\d+) talkgroup(?:s)?$/))) return `${m[1]} 个活动呼叫，分布于 ${m[2]} 个通话组`;
  if ((m = str.match(/^(.+? \(.+\)) on (.+)$/))) return `${m[1]} 在 ${m[2]}`;
  if ((m = str.match(/^Latest: (.+)$/))) return `最新：${m[1]}`;
  if ((m = str.match(/^Out of (\d+) recent records$/))) return `在最近 ${m[1]} 条记录中`;
  if ((m = str.match(/^(\d+) rows loaded$/))) return `已加载 ${m[1]} 行`;
  if ((m = str.match(/^Server sec (\d+)$/))) return `服务器秒数 ${m[1]}`;
  if ((m = str.match(/^(\d+) node(?:s)? across (\d+) vector(?:s)?$/))) return `${m[1]} 个节点，${m[2]} 个向量`;
  if ((m = str.match(/^(\d+) node(?:s)? loaded$/))) return `已加载 ${m[1]} 个节点`;
  if ((m = str.match(/^(.+) seen (.+) · (\d+) unique static talkgroup(?:s)?$/))) return `${m[1]} 最近出现于 ${m[2]} · ${m[3]} 个唯一静态通话组`;
  if ((m = str.match(/^(\d+) APRS station(?:s)? · (\d+) hotspot(?:s)?$/))) return `${m[1]} 个 APRS 台站 · ${m[2]} 个热点`;
  if ((m = str.match(/^(\d+) marker(?:s)? on map$/))) return `地图上有 ${m[1]} 个标记`;
  if ((m = str.match(/^(\d+)m ago$/))) return `${m[1]} 分钟前`;
  if ((m = str.match(/^(\d+)h ago$/))) return `${m[1]} 小时前`;
  if ((m = str.match(/^(\d+)d ago$/))) return `${m[1]} 天前`;
  if ((m = str.match(/^(\d+)s ago$/))) return `${m[1]} 秒前`;
  if ((m = str.match(/^(.+) from (.+)$/))) return `${m[1]}，来自 ${m[2]}`;
  if ((m = str.match(/^(.+) to (.+)$/))) return `${m[1]}，发往 ${m[2]}`;
  return translateExact(str, lang);
}

export function localizeText(value, lang = getLanguage()) {
  if (value === null || value === undefined) return value;
  if (typeof value !== 'string') return value;
  return localizePattern(value, lang);
}

function translateTextNode(node, lang = getLanguage()) {
  const raw = node[TEXT_NODE_KEY] ?? node.textContent;
  if (node[TEXT_NODE_KEY] === undefined) node[TEXT_NODE_KEY] = raw;
  const trimmed = raw.trim();
  if (!trimmed) return;
  const translated = localizeText(trimmed, lang);
  node.textContent = raw.replace(trimmed, translated);
}

function translateAttribute(el, attr, lang = getLanguage()) {
  if (!el.hasAttribute(attr)) return;
  const storeKey = `i18nOriginal${attr.replace(/(^|-)([a-z])/g, (_, __, c) => c.toUpperCase())}`;
  const raw = el.dataset[storeKey] || el.getAttribute(attr) || '';
  if (!el.dataset[storeKey]) el.dataset[storeKey] = raw;
  el.setAttribute(attr, localizeText(raw, lang));
}

export function applyTranslations(root = document) {
  const lang = getLanguage();
  const scope = root instanceof Document ? root.documentElement : root;
  if (!scope) return;

  const walker = document.createTreeWalker(scope, NodeFilter.SHOW_TEXT, {
    acceptNode(node) {
      const parent = node.parentElement;
      if (!parent) return NodeFilter.FILTER_REJECT;
      if (['SCRIPT', 'STYLE', 'TEXTAREA'].includes(parent.tagName)) return NodeFilter.FILTER_REJECT;
      return node.textContent.trim() ? NodeFilter.FILTER_ACCEPT : NodeFilter.FILTER_REJECT;
    }
  });

  const nodes = [];
  while (walker.nextNode()) nodes.push(walker.currentNode);
  nodes.forEach((node) => translateTextNode(node, lang));

  scope.querySelectorAll?.('*').forEach((el) => {
    translateAttribute(el, 'placeholder', lang);
    translateAttribute(el, 'aria-label', lang);
    translateAttribute(el, 'title', lang);
  });

  document.documentElement.lang = lang === 'zh' ? 'zh-CN' : lang === 'de' ? 'de-DE' : 'en';
}

export function initLang() {
  applyTranslations(document);
}

export function setLanguage(lang) {
  const next = getSupportedLanguages().includes(lang) ? lang : DEFAULT_LANG;
  localStorage.setItem(LANG_KEY, next);
  applyTranslations(document);
  document.dispatchEvent(new CustomEvent('dmr:languagechange', { detail: { lang: next } }));
}
