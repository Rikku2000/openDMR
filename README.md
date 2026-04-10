# openDMR

openDMR is a cross-platform C++ Digital Mobile Radio (DMR) network server/master for Homebrew-style repeaters and hotspots. It authenticates nodes, routes group and private traffic, supports static TS1/TS2 talkgroup configuration, and can optionally add APRS forwarding, DMR-SMS handling, SQLite activity logging, an embedded HTTP monitor, OpenBridge peering, and a unified uplink subsystem.

This README reflects the current codebase and packaged web UI, including the newer Hotspot and Map pages, `/api/aprs`, and the current HTTP/session APIs implemented in `server.cpp` and `server.h`.

## Highlights

- Homebrew-style UDP server for `DMRD`, `RPTL`, `RPTK`, `RPTC`/`RPTO`, `RPTPING`, `FMRPING`, and `RPTCL`
- SHA-256 challenge/response repeater authentication with CSV-backed per-DMR-ID passwords
- Talkgroup routing with slot ownership arbitration, scanner TG fan-out, and static TS1/TS2 subscriptions
- Private-call forwarding to the destination radio’s last-seen slot
- Parrot / echo talkgroup support (`9990` by default)
- Optional APRS heard forwarding and APRS-IS position ingestion
- Optional DMR-SMS buffering with UDP export to external tools
- Optional SQLite logging for recent activity, active calls, and online-state views
- Optional parallel DMRD receive workers via `WorkerThreads` (`1..32`)
- Embedded multi-page HTTP monitor with static asset serving and JSON/text APIs
- Web registration, login, logout, and profile editing with in-memory session tokens
- Classic OpenBridge peering plus optional unified uplinks via `USE_UPLINK`
- Browser-side DV Manager, Hotspot, and Map tools in `content/www`
- Linux and Windows / MinGW support

## Repository layout

```text
.
├── server.cpp              # main server implementation
├── server.h                # declarations, monitor hooks, helpers
├── readme.html             # full HTML documentation
├── README.md               # this GitHub README
├── content/
│   ├── dmr.conf            # runtime configuration
│   ├── auth_users.csv      # auth + web login credential store
│   ├── aprs_map.csv        # optional APRS DMR-ID to callsign map
│   ├── talkgroup.dat       # talkgroup names / aliases
│   ├── banned.dat          # blocked radio IDs
│   ├── log.sqlite          # SQLite log database when enabled
│   ├── server.exe          # Windows build artifact in packaged bundle
│   └── www/
│       ├── index.html      # dashboard
│       ├── monitor.html    # /STAT monitor view
│       ├── openbridge.html # OpenBridge status page
│       ├── hotspot.html    # hotspot static TG overview
│       ├── map.html        # APRS + hotspot map page
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
- Talkgroup routing for entries loaded from `talkgroup.dat`
- Static `TS1` / `TS2` subscriptions parsed from repeater config payloads (`RPTC` / `RPTO`)
- Slot ownership / anti-collision logic per talkgroup
- Scanner TG fan-out (`777` by default)
- Private-call forwarding when the destination radio is known
- Global unsubscribe control TG (`4000`)
- Optional parallel receive workers for `DMRD` traffic, sharded by stream ID or node ID fallback when `WorkerThreads > 1`

### Authentication

- SHA-256 challenge/response repeater authentication
- CSV-backed DMR-ID/password authentication
- Configurable auth reload timer
- Configurable unknown-node policy
- Web login, logout, registration, and profile editing when auth is enabled

### APRS (optional)

- Heard reporting via the APRS trigger talkgroup (`900999` by default)
- APRS-IS client with keepalive and reconnect handling
- DMR-ID to callsign mapping via `aprs_map.csv`
- `/api/aprs` JSON endpoint for live APRS station positions
- Hotspot location export when repeaters include `LAT` / `LON` style fields in `RPTC` config payloads

### SMS (optional)

- SMS frame buffering by stream
- TG allow-list / permit-all policy
- Optional private-message forwarding
- UDP export to external software

### Logging (optional)

- SQLite `LOG` table support with `SRC` and `SEQ` columns in current builds
- Recent activity, active call, and online-state views for the dashboard
- `/api/log` grouping modes for recent activity by talkgroup, by user, or raw rows
- Callsign enrichment from the configured DMR ID data file for UI output

### Embedded web monitor

- Static file serving from the configured document root
- In-process HTTP listener with explicit client limits and socket timeouts
- JSON/text APIs for config, login, profile, registration, logs, active traffic, hotspot/static TG visibility, APRS positions, OpenBridge status, and raw `/STAT`
- Session-token authentication for protected actions

### Browser UI in `content/www`

- Dashboard page
- Monitor page
- OpenBridge page
- Hotspot page
- Map page
- Register page
- Profile page
- DV Manager page
- Shared theme, polling, and auth helpers in `app.js`

## Quick start

### Linux (full-feature build with unified uplinks)

```bash
g++ server.cpp -o server \
  -DUSE_SQLITE3 -DUSE_OPENSSL -DHAVE_APRS -DHAVE_SMS -DHAVE_HTTPMODE -DUSE_UPLINK \
  -lsqlite3 -lcrypto -lssl -lpthread
./server
```

### Linux (classic build without `USE_UPLINK`)

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

> Run the binary from the runtime directory that contains `dmr.conf`, `talkgroup.dat`, and `banned.dat`. When auth is enabled, `auth_users.csv` and the configured `DMRIds.dat` path are also required.

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

With unified uplinks:

```bash
g++ -O2 -std=c++17 server.cpp -o dmr \
  -DUSE_SQLITE3 -DUSE_OPENSSL -DHAVE_APRS -DHAVE_SMS -DHAVE_HTTPMODE -DUSE_UPLINK \
  -lsqlite3 -lcrypto -lssl -lpthread
```

## Runtime layout

The packaged bundle is designed to run from the `content/` directory. The embedded HTTP server expects the configured document root to contain the web assets.

Typical runtime files:

- `dmr.conf`
- `auth_users.csv`
- `talkgroup.dat`
- `banned.dat`
- `aprs_map.csv`
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
- `WorkerThreads` — number of parallel DMRD receive workers; `1` keeps inline handling and values are clamped to `1..32`

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
- `UnknownPolicy` — unknown-node policy (`1` = use global password, `0` = deny)

### `[SQLite]`

Available when compiled with `USE_SQLITE3`:

- `MaxRows` — maximum retained rows in the rolling `LOG` window
- `ActiveListLimit` — maximum rows returned by `GET /api/active`
- `ActiveTimeout` — stale-active timeout in seconds before the server closes lingering active rows

### `[OpenBridge1]` … `[OpenBridge3]`

Without `USE_UPLINK`, the classic OpenBridge loader reads up to three peers. Common keys include:

- `Enable`
- `AliasName`
- `Port` / `LocalPort`
- `TargetHost`
- `TargetPort`
- `NetworkId`
- `Passphrase`
- `ForceSlot1`
- `PermitAll`
- `PermitTGs`
- `EnhancedOBP`
- `HBLinkCompat`
- `RelaxChecks`
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

### Hotspot location fields from `RPTC`

The current code also parses hotspot location values from repeater config payloads. It accepts these keys case-insensitively:

- `LAT` or `LATITUDE`
- `LON`, `LONG`, or `LONGITUDE`

When present and valid, those values are exposed through `/api/aprs` and shown on `map.html` as hotspot markers.

### Packaged sample config

The bundled `content/dmr.conf` currently ships with these notable defaults:

```ini
[Server]
Host=0.0.0.0
Port=62031
Password=passw0rd
Housekeeping=1
WorkerThreads=4

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

If those keys are omitted, the current code defaults include `WorkerThreads=1`, `[SQLite] MaxRows=20`, `[SQLite] ActiveListLimit=5`, and `[SQLite] ActiveTimeout=8`.

## Defaults and constants

Important built-in values from the current code:

- `DEFAULT_PORT = 62031`
- `DEFAULT_HOUSEKEEPING_MINUTES = 1`
- default worker threads = `1`
- maximum worker threads = `32`
- default SQLite max rows = `20`
- default SQLite active list limit = `5`
- default SQLite active timeout = `8` seconds
- `LOW_DMRID = 1000000`
- `HIGH_DMRID = 8000000`
- `UNSUBSCRIBE_ALL_TG = 4000`
- scanner TG = `777`
- parrot TG = `9990`
- APRS TG = `900999`
- web session TTL = `86400` seconds
- `MONITOR_MAX_CLIENTS = 128`
- `MONITOR_LISTEN_BACKLOG = 256`
- `MONITOR_CLIENT_TIMEOUT_MS = 15000`
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

If the target radio ID is known to the server index, frames can be forwarded directly to that radio’s last-seen slot.

### Parrot

The parrot talkgroup records frames into an in-memory buffer and replays them back with timed pacing after end-of-stream.

## Threading and timing

The server is mostly event-driven around the main UDP loop, with lightweight helper threads for timing, parrot playback, and per-client HTTP handling. When `[Server] WorkerThreads > 1`, the runtime also starts a fixed RX worker pool for `DMRD` traffic.

Key runtime pieces are:

- system timing (`g_tick` / `g_sec` maintenance)
- RX worker pool (`start_rx_workers`) for `DMRD` traffic, sharded by stream ID or node ID fallback so related packets stay on the same worker
- parrot playback after end-of-stream
- monitor client handling in HTTP mode
- APRS reconnect/keepalive, SMS housekeeping, auth reload checks, and stale SQLite active-row cleanup in the main runtime loop

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

## OpenBridge and uplinks

The current code supports multiple OpenBridge peers with per-peer state. When built with `USE_UPLINK`, OpenBridge peers are managed through the unified uplink subsystem, but the runtime behavior and `/api/openbridge` view still focus on OpenBridge-protocol peers.

Capabilities include:

- up to 3 configured OpenBridge peers in classic mode
- alias names for UI display
- hostname resolution and periodic re-resolution
- TG filtering via `PermitAll` / `PermitTGs`
- optional enhanced mode for HMAC-aware operation
- relaxed packet checks for interoperability testing
- per-peer last RX / TX / ping timing exposed in the UI
- up to 8 additional generic uplinks with Homebrew or OpenBridge transport when `USE_UPLINK` is enabled
- Homebrew uplinks that can perform upstream login/auth/ping and push upstream `StaticTS1` / `StaticTS2` values

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

When built with `HAVE_HTTPMODE` and enabled in config, the server serves static files from the configured `Root` and exposes JSON/text endpoints. The embedded listener enforces explicit client caps and per-client read/write timeouts.

### Included pages

- `/` or `/index.html` — dashboard
- `/monitor.html` — parsed and raw `/STAT` view
- `/openbridge.html` — OpenBridge health page
- `/hotspot.html` — hotspot static talkgroup dashboard sourced from `/api/systemstg`
- `/map.html` — merged APRS station and hotspot map sourced from `/api/aprs`
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

Returns active transmissions from SQLite when logging is enabled, limited by `[SQLite] ActiveListLimit`. Rows include callsign and talkgroup metadata plus `src`, APRS, and SMS markers.

#### `GET /api/systemstg`

Lists authenticated nodes that currently have static `TS1` / `TS2` subscriptions, including node ID, DMR-ID, callsign/name, last-seen age, current slot talkgroups, and configured static CSVs.

#### `GET /api/aprs`

Returns merged position data for:

- APRS stations received from APRS-IS
- hotspots that reported valid location fields in their config payloads

Hotspot rows include node ID, DMR ID, auth state, current TS1/TS2 values, and configured static TG CSVs. This endpoint powers `map.html`.

#### `GET /api/openbridge`

Returns current OpenBridge peer status as JSON.

#### `GET /api/stat`

Fetches the raw local `/STAT` text output and exposes it via HTTP.

#### `GET /api/log?limit=N&mode=tg|user|raw`

Returns recent log rows from SQLite, enriched with callsign, talkgroup metadata, and source markers. `mode=tg` groups by source/TG/slot/node, `mode=user` groups by source/radio, and `mode=raw` returns the latest raw rows. `limit` accepts `1..500`.

### Authentication header

The browser UI stores the token client-side and normally sends it in:

```http
X-Auth-Token: <session-token>
```

The server also accepts the standard `Authorization` header as a fallback. Sessions are stored in memory and refreshed on lookup until expiry.

## Hotspot and Map pages

### Hotspot page

`content/www/hotspot.html` presents hotspot static talkgroup state from `/api/systemstg`, including:

- DMR ID
- callsign
- node ID
- auth state
- current TS1 / TS2
- static TS1 / TS2 assignments
- last-seen age

### Map page

`content/www/map.html` displays merged APRS station and hotspot markers, plus a searchable table with:

- type (`station` or `hotspot`)
- display / callsign label
- DMR ID
- node ID for hotspots
- latitude / longitude
- last-seen age
- marker details

The page uses Leaflet from the public `unpkg` CDN, so browser access to that CDN is required unless you vendor those assets locally.

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

- `app.js` — shared auth, polling, rendering, theme persistence, map rendering hooks, and local storage helpers
- `styles.css` — common dashboard/theme/card/table styling
- the Login/Profile flow keeps browser state in `localStorage` and sends `X-Auth-Token` unless a header is already supplied
- dashboard-style pages poll the main monitor endpoints roughly every 3 seconds

## Running the server

From the packaged runtime directory:

```bash
./server
```

Useful flags:

```text
-d         enable verbose/debug packet logging
-s         query a running local instance via UDP /STAT
--create   create the `LOG` table when built with `USE_SQLITE3`
--help     exits immediately (no usage text is printed)
```

Useful local URLs:

```text
http://127.0.0.1:8080/
http://127.0.0.1:8080/monitor.html
http://127.0.0.1:8080/openbridge.html
http://127.0.0.1:8080/hotspot.html
http://127.0.0.1:8080/map.html
http://127.0.0.1:8080/dvmanager.html
http://127.0.0.1:8080/register.html
http://127.0.0.1:8080/profile.html
```

Raw UDP status request:

```bash
echo "/STAT" | nc -u 127.0.0.1 62031
```

On startup the server loads `talkgroup.dat` and `banned.dat`, normalizes the SQLite schema when logging is enabled, resets stale `ACTIVE` / `CONNECT` flags, opens the main UDP and uplink/OpenBridge sockets, and starts the RX worker pool when `WorkerThreads` is greater than `1`.

## Logging details

With SQLite enabled, the server writes a `LOG` table with columns `DATE`, `RADIO`, `TG`, `TIME`, `SLOT`, `NODE`, `ACTIVE`, `CONNECT`, `SRC`, and `SEQ`. On startup, current builds backfill missing `SRC` / `SEQ` columns when needed, seed `SEQ` from existing row IDs, reset stale `ACTIVE` / `CONNECT` flags, and create helper indexes for active and grouped monitor queries.

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

### What changes when I build with `USE_UPLINK`?

The server switches from the classic OpenBridge-only loader to the unified uplink engine. That adds `[Uplink1..8]` support, Homebrew upstream login/auth/ping handling, upstream static `TS1` / `TS2` pushes, and still keeps `[OpenBridge1..3]` available for OpenBridge-class links.

### How does Parrot work?

On start, the server records frames into an in-memory `memfile`. After end-of-stream it spawns a playback thread and echoes the captured frames back with timed pacing.

### Does DV Manager depend on the local server database?

No. It is primarily a browser-side utility that fetches public data and exports files locally for the user.

### Why are hotspot markers missing from the map?

Hotspot markers only appear after a hotspot sends valid location fields in its config payload. The current code accepts `LAT` / `LATITUDE` and `LON` / `LONG` / `LONGITUDE`.

## Contributing

- keep changes focused and reviewable
- follow the existing C++ style
- prefer minimal new runtime dependencies
- test on Linux and Windows where practical
- document behavior changes that affect network semantics or config

## License

Provided as-is for educational and amateur radio experimentation. Make sure your use complies with local regulations and spectrum rules.
