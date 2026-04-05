# openDMR

openDMR is a cross-platform C++ Digital Mobile Radio (DMR) network server/master that links repeaters, routes talkgroups, authenticates nodes, and can optionally provide APRS forwarding, DMR-SMS handling, SQLite logging, OpenBridge peering, a newer optional unified uplink subsystem, and an embedded multi-page web UI.

This README reflects the current codebase, including the newer HTTP/session APIs in `server.cpp` / `server.h` and the newer browser assets in `content/www`.

## Highlights

- Homebrew-style DMR UDP server with `DMRD`, login/auth, config, ping, and logout handling
- Talkgroup routing with slot ownership arbitration for preloaded and static-configured talkgroups
- Static TS1 / TS2 subscriptions from repeater config frames
- Parrot / echo talkgroup support (`9990` by default)
- Optional APRS forwarding (`900999` by default)
- Optional DMR-SMS buffering and UDP emission to external tools
- Optional SQLite activity logging for active calls, recent history, and online state
- Embedded HTTP monitor with static asset serving and JSON endpoints
- Web registration, login, logout, session refresh, and profile editing
- OpenBridge peer support with filtering, alias names, hostname re-resolution, and optional enhanced/HMAC framing
- Optional unified uplink support (`USE_UPLINK`) for `Uplink1..8` plus `OpenBridge1..3` sections
- Browser-side DV Manager for contact export and APRS passcode generation
- Linux and Windows / MinGW support

## Repository layout

```text
.
├── server.cpp              # main server implementation
├── server.h                # public declarations, protocol structs, monitor hooks
├── readme.html             # full HTML documentation
├── content/
│   ├── dmr.conf            # runtime configuration
│   ├── auth_users.csv      # auth + web login credential store
│   ├── talkgroup.dat       # talkgroup names / aliases
│   ├── banned.dat          # blocked radio IDs
│   ├── log.sqlite          # SQLite log database (if enabled)
│   └── www/
│       ├── index.html      # dashboard
│       ├── monitor.html    # /STAT monitor view
│       ├── openbridge.html # OpenBridge status page
│       ├── dvmanager.html  # contact export / APRS passcode tool
│       ├── register.html   # self-service registration
│       ├── profile.html    # authenticated profile editor
│       ├── app.js          # shared front-end runtime helpers
│       ├── styles.css      # shared styles / themes
│       └── dvmanager.js    # DV Manager client logic
├── Makefile
├── Makefile.mingw
└── build_mingw.bat
```

## Features

### Core DMR server

- UDP packet handling for login, authentication, config, keepalive, traffic, and logout frames
- Talkgroup routing for entries loaded from `talkgroup.dat` and static `TS1` / `TS2` repeater config payloads
- Slot ownership / anti-collision logic per talkgroup
- Scanner TG fan-out (`777` by default)
- Private-call routing when the destination radio is known
- Global unsubscribe control TG (`4000`)

### Authentication

- SHA-256 challenge/response repeater authentication
- CSV-backed DMR-ID/password authentication
- Auth reload timer from config
- Unknown node policy control

### APRS (optional)

- Heard reporting via APRS-IS
- DMR-ID to callsign mapping via `aprs_map.csv`
- Configurable APRS keepalive and reconnect timers

### SMS (optional)

- SMS frame buffering by stream
- TG allow-list / permit-all policy
- Optional private-message forwarding
- UDP export to external software

### Logging (optional)

- SQLite `LOG` table support
- Recent activity / active call / online-state views for the dashboard
- Callsign enrichment from the DMR ID data file for UI output

### Embedded web monitor

- Static file serving from the configured document root
- In-process HTTP listener with client limits and socket timeouts
- JSON APIs for config, login, profile, logs, active traffic, OpenBridge status, and raw `/STAT`
- Session-token authentication for protected actions

### Browser UI in `content/www`

- Dashboard page
- Monitor page
- OpenBridge page
- Register page
- Profile page
- DV Manager page
- Shared theme, polling, and auth helpers in `app.js`

## Quick start

### Linux (full-feature build)

```bash
make
./server
```

### Linux (manual build)

```bash
g++ server.cpp -o server \
  -DUSE_SQLITE3 -DUSE_OPENSSL -DHAVE_APRS -DHAVE_SMS -DHAVE_HTTPMODE \
  -lsqlite3 -lcrypto -lssl -lpthread
./server
```

### Windows (MinGW)

```bat
build_mingw.bat
content\server.exe
```

## Build notes

### Dependencies

Required:

- C++17 compiler
- sockets (`POSIX` on Linux, `WinSock` on Windows)
- pthreads or platform thread support

Optional:

- SQLite3
- OpenSSL

### Common feature flags

- `USE_SQLITE3` — SQLite logging
- `USE_OPENSSL` — OpenSSL helpers for enhanced hashing/HMAC use
- `HAVE_APRS` — APRS support
- `HAVE_SMS` — SMS support
- `HAVE_HTTPMODE` — embedded HTTP monitor/UI
- `USE_UPLINK` — unified uplink subsystem (`Uplink1..8` plus `OpenBridge1..3`)

### Example builds

Minimal:

```bash
g++ -O2 -std=c++17 server.cpp -o dmr -lpthread
```

With common optional modules:

```bash
g++ -O2 -std=c++17 server.cpp -o dmr \
  -DUSE_SQLITE3 -DUSE_OPENSSL -DHAVE_APRS -DHAVE_SMS -DHAVE_HTTPMODE \
  -lsqlite3 -lcrypto -lssl -lpthread
```

## Runtime layout

The shipped package is designed to run from the `content/` directory. The embedded HTTP server expects the configured document root to contain the web assets.

Typical runtime files:

- `dmr.conf`
- `auth_users.csv`
- `talkgroup.dat`
- `banned.dat`
- `log.sqlite` (optional)
- `www/DMRIds.dat`
- `www/*.html`, `app.js`, `styles.css`, `dvmanager.js`

## Configuration reference

openDMR uses an INI-style `dmr.conf`.

### `[General]`

- `Debug` — verbose console logging

### `[Server]`

- `Host` — bind / advertised host string
- `Port` — main DMR UDP port
- `Password` — shared network password
- `Housekeeping` — housekeeping interval in minutes

### `[Homebrew]`

- `KeepNodesAlive` — keep idle nodes present longer
- `NodeTimeout` — stale-node timeout in seconds
- `RelaxIPChange` — allow IP changes for a known node

### `[Monitor]`

- `Enable` — enable embedded HTTP mode
- `Port` — monitor HTTP port
- `Root` — static document root

### `[File]`

- `Auth` — auth CSV path
- `DMRIds` — DMR ID profile database path
- `Log` — SQLite database path
- `Talkgroup` — talkgroup file path
- `Banned` — banned-radio list path

### `[DMR]`

- `Scanner` — scanner TG
- `Parrot` — parrot TG
- `APRS` — APRS trigger TG

### `[Auth]`

- `Enable` — enable login/registration/profile flows
- `Reload` — auth CSV reload interval
- `UnknownPolicy` — unknown-node policy

### `[OpenBridge1]` … `[OpenBridge3]`

Without `USE_UPLINK`, the classic OpenBridge loader reads up to three peers. The current code looks for keys including:

- `Enable`
- `LocalPort`
- `TargetHost`
- `TargetPort`
- `AliasName`
- `NetworkId`
- `Passphrase`
- `ForceSlot1`
- `PermitAll`
- `PermitTGs`
- `EnhancedOBP`
- `RelaxChecks`
- `HBLinkCompat`
- `ResolveInterval`

With `USE_UPLINK`, those same `OpenBridge1..3` sections are folded into the newer unified uplink loader.

### `[Uplink1]` … `[Uplink8]` (`USE_UPLINK`)

When compiled with `USE_UPLINK`, the server also supports general uplink sections with options such as:

- `Enable`
- `Protocol` (`homebrew` or `openbridge`)
- `Type` (`custom`, `brandmeister`, `tgif`)
- `Name` / `AliasName`
- `LocalPort`
- `TargetHost`
- `TargetPort`
- `RadioId` / `NodeId` (Homebrew uplinks)
- `Password` / `Passphrase`
- `StaticTS1` / `StaticTS2`
- `ForceSlot1`
- `PermitAll` / `PermitTGs`
- `RelaxChecks`
- `ResolveInterval`

### `[APRS]`

- `Enable`
- `Server`
- `Port`
- `Callsign`
- `Passcode`
- `Filter`
- `Keepalive`
- `Reconnect`
- `IdMap`

### `[SMS]`

- `Enable`
- `UDPHost`
- `UDPPort`
- `AllowPrivate`
- `PermitAll`
- `PermitTGs`
- `MaxFrames`
- `MaxSeconds`

### Packaged sample config

The bundled `content/dmr.conf` currently ships with these notable defaults:

```ini
[Server]
Host=0.0.0.0
Port=62031
Password=passw0rd
Housekeeping=1

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
```

## Defaults and constants

Important built-in values from the current code:

- `DEFAULT_PORT = 62031`
- `DEFAULT_HOUSEKEEPING_MINUTES = 1`
- `LOW_DMRID = 1000000`
- `HIGH_DMRID = 8000000`
- `UNSUBSCRIBE_ALL_TG = 4000`
- scanner TG = `777`
- parrot TG = `9990`
- APRS TG = `900999`
- web session TTL = `86400` seconds
- max HTTP clients = `128`
- HTTP listen backlog = `256`
- client timeout = `15000 ms`
- default OpenBridge local/remote port = `62000`

## DMR protocol handled by the server

### Traffic frames

Classic `DMRD` frames are handled as 55-byte packets without HMAC.

Important fields include:

- magic: `DMRD`
- radio ID
- destination TG or private target ID
- node ID
- slot / stream flags
- stream ID
- 35-byte DMR payload section

### Control / login flow

The current implementation handles:

- `RPTL` — login request
- `RPTK` — auth response using `SHA256(salt || password)`
- `RPTC` / `RPTO` — config / static TG payloads
- `RPTPING` → `MSTPONG`
- `FMRPING` → `FMRPONG`
- `RPTCL` — logout
- `/STAT` — UDP status dump request

### Packet flow summary

1. Node sends `RPTL`
2. Server replies with `RPTACK` containing a salt
3. Node sends `RPTK`
4. Server accepts with `RPTACK` or rejects with `MSTNAK`
5. Node sends config and traffic
6. Periodic ping/pong keeps the node alive
7. Logout removes the node

## Core behavior

### Talkgroup routing

- talkgroups are loaded from `talkgroup.dat` and can also be created from static `TS1` / `TS2` config payloads
- unknown runtime group traffic is not auto-created on first key-up; it is dropped after unsubscribe handling
- the first active slot owns the talkgroup until end/timeout
- static subscribers are tracked separately from dynamic activity
- scanner behavior follows the current active source until end/timeout

### Private calls

If the target radio ID is known in the server index, frames can be forwarded directly to that radio’s last-seen slot.

### Parrot

The parrot talkgroup records frames into an in-memory buffer and replays them back with timed pacing after end-of-stream.

## Threading and timing

The server is mostly event-driven around the main UDP loop, with lightweight helper threads for:

- system timing (`g_tick` / `g_sec` maintenance)
- parrot playback
- monitor client handling in HTTP mode
- optional APRS/SMS housekeeping behavior

## `server.h` monitor API

When compiled with `HAVE_HTTPMODE`, `server.h` exposes:

```cpp
typedef struct {
    const char* bind_addr;
    int port;
    const char* doc_root;
} MonitorConfig;

typedef enum {
    MON_SRC_UNKNOWN = 0,
    MON_SRC_LOCAL   = 1,
    MON_SRC_OBP     = 2
} MonSrc;

void monitor_note_event(int radio, int tg, MonSrc src, int is_aprs, int is_sms);
void monitor_start(const MonitorConfig* cfg);
void monitor_stop(void);
```

These hooks are used by the embedded monitor to tag recent events by source and to serve the browser UI.

## OpenBridge

The current code supports multiple OpenBridge peers with per-peer state. When built with `USE_UPLINK`, OpenBridge peers are managed through the unified uplink subsystem, but the runtime behavior and `/api/openbridge` view still focus on OpenBridge-protocol peers.

Capabilities include:

- up to 3 configured peers
- alias names for UI display
- hostname resolution and re-resolution
- TG filtering via `PermitAll` / `PermitTGs`
- enhanced mode flag for HMAC-aware operation
- relaxed packet checks for interoperability testing
- per-peer last RX / TX / ping timing exposed in the UI

The `/api/openbridge` endpoint returns peer records with fields such as:

- peer name and alias
- enabled state
- local port
- target host and port
- network ID
- resolved IP
- last RX / TX / ping timestamps
- seconds since RX / TX
- `enhanced`
- `permitAll`
- `permitTGs`
- computed status like `active`, `idle`, or `unresolved`

## Web UI and HTTP API

When built with `HAVE_HTTPMODE` and enabled in config, the server serves static files from the configured `Root` and exposes JSON/text endpoints.

### Included pages

- `/` or `/index.html` — dashboard
- `/monitor.html` — parsed and raw `/STAT` view
- `/openbridge.html` — OpenBridge health page
- `/dvmanager.html` — browser-side contact export + APRS passcode tool
- `/register.html` — registration page
- `/profile.html` — authenticated profile page

### API endpoints

#### `GET /api/config`

Returns feature flags and runtime metadata such as whether auth, registration, and profile flows are enabled, the configured `DMRIds.dat` path, and a build/version string.

#### `POST /api/login`

Form fields:

- `dmrid`
- `password`

Returns a session token plus:

- `dmrid`
- `callsign`
- `name`

#### `POST /api/logout`

Invalidates the current session token.

#### `GET /api/profile`

Requires a valid session token. Returns the current account profile.

#### `POST /api/profile`

Requires a valid session token. Supports:

- `name`
- `currentPassword`
- `newPassword`

Used to update display name and/or password.

#### `POST /api/register`

Form fields:

- `dmrid`
- `callsign`
- `name`
- `password`

Adds a new auth entry and profile entry, backed by the configured auth CSV and DMR ID data file.

#### `GET /api/active`

Returns active-call style data from SQLite when logging is enabled.

#### `GET /api/openbridge`

Returns current OpenBridge peer status as JSON.

#### `GET /api/stat`

Fetches the raw local `/STAT` text output and exposes it via HTTP.

#### `GET /api/log?limit=N`

Returns recent per-radio log rows from SQLite, enriched with callsign and source markers.

### Authentication header

The browser UI stores the token client-side and normally sends it in:

```http
X-Auth-Token: <session-token>
```

The server also accepts the standard `Authorization` header as a fallback. Sessions are stored in memory and refreshed on lookup until expiry.

## DV Manager

`content/www/dvmanager.html` and `dvmanager.js` add a browser-only utility page that is separate from the server’s own JSON API.

It can:

- download public radio/user datasets in the browser
- filter by country
- show undefined-country entries
- export device-specific contact files for multiple radios and tools
- generate APRS passcodes locally in the browser

This page does not need to write back to the server database. It does not depend on `/api/log`, `/api/active`, or other embedded monitor endpoints for its export workflow.

## Front-end runtime additions

The newer shared web assets include:

- `app.js` — shared auth, polling, rendering, theme persistence, and local storage helpers
- `styles.css` — common dashboard/theme/card/table styling

The packaged pages use these shared assets for a consistent multi-page UI, and the dashboard-style pages poll the main monitor endpoints roughly every 3 seconds.

## Running the server

From the packaged runtime directory:

```bash
./server
```

Useful flags:

```text
-d   enable verbose/debug packet logging
-s   query a running local instance via UDP /STAT
```

Useful local URLs:

```text
http://127.0.0.1:8080/
http://127.0.0.1:8080/monitor.html
http://127.0.0.1:8080/openbridge.html
http://127.0.0.1:8080/dvmanager.html
http://127.0.0.1:8080/register.html
http://127.0.0.1:8080/profile.html
```

Raw UDP status request:

```bash
echo "/STAT" | nc -u 127.0.0.1 62031
```

## Logging details

With SQLite enabled, the server writes a `LOG` table with fields used by the dashboard and APIs, including timestamps, radio ID, TG, slot, node, duration, connection state, and whether a radio is currently active/online.

The monitor layer also tags recent activity with source metadata so the UI can distinguish local traffic, OpenBridge traffic, APRS-related events, and SMS-related events.

## FAQ

### What happens when traffic arrives on an unknown TG?

The server does not auto-create a new talkgroup from an arbitrary runtime group call. In practice, TGs need to come from `talkgroup.dat` or from static `TS1` / `TS2` config payloads before normal group routing applies.

### How are private calls handled?

If the target radio is known to the node index, the server can forward directly to its last-seen slot.

### What does the embedded web server need?

Build with `HAVE_HTTPMODE`, enable `[Monitor]`, and make sure the configured `Root` contains the bundled web files.

### How do registration and profile storage work?

Registration writes to both the auth CSV and the configured `DMRIds.dat`-style file. Login returns a session token. Profile reads/updates are then performed against the authenticated DMR-ID.

### Does DV Manager depend on the local server database?

No. It is primarily a browser-side utility that fetches public data and exports files locally for the user.

## Contributing

- keep changes focused and reviewable
- follow the existing C++ style
- prefer minimal new runtime dependencies
- test on Linux and Windows where practical
- document behavior changes that affect network semantics or config

## License

Provided as-is for educational and amateur radio experimentation. Make sure your use complies with local regulations and spectrum rules.
