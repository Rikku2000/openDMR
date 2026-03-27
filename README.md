# openDMR

<p align="center">
  <img src="https://raw.githubusercontent.com/github/explore/main/topics/cpp/cpp.png" alt="C++ Logo" width="80" height="80" />
</p>

<p align="center">
  <b>openDMR</b> — A cross-platform C++ Digital Mobile Radio (DMR) master/server for repeater linking, talkgroup routing, authentication, APRS, SMS, embedded web monitoring, and OpenBridge peering.
</p>

<p align="center">
  <a href="#"><img src="https://img.shields.io/badge/C%2B%2B-17-blue.svg" alt="C++17"></a>
  <a href="#"><img src="https://img.shields.io/badge/platform-Linux%20%7C%20Windows-lightgrey.svg" alt="Platforms"></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-Educational-green.svg" alt="License"></a>
  <a href="https://github.com/Rikku2000/openDMR/"><img src="https://img.shields.io/badge/runtime-single%20binary-brightgreen.svg" alt="Runtime"></a>
  <a href="https://github.com/Rikku2000/openDMR/graphs/contributors"><img src="https://img.shields.io/github/contributors/openDMR/openDMR.svg?color=blueviolet" alt="Contributors"></a>
</p>

---

## Overview

**openDMR** is a lightweight, cross-platform DMR master server that:

- links repeaters and routes digital voice/data traffic,
- authenticates nodes with SHA-256 challenge/response,
- supports talkgroups, parrot playback, APRS, and DMR-SMS,
- exposes an embedded HTTP dashboard and JSON API,
- supports up to 3 OpenBridge peers with filtering and aliasing,
- logs traffic and events to console or SQLite3.

The current package ships a runtime layout under `content/` with config, auth files, talkgroup data, optional SQLite log storage, and a bundled `www/` web UI.

---

## Features

- **UDP repeater protocol support** (`DMRD`, `RPTL`, `RPTK`, `RPTC`, `RPTPING`, etc.)
- **SHA-256 authentication** with per-node authorization
- **Dynamic talkgroup routing** and scanner group (`TG 777`)
- **Parrot echo test** (`TG 9990`)
- **Optional APRS and DMR-SMS modules**
- **SQLite3 logging** for recent and active traffic history
- **Embedded HTTP monitor and dashboard** with live JSON endpoints
- **Self-service web registration and profile management** backed by auth + DMR ID data files
- **Up to 3 OpenBridge peers** with talkgroup filters, aliases, DNS refresh, and optional enhanced HMAC mode
- **Cross-platform deployment** for Linux and Windows
- **Compact core** with minimal runtime complexity

---

## Build

### Linux

Basic build:

```bash
g++ -O2 -std=c++17 server.cpp -o dmr -lpthread
```

Recommended feature build (matches current project layout more closely):

```bash
g++ -O2 -std=c++17 server.cpp \
  -DUSE_SQLITE3 -DHAVE_APRS -DHAVE_HTTPSERVER -DUSE_OPENSSL \
  -lsqlite3 -lssl -lcrypto -lpthread -o dmr
```

Using the provided Makefile:

```bash
make
```

### Windows (MinGW)

```bash
g++ -O2 -std=c++17 server.cpp -lws2_32 -o dmr.exe
```

Feature-rich MinGW build usually also links SQLite/OpenSSL and uses the supplied recipe:

```bash
mingw32-make -f Makefile.mingw
```

The package also includes `build_mingw.bat` and a ready-to-run `content/server.exe` runtime.

---

## Runtime Layout

```text
.
├── server.cpp
├── server.h
├── readme.html
├── content/
│   ├── dmr.conf
│   ├── auth_users.csv
│   ├── talkgroup.dat
│   ├── banned.dat
│   ├── log.sqlite
│   └── www/
│       ├── index.html
│       ├── monitor.html
│       ├── openbridge.html
│       ├── register.html
│       ├── profile.html
│       ├── app.js
│       └── styles.css
```

Important runtime files:

| File | Purpose |
|------|---------|
| `content/dmr.conf` | Main server configuration |
| `content/auth_users.csv` | DMR-ID/password database for auth and web login |
| `content/talkgroup.dat` | Static talkgroup definitions |
| `content/banned.dat` | Ban list |
| `content/log.sqlite` | SQLite traffic/event log |
| `content/www/DMRIds.dat` | DMR profile database used by registration/profile pages |
| `content/www/` | Embedded web dashboard assets |

---

## Configuration

The current sample config is split into several sections:

```ini
[General]
Debug=1

[Server]
Host=0.0.0.0
Port=62031
Password=passw0rd
Housekeeping=1

[Homebrew]
KeepNodesAlive=1
NodeTimeout=1800
RelaxIPChange=1

[Monitor]
Enable=1
Port=8080
Root=www

[File]
Auth=auth_users.csv
DMRIds=www/DMRIds.dat
Log=log.sqlite
Talkgroup=talkgroup.dat
Banned=banned.dat

[DMR]
Scanner=777
Parrot=9990
APRS=900999

[Auth]
Enable=1
Reload=60
UnknownPolicy=0

[OpenBridge1]
Enable=0

[OpenBridge2]
Enable=0

[OpenBridge3]
Enable=0

[APRS]
Enable=1
Server=0.0.0.0
Port=14580
Callsign=N0CALL
Passcode=13023
Filter=r/0/0/0
Keepalive=60
Reconnect=30
IdMap=aprs_map.csv

[SMS]
Enable=1
UDPHost=127.0.0.1
UDPPort=5555
AllowPrivate=1
PermitAll=1
PermitTGs=900999
MaxFrames=30
MaxSeconds=5
```

### Key Sections

#### `[Server]`

| Key | Description | Sample |
|-----|-------------|--------|
| `Host` | Bind address | `0.0.0.0` |
| `Port` | Main DMR UDP port | `62031` |
| `Password` | Shared network password | `passw0rd` |
| `Housekeeping` | Enables periodic cleanup logic | `1` |

#### `[Homebrew]`

| Key | Description | Sample |
|-----|-------------|--------|
| `KeepNodesAlive` | Periodic keepalive support | `1` |
| `NodeTimeout` | Node timeout in seconds | `1800` |
| `RelaxIPChange` | Allows controlled node IP changes | `1` |

#### `[Monitor]`

| Key | Description | Sample |
|-----|-------------|--------|
| `Enable` | Enable embedded HTTP monitor/UI | `1` |
| `Port` | HTTP port for dashboard and API | `8080` |
| `Root` | Static web root | `www` |

#### `[File]`

| Key | Description | Sample |
|-----|-------------|--------|
| `Auth` | Auth CSV file | `auth_users.csv` |
| `DMRIds` | DMR ID profile database | `www/DMRIds.dat` |
| `Log` | SQLite log path | `log.sqlite` |
| `Talkgroup` | Talkgroup data file | `talkgroup.dat` |
| `Banned` | Ban list path | `banned.dat` |

#### `[DMR]`

| Key | Description | Sample |
|-----|-------------|--------|
| `Scanner` | Scanner talkgroup | `777` |
| `Parrot` | Echo/parrot talkgroup | `9990` |
| `APRS` | APRS heard trigger TG | `900999` |

#### `[Auth]`

| Key | Description | Sample |
|-----|-------------|--------|
| `Enable` | Turn repeater auth on/off | `1` |
| `Reload` | Reload auth data interval in seconds | `60` |
| `UnknownPolicy` | Default policy for unknown nodes | `0` |

#### `[OpenBridge1]` .. `[OpenBridge3]`

Each peer can be enabled independently. Current code/docs support:

- peer alias names for the UI,
- target host and port,
- local bind port,
- talkgroup allow/deny filters,
- DNS refresh,
- optional enhanced OpenBridge HMAC mode when built with OpenSSL.

#### `[APRS]`

| Key | Description | Sample |
|-----|-------------|--------|
| `Enable` | Enable APRS integration | `1` |
| `Server` | APRS-IS host | `0.0.0.0` |
| `Port` | APRS-IS port | `14580` |
| `Callsign` | Login callsign | `N0CALL` |
| `Passcode` | APRS passcode | `13023` |
| `Filter` | APRS filter string | `r/0/0/0` |
| `Keepalive` | APRS keepalive seconds | `60` |
| `Reconnect` | Reconnect interval seconds | `30` |
| `IdMap` | CSV mapping file | `aprs_map.csv` |

#### `[SMS]`

| Key | Description | Sample |
|-----|-------------|--------|
| `Enable` | Enable DMR-SMS integration | `1` |
| `UDPHost` | UDP destination host | `127.0.0.1` |
| `UDPPort` | UDP destination port | `5555` |
| `AllowPrivate` | Allow private SMS | `1` |
| `PermitAll` | Permit all senders/TGs | `1` |
| `PermitTGs` | Allowed TGs list | `900999` |
| `MaxFrames` | Maximum SMS frames | `30` |
| `MaxSeconds` | Maximum message window | `5` |

### Important Defaults

| Parameter | Default | Description |
|-----------|---------|-------------|
| UDP Port | `62031` | Main DMR network port |
| HTTP Port | `8080` | Embedded monitor/API port |
| Scanner TG | `777` | Monitor group |
| Parrot TG | `9990` | Echo test |
| APRS TG | `900999` | APRS heard trigger |
| Web session TTL | `86400` | Session lifetime in seconds |
| OpenBridge default port | `62044` | Base OBP local/remote port |

---

## Web Monitor & JSON API

When monitor mode is enabled, openDMR serves a bundled web UI from `content/www`.

### Included Pages

- `index.html` — dashboard/home
- `monitor.html` — browser view of `/STAT`
- `openbridge.html` — OpenBridge peer status
- `register.html` — self-service registration
- `profile.html` — authenticated profile/password management

### HTTP API

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/api/config` | `GET` | Returns auth/registration capability flags and configured DMR IDs file |
| `/api/login` | `POST` | Authenticates a DMR-ID/password and returns a session token |
| `/api/logout` | `POST` | Invalidates the current session |
| `/api/profile` | `GET` / `POST` | Reads or updates stored profile name/password |
| `/api/register` | `POST` | Creates a new auth + DMR ID entry if registration is enabled |
| `/api/active` | `GET` | Returns current active transmissions |
| `/api/log?limit=20` | `GET` | Returns recent call history from SQLite |
| `/api/openbridge` | `GET` | Returns OpenBridge health/timing data |
| `/api/stat` | `GET` | Proxies local `/STAT` output for the browser |

### Example

```bash
# local UDP status
echo "/STAT" | nc -u 127.0.0.1 62031

# web dashboard
# open http://127.0.0.1:8080/
# open http://127.0.0.1:8080/openbridge.html
```

---

## OpenBridge

The current server supports multiple outbound/inbound OpenBridge peers.

### Highlights

- up to **3 peers** via `[OpenBridge1]` .. `[OpenBridge3]`
- per-peer socket and resolution state
- alias names surfaced in the web UI
- talkgroup filtering and forwarding control
- DNS refresh / target re-resolution
- optional **enhanced OpenBridge** mode with HMAC validation/signing when OpenSSL is enabled

OpenBridge status is exposed through the embedded web API and web UI.

---

## Protocol Summary

| Type | Description |
|------|-------------|
| `DMRD` | Main voice/data frame |
| `RPTL` / `RPTACK` | Login handshake |
| `RPTK` | SHA-256 challenge/response auth |
| `RPTC` / `RPTO` | Static config / talkgroup setup |
| `RPTPING` / `MSTPONG` | Keepalive / heartbeat |
| `RPTCL` | Logout |
| `/STAT` | Local status command |

### Common Talkgroups

| TG | Function |
|----|----------|
| `4000` | Unsubscribe all |
| `777` | Scanner |
| `9990` | Parrot |
| `900999` | APRS heard trigger |

---

## Core Components

| Component | Purpose |
|-----------|---------|
| `node` | Connected repeater state |
| `slot` | TDMA slot abstraction |
| `talkgroup` | Subscriber/routing structure |
| `memfile` | In-memory buffers for parrot/SMS |
| `config_file` | Lightweight INI parser |

---

## API Overview

Representative functions from `server.h` include:

| Function | Description |
|----------|-------------|
| `init_process()` | Initialize runtime |
| `open_udp(port)` | Create and bind UDP socket |
| `make_sha256_hash()` | Compute auth hash |
| `auth_load_initial()` | Load auth file |
| `aprs_send_heard()` | Send APRS heard report |
| `sms_emit_udp()` | Forward SMS over UDP |
| `obp_forward_dmrd()` | Forward DMRD frames to OpenBridge peers |
| `obp_init()` | Initialize OpenBridge peer state |
| `obp_resolve_now()` | Force peer target resolution |
| `obp_housekeeping_all()` | Periodic peer maintenance |
| `http` / monitor handlers | Serve monitor/API responses |

---

## Threading Model

| Thread | Purpose |
|--------|---------|
| Main | UDP I/O loop |
| Timer | Tick/second counters and periodic tasks |
| Parrot playback | Replays buffered audio |
| APRS / SMS | Optional worker activity |
| HTTP monitor | Embedded web/API servicing when enabled |

---

## Run

### Linux

```bash
./dmr
```

### Windows

```bat
dmr.exe
```

With SQLite enabled, the server creates the `LOG` table when needed. The web UI polls JSON endpoints roughly every few seconds for active state and recent traffic.

---

## Logging

- **Console:** colorized live logging
- **SQLite3:** persistent `LOG` table with columns such as `DATE`, `RADIO`, `TG`, `TIME`, `SLOT`, `NODE`, `ACTIVE`, `CONNECT`
- **Web dashboard:** reads current runtime state and the log database for active calls, recent traffic, and OpenBridge health

---

## Contributing

1. Fork the repo and create a focused branch.
2. Keep changes small and documented.
3. Preserve compatibility on Linux and Windows.
4. Avoid unnecessary runtime dependencies.
5. Include notes for changes affecting routing, network protocol handling, web API, or OpenBridge behavior.

---

## License

Licensed for **educational and amateur radio experimentation**.
Ensure compliance with local radio, DMR, and spectrum regulations.

---

## Documentation

Full developer documentation, protocol details, config reference, architecture diagrams, and the expanded server notes are available in:

- [`readme.html`](./readme.html)
