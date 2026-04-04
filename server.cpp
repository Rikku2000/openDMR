#include "server.h"

int g_sock = -1;
int g_debug = 0;
char g_host[MAX_PASSWORD_SIZE];
int g_udp_port = DEFAULT_PORT;
char g_password[MAX_PASSWORD_SIZE];
#ifdef USE_SQLITE3
char g_log[256];
#endif
char g_talkgroup[256];
char g_banned[256];
int u_banned[MAX_BANNED_SIZE];
int g_housekeeping_minutes = DEFAULT_HOUSEKEEPING_MINUTES;
int g_keep_nodes_alive = 1;
int g_node_timeout = 1800;
int g_relax_ip_change = 1;
dword volatile g_tick;
dword volatile g_sec;
int g_scanner_tg = 777;
int g_parrot_tg = 9990;
int g_aprs_tg = 900999;

dword radioid_old;
dword tg_old;
dword slotid_old;
dword nodeid_old;

#ifdef HAVE_HTTPMODE
int g_monitor_enabled = 1;
int g_monitor_port = 8080;
char g_monitor_root[256] = "www";

typedef struct {
    int  used;
    int  radio;
    int  tg;
    int  src;
    int  aprs;
    int  sms;
    unsigned last_sec;
} MonMark;

#define MONMARK_N 1024
static MonMark g_marks[MONMARK_N];
#endif

int obp_local_port = 62000;
char ob_host[MAX_PASSWORD_SIZE];
int obp_remote_port = 62000;

std::vector<ob_peer> g_obp_peers;

#ifdef HAVE_APRS
aprs_client g_aprs = {0};

static std::map<dword, std::string> g_aprs_idmap;

static const char* aprs_safe_callsign(dword id) {
    static char buf[40];
    const char* cs = aprs_lookup_callsign(id);
    if (cs && *cs) return cs;
    sprintf(buf, "DMR%u-9", (unsigned)id);
    return buf;
}

static bool aprs_connected() {
    return g_aprs.sock > 0;
}

static bool aprs_connect() {
    if (!g_aprs.enabled) return false;
    if (!g_aprs.server_host[0] || !g_aprs.callsign[0]) return false;

    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == -1) return false;

    sockaddr_in a; memset(&a,0,sizeof(a));
    a.sin_family = AF_INET;
    a.sin_port   = htons(g_aprs.server_port);

#ifdef WIN32
    unsigned long ip = inet_addr(g_aprs.server_host);
    if (ip == INADDR_NONE) {
        hostent* he = gethostbyname(g_aprs.server_host);
        if (!he || he->h_addrtype != AF_INET) { CLOSESOCKET(s); return false; }
        a.sin_addr.S_un.S_addr = *(u_long*)he->h_addr_list[0];
    } else a.sin_addr.S_un.S_addr = ip;
#else
    in_addr_t ip = inet_addr(g_aprs.server_host);
    if (ip == INADDR_NONE) {
        hostent* he = gethostbyname(g_aprs.server_host);
        if (!he || he->h_addrtype != AF_INET) { CLOSESOCKET(s); return false; }
        a.sin_addr.s_addr = *(in_addr_t*)he->h_addr_list[0];
    } else a.sin_addr.s_addr = ip;
#endif

    if (connect(s, (sockaddr*)&a, sizeof(a)) == -1) {
        CLOSESOCKET(s);
        return false;
    }

    g_aprs.sock = s;
    g_aprs.last_io_sec = g_sec;

    char line[512];
    sprintf(line, "user %s pass %s vers DMRServer 0.30 filter %s\r\n",
            g_aprs.callsign, g_aprs.passcode,
            g_aprs.filter[0] ? g_aprs.filter : "m/0");
    send(g_aprs.sock, line, (int)strlen(line), 0);
    return true;
}

static void aprs_disconnect() {
    if (g_aprs.sock > 0) {
        CLOSESOCKET(g_aprs.sock);
        g_aprs.sock = -1;
    }
}

bool aprs_init_from_config() {
    g_aprs.sock = -1;
    g_aprs.last_io_sec = g_sec;
    g_aprs.last_try_sec = 0;
    if (!g_aprs.enabled) return false;
    return aprs_connect();
}

static void aprs_send_line(const char* s) {
    if (!aprs_connected()) return;
    if (!s || !*s) return;
    int n = (int)strlen(s);
    send(g_aprs.sock, s, n, 0);
    g_aprs.last_io_sec = g_sec;
}

void aprs_housekeeping() {
    if (!g_aprs.enabled) return;

    if (aprs_connected() && g_aprs.keepalive_secs > 0 &&
        g_sec - g_aprs.last_io_sec >= (dword)g_aprs.keepalive_secs) {
        aprs_send_line("# keepalive\r\n");
    }

    if (!aprs_connected() && g_sec - g_aprs.last_try_sec >= (dword)g_aprs.reconnect_secs) {
        g_aprs.last_try_sec = g_sec;
        if (!aprs_connect()) {
        }
    }

    if (aprs_connected() && select_rx(g_aprs.sock, 0)) {
        char buf[512];
        recv(g_aprs.sock, buf, sizeof(buf), 0);
        g_aprs.last_io_sec = g_sec;
    }
}

bool aprs_load_idmap(const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) return false;
    g_aprs_idmap.clear();
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        char* p = line;
        while (*p==' '||*p=='\t') ++p;
        if (*p=='#' || *p=='\r' || *p=='\n' || !*p) continue;
        char* comma = strchr(p, ',');
        if (!comma) continue;
        *comma = 0;
        dword id = (dword)atoi(p);
        char* cs = comma + 1;
        char* e = cs + strlen(cs);
        while (e>cs && (e[-1]=='\r'||e[-1]=='\n'||e[-1]==' '||e[-1]=='\t')) *--e=0;
        if (id) g_aprs_idmap[id] = cs;
    }
    fclose(f);
    return true;
}

const char* aprs_lookup_callsign(dword dmrid) {
    std::map<dword,std::string>::iterator it = g_aprs_idmap.find(dmrid);
    if (it == g_aprs_idmap.end()) return NULL;
    return (*it).second.c_str();
}

void aprs_send_heard(dword dmrid, dword tg, dword nodeid)
{
    if (!g_aprs.enabled || !aprs_connected()) return;

    const char* heard = aprs_safe_callsign(dmrid);
    char line[512];

    sprintf(line,
        "%s>APDMR,TCPIP*:>Heard %s on TG %u via NODE %u (DMR APRS)\r\n",
        g_aprs.callsign, heard, (unsigned)tg, (unsigned)nodeid);

    aprs_send_line(line);
}
#endif

#ifdef HAVE_SMS
sms_settings g_sms = {0};
#endif

int  g_auth_enabled = 0;
char g_auth_file[260] = {0};
char g_dmrids_file[260] = {0};
int  g_auth_reload_secs = 0;
int  g_auth_unknown_default = 0;

static std::map<dword, std::string> g_auth_map;
static dword g_auth_last_load_sec = 0;

struct WebSession {
    dword dmrid;
    unsigned expires;
};

static std::map<std::string, WebSession> g_web_sessions;
static const unsigned WEB_SESSION_TTL = 24u * 60u * 60u;

#ifdef USE_SQLITE3
sqlite3 *db;
char *zErrMsg = 0;
char sql[1024];
int rc;

static std::map<std::string,int> g_obp_timers;

static dword obp_radioid_old = 0;
static dword obp_tg_old = 0;
static dword obp_slotid_old = 0;
static dword obp_nodeid_old = 0;
#endif

struct slot
{
	struct node		*node;
	dword			slotid;
	dword			tg;
	slot			*prev, *next;
	dword			parrotstart;
	int				parrotendcount;	
	memfile			*parrot;
	byte volatile	parrotseq;
#ifdef HAVE_SMS
	sms_buf			sms;
#endif

	slot() : node(NULL), slotid(0), tg(0), prev(NULL), next(NULL), parrotstart(0), parrotendcount(0), parrot(NULL), parrotseq(0) {
	}
};

struct node
{
	dword			nodeid;
	dword			dmrid;
	dword			salt;
	sockaddr_in		addr;
	dword			hitsec;
	slot			slots[2];
	bool			bAuth;
	dword			timer;

	node() : nodeid(0), dmrid(0), salt(0), hitsec(0), bAuth(false), timer(0) {
		memset(&addr, 0, sizeof(addr));
		slots[0].node = this;
		slots[1].node = this;
	}

	std::vector<dword> static_tgs_ts1;
	std::vector<dword> static_tgs_ts2;
};

struct nodevector {
	dword			radioslot;
	struct node *sub[100];
	nodevector() {
		memset (this, 0, sizeof(*this));
	}
};

nodevector * g_node_index [HIGH_DMRID-LOW_DMRID];

struct parrot_exec
{
	sockaddr_in		addr;
	memfile			*file;
	parrot_exec() {
		file = NULL;
	}
	~parrot_exec() {
		delete file;
		file = NULL;
	}
};

struct talkgroup
{	
	dword		tg;
	dword		ownerslot;
	dword		tick;
	slot		*subscribers;

	talkgroup() {
		
		tg = 0;	
		ownerslot = 0;
		tick = 0;
		subscribers = NULL;
	}
};

static std::map<dword, talkgroup*> g_talkgroups;
talkgroup *g_scanner;

std::string my_inet_ntoa (in_addr in)
{
	char buf[20];

	dword n = *(dword*)&in;

	sprintf (buf, "%03d.%03d.%03d.%03d", 
		(byte)(n),
		(byte)(n >> 8),
		(byte)(n >> 16),
		(byte)(n >> 24));

	return buf;
}

std::string slotid_str (dword slotid)
{
	char buf[20];

	sprintf (buf, "%u:%u", NODEID(slotid), SLOT(slotid)+1);

	return buf;
}

void dumphex (PCSTR pName, void const *p, int nSize)
{
	printf ("%s: ", pName);
	for (int i=0; i < nSize; i++)
		printf ("%02X", ((BYTE*)p)[i]);
	putchar ('\n');
}

bool select_rx (int sock, int wait_secs)
{
	fd_set read;

	FD_ZERO (&read);

	FD_SET (sock, &read);

	timeval t;

	t.tv_sec = wait_secs;
	t.tv_usec = 0;

	int ret = select (sock + 1, &read, NULL, NULL, &t);     

	if (ret == -1)
		return false;

	return !!ret;
}

int open_udp (int port)
{
	int err;

	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == -1) 
		return -1;

	int on = true;
	
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*) &on, sizeof(on));

	sockaddr_in addr;

	memset (&addr, 0, sizeof(addr));

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

	if (bind (sock, (sockaddr*) &addr, sizeof(addr)) == -1) {
		err = errno;
		CLOSESOCKET(sock);
		errno = err;
		return -1;
	}

	int bArg = true;

	if (setsockopt (sock, SOL_SOCKET, SO_BROADCAST, (char*) &bArg, sizeof(bArg)) == -1) {

		err = errno;
		CLOSESOCKET(sock);
		errno = err;
		return -1;
	}

	return sock;
}

#ifdef WIN32
int pthread_create (pthread_t *th, const pthread_attr_t *pAttr, PTHREADPROC pProc, void *pArg)
{
	assert(th);

#ifdef VS12
	ptw32_handle_t hThread;

	if (_beginthreadex (NULL, 0, pProc, pArg, 0, &hThread.x) == 0)
		return errno;
	
	*th = hThread;
#else
	unsigned hThread = 0;

	if (_beginthreadex (NULL, 0, pProc, pArg, 0, &hThread) == 0)
		return errno;
	
	*th = (pthread_t) hThread;
#endif

	return 0;
}
#endif

typedef struct {
	BYTE data[64];
	DWORD datalen;
	u64 bitlen;
	DWORD state[8];
} SHA256_CTX;

static const DWORD k[64] = {
	
	0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
	0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
	0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
	0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
	0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
	0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
	0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
	0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

static void sha256_transform(SHA256_CTX *ctx, const BYTE data[])
{
	DWORD a, b, c, d, e, f, g, h, i, j, t1, t2, m[64];

	for (i = 0, j = 0; i < 16; ++i, j += 4)
		m[i] = (data[j] << 24) | (data[j + 1] << 16) | (data[j + 2] << 8) | (data[j + 3]);
	for ( ; i < 64; ++i)
		m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];

	a = ctx->state[0];
	b = ctx->state[1];
	c = ctx->state[2];
	d = ctx->state[3];
	e = ctx->state[4];
	f = ctx->state[5];
	g = ctx->state[6];
	h = ctx->state[7];

	for (i = 0; i < 64; ++i) {
		t1 = h + EP1(e) + CH(e,f,g) + k[i] + m[i];
		t2 = EP0(a) + MAJ(a,b,c);
		h = g;
		g = f;
		f = e;
		e = d + t1;
		d = c;
		c = b;
		b = a;
		a = t1 + t2;
	}

	ctx->state[0] += a;
	ctx->state[1] += b;
	ctx->state[2] += c;
	ctx->state[3] += d;
	ctx->state[4] += e;
	ctx->state[5] += f;
	ctx->state[6] += g;
	ctx->state[7] += h;
}

static void sha256_init(SHA256_CTX *ctx)
{
	ctx->datalen = 0;
	ctx->bitlen = 0;
	ctx->state[0] = 0x6a09e667;
	ctx->state[1] = 0xbb67ae85;
	ctx->state[2] = 0x3c6ef372;
	ctx->state[3] = 0xa54ff53a;
	ctx->state[4] = 0x510e527f;
	ctx->state[5] = 0x9b05688c;
	ctx->state[6] = 0x1f83d9ab;
	ctx->state[7] = 0x5be0cd19;
}

static void sha256_update(SHA256_CTX *ctx, const BYTE data[], size_t len)
{
	int i;

	for (i = 0; i < len; ++i) {
		ctx->data[ctx->datalen] = data[i];
		ctx->datalen++;
		if (ctx->datalen == 64) {
			sha256_transform(ctx, ctx->data);
			ctx->bitlen += 512;
			ctx->datalen = 0;
		}
	}
}

static void sha256_final(SHA256_CTX *ctx, BYTE hash[])
{
	int i;

	i = ctx->datalen;

	if (ctx->datalen < 56) {
		ctx->data[i++] = 0x80;
		while (i < 56)
			ctx->data[i++] = 0x00;
	}
	else {
		ctx->data[i++] = 0x80;
		while (i < 64)
			ctx->data[i++] = 0x00;
		sha256_transform(ctx, ctx->data);
		memset(ctx->data, 0, 56);
	}

	ctx->bitlen += ctx->datalen * 8;
	ctx->data[63] = ctx->bitlen;
	ctx->data[62] = ctx->bitlen >> 8;
	ctx->data[61] = ctx->bitlen >> 16;
	ctx->data[60] = ctx->bitlen >> 24;
	ctx->data[59] = ctx->bitlen >> 32;
	ctx->data[58] = ctx->bitlen >> 40;
	ctx->data[57] = ctx->bitlen >> 48;
	ctx->data[56] = ctx->bitlen >> 56;
	sha256_transform(ctx, ctx->data);

	for (i = 0; i < 4; ++i) {
		hash[i]      = (ctx->state[0] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 4]  = (ctx->state[1] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 8]  = (ctx->state[2] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 12] = (ctx->state[3] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 16] = (ctx->state[4] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 20] = (ctx->state[5] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 24] = (ctx->state[6] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 28] = (ctx->state[7] >> (24 - i * 8)) & 0x000000ff;
	}
}

byte * make_sha256_hash (void const *pSrc, int nSize, byte *dest, void const *pSalt, int nSaltSize)
{
	SHA256_CTX ctx;

	sha256_init (&ctx);

	sha256_update(&ctx, (byte*)pSrc, nSize);
	
	if (pSalt)
		sha256_update(&ctx, (byte*)pSalt, nSaltSize);

	sha256_final(&ctx, dest);

	return dest;	
}

bool IsOptionPresent (int argc, char **argv, PCSTR arg)
{
	for (int i=1; i < argc; i++) {

		if (strcmp(argv[i],arg)==0)
			return true;
	}

	return false;
}

void trim (std::string &s) {

	int x = s.size() - 1;

	while (x >= 0 && isspace(s[x]))
		s.erase(x--);
}

PCSTR skipspaces (PCSTR p, bool bSkipTabs, bool bSkipCtrl)
{
	while (*p) {

		if (*p == ' ') {

			p ++;
		}

		else if (bSkipCtrl && *p > 0 && *p < ' ') {

			p ++;
		}

		else if (bSkipTabs && *p == '\t') {

			p ++;
		}

		else
			break;
	}

	return p;
}

static void _init_process() 
{
	assert(sizeof(WORD) == 2);
	assert(sizeof(word) == 2);
#ifdef WIN32
	assert(sizeof(DWORD) == 4);
	assert(sizeof(dword) == 4);
#endif
	assert(sizeof(u64) == 8);
	setbuf(stdout,NULL);
}

void init_process()
{
	_init_process();
#ifdef WIN32
	WSADATA wsa;
	WSAStartup (MAKEWORD(1, 1), &wsa);
	_umask(0);
#else
	signal(SIGPIPE,SIG_IGN);
	signal(SIGCHLD,SIG_DFL);
	struct rlimit r;
	memset (&r, 0, sizeof(r));
	setrlimit(RLIMIT_CORE, &r);
	umask(0);
#endif
}

void unsubscribe_from_group(slot *s);

void logmsg(enum LogColorLevel level, int timed, const char *fmt, ...) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timestr[64];
    strftime(timestr, sizeof(timestr), "[%d.%m.%Y / %H:%M:%S]: ", t);

    va_list args;
    va_start(args, fmt);

#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    WORD saved_attributes;

    GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
    saved_attributes = consoleInfo.wAttributes;

    switch (level) {
        case LOG_RED: SetConsoleTextAttribute(hConsole, FOREGROUND_INTENSITY | FOREGROUND_RED); break;
        case LOG_GREEN: SetConsoleTextAttribute(hConsole, FOREGROUND_INTENSITY | FOREGROUND_GREEN); break;
        case LOG_YELLOW: SetConsoleTextAttribute(hConsole, FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN); break;
        case LOG_BLUE: SetConsoleTextAttribute(hConsole, FOREGROUND_INTENSITY | FOREGROUND_BLUE); break;
        case LOG_PURPLE: SetConsoleTextAttribute(hConsole, FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE); break;
        case LOG_CYAN: SetConsoleTextAttribute(hConsole, FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_BLUE); break;
        case LOG_WHITE: SetConsoleTextAttribute(hConsole, FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); break;
    }

	if (timed == 1)
		printf("%s", timestr);
    vprintf(fmt, args);

    SetConsoleTextAttribute(hConsole, saved_attributes);
#else
    const char *color = "\033[0m";
    switch (level) {
        case LOG_RED: color = "\033[31m"; break;
        case LOG_GREEN: color = "\033[32m"; break;
        case LOG_YELLOW: color = "\033[33m"; break;
        case LOG_BLUE: color = "\033[34m"; break;
        case LOG_PURPLE: color = "\033[35m"; break;
        case LOG_CYAN: color = "\033[36m"; break;
        case LOG_WHITE: color = "\033[37m"; break;
    }

	if (timed == 1)
		printf("%s%s", color, timestr);
	else
		printf("%s", color);
    vprintf(fmt, args);
    printf("\033[0m");
#endif

    va_end(args);
}

void log (sockaddr_in *addr, PCSTR fmt, ...) {
	int err = errno;
	int nerr = GetInetError();

	try {
		char temp[300];
		va_list marker;
		va_start (marker, fmt);
		time_t tt = time(NULL);
		tm *t = localtime(&tt);

		vsprintf (temp, fmt, marker);
		char *p = temp + strlen(temp) - 1;
		while (p >= temp && (*p == '\r' || *p == '\n'))
			*p-- = 0;

		puts (temp);
	} catch (...) {
	}

	errno = err;
	SetInetError (nerr);
}

word inline get2 (byte const *p)
{
#ifdef BIG_ENDIAN_CPU
	return *(word*) p;
#else
	return ((word)p[0] << 8) + p[1];
#endif
}

dword inline get3 (byte const *p)
{
	return (dword)p[0] << 16 | ((word)p[1] << 8) | p[2];
}

dword inline get4 (byte const *p)
{
#ifdef BIG_ENDIAN_CPU
	return *(dword*) p;
#else
	return (dword)p[0] << 24 | (dword)p[1] << 16 | (word)p[2] << 8 | p[3];
#endif
}

void inline set3 (byte *p, dword n)
{
	*p++ = n >> 16;
	*p++ = n >> 8;
	*p++ = n;
}

void inline set4 (byte *p, dword n)
{
	*p++ = n >> 24;
	*p++ = n >> 16;
	*p++ = n >> 8;
	*p++ = n;
}

struct talkgroup;
talkgroup * findgroup (dword tg, bool bCreateIfNecessary);

static int csv_first_int(const std::string& csv) {
    char buf[512]; if (csv.empty()) return 0;
    strncpy(buf, csv.c_str(), sizeof(buf)-1); buf[sizeof(buf)-1]=0;
    char* p = buf;
    while (*p) {
        while (*p==' '||*p==',') ++p;
        if (!*p) break;
        char* q=p; while (*q && *q!=',') ++q;
        if (*q) *q++=0;
        int v = atoi(p);
        if (v>0) return v;
        p=q;
    }
    return 0;
}

static std::vector<dword> csv_all_ints(const std::string& csv) {
    std::vector<dword> ret;
    char buf[1024];

    if (csv.empty())
        return ret;

    strncpy(buf, csv.c_str(), sizeof(buf)-1);
    buf[sizeof(buf)-1] = 0;

    char* p = buf;
    while (*p) {
        while (*p==' ' || *p=='	' || *p==',') ++p;
        if (!*p) break;

        char* q = p;
        while (*q && *q != ',') ++q;
        if (*q) *q++ = 0;

        int v = atoi(p);
        if (v > 0) {
            bool seen = false;
            for (size_t i = 0; i < ret.size(); ++i) {
                if (ret[i] == (dword)v) {
                    seen = true;
                    break;
                }
            }
            if (!seen)
                ret.push_back((dword)v);
        }

        p = q;
    }

    return ret;
}

static std::string kv_value(const std::string& s, const char* key) {
    size_t k = s.find(key);
    if (k == std::string::npos) return "";
    k += strlen(key);
    size_t end = s.find(';', k);
    return s.substr(k, end==std::string::npos ? end : end - k);
}

static std::string csv_join_dwords(const std::vector<dword>& vals) {
    std::string out;
    char buf[32];

    for (size_t i = 0; i < vals.size(); ++i) {
        if (!out.empty())
            out += ",";
        sprintf(buf, "%u", (unsigned)vals[i]);
        out += buf;
    }

    return out;
}

static std::map<dword, std::vector<slot*> > g_static_subscribers;

static std::vector<dword>& slot_static_tgs(slot* s) {
    return SLOT(s->slotid) ? s->node->static_tgs_ts2 : s->node->static_tgs_ts1;
}

static void static_remove_slot_from_tg(slot* s, dword tg) {
    std::map<dword, std::vector<slot*> >::iterator it = g_static_subscribers.find(tg);
    if (it == g_static_subscribers.end())
        return;

    std::vector<slot*>& subs = (*it).second;
    for (std::vector<slot*>::iterator sit = subs.begin(); sit != subs.end(); ) {
        if (*sit == s)
            sit = subs.erase(sit);
        else
            ++sit;
    }

    if (subs.empty())
        g_static_subscribers.erase(it);
}

static void static_unregister_slot(slot* s) {
    std::vector<dword>& tgs = slot_static_tgs(s);
    for (size_t i = 0; i < tgs.size(); ++i)
        static_remove_slot_from_tg(s, tgs[i]);
    tgs.clear();
}

static void static_register_slot(slot* s, const std::vector<dword>& tgs) {
    static_unregister_slot(s);

    std::vector<dword>& store = slot_static_tgs(s);
    for (size_t i = 0; i < tgs.size(); ++i) {
        dword tg = tgs[i];
        store.push_back(tg);
        findgroup(tg, true);

        std::vector<slot*>& subs = g_static_subscribers[tg];
        bool seen = false;
        for (size_t j = 0; j < subs.size(); ++j) {
            if (subs[j] == s) {
                seen = true;
                break;
            }
        }
        if (!seen)
            subs.push_back(s);
    }
}

void show_packet (PCSTR title, char const *ip, byte const *pk, int sz, bool bShowDMRD=false) 
{
	int i;

	if (g_debug) {
		logmsg (LOG_YELLOW, 0, "%s %s size %d\n", title, ip, sz);
		for (i=0; i < sz; i++) 
			logmsg (LOG_YELLOW, 0, "%02X ", pk[i]);

		putchar ('\n');
		for (i=0; i < sz; i++) 
			logmsg (LOG_YELLOW, 0, "%c", inrange(pk[i],32,127) ? pk[i] : '.');
		putchar ('\n');

		if (bShowDMRD && sz == 55 && memcmp(pk, "DMRD", 4)==0) {
			dword radioid = get3(pk + 5);
			dword tg = get3 (pk + 8);
			dword nodeid = get4(pk + 11);
			dword streamid = get4(pk + 16);
			int flags = pk[15];
			dword slotid = SLOTID(nodeid, flags & 0x80);
			logmsg (LOG_CYAN, 0, "node %d slot %d radio %d group %d stream %08X flags %02X\n\n", nodeid, SLOT(slotid)+1, radioid, tg, streamid, flags);
		}

		putchar ('\n');
	}
}

void sendpacket (sockaddr_in addr, void const *p, int sz)
{
	show_packet ("TX", my_inet_ntoa(addr.sin_addr).c_str(), (byte const*)p, sz, true);

	sendto (g_sock, (char*)p, sz, 0, (sockaddr*)&addr, sizeof(addr));
}

static void sendpacket_sock (int sock, sockaddr_in addr, void const *p, int sz)
{
	show_packet ("TX", my_inet_ntoa(addr.sin_addr).c_str(), (byte const*)p, sz, true);

	sendto (sock, (char*)p, sz, 0, (sockaddr*)&addr, sizeof(addr));
}

node * findnode (dword nodeid, bool bCreateIfNecessary)
{
	node *n = NULL;

	nodeid = NODEID(nodeid);

	dword dmrid, essid;

	if (nodeid > 0xFFFFFF) {

		dmrid = nodeid / 100;
		essid = nodeid % 100;
	}

	else {

		dmrid = nodeid;
		essid = 0;
	}

	if (!inrange(dmrid,LOW_DMRID,HIGH_DMRID)) 
		return NULL;

	if (essid >= 100)
		return NULL;

	int ix = dmrid-LOW_DMRID;

	if (!g_node_index[ix]) {
		if (!bCreateIfNecessary)
			return NULL;

		g_node_index[ix] = new nodevector;
	}

	if (!g_node_index[ix]->sub[essid]) {
		if (!bCreateIfNecessary)
			return NULL;

		n = g_node_index[ix]->sub[essid] = new node;

		n->nodeid = nodeid;
		n->dmrid = dmrid;

		n->slots[0].slotid = SLOTID(nodeid,0);
		n->slots[1].slotid = SLOTID(nodeid,1);
	}

	else {

		n = g_node_index[ix]->sub[essid];
	}

	return n;
}

void delete_node (dword nodeid)
{
	nodeid = NODEID(nodeid);

	node *n = NULL;

	int dmrid, essid;

	if (nodeid > 0xFFFFFF) {
		dmrid = nodeid / 100;
		essid = nodeid % 100;
	} else {
		dmrid = nodeid;
		essid = 0;
	}

#ifdef USE_SQLITE3
	sprintf(sql, "UPDATE LOG set ACTIVE=0, CONNECT=0 where NODE=%d; SELECT * from LOG", nodeid);
	rc = sqlite3_exec(db, sql, 0, 0, &zErrMsg);
	if (rc != SQLITE_OK)
		sqlite3_free(zErrMsg);
#endif

	if (inrange(dmrid,LOW_DMRID,HIGH_DMRID)) {
		int ix = dmrid-LOW_DMRID;

		if (g_node_index[ix]) {
			node *n = g_node_index[ix]->sub[essid];

			if (n) {
				log (&n->addr, "Delete node %d\n", nodeid);

				static_unregister_slot (&n->slots[0]);
				static_unregister_slot (&n->slots[1]);

				unsubscribe_from_group (&n->slots[0]);

				unsubscribe_from_group (&n->slots[1]);

				g_node_index[ix]->sub[essid] = NULL;

				bool bNodes = false;

				for (int i=0; i < 100; i++) {

					if (g_node_index[ix]->sub[i]) {

						bNodes = true;
						break;
					}
				}

				if (!bNodes) {

					delete g_node_index[ix];
					
					g_node_index[ix] = NULL;
				}

				delete n;
			}
		}
	}
}

slot * findslot (int slotid, bool bCreateIfNecessary)
{
	node *n = findnode (NODEID(slotid), bCreateIfNecessary);

	if (!n)
		return NULL;

	return &n->slots[SLOT(slotid)];
}

talkgroup * findgroup (dword tg, bool bCreateIfNecessary)
{
	if (!inrange(tg,1,MAX_TALK_GROUPS-1))
		return NULL;

	std::map<dword, talkgroup*>::iterator it = g_talkgroups.find(tg);
	if (it == g_talkgroups.end()) {
		if (!bCreateIfNecessary)
			return NULL;

		talkgroup *g = new talkgroup;
		g->tg = tg;
		g_talkgroups[tg] = g;
		return g;
	}

	return (*it).second;
}

void _dump_groups(std::string &ret)
{
	char temp[200];

	for (std::map<dword, talkgroup*>::const_iterator it = g_talkgroups.begin(); it != g_talkgroups.end(); ++it) {
		talkgroup const *g = (*it).second;
		if (!g)
			continue;

		sprintf (temp, "TALKGROUP %u owner %u slot %u head %p %u\n",
			(unsigned)g->tg,
			(unsigned)NODEID(g->ownerslot),
			(unsigned)(SLOT(g->ownerslot)+1),
			g->subscribers,
			g->subscribers ? (unsigned)g->subscribers->node->nodeid : 0u);

		ret += temp;
		slot *s = g->subscribers;

		while (s) {
			sprintf (temp, "\t%p node %u slot %u prev %p next %p\n",
				s,
				(unsigned)s->node->nodeid,
				(unsigned)(SLOT(s->slotid)+1),
				s->prev,
				s->next);

			ret += temp;
			s = s->next;
		}
	}
}

void dump_groups()
{
	if (g_debug) {
		std::string str;
		puts (str.c_str());
	}
}

void _dump_nodes(std::string &ret)
{
	char temp[200];

	sprintf (temp, "Sec %d tick %u\n", g_sec, g_tick);
	ret += temp;

	for (int ix=0; ix < HIGH_DMRID - LOW_DMRID; ix++) {
		if (g_node_index[ix]) {
			sprintf (temp, "Node vector %d, radioslot %d\n", ix + LOW_DMRID, g_node_index[ix]->radioslot);
			ret += temp;

			for (int essid=0; essid < 100; essid++) {
				node const *n = g_node_index[ix]->sub[essid];

				if (n) {
					sprintf (temp, "\t%s ID %d dmrid %d auth %d sec %u\n", my_inet_ntoa(n->addr.sin_addr).c_str(), n->nodeid, n->dmrid, n->bAuth, n->hitsec);
					ret += temp;

					if (n->slots[0].tg) {
						sprintf (temp, "\t\tS1 TG %d\n", n->slots[0].tg);
						ret += temp;
					}

					if (n->slots[1].tg) {
						sprintf (temp, "\t\tS2 TG %d\n", n->slots[1].tg);
						ret += temp;
					}
				}
			}
		}
	}
}

void dump_nodes()
{
	if (g_debug) {
		std::string str;
		puts (str.c_str());
	}
}

void unsubscribe_from_group(slot *s)
{
	if (s->tg) {

		log (&s->node->addr, "Unsubscribe group %u node %d slot %d from talkgroup %d\n", s->tg, s->node->nodeid, SLOT(s->slotid)+1);

		talkgroup *g = findgroup (s->tg, false);

		if (g) {

			if (g->ownerslot == s->slotid)
				g->ownerslot = 0;

			if (s->prev)
				s->prev->next = s->next;

			if (s->next)
				s->next->prev = s->prev;

			if (g->subscribers == s)
				g->subscribers = s->next;
		}

		s->next = s->prev = NULL;

		s->tg = 0;

		dump_groups ();
	}
}

void subscribe_to_group(slot *s, talkgroup *g)
{
	if (s->tg != g->tg) {

		log (&s->node->addr, "Subscribe group %u node %d slot %d to talkgroup %d\n", g->tg, s->node->nodeid, SLOT(s->slotid)+1);

		unsubscribe_from_group(s);

		s->tg = g->tg;
		s->prev = NULL;
		s->next = g->subscribers;

		if (s->next)
			s->next->prev = s;

		g->subscribers = s;

		dump_groups ();
	}
}

int check_banned(byte *pk)
{
	dword const rid = get3(pk + 5);
	dword const nid = get4(pk + 11);
	int i;

	for (i = 0; i < MAX_BANNED_SIZE; i++) {
		if (u_banned[i] == rid) {
			logmsg (LOG_RED, 0, "Banned radioid %d found\n", rid);
			delete_node (nid);
			return 1;
		}
	}

	return 0;
}

void swapbytes (byte *a, byte *b, int sz)
{
	while (sz--) 
		swap (*a++, *b++);
}

PTHREAD_PROC(time_thread_proc) 
{
	for (;;) {

		Sleep (50);

		g_tick += 50;

		if (!(g_tick % 1000))
			g_sec ++;
	}

	return 0;
}

PTHREAD_PROC(parrot_playback_thread_proc)
{
	parrot_exec *e = (parrot_exec*) threadcookie;

	e->file->Seek(0);

	byte buf[55];

	Sleep (1000);

	while (e->file->Read (buf, 55)) {

		sendpacket (e->addr, buf, 55);

		Sleep (20);
	}

	delete e;

	return 0;
}

void handle_rx (sockaddr_in &addr, byte *pk, int pksize)
{
    char date_now[100];
    time_t now = time (0);

	if (pksize == 55 && memcmp(pk, "DMRD", 4)==0) {
		if (check_banned (pk) == 1)
			return;
		dword const radioid = get3(pk + 5);
		dword const tg = get3 (pk + 8);
		dword const nodeid = get4(pk + 11);
		dword const streamid = get4(pk + 16);

		int const flags = pk[15];

		bool const bStartStream = (flags & 0x23) == 0x21;
		bool const bEndStream = (flags & 0x23) == 0x22;
		bool const bPrivateCall = (flags & 0x40) == 0x40;
		dword const slotid = SLOTID(nodeid, flags & 0x80);

#ifdef HAVE_APRS
		if (tg == g_aprs_tg && bStartStream)
			aprs_send_heard(radioid, tg, nodeid);
#endif

#ifdef HAVE_HTTPMODE
		int is_aprs_flag = 0;
		if (tg == g_aprs_tg)
			is_aprs_flag = 1;
#endif

		if (g_debug)
			logmsg (LOG_CYAN, 0, "node %d slot %d radio %d group %d stream %08X flags %02X\n\n", nodeid, SLOT(slotid)+1, radioid, tg, streamid, flags);

		slot *s = findslot (slotid, true);
		if (!s) {
			log (&addr, "Slotid %s not found\n", slotid_str(slotid).c_str());
			return;
		}

#ifdef HAVE_SMS
		if (g_sms.enabled) {
			bool candidate = (bPrivateCall && g_sms.allow_private) || (!bPrivateCall && sms_tg_permitted(tg));

			if (candidate) {
				const byte *block51 = pk + 4;
				if (bStartStream) {
					sms_reset(s->sms);
					s->sms.streamid  = streamid;
					s->sms.start_sec = g_sec;
				}
				if (s->sms.streamid == streamid &&
					s->sms.frames < g_sms.max_frames &&
					(int)(g_sec - s->sms.start_sec) <= g_sms.max_seconds) {
					sms_append(s->sms, block51);
				}
				if (bEndStream && s->sms.streamid == streamid) {
					sms_emit_udp(radioid, tg, bPrivateCall ? true : false, s->sms);
#ifdef HAVE_HTTPMODE
					monitor_note_event((int)radioid, (int)tg, MON_SRC_LOCAL, is_aprs_flag, 1);
#endif
				}
			} else {
				if (bEndStream && s->sms.streamid == streamid) sms_reset(s->sms);
			}
		}
#endif

		if (!s->node->bAuth) {
			log (&addr, "Node %d not authenticated\n", nodeid);
			return;
		}

		if (getinaddr(s->node->addr) != getinaddr(addr)) {
			log (&addr, "Node %d invalid IP. Should be %s\n", nodeid, my_inet_ntoa(addr.sin_addr).c_str());
			return;
		}

#ifdef USE_SQLITE3
		strftime (date_now, 100, "%d.%m.%Y / %H:%M:%S", localtime (&now));
#ifdef VS12
		if ((radioid != radioid_old) || (tg != tg_old) || (slotid != slotid_old) || (nodeid != nodeid_old)) {
#else
		if ((radioid != radioid_old) or (tg != tg_old) or (slotid != slotid_old) or (nodeid != nodeid_old)) {
#endif
			sprintf(sql, "INSERT INTO LOG (DATE,RADIO,TG,TIME,SLOT,NODE,ACTIVE,CONNECT) VALUES ('%s',%d,%d,0,%d,%d,1,1);", date_now, radioid, tg, SLOT(slotid)+1, nodeid);
			rc = sqlite3_exec(db, sql, 0, 0, &zErrMsg);
			if( rc != SQLITE_OK )
				sqlite3_free(zErrMsg);
		} else {
			sprintf(sql, "UPDATE LOG set DATE='%s', ACTIVE=1, TIME=%d where RADIO=%d and TG=%d; SELECT * from LOG", date_now, s->node->timer / 15, radioid, tg);
			rc = sqlite3_exec(db, sql, 0, 0, &zErrMsg);
			if( rc != SQLITE_OK )
				sqlite3_free(zErrMsg);
			s->node->timer++;
		}

#ifdef HAVE_HTTPMODE
		monitor_note_event((int)radioid, (int)tg, MON_SRC_LOCAL, is_aprs_flag, 0);
#endif

		radioid_old = radioid;
		tg_old = tg;
		slotid_old = slotid;
		nodeid_old = nodeid;
#endif

		s->node->addr = addr;
		s->node->hitsec = g_sec;

		if (inrange(radioid,LOW_DMRID,HIGH_DMRID) && g_node_index[radioid-LOW_DMRID])
			g_node_index[radioid-LOW_DMRID]->radioslot = slotid;

		if (tg == UNSUBSCRIBE_ALL_TG) {
			if (bStartStream) {
				log (&addr, "Unsubscribe all, slotid %s\n", slotid_str(slotid).c_str());
				unsubscribe_from_group (s);
			}

			return;
		}

		if (bPrivateCall) {
			if (tg == g_parrot_tg) {

				if (bEndStream) {

					log (&addr, "Parrot stream end on nodeid %u slotid %s radioid %u\n", nodeid, slotid_str(slotid).c_str(), g_parrot_tg);

					if (s->parrot) {

						s->parrot->Write (pk, pksize);

						parrot_exec *e = new parrot_exec;

						e->addr = s->node->addr;
						e->file = s->parrot;
						s->parrot = NULL;

						pthread_t th;

						pthread_create (&th, NULL, parrot_playback_thread_proc, e);
					}
				}

				else {

					if (bStartStream) {
						log (&addr, "Parrot stream start on nodeid %u slotid %s radioid %u\n", nodeid, slotid_str(slotid).c_str(), g_parrot_tg);

						unsubscribe_from_group (s);

						if (!s->parrot) {	
							s->parrot = new memfile;
							s->parrotseq ++;
							s->parrotstart = g_sec;
						}
					}

					if (s->parrot && g_sec - s->parrotstart < 6) {

						s->parrot->Write (pk, pksize);
					}
				}
			}
			else {
				unsubscribe_from_group (s);

				if (bStartStream) {
					log (&addr, "Private stream start, from radioid %u to radioid %u\n", radioid, tg);
				}
				else if (bEndStream) {
					log (&addr, "Private stream end, from radioid %u to radioid %u\n", radioid, tg);
				}

				if (inrange(tg,LOW_DMRID,HIGH_DMRID)) {
				
					if (g_node_index[tg-LOW_DMRID]) {

						dword slotid = g_node_index[tg-LOW_DMRID]->radioslot;

						slot const *dest = findslot (slotid, false);

						if (dest) {
							if (bStartStream || bEndStream) {
								log (&addr, "Private stream dest slotid %s found, from radioid %u to radioid %u\n", slotid_str(slotid).c_str(), radioid, tg);
							}

							if (SLOT(slotid))
								pk[15] |= 0x80;
							else
								pk[15] &= 0x7F;

							sendpacket (dest->node->addr, pk, pksize);
						} else {
							if (bStartStream || bEndStream) {
								log (&addr, "Private stream dest slotid %s not found, from radioid %u to radioid %u\n", slotid_str(slotid).c_str(), radioid, tg);
							}
						}
					}
					else {
						if (bStartStream || bEndStream) {
							log (&addr, "Private stream dest radioid not in node index, from radioid %u to radioid %u\n", radioid, tg);
						}
					}
				}
				else {

					if (bStartStream || bEndStream) {

						log (&addr, "Private stream dest radioid out of range, from radioid %u to radioid %u\n", radioid, tg);
					}
				}
			}
		}

		else {
			talkgroup *g = findgroup (tg, false);

			if (g) {
				if (s->tg != tg)
					subscribe_to_group(s, g);

				if (tg != g_scanner_tg) {
					if (g->ownerslot && g_tick - g->tick >= 1500) {
						log (&addr, "Timeout group %u, slotid %s", tg, slotid_str(g->ownerslot).c_str());
						g->ownerslot = 0;
					}

					if (bStartStream && !g->ownerslot) {
						log (&addr, "Take group %u, nodeid %u slotid %s radioid %u", tg, nodeid, slotid_str(slotid).c_str(), radioid);
						g->ownerslot = slotid;
						g->tick = g_tick;
					}

					bool forward_group_frame = (g->ownerslot == slotid);
					
					if (forward_group_frame) {
						g->tick = g_tick;

						slot const *dest = g->subscribers;

						while (dest) {
							if (dest->slotid != slotid) {
								if (SLOT(dest->slotid))
									pk[15] |= 0x80;
								else
									pk[15] &= 0x7F;

								sendpacket (dest->node->addr, pk, pksize);
							}

							dest = dest->next;
						}

						std::map<dword, std::vector<slot*> >::iterator sit = g_static_subscribers.find(tg);
						if (sit != g_static_subscribers.end()) {
							std::vector<slot*>& static_slots = (*sit).second;
							for (size_t i = 0; i < static_slots.size(); ++i) {
								slot* sdest = static_slots[i];
								if (!sdest)
									continue;
								if (sdest->slotid == slotid)
									continue;
								if (sdest->tg == tg)
									continue;
								if (!sdest->node || !sdest->node->bAuth || !getinaddr(sdest->node->addr))
									continue;

								if (SLOT(sdest->slotid))
									pk[15] |= 0x80;
								else
									pk[15] &= 0x7F;

								sendpacket (sdest->node->addr, pk, pksize);
							}
						}

                        obp_forward_dmrd(pk, pksize, 0);
					}

					if (bEndStream && forward_group_frame) {
						log (&addr, "Drop group %u, nodeid %u slotid %s radioid %u", tg, nodeid, slotid_str(slotid).c_str(), radioid);
						g->ownerslot = 0;
					}

					if (g_scanner->ownerslot && g_tick - g_scanner->tick >= 1500) {
						log (&addr, "Timeout scanner, nodeid %u slotid %s radioid %u", nodeid, slotid_str(slotid).c_str(), radioid);
						g_scanner->ownerslot = 0;
					}

					if (bEndStream) {
#ifdef USE_SQLITE3
						sprintf(sql, "UPDATE LOG set ACTIVE=0, TIME=%d where RADIO=%d and TG=%d; SELECT * from LOG", s->node->timer / 15, radioid, tg);
						rc = sqlite3_exec(db, sql, 0, 0, &zErrMsg);
						if(rc != SQLITE_OK)
							sqlite3_free(zErrMsg);
#endif
						s->node->timer = 0;
					}

					if (!g_scanner->ownerslot && !bEndStream) {
						log (&addr, "Take scanner, nodeid %u slotid %s radioid %u", nodeid, slotid_str(slotid).c_str(), radioid);
						g_scanner->ownerslot = s->slotid;
						g_scanner->tick = g_tick;
					}

					bool forward_scanner_frame = (s->slotid == g_scanner->ownerslot);

					if (forward_scanner_frame) {
						g_scanner->tick = g_tick;

						slot const *dest = g_scanner->subscribers;

						while (dest) {
							if (SLOT(dest->slotid))
								pk[15] |= 0x80;
							else
								pk[15] &= 0x7F;

							sendpacket (dest->node->addr, pk, pksize);
	
							dest = dest->next;
						}
					}

					if (bEndStream && forward_scanner_frame) {
						log (&addr, "Drop scanner, nodeid %u slotid %s radioid %u", nodeid, slotid_str(slotid).c_str(), radioid);
						g_scanner->ownerslot = 0;
					}
				}
			}
			else {
				if (bStartStream)
					log (&addr, "Nodeid %u keyup on non-existent group %u", nodeid, tg);

				unsubscribe_from_group (s);
			}
		}
	}
	else if (pksize == 8 && memcmp(pk, "RPTL", 4)==0) {
		dword nodeid = get4(pk + 4);
		log (&addr, "RPTL node %d\n", nodeid);
		node *n = findnode (nodeid, false);
		if (n) {
			if (n->bAuth && getinaddr(addr) != getinaddr(n->addr)) {
				log (&addr, "Node %d already logged in at %s\n", nodeid, my_inet_ntoa(n->addr.sin_addr).c_str());
				return;
			}
		}

		if (!n)
			n = findnode (nodeid, true);

		n->hitsec = g_sec;
		if (!getinaddr(n->addr))
			n->addr = addr;

		n->salt = ((dword)rand() << 16) ^ g_tick;
		memcpy(pk, "RPTACK", 6);
		set4(pk + 6, n->salt);
		sendpacket(addr, pk, 10);
	}
	else if (pksize == 40 && memcmp(pk, "RPTK", 4) == 0) {
		dword nodeid = get4(pk + 4);
		node* n = findnode(nodeid, false);
		if (!n) return;
		if (getinaddr(n->addr) != getinaddr(addr)) return;

		n->hitsec = g_sec;

		const byte* remotehash = pk + 8;

		const char* pass = NULL;
		bool permitted = true;

		if (g_auth_enabled) {
			pass = auth_lookup_pass(nodeid);
			if (!pass) {
				if (g_auth_unknown_default && g_password)
					pass = g_password;
				else
					permitted = false;
			}
		} else
			pass = g_password;

		bool ok = false;
		if (permitted && pass && pass[0]) {
			byte localhash[32];
			byte temp[MAX_PASSWORD_SIZE + 4];
			set4(temp, n->salt);
			strcpy((char*)temp + 4, pass);
			make_sha256_hash(temp, 4 + (int)strlen(pass), localhash, NULL, 0);
			ok = (memcmp(localhash, remotehash, 32) == 0);
		} else {
			ok = false;
		}

		n->bAuth = ok;

		if (!ok && g_debug)
			log(&addr, "RPTK auth FAIL for node %u (per-user auth %s, policy=%s)", nodeid, g_auth_enabled ? "ON" : "OFF", g_auth_unknown_default ? "default" : "deny");

		memcpy(pk, ok ? "RPTACK" : "MSTNAK", 6);
		set4(pk + 6, nodeid);
		sendpacket(addr, pk, 10);
	}
	else if (pksize >= 12 && (memcmp(pk, "RPTC", 4) == 0 || memcmp(pk, "RPTO", 4) == 0)) {
		dword nodeid = get4(pk + 4);
		log (&addr, "RPTC node %d\n", nodeid);

		node *n = findnode (nodeid, false);
		if (!n) {
			log (&addr, "Node %d not found for RPTC", nodeid);
			return;
		}
		if (getinaddr(n->addr) != getinaddr(addr)) {
			log (&addr, "Invalid RPTC IP address for node %d, should be %s\n", nodeid, my_inet_ntoa(n->addr.sin_addr).c_str());
			return;
		}

		n->hitsec = g_sec;

		std::string cfg((char*)pk + 8, pksize - 8);
		std::vector<dword> ts1 = csv_all_ints(kv_value(cfg, "TS1="));
		std::vector<dword> ts2 = csv_all_ints(kv_value(cfg, "TS2="));

		static_register_slot(&n->slots[0], ts1);
		static_register_slot(&n->slots[1], ts2);

		if (!ts1.empty()) {
			talkgroup* g1 = findgroup(ts1[0], true);
			if (g1) subscribe_to_group(&n->slots[0], g1);
		} else {
			unsubscribe_from_group(&n->slots[0]);
		}

		if (!ts2.empty()) {
			talkgroup* g2 = findgroup(ts2[0], true);
			if (g2) subscribe_to_group(&n->slots[1], g2);
		} else {
			unsubscribe_from_group(&n->slots[1]);
		}

		log (&addr, "RPTC static TGs node %d TS1=%s TS2=%s\n", nodeid, csv_join_dwords(ts1).c_str(), csv_join_dwords(ts2).c_str());

		memcpy (pk, "RPTACK", 6);
		set4(pk + 6, nodeid);
		sendpacket (addr, pk, 10);
	}
	else if (pksize == 11 && memcmp(pk, "RPTPING", 7) == 0) {
		dword nodeid = get4(pk + 7);
		node* n = findnode(nodeid, false);
		if (!n) return;

		if (!g_relax_ip_change && getinaddr(n->addr) != getinaddr(addr)) return;

		n->hitsec = g_sec;
		memcpy(pk, "MSTPONG", 7);
		set4(pk + 7, nodeid);
		sendpacket(addr, pk, 11);
	}
	else if (pksize == 11 && memcmp(pk, "FMRPING", 7) == 0) {
		log(&addr, "FreeDMR ping received.\n");
		memcpy(pk, "FMRPONG", 7);
		sendpacket(addr, pk, 11);
	}
	else if (pksize == 9 && memcmp(pk, "RPTCL", 5)==0) {
		dword nodeid = get4(pk + 5);
		log (&addr, "RPTCL node %d\n", nodeid);
		node *n = findnode (nodeid, false);
		if (!n) {
			log (&addr, "Node %d doesn't exist for RPTCL", nodeid);
			return;
		}

		if (getinaddr(addr) == getinaddr(n->addr))
			delete_node (nodeid);
		else
			log (&addr, "Invalid RPTCL IP address for node %d, should be %s\n", nodeid, my_inet_ntoa(n->addr.sin_addr).c_str());
	}
	else if (pksize >= 5 && memcmp(pk, "/STAT", 5)==0) {
		char temp[500];
		std::string str;
		_dump_nodes(str);
		memset (temp, 0, sizeof(temp));
		strncpy ((char*)temp, str.c_str(), sizeof(temp)-1);
		sendpacket (addr, temp, strlen((char*)temp));
	}

	dump_groups();
}

bool show_running_status() {
	int sock;
	if ((sock = open_udp(62111)) == -1) {
		log (NULL, "Failed to open UDP port (%d)\n", GetInetError());
		return 1;
	}

	sockaddr_in addr;

	memset (&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(g_udp_port);
#ifdef WIN32
	addr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
#else
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
#endif

	if (sendto (sock, "/STAT", 5, 0, (sockaddr*)&addr, sizeof(addr)) == -1) {
		printf ("sendto() failed (%d)\n", GetInetError());
		CLOSESOCKET(sock);
		return false;
	}

	if (!select_rx (sock, 5)) {
		puts ("No reply from server");
		CLOSESOCKET(sock);
		return false;
	}

	char buf[1001];

	memset (buf, 0, sizeof(buf));
	int sz = recvfrom (sock, (char*) buf, sizeof(buf)-1, 0, NULL, 0);
	if (sz == -1) {
		printf ("recvfrom() failed (%d)\n", GetInetError());
		CLOSESOCKET(sock);
		return false;
	}

	puts (buf);

	CLOSESOCKET(sock);

	return true;
}

static void trim_spaces(char* s) {
    if (!s) return;
    char* p = s; while (*p==' '||*p=='\t'||*p=='\r'||*p=='\n') ++p;
    if (p != s) memmove(s, p, strlen(p)+1);
    int n = (int)strlen(s);
    while (n>0 && (s[n-1]==' '||s[n-1]=='\t'||s[n-1]=='\r'||s[n-1]=='\n')) s[--n]=0;
}

static bool str_ieq_n(const char* a, const char* b, size_t n) {
    for (size_t i=0; i<n; ++i) {
        unsigned char ca = (unsigned char)a[i];
        unsigned char cb = (unsigned char)b[i];
        if (tolower(ca) != tolower(cb)) return false;
    }
    return true;
}

static std::string json_escape(const std::string& s) {
    std::string out;
    for (size_t i=0; i<s.size(); ++i) {
        unsigned char c = (unsigned char)s[i];
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 32) {
                    char tmp[8];
                    snprintf(tmp, sizeof(tmp), "\\u%04x", (unsigned)c);
                    out += tmp;
                } else out += (char)c;
        }
    }
    return out;
}

static std::string url_decode(const char* s) {
    std::string out;
    if (!s) return out;
    for (; *s; ++s) {
        if (*s == '+') out += ' ';
        else if (*s == '%' && isxdigit((unsigned char)s[1]) && isxdigit((unsigned char)s[2])) {
            char hex[3] = { s[1], s[2], 0 };
            out += (char)strtol(hex, NULL, 16);
            s += 2;
        } else out += *s;
    }
    return out;
}

static std::string form_get_value(const char* body, const char* key) {
    std::string out;
    if (!body || !key || !*key) return out;
    std::string want = key;
    const char* p = body;
    while (*p) {
        const char* amp = strchr(p, '&');
        size_t len = amp ? (size_t)(amp - p) : strlen(p);
        std::string part(p, len);
        size_t eq = part.find('=');
        std::string k = url_decode(part.substr(0, eq).c_str());
        std::string v = url_decode(eq == std::string::npos ? "" : part.substr(eq+1).c_str());
        if (k == want) return v;
        if (!amp) break;
        p = amp + 1;
    }
    return out;
}

static void collapse_spaces(std::string& s) {
    std::string out;
    bool prev_space = true;
    for (size_t i=0; i<s.size(); ++i) {
        unsigned char c = (unsigned char)s[i];
        if (c=='\r' || c=='\n' || c=='\t') c = ' ';
        if (c == ' ') {
            if (!prev_space) out += ' ';
            prev_space = true;
        } else {
            out += (char)c;
            prev_space = false;
        }
    }
    while (!out.empty() && out[0]==' ') out.erase(0,1);
    while (!out.empty() && out[out.size()-1]==' ') out.erase(out.size()-1,1);
    s.swap(out);
}

static bool sanitize_callsign(std::string& s) {
    collapse_spaces(s);
    if (s.empty() || s.size() > 32) return false;
    for (size_t i=0; i<s.size(); ++i) {
        unsigned char c = (unsigned char)s[i];
        if (c >= 'a' && c <= 'z') s[i] = (char)(c - 'a' + 'A');
        c = (unsigned char)s[i];
        if (!(isalnum(c) || c=='-' || c=='/' || c=='.')) return false;
    }
    return true;
}

static bool sanitize_name(std::string& s) {
    collapse_spaces(s);
    if (s.empty() || s.size() > 96) return false;
    for (size_t i=0; i<s.size(); ++i) {
        unsigned char c = (unsigned char)s[i];
        if (c < 32 || c == ',') return false;
    }
    return true;
}

static bool validate_password(std::string& s) {
    if (s.size() < 4 || s.size() >= MAX_PASSWORD_SIZE) return false;
    for (size_t i=0; i<s.size(); ++i) {
        unsigned char c = (unsigned char)s[i];
        if (c < 33 || c > 126 || c == ',') return false;
    }
    return true;
}

static bool load_text_lines(const char* path, std::vector<std::string>& lines) {
    lines.clear();
    if (!path || !*path) return false;
    FILE* f = fopen(path, "rb");
    if (!f) return true;
    char buf[1024];
    while (fgets(buf, sizeof(buf), f)) {
        size_t n = strlen(buf);
        while (n>0 && (buf[n-1]=='\r' || buf[n-1]=='\n')) buf[--n] = 0;
        lines.push_back(buf);
    }
    fclose(f);
    return true;
}

static bool write_text_lines(const char* path, const std::vector<std::string>& lines, std::string& err) {
    if (!path || !*path) { err = "Path not configured"; return false; }
    std::string tmp = std::string(path) + ".tmp";
    FILE* f = fopen(tmp.c_str(), "wb");
    if (!f) { err = "Cannot write temporary file"; return false; }
    for (size_t i=0; i<lines.size(); ++i) {
        if (fputs(lines[i].c_str(), f) == EOF || fputc('\n', f) == EOF) {
            fclose(f); remove(tmp.c_str()); err = "Write failed"; return false;
        }
    }
    if (fclose(f) != 0) { remove(tmp.c_str()); err = "Cannot close temporary file"; return false; }
    remove(path);
    if (rename(tmp.c_str(), path) != 0) { remove(tmp.c_str()); err = "Cannot replace destination file"; return false; }
    return true;
}

static bool parse_leading_id(const std::string& line, long* out_id) {
    if (out_id) *out_id = 0;
    char buf[1024];
    strncpy(buf, line.c_str(), sizeof(buf)-1);
    buf[sizeof(buf)-1] = 0;
    trim_spaces(buf);
    if (!buf[0] || buf[0] == '#') return false;
    char* end = buf;
    long id = strtol(buf, &end, 10);
    if (end == buf || id <= 0) return false;
    if (out_id) *out_id = id;
    return true;
}

static bool parse_dmrids_line(const std::string& line, long* out_id, std::string* out_callsign, std::string* out_name) {
    if (out_id) *out_id = 0;
    if (out_callsign) out_callsign->clear();
    if (out_name) out_name->clear();

    char buf[1024];
    strncpy(buf, line.c_str(), sizeof(buf)-1);
    buf[sizeof(buf)-1] = 0;
    char* p = buf;
    trim_spaces(p);
    if (!*p || *p == '#') return false;

    char* end = p;
    long id = strtol(p, &end, 10);
    if (end == p || id <= 0) return false;
    while (*end == ' ' || *end == '	') ++end;

    char callsign[128] = {0};
    int ci = 0;
    while (*end && *end != ' ' && *end != '	' && ci < (int)sizeof(callsign)-1) callsign[ci++] = *end++;
    callsign[ci] = 0;
    while (*end == ' ' || *end == '	') ++end;

    std::string name = end;
    collapse_spaces(name);
    if (out_id) *out_id = id;
    if (out_callsign) *out_callsign = callsign;
    if (out_name) *out_name = name;
    return true;
}

static bool dmrid_exists_in_auth_file(const char* path, dword dmrid, std::string& err) {
    std::vector<std::string> lines;
    if (!load_text_lines(path, lines)) { err = "Cannot read auth file"; return false; }
    for (size_t i=0; i<lines.size(); ++i) {
        char buf[1024];
        strncpy(buf, lines[i].c_str(), sizeof(buf)-1);
        buf[sizeof(buf)-1] = 0;
        trim_spaces(buf);
        if (!buf[0] || buf[0] == '#') continue;
        char* comma = strchr(buf, ',');
        if (!comma) continue;
        *comma = 0;
        trim_spaces(buf);
        long id = strtol(buf, NULL, 10);
        if (id == (long)dmrid) return true;
    }
    return false;
}

static bool dmrid_exists_in_dmrids_file(const char* path, dword dmrid, std::string& err) {
    std::vector<std::string> lines;
    if (!load_text_lines(path, lines)) { err = "Cannot read DMRIds file"; return false; }
    for (size_t i=0; i<lines.size(); ++i) {
        long id = 0;
        if (parse_leading_id(lines[i], &id) && id == (long)dmrid) return true;
    }
    return false;
}

static bool auth_verify_password(dword dmrid, const std::string& password) {
    const char* saved = auth_lookup_pass(dmrid);
    return saved && password == saved;
}

static bool auth_update_password_file(const char* path, dword dmrid, const char* pass, std::string& err) {
    std::vector<std::string> lines;
    if (!load_text_lines(path, lines)) { err = "Cannot read auth file"; return false; }

    bool found = false;
    for (size_t i=0; i<lines.size(); ++i) {
        char buf[1024];
        strncpy(buf, lines[i].c_str(), sizeof(buf)-1);
        buf[sizeof(buf)-1] = 0;
        trim_spaces(buf);
        if (!buf[0] || buf[0] == '#') continue;
        char* comma = strchr(buf, ',');
        if (!comma) continue;
        *comma = 0;
        trim_spaces(buf);
        long id = strtol(buf, NULL, 10);
        if (id == (long)dmrid) {
            char newline[512];
            snprintf(newline, sizeof(newline), "%u,%s", (unsigned)dmrid, pass);
            lines[i] = newline;
            found = true;
            break;
        }
    }

    if (!found) { err = "DMR-ID not found in auth_users.csv"; return false; }
    return write_text_lines(path, lines, err);
}

static bool dmrids_read_profile(const char* path, dword dmrid, std::string& callsign, std::string& name, std::string& err) {
    callsign.clear();
    name.clear();
    std::vector<std::string> lines;
    if (!load_text_lines(path, lines)) { err = "Cannot read DMRIds file"; return false; }
    for (size_t i=0; i<lines.size(); ++i) {
        long id = 0;
        std::string cs, nm;
        if (!parse_dmrids_line(lines[i], &id, &cs, &nm)) continue;
        if (id == (long)dmrid) {
            callsign = cs;
            name = nm;
            return true;
        }
    }
    return true;
}

static bool dmrids_update_name_file(const char* path, dword dmrid, const char* name, std::string& callsign_inout, std::string& err) {
    std::vector<std::string> lines;
    if (!load_text_lines(path, lines)) { err = "Cannot read DMRIds file"; return false; }

    bool found = false;
    for (size_t i=0; i<lines.size(); ++i) {
        long id = 0;
        std::string cs, nm;
        if (!parse_dmrids_line(lines[i], &id, &cs, &nm)) continue;
        if (id == (long)dmrid) {
            if (!cs.empty()) callsign_inout = cs;
            found = true;
            lines[i] = std::to_string((unsigned)dmrid) + " " + callsign_inout + " " + name;
            break;
        }
    }

    if (!found) {
        if (callsign_inout.empty()) callsign_inout = std::string("DMR") + std::to_string((unsigned)dmrid);
        lines.push_back(std::to_string((unsigned)dmrid) + " " + callsign_inout + " " + name);
    }

    return write_text_lines(path, lines, err);
}

static bool append_auth_user_file(const char* path, dword dmrid, const char* pass, std::string& err) {
    std::vector<std::string> lines;
    if (!load_text_lines(path, lines)) { err = "Cannot read auth file"; return false; }
    char newline[512];
    snprintf(newline, sizeof(newline), "%u,%s", (unsigned)dmrid, pass);
    lines.push_back(newline);
    return write_text_lines(path, lines, err);
}

static bool append_dmrids_file(const char* path, dword dmrid, const char* callsign, const char* name, std::string& err) {
    std::vector<std::string> lines;
    if (!load_text_lines(path, lines)) { err = "Cannot read DMRIds file"; return false; }
    lines.push_back(std::to_string((unsigned)dmrid) + " " + callsign + " " + name);
    return write_text_lines(path, lines, err);
}

static bool auth_load_now(const char* path) {
    if (!path || !*path) return false;
    FILE* f = fopen(path, "r");
    if (!f) {
		logmsg (LOG_RED, 0, "Auth: cannot open %s\n\n", g_auth_file);
		return false;
	}

    std::map<dword, std::string> tmp;

    char line[512];
    int lineno = 0, added = 0;
    while (fgets(line, sizeof(line), f)) {
        ++lineno;
        char* nl = strchr(line, '\n'); if (nl) *nl = 0;
        char* s = line; trim_spaces(s);
        if (!*s || *s=='#') continue;

        char* comma = strchr(s, ',');
        if (!comma) continue;
        *comma = 0;
        char* idstr = s;
        char* pass = comma + 1;

        trim_spaces(idstr);
        trim_spaces(pass);

        long id = strtol(idstr, NULL, 10);
        if (id <= 0 || !*pass) continue;

        if ((int)strlen(pass) >= MAX_PASSWORD_SIZE) pass[MAX_PASSWORD_SIZE-1] = 0;

        tmp[(dword)id] = pass;
        added++;
    }
    fclose(f);

    g_auth_map.swap(tmp);
    g_auth_last_load_sec = g_sec;
	logmsg (LOG_YELLOW, 0, "Auth: loaded %d entries from %s\n\n", added, g_auth_file);
    return true;
}

void auth_load_initial() {
    if (!g_auth_enabled) return;
    auth_load_now(g_auth_file);
}

void auth_housekeeping() {
    if (!g_auth_enabled) return;
    if (g_auth_reload_secs > 0 && (int)(g_sec - g_auth_last_load_sec) >= g_auth_reload_secs)
        auth_load_now(g_auth_file);
}

const char* auth_lookup_pass(dword dmrid) {
    std::map<dword, std::string>::iterator it = g_auth_map.find(dmrid);
    if (it == g_auth_map.end()) return NULL;
    return it->second.c_str();
}

#ifdef HAVE_SMS
static bool tg_in_csv(dword tg, PCSTR csv) {
    if (!csv || !*csv) return false;
    char buf[512]; strncpy(buf, csv, sizeof(buf)-1); buf[sizeof(buf)-1]=0;
    char *p = buf;
    while (*p) {
        while (*p==' '||*p==',') ++p;
        if (!*p) break;
        char *q = p; while (*q && *q!=',') ++q;
        if (*q) *q++ = 0;
        if (atoi(p) == (int)tg) return true;
        p = q;
    }
    return false;
}

bool sms_tg_permitted(dword tg) {
    if (g_sms.permit_all) return true;
    return tg_in_csv(tg, g_sms.permit_tgs);
}

void sms_reset(sms_buf &sb) {
    if (sb.buf) { delete sb.buf; sb.buf = NULL; }
    sb.streamid = 0;
    sb.start_sec = 0;
    sb.frames = 0;
}

void sms_append(sms_buf &sb, const byte *block51) {
    if (!sb.buf) sb.buf = new memfile;
    sb.buf->Write(block51, DMRD_BLOCK_LEN);
    sb.frames++;
}

static void ascii_best_effort(const byte *src, int n, char *dst, int dstcap) {
    int j=0;
    for (int i=0;i<n && j<dstcap-1;i++) {
        byte b = src[i];
        dst[j++] = (b>=32 && b<=126) ? (char)b : '.';
    }
    dst[j]=0;
}

static void sms_send_udp_line(PCSTR host, int port, PCSTR line) {
    if (!host || !*host || port<=0) return;
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s == -1) return;
    sockaddr_in a; memset(&a,0,sizeof(a));
    a.sin_family = AF_INET; a.sin_port = htons(port);
#ifdef WIN32
    unsigned long ip = inet_addr(host);
    if (ip == INADDR_NONE) { hostent* he = gethostbyname(host); if (!he) { CLOSESOCKET(s); return; } a.sin_addr.S_un.S_addr = *(u_long*)he->h_addr_list[0]; }
    else a.sin_addr.S_un.S_addr = ip;
#else
    in_addr_t ip = inet_addr(host);
    if (ip == INADDR_NONE) { hostent* he = gethostbyname(host); if (!he) { CLOSESOCKET(s); return; } a.sin_addr.s_addr = *(in_addr_t*)he->h_addr_list[0]; }
    else a.sin_addr.s_addr = ip;
#endif
    sendto(s, line, (int)strlen(line), 0, (sockaddr*)&a, sizeof(a));
    CLOSESOCKET(s);
}

void sms_emit_udp(dword radioid, dword dest, bool is_private, sms_buf &sb)
{
    if (!g_sms.enabled || !sb.buf || !sb.buf->GetSize()) { sms_reset(sb); return; }

    char *hex = (char*)malloc(sb.buf->GetSize()*2 + 1);
    char *asc = (char*)malloc(sb.buf->GetSize() + 1);
    if (!hex || !asc) { if(hex) free(hex); if(asc) free(asc); sms_reset(sb); return; }

    for (dword i=0;i<sb.buf->GetSize();i++) {
        byte b = sb.buf->m_pData[i];
        sprintf(hex + i*2, "%02X", b);
    }
    hex[sb.buf->GetSize()*2] = 0;

    ascii_best_effort(sb.buf->m_pData, sb.buf->GetSize(), asc, (int)sb.buf->GetSize()+1);

    char line[1024];
    int dur = (g_sec > sb.start_sec) ? (int)(g_sec - sb.start_sec) : 0;
    snprintf(line, sizeof(line),
             "DMRSMS from=%u to=%u type=%c frames=%d secs=%d hex=%s ascii=\"%s\"",
             (unsigned)radioid, (unsigned)dest, is_private ? 'P':'G',
             sb.frames, dur, hex, asc);

    sms_send_udp_line(g_sms.udphost, g_sms.udpport, line);

    free(hex); free(asc);
    sms_reset(sb);
}
#endif

#ifdef HAVE_HTTPMODE
static unsigned now_sec(void){ return (unsigned)time(NULL); }

#ifdef _WIN32
static CRITICAL_SECTION g_mon_lock;
static CRITICAL_SECTION g_mon_client_lock;
static volatile LONG g_mon_lock_ready = 0;
static volatile LONG g_monitor_client_count = 0;
static void mon_init_locks(void){
    if (InterlockedCompareExchange(&g_mon_lock_ready, 1, 0) == 0) {
        InitializeCriticalSection(&g_mon_lock);
        InitializeCriticalSection(&g_mon_client_lock);
    }
}
#define MON_LOCK() EnterCriticalSection(&g_mon_lock)
#define MON_UNLOCK() LeaveCriticalSection(&g_mon_lock)
#define MON_CLIENT_LOCK() EnterCriticalSection(&g_mon_client_lock)
#define MON_CLIENT_UNLOCK() LeaveCriticalSection(&g_mon_client_lock)
#else
static pthread_mutex_t g_mon_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_mon_client_lock = PTHREAD_MUTEX_INITIALIZER;
static volatile int g_monitor_client_count = 0;
static void mon_init_locks(void){}
#define MON_LOCK() pthread_mutex_lock(&g_mon_lock)
#define MON_UNLOCK() pthread_mutex_unlock(&g_mon_lock)
#define MON_CLIENT_LOCK() pthread_mutex_lock(&g_mon_client_lock)
#define MON_CLIENT_UNLOCK() pthread_mutex_unlock(&g_mon_client_lock)
#endif

static const int MONITOR_MAX_CLIENTS = 128;
static const int MONITOR_LISTEN_BACKLOG = 256;
static const int MONITOR_CLIENT_TIMEOUT_MS = 15000;

static MonMark* mark_get_slot_unsafe(int radio){
    if (radio <= 0) return NULL;
    unsigned h = (unsigned)radio * 2654435761u;
    unsigned i = (h >> 22) % MONMARK_N;
    for (unsigned k=0; k<MONMARK_N; ++k){
        MonMark* m = &g_marks[(i+k)%MONMARK_N];
        if (!m->used || m->radio == radio) return m;
    }
    return &g_marks[i];
}

static MonMark mark_copy_for_radio(int radio){
    MonMark out;
    memset(&out, 0, sizeof(out));
    MON_LOCK();
    MonMark* m = mark_get_slot_unsafe(radio);
    if (m && m->used && m->radio == radio) out = *m;
    MON_UNLOCK();
    return out;
}

void monitor_note_event(int radio, int tg, MonSrc src, int is_aprs, int is_sms){
    MON_LOCK();
    MonMark* m = mark_get_slot_unsafe(radio);
    if (m) {
        m->used = 1;
        m->radio = radio;
        if (tg>0) m->tg = tg;
        if (src != MON_SRC_UNKNOWN) m->src = src;
        if (is_aprs>=0) m->aprs = is_aprs ? 1:0;
        if (is_sms>=0)  m->sms  = is_sms  ? 1:0;
        m->last_sec = now_sec();
    }
    MON_UNLOCK();
}

static struct { char bind_ip[64]; int port; char doc_root[512]; } G = {"127.0.0.1", 8080, "www"};
#ifdef _WIN32
static HANDLE g_thread = NULL; static volatile LONG g_run = 0; static sock_t g_listen = SOCK_INVALID;
#else
static pthread_t g_thread; static volatile int g_run = 0; static sock_t g_listen = SOCK_INVALID;
#endif

static void appendf(char **pp, size_t *left, const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    int n = vsnprintf(*pp, *left, fmt, ap);
    va_end(ap);
    if (n < 0) { *left = 0; return; }
    if ((size_t)n >= *left) { *left = 0; return; }
    *pp += n; *left -= (size_t)n;
}

static const char* http_time_now(void){
    static char buf[64]; time_t now=time(NULL);
#ifdef _WIN32
    struct tm t; localtime_s(&t,&now);
#else
    struct tm t; localtime_r(&now,&t);
#endif
    strftime(buf,sizeof(buf),"%a, %d %b %Y %H:%M:%S %Z",&t); return buf;
}

#ifdef _WIN32
  #define STRCASECMP _stricmp
#else
  #include <strings.h>
  #define STRCASECMP strcasecmp
#endif

static const char* mime_from_ext(const char* path){
    const char* ext = strrchr(path, '.'); if(!ext) return "application/octet-stream"; ++ext;
    if(!STRCASECMP(ext,"html")||!STRCASECMP(ext,"htm")) return "text/html; charset=utf-8";
    if(!STRCASECMP(ext,"css")) return "text/css; charset=utf-8";
    if(!STRCASECMP(ext,"js")) return "application/javascript";
    if(!STRCASECMP(ext,"json")) return "application/json";
    if(!STRCASECMP(ext,"png")) return "image/png";
    if(!STRCASECMP(ext,"jpg")||!STRCASECMP(ext,"jpeg")) return "image/jpeg";
    if(!STRCASECMP(ext,"svg")) return "image/svg+xml";
    if(!STRCASECMP(ext,"ico")) return "image/x-icon";
    return "application/octet-stream";
}

static int fetch_stat(char* out, size_t cap) {
    if (!out || cap==0) return -1; out[0]=0;
    sock_t s = socket(AF_INET, SOCK_DGRAM, 0); if (s == SOCK_INVALID) return -1;
#ifdef _WIN32
    DWORD tv = 2500; setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
#else
    struct timeval tv = {2,500000}; setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif
    struct sockaddr_in dst; memset(&dst,0,sizeof(dst));
    dst.sin_family = AF_INET; dst.sin_port = htons((uint16_t)g_udp_port);
#ifdef _WIN32
    InetPtonA(AF_INET, "127.0.0.1", &dst.sin_addr);
#else
    inet_pton(AF_INET, "127.0.0.1", &dst.sin_addr);
#endif
    const char* msg = "/STAT"; int mlen = 5;
    if (sendto(s, msg, mlen, 0, (struct sockaddr*)&dst, sizeof(dst)) < 0) { CLOSESOCK(s); return -1; }
    struct sockaddr_in from; socklen_t fl = sizeof(from);
    int n = recvfrom(s, out, (int)cap-1, 0, (struct sockaddr*)&from, &fl);
    if (n < 0) { CLOSESOCK(s); return -1; }
    out[n]=0; CLOSESOCK(s); return n;
}

struct io { sock_t fd; };
static int io_recv(struct io* io, char* buf, int cap){
#ifdef _WIN32
    return recv(io->fd, buf, cap, 0);
#else
    return (int)recv(io->fd, buf, cap, 0);
#endif
}
static int io_send_all(struct io* io, const char* data, size_t len){ size_t off=0; while(off<len){
#ifdef _WIN32
    int n = send(io->fd, data+off, (int)(len-off), 0);
#else
    int n = (int)send(io->fd, data+off, (int)(len-off), 0);
#endif
    if (n<=0) return -1; off += (size_t)n; } return 0; }
static void io_close(struct io* io){ CLOSESOCK(io->fd); }
static void http_send(int code, const char* reason, const char* ctype, const char* body, struct io* io){
    char hdr[512]; int n = snprintf(hdr,sizeof(hdr),
        "HTTP/1.1 %d %s\r\nDate: %s\r\nServer: dmr-monitor/2\r\nContent-Type: %s\r\nCache-Control: no-store\r\nContent-Length: %zu\r\n\r\n",
        code,reason,http_time_now(),ctype,(size_t)strlen(body));
    io_send_all(io,hdr,(size_t)n); io_send_all(io,body,strlen(body));
}
static void http_404(struct io* io){ http_send(404, "Not Found", "text/plain", "Not found", io); }

static int serve_static(struct io* io, const char* url){
    char rel[768]; const char* p = url; if (*p=='/') ++p; size_t i=0;
    for (; *p && *p!='?' && i<sizeof(rel)-1; ++p){ char c=*p; if (c=='\\') c='/'; rel[i++]=c; } rel[i]=0;
    if (rel[0]==0) strcpy(rel, "index.html");
    if (strstr(rel,"../")||strstr(rel,"/..")||strstr(rel,"..\\")) return 0;

    char full[1024];
#ifdef _WIN32
    snprintf(full,sizeof(full), "%s\\%s", G.doc_root, rel); for(char* q=full; *q; ++q) if (*q=='/') *q='\\';
#else
    snprintf(full,sizeof(full), "%s/%s", G.doc_root, rel);
#endif

    FILE* f = fopen(full, "rb"); if (!f) return 0;
    if (fseek(f,0,SEEK_END)!=0) { fclose(f); return 0; }
    long sz = ftell(f); if (sz<0) { fclose(f); return 0; } rewind(f);

	char lm[64] = "";
#ifdef _WIN32
    int fd = _fileno(f);
    if (fd >= 0) {
        HANDLE h = (HANDLE)_get_osfhandle(fd);
        if (h != INVALID_HANDLE_VALUE) {
            FILETIME ftWrite, ftCreate, ftAccess;
            if (GetFileTime(h, &ftCreate, &ftAccess, &ftWrite)) {
                ULARGE_INTEGER uli;
                uli.LowPart  = ftWrite.dwLowDateTime;
                uli.HighPart = ftWrite.dwHighDateTime;
                unsigned long long secs_since_1601 = uli.QuadPart / 10000000ULL;
                const unsigned long long EPOCH_DIFF = 11644473600ULL;
                if (secs_since_1601 > EPOCH_DIFF) {
                    time_t mt = (time_t)(secs_since_1601 - EPOCH_DIFF);
                    struct tm tt;
                    localtime_s(&tt, &mt);
                    strftime(lm, sizeof(lm), "%a, %d %b %Y %H:%M:%S %Z", &tt);
                }
            }
        }
    }
#else
    int fd = fileno(f);
    if (fd >= 0) {
        struct stat st;
        if (fstat(fd, &st) == 0) {
            time_t mt = st.st_mtime;
            struct tm tt;
            localtime_r(&mt, &tt);
            strftime(lm, sizeof(lm), "%a, %d %b %Y %H:%M:%S %Z", &tt);
        }
    }
#endif

	char hdr[512];
	int n = snprintf(hdr, sizeof(hdr),
		"HTTP/1.1 200 OK\r\nDate: %s\r\nServer: dmr-monitor/2\r\n"
		"Content-Type: %s\r\nCache-Control: no-store\r\nContent-Length: %ld\r\n",
		http_time_now(), mime_from_ext(full), sz);
	if (lm[0]) n += snprintf(hdr+n, sizeof(hdr)-n, "Last-Modified: %s\r\n", lm);
	n += snprintf(hdr+n, sizeof(hdr)-n, "\r\n");
	io_send_all(io, hdr, (size_t)n);

    char buf[16384]; size_t r; while ((r=fread(buf,1,sizeof(buf),f))>0) if (io_send_all(io,buf,r)!=0) break; fclose(f); return 1;
}

static void mark_read(int radio, int* src, int* aprs, int* sms){
    if (src) *src = 0; if (aprs) *aprs = 0; if (sms) *sms = 0;
    MonMark m = mark_copy_for_radio(radio);
    if (!m.used || m.radio != radio) return;
    if (src)  *src  = m.src;
    if (aprs) *aprs = m.aprs;
    if (sms)  *sms  = m.sms;
}

static void http_send_json(struct io* io, int code, const char* reason, const std::string& body) {
    http_send(code, reason, "application/json; charset=utf-8", body.c_str(), io);
}

static int parse_content_length(const std::string& req, size_t hdr_end) {
    size_t line_start = 0;
    while (line_start < hdr_end) {
        size_t line_end = req.find("\r\n", line_start);
        if (line_end == std::string::npos || line_end > hdr_end) break;
        size_t colon = req.find(':', line_start);
        if (colon != std::string::npos && colon < line_end) {
            std::string key = req.substr(line_start, colon - line_start);
            std::string val = req.substr(colon + 1, line_end - colon - 1);
            while (!val.empty() && (val[0] == ' ' || val[0] == '\t')) val.erase(0,1);
            if (key.size() == 14 && str_ieq_n(key.c_str(), "Content-Length", 14)) return atoi(val.c_str());
        }
        line_start = line_end + 2;
    }
    return 0;
}

static bool read_http_request(struct io* io, std::string& out) {
    out.clear();
    char buf[2048];
    size_t hdr_end = std::string::npos;
    int content_len = 0;
    while (out.size() < 16384) {
        int n = io_recv(io, buf, (int)sizeof(buf));
        if (n <= 0) break;
        out.append(buf, (size_t)n);
        if (hdr_end == std::string::npos) {
            size_t pos = out.find("\r\n\r\n");
            if (pos != std::string::npos) {
                hdr_end = pos + 4;
                content_len = parse_content_length(out, hdr_end);
                if ((int)(out.size() - hdr_end) >= content_len) return true;
                if (content_len <= 0) return true;
            }
        } else if ((int)(out.size() - hdr_end) >= content_len) return true;
    }
    return !out.empty();
}

static std::string request_header_value(const std::string& req, const char* key) {
    if (!key || !*key) return "";
    size_t hdr_end = req.find("\r\n\r\n");
    if (hdr_end == std::string::npos) hdr_end = req.size();
    size_t line_start = req.find("\r\n");
    if (line_start == std::string::npos) return "";
    line_start += 2;
    while (line_start < hdr_end) {
        size_t line_end = req.find("\r\n", line_start);
        if (line_end == std::string::npos || line_end > hdr_end) line_end = hdr_end;
        size_t colon = req.find(':', line_start);
        if (colon != std::string::npos && colon < line_end) {
            std::string k = req.substr(line_start, colon - line_start);
            if (STRCASECMP(k.c_str(), key) == 0) {
                std::string v = req.substr(colon + 1, line_end - colon - 1);
                while (!v.empty() && (v[0] == ' ' || v[0] == '\t')) v.erase(0, 1);
                while (!v.empty() && (v[v.size()-1] == ' ' || v[v.size()-1] == '\t')) v.erase(v.size()-1, 1);
                return v;
            }
        }
        line_start = line_end + 2;
    }
    return "";
}

static std::string hex_encode(const BYTE* data, size_t len) {
    static const char* hex = "0123456789abcdef";
    std::string out;
    out.reserve(len * 2);
    for (size_t i=0; i<len; ++i) {
        unsigned char b = data[i];
        out.push_back(hex[(b >> 4) & 0x0F]);
        out.push_back(hex[b & 0x0F]);
    }
    return out;
}

static void web_session_cleanup_unsafe() {
    unsigned now = now_sec();
    for (std::map<std::string, WebSession>::iterator it = g_web_sessions.begin(); it != g_web_sessions.end(); ) {
        if (it->second.expires <= now) g_web_sessions.erase(it++);
        else ++it;
    }
}

static std::string web_session_create(dword dmrid) {
    unsigned now = now_sec();
    char seed[256];
    MON_LOCK();
    web_session_cleanup_unsafe();
    unsigned long session_count = (unsigned long)g_web_sessions.size();
    snprintf(seed, sizeof(seed), "%u|%u|%u|%u|%u|%lu", (unsigned)dmrid, now, (unsigned)g_tick, (unsigned)g_sec, (unsigned)rand(), session_count);
    BYTE hash[SHA256_BLOCK_SIZE];
    make_sha256_hash(seed, (int)strlen(seed), hash, NULL, 0);
    std::string token = hex_encode(hash, sizeof(hash));
    WebSession ws;
    ws.dmrid = dmrid;
    ws.expires = now + WEB_SESSION_TTL;
    g_web_sessions[token] = ws;
    MON_UNLOCK();
    return token;
}

static bool web_session_lookup(const std::string& token, dword* out_dmrid) {
    if (out_dmrid) *out_dmrid = 0;
    if (token.empty()) return false;
    bool ok = false;
    MON_LOCK();
    web_session_cleanup_unsafe();
    std::map<std::string, WebSession>::iterator it = g_web_sessions.find(token);
    if (it != g_web_sessions.end()) {
        unsigned now = now_sec();
        if (it->second.expires <= now) g_web_sessions.erase(it);
        else {
            it->second.expires = now + WEB_SESSION_TTL;
            if (out_dmrid) *out_dmrid = it->second.dmrid;
            ok = true;
        }
    }
    MON_UNLOCK();
    return ok;
}

static void web_session_remove(const std::string& token) {
    if (!token.empty()) {
        MON_LOCK();
        g_web_sessions.erase(token);
        MON_UNLOCK();
    }
}

static std::string request_auth_token(const std::string& req) {
    std::string token = request_header_value(req, "X-Auth-Token");
    if (!token.empty()) return token;
    return request_header_value(req, "Authorization");
}

static bool read_profile_for_dmrid(dword dmrid, std::string& callsign, std::string& name, std::string& err) {
    callsign.clear();
    name.clear();
    if (!g_dmrids_file[0]) return true;
    return dmrids_read_profile(g_dmrids_file, dmrid, callsign, name, err);
}

static std::string callsign_json_for_radio(int radio) {
    if (radio <= 0) return std::string();
    std::string callsign, name, err;
    if (!read_profile_for_dmrid((dword)radio, callsign, name, err)) return std::string();
    return callsign;
}

static void api_config(struct io* io) {
    std::string body = std::string("{\"authEnabled\":") + (g_auth_enabled ? "1" : "0")
        + ",\"registrationEnabled\":" + (g_auth_enabled ? "1" : "0")
        + ",\"profileEnabled\":" + (g_auth_enabled ? "1" : "0")
        + ",\"dmrIdsFile\":\"" + json_escape(g_dmrids_file) + "\"}";
    http_send_json(io, 200, "OK", body);
}

static void api_login(struct io* io, const char* method, const char* body) {
    if (!g_auth_enabled) {
        http_send_json(io, 403, "Forbidden", "{\"ok\":false,\"message\":\"Login is disabled. Enable [Auth] -> Enable=1 first.\"}");
        return;
    }
    if (!method || strcmp(method, "POST") != 0) {
        http_send_json(io, 405, "Method Not Allowed", "{\"ok\":false,\"message\":\"POST required\"}");
        return;
    }

    std::string dmrid_s = form_get_value(body, "dmrid");
    std::string pass = form_get_value(body, "password");
    collapse_spaces(dmrid_s);

    long dmrid = strtol(dmrid_s.c_str(), NULL, 10);
    if (dmrid <= 0 || dmrid > 99999999L) {
        http_send_json(io, 400, "Bad Request", "{\"ok\":false,\"message\":\"Please enter a valid DMR-ID.\"}");
        return;
    }
    if (!validate_password(pass)) {
        http_send_json(io, 400, "Bad Request", "{\"ok\":false,\"message\":\"Password is invalid.\"}");
        return;
    }
    if (!auth_verify_password((dword)dmrid, pass)) {
        http_send_json(io, 401, "Unauthorized", "{\"ok\":false,\"message\":\"Invalid DMR-ID or password.\"}");
        return;
    }

    std::string callsign, name, err;
    if (!read_profile_for_dmrid((dword)dmrid, callsign, name, err)) {
        std::string msg = std::string("{\"ok\":false,\"message\":\"") + json_escape(err) + "\"}";
        http_send_json(io, 500, "Server Error", msg);
        return;
    }

    std::string token = web_session_create((dword)dmrid);
    std::string msg = std::string("{\"ok\":true,\"message\":\"Login successful.\",\"token\":\"")
        + token + "\",\"dmrid\":" + std::to_string((unsigned)dmrid)
        + ",\"callsign\":\"" + json_escape(callsign) + "\""
        + ",\"name\":\"" + json_escape(name) + "\"}";
    http_send_json(io, 200, "OK", msg);
}

static bool api_require_session(struct io* io, const std::string& req, dword* out_dmrid, std::string* out_token) {
    std::string token = request_auth_token(req);
    dword dmrid = 0;
    if (!web_session_lookup(token, &dmrid)) {
        http_send_json(io, 401, "Unauthorized", "{\"ok\":false,\"message\":\"Please log in first.\"}");
        return false;
    }
    if (out_dmrid) *out_dmrid = dmrid;
    if (out_token) *out_token = token;
    return true;
}

static void api_profile(struct io* io, const std::string& req, const char* method, const char* body) {
    dword dmrid = 0;
    if (!api_require_session(io, req, &dmrid, NULL)) return;

    if (!method || strcmp(method, "GET") == 0) {
        std::string callsign, name, err;
        if (!read_profile_for_dmrid(dmrid, callsign, name, err)) {
            std::string msg = std::string("{\"ok\":false,\"message\":\"") + json_escape(err) + "\"}";
            http_send_json(io, 500, "Server Error", msg);
            return;
        }
        std::string msg = std::string("{\"ok\":true,\"dmrid\":") + std::to_string((unsigned)dmrid)
            + ",\"callsign\":\"" + json_escape(callsign) + "\""
            + ",\"name\":\"" + json_escape(name) + "\"}";
        http_send_json(io, 200, "OK", msg);
        return;
    }

    if (strcmp(method, "POST") != 0) {
        http_send_json(io, 405, "Method Not Allowed", "{\"ok\":false,\"message\":\"GET or POST required\"}");
        return;
    }

    std::string name = form_get_value(body, "name");
    std::string current_pass = form_get_value(body, "currentPassword");
    std::string new_pass = form_get_value(body, "newPassword");

    collapse_spaces(name);

    bool wants_name = !name.empty();
    bool wants_pass = !new_pass.empty();
    if (!wants_name && !wants_pass) {
        http_send_json(io, 400, "Bad Request", "{\"ok\":false,\"message\":\"Nothing to update.\"}");
        return;
    }
    if (wants_name && !sanitize_name(name)) {
        http_send_json(io, 400, "Bad Request", "{\"ok\":false,\"message\":\"Name is invalid.\"}");
        return;
    }
    if (wants_pass && !validate_password(new_pass)) {
        http_send_json(io, 400, "Bad Request", "{\"ok\":false,\"message\":\"New password must be 4-127 visible characters with no spaces or commas.\"}");
        return;
    }
    if (wants_pass && !auth_verify_password(dmrid, current_pass)) {
        http_send_json(io, 401, "Unauthorized", "{\"ok\":false,\"message\":\"Current password is incorrect.\"}");
        return;
    }

    std::string callsign, old_name, err;
    if (!read_profile_for_dmrid(dmrid, callsign, old_name, err)) {
        std::string msg = std::string("{\"ok\":false,\"message\":\"") + json_escape(err) + "\"}";
        http_send_json(io, 500, "Server Error", msg);
        return;
    }

    if (wants_name) {
        if (!g_dmrids_file[0]) {
            http_send_json(io, 500, "Server Error", "{\"ok\":false,\"message\":\"DMRIds.dat path is not configured.\"}");
            return;
        }
        if (!dmrids_update_name_file(g_dmrids_file, dmrid, name.c_str(), callsign, err)) {
            std::string msg = std::string("{\"ok\":false,\"message\":\"") + json_escape(err) + "\"}";
            http_send_json(io, 500, "Server Error", msg);
            return;
        }
    } else name = old_name;

    if (wants_pass) {
        if (!g_auth_file[0]) {
            http_send_json(io, 500, "Server Error", "{\"ok\":false,\"message\":\"Auth file is not configured.\"}");
            return;
        }
        if (!auth_update_password_file(g_auth_file, dmrid, new_pass.c_str(), err)) {
            std::string msg = std::string("{\"ok\":false,\"message\":\"") + json_escape(err) + "\"}";
            http_send_json(io, 500, "Server Error", msg);
            return;
        }
        auth_load_now(g_auth_file);
    }

    std::string msg = std::string("{\"ok\":true,\"message\":\"Profile updated.\",\"dmrid\":")
        + std::to_string((unsigned)dmrid)
        + ",\"callsign\":\"" + json_escape(callsign) + "\""
        + ",\"name\":\"" + json_escape(name) + "\"}";
    http_send_json(io, 200, "OK", msg);
}

static void api_logout(struct io* io, const std::string& req, const char* method) {
    if (!method || strcmp(method, "POST") != 0) {
        http_send_json(io, 405, "Method Not Allowed", "{\"ok\":false,\"message\":\"POST required\"}");
        return;
    }
    std::string token = request_auth_token(req);
    web_session_remove(token);
    http_send_json(io, 200, "OK", "{\"ok\":true,\"message\":\"Logged out.\"}");
}

static void api_register(struct io* io, const char* method, const char* body) {
    if (!g_auth_enabled) {
        http_send_json(io, 403, "Forbidden", "{\"ok\":false,\"message\":\"Registration is disabled. Enable [Auth] -> Enable=1 first.\"}");
        return;
    }
    if (!method || strcmp(method, "POST") != 0) {
        http_send_json(io, 405, "Method Not Allowed", "{\"ok\":false,\"message\":\"POST required\"}");
        return;
    }

    std::string dmrid_s  = form_get_value(body, "dmrid");
    std::string callsign = form_get_value(body, "callsign");
    std::string name     = form_get_value(body, "name");
    std::string pass     = form_get_value(body, "password");

    collapse_spaces(dmrid_s);
    collapse_spaces(callsign);
    collapse_spaces(name);

    long dmrid = strtol(dmrid_s.c_str(), NULL, 10);
    if (dmrid <= 0 || dmrid > 99999999L) {
        http_send_json(io, 400, "Bad Request", "{\"ok\":false,\"message\":\"Please enter a valid DMR-ID.\"}");
        return;
    }
    if (!sanitize_callsign(callsign)) {
        http_send_json(io, 400, "Bad Request", "{\"ok\":false,\"message\":\"Callsign is invalid. Use letters, numbers, dash, slash or dot only.\"}");
        return;
    }
    if (!sanitize_name(name)) {
        http_send_json(io, 400, "Bad Request", "{\"ok\":false,\"message\":\"Name is invalid.\"}");
        return;
    }
    if (!validate_password(pass)) {
        http_send_json(io, 400, "Bad Request", "{\"ok\":false,\"message\":\"Password must be 4-127 visible characters with no spaces or commas.\"}");
        return;
    }
    if (!g_auth_file[0]) {
        http_send_json(io, 500, "Server Error", "{\"ok\":false,\"message\":\"Auth file is not configured.\"}");
        return;
    }
    if (!g_dmrids_file[0]) {
        http_send_json(io, 500, "Server Error", "{\"ok\":false,\"message\":\"DMRIds.dat path is not configured.\"}");
        return;
    }
    std::string err;
    bool exists_in_auth = dmrid_exists_in_auth_file(g_auth_file, (dword)dmrid, err);
    if (!err.empty()) {
        std::string msg = std::string("{\"ok\":false,\"message\":\"") + json_escape(err) + "\"}";
        http_send_json(io, 500, "Server Error", msg);
        return;
    }

    bool exists_in_dmrids = dmrid_exists_in_dmrids_file(g_dmrids_file, (dword)dmrid, err);
    if (!err.empty()) {
        std::string msg = std::string("{\"ok\":false,\"message\":\"") + json_escape(err) + "\"}";
        http_send_json(io, 500, "Server Error", msg);
        return;
    }

    if (exists_in_auth || exists_in_dmrids) {
        std::string msg = std::string("{\"ok\":false,\"message\":\"DMR-ID already exists!\"}");
        http_send_json(io, 409, "Conflict", msg);
        return;
    }

    if (!append_dmrids_file(g_dmrids_file, (dword)dmrid, callsign.c_str(), name.c_str(), err)) {
        std::string msg = std::string("{\"ok\":false,\"message\":\"") + json_escape(err) + "\"}";
        http_send_json(io, 500, "Server Error", msg);
        return;
    }
    if (!append_auth_user_file(g_auth_file, (dword)dmrid, pass.c_str(), err)) {
        std::string msg = std::string("{\"ok\":false,\"message\":\"") + json_escape(err) + "\"}";
        http_send_json(io, 500, "Server Error", msg);
        return;
    }

    auth_load_now(g_auth_file);
    logmsg(LOG_GREEN, 0, "Web registration saved DMR-ID %u (%s %s)\n", (unsigned)dmrid, callsign.c_str(), name.c_str());

    std::string msg = std::string("{\"ok\":true,\"message\":\"Registration saved for DMR-ID ")
        + std::to_string((unsigned)dmrid) + "\",\"dmrid\":" + std::to_string((unsigned)dmrid)
        + ",\"callsign\":\"" + json_escape(callsign) + "\""
        + ",\"name\":\"" + json_escape(name) + "\"}";
    http_send_json(io, 200, "OK", msg);
}

static void api_log(struct io* io, const char* path){
    if (!db){ http_send(500, "DB Closed", "application/json", "[]", io); return; }

    int limit = 20;
    const char* k = strstr(path, "limit=");
    if (k) { int v = atoi(k+6); if (v >= 1 && v <= 500) limit = v; }

    const char* q =
        "SELECT l.ID, l.DATE, l.RADIO, l.TG, l.SLOT, l.NODE, l.TIME, l.ACTIVE, l.CONNECT, "
        "       CASE WHEN EXISTS (SELECT 1 FROM LOG la WHERE la.RADIO = l.RADIO AND la.ACTIVE = 1) "
        "            THEN 1 ELSE 0 END AS ONLINE "
        "FROM LOG l "
        "JOIN ( "
        "   SELECT RADIO, MAX(ID) AS max_id "
        "   FROM LOG "
        "   GROUP BY RADIO "
        ") m ON l.RADIO = m.RADIO AND l.ID = m.max_id "
        "ORDER BY l.ID DESC "
        "LIMIT ?";

    sqlite3_stmt* st = NULL;
    if (sqlite3_prepare_v2(db, q, -1, &st, NULL) != SQLITE_OK) {
        http_send(500, "Query Error", "application/json", "[]", io);
        return;
    }
    sqlite3_bind_int(st, 1, limit);

    char* out = (char*)malloc(65536);
    char* p = out; size_t left = 65536;
    appendf(&p, &left, "["); int first = 1;

    while (sqlite3_step(st) == SQLITE_ROW){
        int id    = sqlite3_column_int(st, 0);
        const unsigned char* date = sqlite3_column_text(st, 1);
        int radio = sqlite3_column_int(st, 2);
        int tg    = sqlite3_column_int(st, 3);
        int slot  = sqlite3_column_int(st, 4);
        int node  = sqlite3_column_int(st, 5);
        int sec   = sqlite3_column_int(st, 6);
        int active= sqlite3_column_int(st, 7);
        int conn  = sqlite3_column_int(st, 8);
        int online= sqlite3_column_int(st, 9);
		int src=0, aprs=0, sms=0;
		mark_read(radio, &src, &aprs, &sms);
        std::string callsign = callsign_json_for_radio(radio);

		appendf(&p,&left,
		  "%s{\"id\":%d,\"date\":\"%s\",\"radio\":%d,\"callsign\":\"%s\",\"tg\":%d,\"slot\":%d,"
		  "\"node\":%d,\"time\":%d,\"active\":%d,\"online\":%d,\"connect\":%d,"
		  "\"src\":%d,\"aprs\":%d,\"sms\":%d}",
		  first?"":",",
		  id, date?(const char*)date:"", radio, json_escape(callsign).c_str(), tg, slot, node, sec, active, online, conn,
		  src, aprs, sms);

        first = 0;
        if (left < 256) break;
    }
    sqlite3_finalize(st);

    appendf(&p, &left, "]");
    http_send(200, "OK", "application/json", out, io);
    free(out);
}

static void api_active(struct io* io){
    if (!db){ http_send(500, "DB Closed", "application/json", "[]", io); return; }

    const char* q =
        "SELECT DATE, RADIO, TG, SLOT, NODE, TIME "
        "FROM LOG WHERE ACTIVE=1 "
        "ORDER BY ID DESC LIMIT 1";

    sqlite3_stmt* st=NULL; sqlite3_prepare_v2(db,q,-1,&st,NULL);

    char* out = (char*)malloc(4096); char* p=out; size_t left=4096;
    appendf(&p,&left,"["); int first=1;

    while (sqlite3_step(st)==SQLITE_ROW){
        const unsigned char* date=sqlite3_column_text(st,0);
        int radio=sqlite3_column_int(st,1), tg=sqlite3_column_int(st,2),
            slot=sqlite3_column_int(st,3), node=sqlite3_column_int(st,4),
            sec=sqlite3_column_int(st,5);
		int src=0, aprs=0, sms=0;
		mark_read(radio, &src, &aprs, &sms);
        std::string callsign = callsign_json_for_radio(radio);

		appendf(&p,&left,
		  "%s{\"date\":\"%s\",\"radio\":%d,\"callsign\":\"%s\",\"tg\":%d,\"slot\":%d,\"node\":%d,\"time\":%d,"
		  "\"src\":%d,\"aprs\":%d,\"sms\":%d}",
		  first?"":",", date?(const char*)date:"", radio, json_escape(callsign).c_str(), tg, slot, node, sec,
		  src, aprs, sms);
        first=0;
    }
    sqlite3_finalize(st);
    appendf(&p,&left,"]");
    http_send(200, "OK", "application/json", out, io); free(out);
}

static void api_openbridge(struct io* io){
    char* out = (char*)malloc(32768);
    char* p = out; size_t left = 32768;
    appendf(&p, &left, "[");

    int first = 1;
    dword now = g_sec ? g_sec : (dword)time(NULL);
    int idx = 0;

    for (auto& ob : g_obp_peers) {
        ++idx;
        std::string resolved = my_inet_ntoa(ob.addr.sin_addr);
        const char* status = "idle";
        if ((int)(now - ob.last_rx_sec) < 60 || (int)(now - ob.last_tx_sec) < 60) status = "active";
        else if (resolved == "0.0.0.0") status = "unresolved";

        appendf(&p, &left,
            "%s{\"name\":\"OpenBridge%d\",\"aliasName\":\"%s\",\"enabled\":1,\"localPort\":%d,\"targetHost\":\"%s\",\"targetPort\":%d,\"networkId\":%u,\"lastRxSec\":%u,\"lastTxSec\":%u,\"lastPingSec\":%u,\"secondsSinceRx\":%u,\"secondsSinceTx\":%u,\"resolvedIp\":\"%s\",\"enhanced\":%d,\"permitAll\":%d,\"permitTGs\":\"%s\",\"status\":\"%s\"}",
            first ? "" : ",",
            idx,
            json_escape(ob.alias_name).c_str(),
            ob.local_port,
            json_escape(ob.target_host).c_str(),
            ob.target_port,
            (unsigned)ob.network_id,
            (unsigned)ob.last_rx_sec,
            (unsigned)ob.last_tx_sec,
            (unsigned)ob.last_ping_sec,
            (unsigned)((now > ob.last_rx_sec) ? (now - ob.last_rx_sec) : 0),
            (unsigned)((now > ob.last_tx_sec) ? (now - ob.last_tx_sec) : 0),
            json_escape(resolved).c_str(),
            ob.enhanced ? 1 : 0,
            ob.permit_all ? 1 : 0,
            json_escape(ob.permit_tgs).c_str(),
            status);

        first = 0;
        if (left < 512) break;
    }

    appendf(&p, &left, "]");
    http_send(200, "OK", "application/json", out, io);
    free(out);
}

static void api_stat(struct io* io){
    char buf[65536]; int n = fetch_stat(buf, sizeof(buf));
    if (n < 0) { http_send(502, "Bad Gateway", "text/plain", "no /STAT reply", io); return; }
    http_send(200, "OK", "text/plain; charset=utf-8", buf, io);
}

static void handle_client(struct io* io){
    std::string req;
    if (!read_http_request(io, req)) { io_close(io); return; }
    char method[8]={0}, path[1024]={0};
    if (sscanf(req.c_str(), "%7s %1023s", method, path)!=2){ http_404(io); io_close(io); return; }
    size_t hdr_end = req.find("\r\n\r\n");
    const char* body = (hdr_end == std::string::npos) ? "" : req.c_str() + hdr_end + 4;
    if      (strncmp(path, "/api/login", 10)==0)    { api_login(io, method, body); io_close(io); return; }
    else if (strncmp(path, "/api/logout", 11)==0)   { api_logout(io, req, method); io_close(io); return; }
    else if (strncmp(path, "/api/profile", 12)==0)  { api_profile(io, req, method, body); io_close(io); return; }
    else if (strncmp(path, "/api/register", 13)==0) { api_register(io, method, body); io_close(io); return; }
    else if (strncmp(path, "/api/config", 11)==0)   { api_config(io); io_close(io); return; }
    else if (strncmp(path, "/api/active", 11)==0)   { api_active(io); io_close(io); return; }
    else if (strncmp(path, "/api/openbridge", 15)==0) { api_openbridge(io); io_close(io); return; }
    else if (strncmp(path, "/api/stat", 9)==0)      { api_stat(io); io_close(io); return; }
    else if (strncmp(path, "/api/log", 8)==0)       { api_log(io, path); io_close(io); return; }
    if (serve_static(io, path)) { io_close(io); return; }
    http_404(io); io_close(io); return;
}

static void io_set_client_timeouts(sock_t fd) {
#ifdef _WIN32
    DWORD tv = MONITOR_CLIENT_TIMEOUT_MS;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(tv));
#else
    struct timeval tv;
    tv.tv_sec = MONITOR_CLIENT_TIMEOUT_MS / 1000;
    tv.tv_usec = (MONITOR_CLIENT_TIMEOUT_MS % 1000) * 1000;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#endif
}

struct monitor_client_ctx { sock_t fd; };

#ifdef _WIN32
static DWORD WINAPI monitor_client_thread(LPVOID arg){
    monitor_client_ctx* ctx = (monitor_client_ctx*)arg;
    if (ctx) {
        struct io io = { ctx->fd };
        handle_client(&io);
        io_close(&io);
        delete ctx;
    }
    MON_CLIENT_LOCK();
    if (g_monitor_client_count > 0) --g_monitor_client_count;
    MON_CLIENT_UNLOCK();
    return 0;
}
#else
static void* monitor_client_thread(void* arg){
    monitor_client_ctx* ctx = (monitor_client_ctx*)arg;
    if (ctx) {
        struct io io = { ctx->fd };
        handle_client(&io);
        io_close(&io);
        delete ctx;
    }
    MON_CLIENT_LOCK();
    if (g_monitor_client_count > 0) --g_monitor_client_count;
    MON_CLIENT_UNLOCK();
    return NULL;
}
#endif

static void http_503_busy(struct io* io){ http_send(503, "Busy", "text/plain", "Server busy", io); }

static int monitor_try_start_client(sock_t c) {
    bool too_busy = false;
    MON_CLIENT_LOCK();
    if (g_monitor_client_count >= MONITOR_MAX_CLIENTS) too_busy = true;
    else ++g_monitor_client_count;
    MON_CLIENT_UNLOCK();

    if (too_busy) {
        struct io io = { c };
        http_503_busy(&io);
        io_close(&io);
        return -1;
    }

    monitor_client_ctx* ctx = new monitor_client_ctx;
    ctx->fd = c;
#ifdef _WIN32
    HANDLE th = CreateThread(NULL, 0, monitor_client_thread, ctx, 0, NULL);
    if (!th) {
        delete ctx;
        MON_CLIENT_LOCK();
        if (g_monitor_client_count > 0) --g_monitor_client_count;
        MON_CLIENT_UNLOCK();
        struct io io = { c };
        io_close(&io);
        return -1;
    }
    CloseHandle(th);
#else
    pthread_t th;
    if (pthread_create(&th, NULL, monitor_client_thread, ctx) != 0) {
        delete ctx;
        MON_CLIENT_LOCK();
        if (g_monitor_client_count > 0) --g_monitor_client_count;
        MON_CLIENT_UNLOCK();
        struct io io = { c };
        io_close(&io);
        return -1;
    }
    pthread_detach(th);
#endif
    return 0;
}

#ifdef _WIN32
static DWORD WINAPI monitor_thread(LPVOID lp){ (void)lp;
    WSADATA wsa; WSAStartup(MAKEWORD(2,2), &wsa);
#else
static void* monitor_thread(void* lp){ (void)lp;
#endif
    sock_t s = socket(AF_INET, SOCK_STREAM, 0);
#ifdef _WIN32
    if (s==SOCK_INVALID) return 0;
#else
    if (s==SOCK_INVALID) return NULL;
#endif
    g_listen = s;
    int one=1; setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char*)&one, sizeof(one));
#ifdef SO_KEEPALIVE
    setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, (char*)&one, sizeof(one));
#endif
#ifdef _WIN32
    DWORD atv = 1000;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&atv, sizeof(atv));
#else
    struct timeval atv; atv.tv_sec = 1; atv.tv_usec = 0;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &atv, sizeof(atv));
#endif
    struct sockaddr_in a; memset(&a,0,sizeof(a)); a.sin_family=AF_INET; a.sin_port=htons((uint16_t)G.port);
    const char* bind_ip = G.bind_ip[0] ? G.bind_ip : "127.0.0.1";
#ifdef _WIN32
    if (InetPtonA(AF_INET, bind_ip, &a.sin_addr) != 1) InetPtonA(AF_INET, "127.0.0.1", &a.sin_addr);
#else
    if (inet_pton(AF_INET, bind_ip, &a.sin_addr) != 1) inet_pton(AF_INET, "127.0.0.1", &a.sin_addr);
#endif
    if (bind(s, (struct sockaddr*)&a, sizeof(a))<0){ perror("monitor bind"); CLOSESOCK(s); g_listen = SOCK_INVALID;
#ifdef _WIN32
        return 0;
#else
        return NULL;
#endif
    }
    if (listen(s, MONITOR_LISTEN_BACKLOG) < 0) { perror("monitor listen"); CLOSESOCK(s); g_listen = SOCK_INVALID;
#ifdef _WIN32
        return 0;
#else
        return NULL;
#endif
    }
#ifdef _WIN32
    InterlockedExchange(&g_run, 1);
#else
    g_run = 1;
#endif
    while (
#ifdef _WIN32
        InterlockedCompareExchange(&g_run, g_run, g_run)
#else
        g_run
#endif
    ){
        struct sockaddr_in ca; socklen_t calen=sizeof(ca); sock_t c=accept(s,(struct sockaddr*)&ca,&calen);
        if (c==SOCK_INVALID) continue;
        io_set_client_timeouts(c);
        monitor_try_start_client(c);
    }
    if (g_listen == s) g_listen = SOCK_INVALID;
    CLOSESOCK(s);
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

void monitor_start(const MonitorConfig* cfg){
    mon_init_locks();
    if (cfg){
        if (cfg->bind_addr && *cfg->bind_addr) { strncpy(G.bind_ip, cfg->bind_addr, sizeof(G.bind_ip)-1); G.bind_ip[sizeof(G.bind_ip)-1]=0; }
        else strcpy(G.bind_ip, "127.0.0.1");
        if (cfg->port>0) G.port=cfg->port;
        if (cfg->doc_root && *cfg->doc_root) { strncpy(G.doc_root, cfg->doc_root, sizeof(G.doc_root)-1); G.doc_root[sizeof(G.doc_root)-1]=0; }
    }
#ifdef _WIN32
    if (InterlockedCompareExchange(&g_run, 0, 0)) return; g_thread = CreateThread(NULL,0,monitor_thread,NULL,0,NULL);
#else
    if (g_run) return; pthread_create(&g_thread,NULL,monitor_thread,NULL);
#endif
}

void monitor_stop(void){
#ifdef _WIN32
    InterlockedExchange(&g_run, 0); if (g_listen!=SOCK_INVALID){ closesocket(g_listen); g_listen=SOCK_INVALID; }
    if (g_thread){ WaitForSingleObject(g_thread, INFINITE); CloseHandle(g_thread); g_thread=NULL; }
#else
    g_run = 0; if (g_listen!=SOCK_INVALID){ close(g_listen); g_listen=SOCK_INVALID; }
    if (g_thread) { pthread_join(g_thread,NULL); g_thread = 0; }
#endif
}
#endif

void obp_init() { }

static void obp_fill_from_section(ob_peer& p, config_file& c, const char* sec, int fallback_local_port) {
    memset(&p, 0, sizeof(p));
    p.sock = -1;
    p.enabled = c.getint(sec, "Enable", 0) != 0;
    p.local_port = c.getint(sec, "Port", fallback_local_port);
    strcpy(p.target_host, c.getstring(sec, "TargetHost", "").c_str());
    strcpy(p.alias_name, c.getstring(sec, "AliasName", "").c_str());
    p.target_port   = c.getint(sec, "TargetPort", obp_remote_port);
    p.network_id    = c.getint(sec, "NetworkId", 0);
    strcpy(p.pass,   c.getstring(sec, "Passphrase", "").c_str());
    p.force_slot1   = c.getint(sec, "ForceSlot1", 0);
    p.permit_all    = c.getint(sec, "PermitAll", 1);
    strcpy(p.permit_tgs, c.getstring(sec, "PermitTGs", "").c_str());
    p.enhanced      = c.getint(sec, "EnhancedOBP", 0);
    p.relax_checks  = c.getint(sec, "RelaxChecks", 0);
    p.hblink_compat = c.getint(sec, "HBLinkCompat", 1) != 0;
    p.resolve_interval = c.getint(sec, "ResolveInterval", 600);
    p.last_resolve_sec = p.last_rx_sec = p.last_tx_sec = p.last_ping_sec = g_sec;
}

void obp_load_extra_from_config(config_file& c) {
    g_obp_peers.clear();
    const char* secs[] = {"OpenBridge1","OpenBridge2","OpenBridge3"};
    int fallback = obp_local_port;

    for (int i=0;i<3;i++) {
        ob_peer p;
        obp_fill_from_section(p, c, secs[i], fallback + i);
        if (p.enabled && p.target_host[0])
            g_obp_peers.push_back(p);
    }
}

static bool obp_resolve_now_one(ob_peer& p) {
    if (!p.enabled) return false;
    in_addr ip = {};
    if (!resolve_hostname_ipv4(p.target_host, &ip)) {
        log(NULL, "OpenBridge: DNS resolve failed for \"%s\"", p.target_host);
        return false;
    }
    memset(&p.addr,0,sizeof(p.addr));
    p.addr.sin_family = AF_INET;
    p.addr.sin_port   = htons(p.target_port);
#ifdef WIN32
    p.addr.sin_addr.S_un.S_addr = ip.S_un.S_addr;
#else
    p.addr.sin_addr.s_addr = ip.s_addr;
#endif
    p.last_resolve_sec = g_sec;
    logmsg (LOG_CYAN, 0, "OpenBridge: target %s -> %s:%d\n", p.target_host, my_inet_ntoa(p.addr.sin_addr).c_str(), p.target_port);
    return true;
}

void obp_show_all() {
	int i = 0;

    for (auto& p : g_obp_peers) {
		i++;
        if (!p.enabled)
			continue;

		logmsg (LOG_GREEN, 0, "\n- OpenBridge%d Config\n\n", i);
		logmsg (LOG_YELLOW, 0, "OB Local      : %d\n", p.local_port);
		logmsg (LOG_YELLOW, 0, "OB Remote     : %s:%d\n", p.target_host, p.target_port);
		logmsg (LOG_YELLOW, 0, "OB Alias      : %s\n", p.alias_name[0] ? p.alias_name : "(none)");
		logmsg (LOG_YELLOW, 0, "OB NetId      : %u\n", p.network_id);
		logmsg (LOG_YELLOW, 0, "OB Force TS1  : %s\n", p.force_slot1 ? "Yes" : "No");
		logmsg (LOG_YELLOW, 0, "OB Permit All : %s\n", p.permit_all ? "Yes" : "No");
		if (!p.permit_all)
			logmsg (LOG_YELLOW, 0, "OB Permit TGs : %s\n", p.permit_tgs);
		logmsg (LOG_YELLOW, 0, "OB Enhanced   : %s\n", p.enhanced ? "Yes" : "No");
		logmsg (LOG_YELLOW, 0, "OB Framing    : %s\n", p.hblink_compat ? "HBLink 53/73" : "Legacy 55/75");
		logmsg (LOG_YELLOW, 0, "OB Relax      : %s\n", p.relax_checks ? "Yes" : "No");
		logmsg (LOG_YELLOW, 0, "OB Resolve    : %d seconds\n", p.resolve_interval);
    }
}

void obp_open_all() {
    for (auto& p : g_obp_peers) {
        if (!p.enabled) continue;
        p.sock = open_udp(p.local_port);
        if (p.sock == -1) {
            log(NULL, "OpenBridge: peer @%s:%d failed to open local UDP (%d)", p.target_host, p.target_port, GetInetError());
            p.enabled = false;
            continue;
        }
        obp_resolve_now_one(p);
    }
}

bool resolve_hostname_ipv4(PCSTR host, in_addr* out)
{
    if (!host || !*host) return false;

#ifdef WIN32
    unsigned long a = inet_addr(host);
    if (a != INADDR_NONE) { out->S_un.S_addr = a; return true; }
#else
    in_addr_t a = inet_addr(host);
    if (a != INADDR_NONE) { out->s_addr = a; return true; }
#endif

    addrinfo hints; memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    addrinfo* res = NULL;
    int rc = getaddrinfo(host, NULL, &hints, &res);
    if (rc != 0 || !res) return false;

    sockaddr_in* sin = (sockaddr_in*)res->ai_addr;
#ifdef WIN32
    out->S_un.S_addr = sin->sin_addr.S_un.S_addr;
#else
    out->s_addr = sin->sin_addr.s_addr;
#endif
    freeaddrinfo(res);
    return true;
}

static int obp_hmac_sha1(const void* msg, size_t len, const char* key, byte out20[20]) {
#ifdef USE_OPENSSL
    unsigned int maclen = 0;
    const byte* mac = HMAC(EVP_sha1(), key, (int)strlen(key),
                           (const unsigned char*)msg, len, NULL, &maclen);
    if (!mac || maclen < 20) return -1;
    memcpy(out20, mac, 20);
    return 0;
#else
    return -1;
#endif
}

static bool ct_memeq(const void* a, const void* b, size_t n) {
    const byte* pa = (const byte*)a;
    const byte* pb = (const byte*)b;
    byte diff = 0;
    for (size_t i = 0; i < n; ++i)
        diff |= (byte)(pa[i] ^ pb[i]);
    return diff == 0;
}

static const size_t HB_OBP_DMRD_BLOCK_LEN = 49;
static const size_t HB_OBP_DMRD_TOTAL_NO_HMAC = 53;
static const size_t HB_OBP_DMRD_TOTAL_WITH_HMAC = HB_OBP_DMRD_TOTAL_NO_HMAC + 20;
static const size_t LEGACY_OBP_DMRD_BLOCK_LEN = 51;
static const size_t LEGACY_OBP_DMRD_TOTAL_NO_HMAC = 55;
static const size_t LEGACY_OBP_DMRD_TOTAL_WITH_HMAC = LEGACY_OBP_DMRD_TOTAL_NO_HMAC + 20;
static const size_t HS_DMRD_EXTRA_LEN = 2;
static const size_t HS_DMRD_TOTAL_NO_METRICS = 55;
static const size_t HS_DMRD_TOTAL_WITH_METRICS = HS_DMRD_TOTAL_NO_METRICS + HS_DMRD_EXTRA_LEN;

static bool obp_verify_dmrd_hmac(const byte* pkt, size_t n, size_t no_hmac_len, const char* key) {
    if (n != no_hmac_len + 20) return false;
    byte mac[20];
    if (obp_hmac_sha1(pkt, no_hmac_len, key, mac) != 0) return false;
    return ct_memeq(mac, pkt + no_hmac_len, 20);
}

static int obp_append_hmac_dmrd(std::vector<byte>& frame, size_t no_hmac_len, const char* key) {
    byte mac[20];
    if (frame.size() != no_hmac_len) return -1;
    if (obp_hmac_sha1(frame.data(), no_hmac_len, key, mac) != 0) return -1;
    frame.insert(frame.end(), mac, mac + 20);
    return 0;
}

static int obp_make_bcka(byte out[24], const char* key) {
    memcpy(out, "BCKA", 4);
    byte mac[20];
     if (obp_hmac_sha1(out, 4, key, mac) != 0) return -1;
    memcpy(out+4, mac, 20);
    return 0;
}

static bool tg_in_list(dword tg, PCSTR csv) {
    if (!csv || !*csv) return false;

    char buf[512]; strncpy(buf, csv, sizeof(buf)-1); buf[sizeof(buf)-1]=0;
    char* p = buf; 
    while (p && *p) {
        while (*p==' '||*p==',') p++;
        char* q = p; while (*q && *q!=',') q++;
        if (*q) *q++ = 0;
        if (atoi(p) == (int)tg) return true;
        p = q;
    }
    return false;
}

static void obp_housekeeping_one(ob_peer& p) {
    if (!p.enabled || p.sock == -1) return;
    if (p.resolve_interval > 0 && g_sec - p.last_resolve_sec >= (dword)p.resolve_interval) {
        obp_resolve_now_one(p);
    }
    if (g_sec - p.last_ping_sec >= 20) {
        if (p.enhanced) {
            byte bcka[24];
            if (obp_make_bcka(bcka, p.pass) != 0) {
                log(NULL, "OpenBridge: unable to build BCKA HMAC - not sending");
                return;
            }
            sendpacket_sock(p.sock, p.addr, bcka, 24);
        } else {
            static const byte bcka[4] = {'B','C','K','A'};
            sendpacket_sock(p.sock, p.addr, bcka, 4);
        }
        p.last_ping_sec = g_sec;
        p.last_tx_sec = g_sec;
    }
}

void obp_housekeeping_all() {
    for (auto& p : g_obp_peers)
        obp_housekeeping_one(p);
}

void obp_forward_dmrd(const byte* pk, int sz, int origin_tag) {
    byte clean[HS_DMRD_TOTAL_NO_METRICS];

    if (!pk || sz <= 0 || memcmp(pk, "DMRD", 4) != 0)
        return;

    if ((size_t)sz == HS_DMRD_TOTAL_WITH_METRICS) {
        memcpy(clean, pk, HS_DMRD_TOTAL_NO_METRICS);
        pk = clean;
        sz = (int)HS_DMRD_TOTAL_NO_METRICS;
    }

    if ((size_t)sz != HS_DMRD_TOTAL_NO_METRICS)
        return;
 
    auto send_one = [&](ob_peer& P){
        if (!P.enabled || P.sock == -1) return;
        dword tg = get3(pk + 8);
        if (!(P.permit_all || tg_in_list(tg, P.permit_tgs))) return;

        const size_t tx_no_hmac_len = P.hblink_compat ? HB_OBP_DMRD_TOTAL_NO_HMAC : LEGACY_OBP_DMRD_TOTAL_NO_HMAC;
        const size_t tx_block_len = P.hblink_compat ? HB_OBP_DMRD_BLOCK_LEN : LEGACY_OBP_DMRD_BLOCK_LEN;
        (void)tx_block_len;

        std::vector<byte> frame(pk, pk + tx_no_hmac_len);
        frame[15] &= 0x7F;
        frame[11] = (P.network_id >> 24) & 0xFF;
        frame[12] = (P.network_id >> 16) & 0xFF;
        frame[13] = (P.network_id >>  8) & 0xFF;
        frame[14] = (P.network_id      ) & 0xFF;

        dword sid = ((dword)frame[16] << 24) | ((dword)frame[17] << 16) | ((dword)frame[18] << 8) | frame[19];
        if (origin_tag == 1) {
            for (int i=0;i<256;i++) if (P.stream_ring[i]==sid) return;
        }
        P.stream_ring[P.ring_ix++] = sid;

        if (P.enhanced) {
            if (obp_append_hmac_dmrd(frame, tx_no_hmac_len, P.pass) != 0) {
                log(NULL, "OpenBridge: HMAC unavailable (build w/ USE_OPENSSL) - not sending");
                return;
            }
        }
        sendpacket_sock(P.sock, P.addr, frame.data(), (int)frame.size());
        P.last_tx_sec = g_sec;
    };

    for (auto& p : g_obp_peers) send_one(p);
}


static void obp_fanout_to_locals(byte* pk, int pksize) {
    dword tg = get3(pk + 8);
    talkgroup *g = findgroup(tg, false);

    if (g) {
        slot const *dest = g->subscribers;
        while (dest) {
            if (dest->node && dest->node->bAuth && getinaddr(dest->node->addr)) {
                if (SLOT(dest->slotid)) pk[15] |= 0x80; else pk[15] &= 0x7F;
                sendpacket(dest->node->addr, pk, pksize);
            }
            dest = dest->next;
        }
    }

    std::map<dword, std::vector<slot*> >::iterator sit = g_static_subscribers.find(tg);
    if (sit != g_static_subscribers.end()) {
        std::vector<slot*>& static_slots = (*sit).second;
        for (size_t i = 0; i < static_slots.size(); ++i) {
            slot* sdest = static_slots[i];
            if (!sdest)
                continue;
            if (sdest->tg == tg)
                continue;
            if (!sdest->node || !sdest->node->bAuth || !getinaddr(sdest->node->addr))
                continue;

            if (SLOT(sdest->slotid)) pk[15] |= 0x80;
            else pk[15] &= 0x7F;

            sendpacket(sdest->node->addr, pk, pksize);
        }
    }

    if (g_scanner) {
        slot const *scan = g_scanner->subscribers;
        while (scan) {
            if (scan->node && scan->node->bAuth && getinaddr(scan->node->addr)) {
                if (SLOT(scan->slotid)) pk[15] |= 0x80;
                else pk[15] &= 0x7F;
                sendpacket(scan->node->addr, pk, pksize);
            }
            scan = scan->next;
        }
    }
}

static void obp_handle_rx_one(ob_peer& P) {
    if (!P.enabled || P.sock == -1) return;

    while (select_rx(P.sock, 0)) {
        byte buf[1000];
        sockaddr_in r; socklen_t rl = sizeof(r);
        int sz = recvfrom(P.sock, (char*)buf, sizeof(buf), 0, (sockaddr*)&r, &rl);
        if (sz <= 0) break;

        if (sz >= 4 && memcmp(buf, "BCKA", 4) == 0) {
            if (P.enhanced) {
                byte mac[20];
                if (sz != 24 || obp_hmac_sha1(buf, 4, P.pass, mac) != 0 || !ct_memeq(mac, buf + 4, 20)) {
                    if (!P.relax_checks) { log(&r, "OpenBridge: bad BCKA HMAC"); continue; }
                }
            }
            P.last_rx_sec = g_sec;
            continue;
        }

        size_t rx_no_hmac_len = 0;
        bool rx_has_hmac = false;

        if ((size_t)sz == HB_OBP_DMRD_TOTAL_NO_HMAC) {
            rx_no_hmac_len = HB_OBP_DMRD_TOTAL_NO_HMAC;
        } else if ((size_t)sz == HB_OBP_DMRD_TOTAL_WITH_HMAC) {
            rx_no_hmac_len = HB_OBP_DMRD_TOTAL_NO_HMAC;
            rx_has_hmac = true;
        } else if ((size_t)sz == LEGACY_OBP_DMRD_TOTAL_NO_HMAC) {
            rx_no_hmac_len = LEGACY_OBP_DMRD_TOTAL_NO_HMAC;
        } else if ((size_t)sz == LEGACY_OBP_DMRD_TOTAL_WITH_HMAC) {
            rx_no_hmac_len = LEGACY_OBP_DMRD_TOTAL_NO_HMAC;
            rx_has_hmac = true;
        }

        if (rx_no_hmac_len) {
            if (memcmp(buf, "DMRD", 4) != 0) continue;

#ifdef USE_SQLITE3
			dword const radioid = get3(buf + 5);
			dword const tg = get3(buf + 8);
			dword const nodeid = get4(buf + 11);
			int   const flags = buf[15];
			dword const slotid = SLOTID(nodeid, flags & 0x80);

			bool const bStartStream = (flags & 0x23) == 0x21;
			bool const bEndStream = (flags & 0x23) == 0x22;

			char date_now[100];
			time_t now = time(0);
			strftime(date_now, sizeof(date_now), "%d.%m.%Y / %H:%M:%S", localtime(&now));

			char keybuf[64];
			sprintf(keybuf, "%u:%u:%u:%u", radioid, tg, SLOT(slotid)+1, nodeid);
			std::string key(keybuf);

#ifdef VS12
			if ((radioid != obp_radioid_old) || (tg != obp_tg_old) || (slotid != obp_slotid_old) || (nodeid != obp_nodeid_old)) {
#else
			if ((radioid != obp_radioid_old) or (tg != obp_tg_old) or (slotid != obp_slotid_old) or (nodeid != obp_nodeid_old)) {
#endif
				sprintf(sql,
					"INSERT INTO LOG (DATE,RADIO,TG,TIME,SLOT,NODE,ACTIVE,CONNECT) "
					"VALUES ('%s',%u,%u,0,%u,%u,1,1);",
					date_now, radioid, tg, (unsigned)(SLOT(slotid)+1), nodeid);
				rc = sqlite3_exec(db, sql, 0, 0, &zErrMsg);
				if (rc != SQLITE_OK) sqlite3_free(zErrMsg);

				g_obp_timers[key] = 0;
			} else {
				int &timer = g_obp_timers[key];
				sprintf(sql,
					"UPDATE LOG set DATE='%s', ACTIVE=1, TIME=%d where RADIO=%u and TG=%u; SELECT * from LOG",
					date_now, timer / 15, radioid, tg);
				rc = sqlite3_exec(db, sql, 0, 0, &zErrMsg);
				if (rc != SQLITE_OK) sqlite3_free(zErrMsg);
				timer++;
			}

#ifdef HAVE_HTTPMODE
			monitor_note_event((int)radioid, (int)tg, MON_SRC_OBP, (tg==g_aprs_tg ? 1:0), 0);
#endif

			obp_radioid_old = radioid;
			obp_tg_old      = tg;
			obp_slotid_old  = slotid;
			obp_nodeid_old  = nodeid;

			if (bEndStream) {
				int &timer = g_obp_timers[key];
				sprintf(sql,
					"UPDATE LOG set ACTIVE=0, TIME=%d where RADIO=%u and TG=%u; SELECT * from LOG",
					timer / 15, radioid, tg);
				rc = sqlite3_exec(db, sql, 0, 0, &zErrMsg);
				if (rc != SQLITE_OK) sqlite3_free(zErrMsg);
				timer = 0;
			}
#endif

            const byte* frame = buf;

            if (P.enhanced) {
                if (rx_has_hmac) {
                    if (!obp_verify_dmrd_hmac(frame, (size_t)sz, rx_no_hmac_len, P.pass)) {
                        if (!P.relax_checks) { log(&r, "OpenBridge: DMRD HMAC fail"); continue; }
                    }
                } else {
                    if (!P.relax_checks) continue;
                }
            }

            const byte* block = frame + 4;
            dword dtg = ((dword)block[4] << 16) | ((dword)block[5] << 8) | block[6];
            if (!(P.permit_all || tg_in_list(dtg, P.permit_tgs))) continue;

            byte out[HS_DMRD_TOTAL_NO_METRICS];
            memset(out, 0, sizeof(out));
            memcpy(out, frame, rx_no_hmac_len);
            out[15] &= 0x7F;

            obp_fanout_to_locals(out, (int)HS_DMRD_TOTAL_NO_METRICS);
            P.last_rx_sec = g_sec;
        }
    }
}

void obp_handle_rx_all() {
    for (auto& p : g_obp_peers)
		obp_handle_rx_one(p);
}

void process_config_file()
{
	config_file c;

	if (c.load ("dmr.conf")) {
		g_debug = c.getint("General", "Debug", g_debug);
		strcpy (g_host, c.getstring ("Server","Host",g_host).c_str());
		g_udp_port = c.getint ("Server","Port", g_udp_port);
		strcpy (g_password, c.getstring ("Server","Password",g_password).c_str());
		g_housekeeping_minutes = c.getint ("Server","Housekeeping", g_housekeeping_minutes);
		g_keep_nodes_alive = c.getint("Homebrew", "KeepNodesAlive", g_keep_nodes_alive);
		g_node_timeout = c.getint("Homebrew", "NodeTimeout", g_node_timeout);
		g_relax_ip_change = c.getint("Homebrew", "RelaxIPChange", g_relax_ip_change);
#ifdef HAVE_HTTPMODE
		g_monitor_enabled = c.getint ("Monitor","Enable", g_monitor_enabled);
		g_monitor_port = c.getint ("Monitor","Port", g_monitor_port);
		strcpy (g_monitor_root, c.getstring ("Monitor","Root",g_monitor_root).c_str());
#endif
#ifdef USE_SQLITE3
		strcpy (g_log, c.getstring ("File","Log",g_log).c_str());
#endif
		strcpy (g_talkgroup, c.getstring ("File","Talkgroup",g_talkgroup).c_str());
		strcpy (g_banned, c.getstring ("File","Banned",g_banned).c_str());
		g_scanner_tg = c.getint("DMR", "Scanner", g_scanner_tg);
		g_parrot_tg = c.getint("DMR", "Parrot", g_parrot_tg);
		g_aprs_tg = c.getint("DMR", "APRS", g_aprs_tg);

		g_auth_enabled = c.getint("Auth", "Enable", g_auth_enabled);
		g_auth_reload_secs = c.getint("Auth", "Reload", g_auth_reload_secs);
		g_auth_unknown_default = c.getint("Auth", "UnknownPolicy", g_auth_unknown_default);
		strcpy (g_auth_file, c.getstring ("File","Auth",g_auth_file).c_str());
		strcpy (g_dmrids_file, c.getstring ("File","DMRIds",g_dmrids_file).c_str());

		obp_load_extra_from_config(c);

#ifdef HAVE_APRS
		g_aprs.enabled = c.getint("APRS","Enable",g_aprs.enabled);
		strcpy (g_aprs.server_host, c.getstring ("APRS","Server",g_aprs.server_host).c_str());
		g_aprs.server_port = (dword)c.getint("APRS","Port",g_aprs.server_port);
		strcpy (g_aprs.callsign, c.getstring ("APRS","Callsign",g_aprs.callsign).c_str());
		strcpy (g_aprs.passcode, c.getstring ("APRS","Passcode",g_aprs.passcode).c_str());
		strcpy (g_aprs.filter, c.getstring ("APRS","Filter",g_aprs.filter).c_str());
		g_aprs.keepalive_secs = c.getint("APRS","Keepalive",g_aprs.keepalive_secs);
		g_aprs.reconnect_secs = c.getint("APRS","Reconnect",g_aprs.reconnect_secs);

		std::string mapfile = c.getstring("APRS","IdMap","");
		if (!mapfile.empty()) aprs_load_idmap(mapfile.c_str());
#endif

#ifdef HAVE_SMS
		g_sms.enabled = c.getint("SMS","Enable",0) != 0;
		strncpy(g_sms.udphost, c.getstring("SMS","UDPHost","127.0.0.1").c_str(), sizeof(g_sms.udphost)-1);
		g_sms.udpport = c.getint("SMS","UDPPort",5555);
		g_sms.allow_private = c.getint("SMS","AllowPrivate",1) != 0;
		g_sms.permit_all = c.getint("SMS","PermitAll",0) != 0;
		strncpy(g_sms.permit_tgs, c.getstring("SMS","PermitTGs","").c_str(), sizeof(g_sms.permit_tgs)-1);
		g_sms.max_frames = c.getint("SMS","MaxFrames",30);
		g_sms.max_seconds = c.getint("SMS","MaxSeconds",5);
#endif
	}

	logmsg (LOG_GREEN, 0, "\n- Server Config\n\n");
	logmsg (LOG_YELLOW, 0, "Debug         : %s\n", g_debug ? "Yes" : "No");
	logmsg (LOG_YELLOW, 0, "Port          : %d\n", g_udp_port);
	logmsg (LOG_YELLOW, 0, "Password      : %s\n", g_password);
#ifdef USE_SQLITE3
	logmsg (LOG_YELLOW, 0, "Log           : %s\n", g_log);
#endif
	logmsg (LOG_YELLOW, 0, "Talkgroup     : %s\n", g_talkgroup);
	logmsg (LOG_YELLOW, 0, "Banned        : %s\n", g_banned);
	logmsg (LOG_YELLOW, 0, "Scanner TG    : %d\n", g_scanner_tg);
	logmsg (LOG_YELLOW, 0, "Parrot TG     : %d\n", g_parrot_tg);
	logmsg (LOG_YELLOW, 0, "APRS TG       : %d\n", g_aprs_tg);

#ifdef HAVE_HTTPMODE
	if (g_monitor_enabled) {
		logmsg (LOG_GREEN, 0, "\n- Web Config\n\n");
		logmsg (LOG_YELLOW, 0, "Port          : %d\n", g_monitor_port);
		logmsg (LOG_YELLOW, 0, "Root          : %s\n", g_monitor_root);
	}
#endif

	if (g_auth_enabled) {
		logmsg (LOG_GREEN, 0, "\n- Auth Config\n\n");
		logmsg (LOG_YELLOW, 0, "File          : %s\n", g_auth_file);
		logmsg (LOG_YELLOW, 0, "DMRIds        : %s\n", g_dmrids_file);
		logmsg (LOG_YELLOW, 0, "Reload        : %d\n", g_auth_reload_secs);
		logmsg (LOG_YELLOW, 0, "Policy        : %s\n", g_auth_unknown_default ? "Default" : "Deny");
	}

	obp_show_all();

#ifdef HAVE_APRS
	if (g_aprs.enabled) {
		logmsg (LOG_GREEN, 0, "\n- APRS Config\n\n");
		logmsg (LOG_YELLOW, 0, "Server        : %s:%d\n", g_aprs.server_host, g_aprs.server_port);
		logmsg (LOG_YELLOW, 0, "Login         : %s / %s\n", g_aprs.callsign, g_aprs.passcode);
		logmsg (LOG_YELLOW, 0, "Filter        : %s\n", g_aprs.filter);
		logmsg (LOG_YELLOW, 0, "Keepalive     : %d seconds\n", g_aprs.keepalive_secs);
		logmsg (LOG_YELLOW, 0, "Reconnect     : %d seconds\n", g_aprs.reconnect_secs);
	}
#endif

#ifdef HAVE_SMS
	if (g_sms.enabled) {
		logmsg (LOG_GREEN, 0, "\n- SMS Config\n\n");
		logmsg (LOG_YELLOW, 0, "Server        : %s:%d\n", g_sms.udphost, g_sms.udpport);
		logmsg (LOG_YELLOW, 0, "Allow Private : %s\n", g_sms.allow_private ? "Yes" : "No");
		logmsg (LOG_YELLOW, 0, "Permit All    : %s\n", g_sms.permit_all ? "Yes" : "No");
		if (!g_sms.permit_all)
			logmsg (LOG_YELLOW, 0, "Permit TGs    : %s\n", g_sms.permit_tgs);
		logmsg (LOG_YELLOW, 0, "MaxFrames     : %d\n", g_sms.max_frames);
		logmsg (LOG_YELLOW, 0, "MaxSeconds    : %d seconds\n", g_sms.max_seconds);
	}
#endif

	logmsg (LOG_GREEN, 0, "\n- Start Server\n\n");
	logmsg (LOG_YELLOW, 0, "Node size %d / housekeeping %d minutes\n", sizeof(node), g_housekeeping_minutes);
	logmsg (LOG_YELLOW, 0, "Homebrew: Keep=%d, Timeout=%d, RelaxIPChange=%d\n", g_keep_nodes_alive, g_node_timeout, g_relax_ip_change);

}

#ifdef VS12
int main(int argc, char **argv)
#else
int main(int argc, char **argv)
#endif
{
	int active = 0, dropped_nodes = 0, radios = 0, dropped_radios = 0;
	dword g_last_housekeeping_sec = 0;
	dword seq = 1;
	dword t = g_sec;
	dword starttick = g_tick;
    FILE *fp_tg, *fp_ban;
    char line[2048];
	int i = 0;

	init_process();

	logmsg (LOG_CYAN,   0, "################################################################################################################\n\n");
	logmsg (LOG_RED,    0, " ####   #####   ######  ##    #  ####    ##   ##  #####             #####  ######  #####   #   #  ######  ##### \n");
	logmsg (LOG_GREEN,  0, "#    #  #    #  #       # #   #  #   #   # # # #  #    #           #       #       #    #  #   #  #       #    #\n");
	logmsg (LOG_YELLOW, 0, "#    #  #    #  #       # #   #  #    #  # # # #  #    #           #       #       #    #  #   #  #       #    #\n");
	logmsg (LOG_BLUE,   0, "#    #  #####   #####   #  #  #  #    #  #  #  #  #####    #####    ####   #####   #####   #   #  #####   ##### \n");
	logmsg (LOG_YELLOW, 0, "#    #  #       #       #   # #  #    #  #  #  #  ###                   #  #       ###      # #   #       ###   \n");
	logmsg (LOG_GREEN,  0, "#    #  #       #       #   # #  #   #   #     #  #  #                  #  #       #  #     # #   #       #  #  \n");
	logmsg (LOG_RED,    0, " ####   #       ######  #    ##  ####    #     #  #   #            #####   ######  #   #     #    ######  #   # \n\n");
	logmsg (LOG_CYAN,   0, "################################################################################################################\n\n");

	logmsg (LOG_BLUE, 0, "Version: v%d.%s | Release Date: %s\n\n", DMR_VERSION, DMR_RELEASE, __DATE__ " " __TIME__);

	logmsg (LOG_GREEN, 0, "- Server Module\n\n");
	logmsg (LOG_PURPLE, 0, "OpenBridge v%d.%s\n", OB_VERSION, OB_RELEASE);
#ifdef HAVE_APRS
	logmsg (LOG_PURPLE, 0, "APRS v%d.%s\n", APRS_VERSION, APRS_RELEASE);
#endif
#ifdef HAVE_SMS
	logmsg (LOG_PURPLE, 0, "SMS v%d.%s\n", SMS_VERSION, SMS_RELEASE);
#endif

	if (IsOptionPresent(argc,argv,"--help"))
		return 0;

	srand (time(NULL));

	strcpy (g_host, "fm-funkgateway.de");
	strcpy (g_password, "passw0rd");
#ifdef USE_SQLITE3
	strcpy (g_log, "log.sqlite");
#endif
	strcpy (g_talkgroup, "talkgroup.dat");
	strcpy (g_banned, "banned.dat");
	strcpy (g_auth_file, "auth_users.csv");
	strcpy (g_dmrids_file, "DMRIds.dat");

	process_config_file();
#ifdef HAVE_APRS
	aprs_init_from_config();
#endif
	auth_load_initial();
	obp_open_all();
	printf ("\n");

#ifdef USE_SQLITE3
	rc = sqlite3_open(g_log, &db);
	if(rc)
		return 0;
	else {
		sprintf(sql, "CREATE TABLE LOG(ID INTEGER PRIMARY KEY AUTOINCREMENT, DATE TEXT NOT NULL, RADIO INT NOT NULL, TG INT NOT NULL, TIME INT NOT NULL, SLOT INT NOT NULL, NODE INT NOT NULL, ACTIVE INT NOT NULL, CONNECT INT NOT NULL);");
		rc = sqlite3_exec(db, sql, 0, 0, &zErrMsg);   
		if( rc != SQLITE_OK )
			sqlite3_free(zErrMsg);
		else
			rc = sqlite3_open(g_log, &db);
	}

	if (IsOptionPresent(argc,argv,"--create")) {
		sprintf(sql, "CREATE TABLE LOG(ID INTEGER PRIMARY KEY AUTOINCREMENT, DATE TEXT NOT NULL, RADIO INT NOT NULL, TG INT NOT NULL, TIME INT NOT NULL, SLOT INT NOT NULL, NODE INT NOT NULL, ACTIVE INT NOT NULL, CONNECT INT NOT NULL);");
		rc = sqlite3_exec(db, sql, 0, 0, &zErrMsg);   
		if( rc != SQLITE_OK )
			sqlite3_free(zErrMsg);
	}

	sprintf(sql, "UPDATE LOG set ACTIVE=0, CONNECT=0; SELECT * from LOG");
	rc = sqlite3_exec(db, sql, 0, 0, &zErrMsg);
	if( rc != SQLITE_OK )
		sqlite3_free(zErrMsg);
#endif

	if (IsOptionPresent (argc, argv, "-d"))
		g_debug = true;

	if (IsOptionPresent(argc,argv,"-s"))
		return !show_running_status () ? 0 : 1;

	g_scanner = findgroup (g_scanner_tg, true);

    fp_tg = fopen (g_talkgroup, "r");
    if (fp_tg == NULL)
        exit (EXIT_FAILURE);
    while (fgets (line, sizeof (line), fp_tg))
		findgroup (atoi (line), true);
    fclose (fp_tg);

	logmsg (LOG_GREEN, 0, "- Check Banned\n\n", u_banned[i]);
	fp_ban = fopen (g_banned, "r");
	if (fp_ban == NULL)
		exit (EXIT_FAILURE);
	while (fgets (line, sizeof (line), fp_ban)) {
		if (line)
			u_banned[i] = atoi (line);
		else
			u_banned[i] = 0;
		logmsg (LOG_RED, 0, "%d ", u_banned[i]);
		i++;
	}
	fclose (fp_ban);
	puts ("\n");

	if ((g_sock = open_udp(g_udp_port)) == -1) {
		log (NULL, "Failed to open UDP port (%d)\n", GetInetError());
		return 1;
	}

#ifdef HAVE_HTTPMODE
	if (g_monitor_enabled) {
		MonitorConfig mc = {g_host[0] ? g_host : "127.0.0.1", g_monitor_port, g_monitor_root};
		monitor_start(&mc);
	}
#endif

	pthread_t th;
	pthread_create (&th, NULL, time_thread_proc, NULL);

	for (;;) {
		if (select_rx(g_sock, 1)) {
			byte buf[1000];
			sockaddr_in addr;
			socklen_t addrlen = sizeof(addr);
			int sz = recvfrom (g_sock, (char*) buf, sizeof(buf), 0, (sockaddr*)&addr, &addrlen);

			if (sz > 0) {
				char ip[50];
				strcpy (ip, my_inet_ntoa (addr.sin_addr).c_str());

				if (g_debug) {
					char temp[100];
					sprintf (temp, "RX%u", seq++);
					show_packet (temp, ip, buf, sz);
				}

				handle_rx (addr, buf, sz);
			} else if (sz < 1) {
				int err = GetInetError ();
				log (&addr, "recvfrom error %d\n", err);
				Sleep (50);
			}
		}

        obp_handle_rx_all();
        obp_housekeeping_all();
#ifdef HAVE_APRS
		aprs_housekeeping();
#endif
		auth_housekeeping();

		if (g_sec - g_last_housekeeping_sec >= g_housekeeping_minutes * 60) {
			log (NULL, "Housekeeping, tick %u\n", starttick);

			for (int ix=0; ix < HIGH_DMRID - LOW_DMRID; ix++) {
				for (int essid=0; g_node_index[ix] && essid < 100; essid++) {
					node const *n = g_node_index[ix]->sub[essid];
					if (n) {
						if (g_keep_nodes_alive && n->bAuth)
							active++;
						else if ((int)(g_sec - n->hitsec) >= g_node_timeout) {
							dropped_nodes++;
							delete_node (n->nodeid);
						} else
							active++;
					}
				}
			}
	
			log (NULL, "Done - %u secs, %u active nodes, %u dropped nodes, %d radios, %d dropped radios, %u ticks\n", g_sec, active, dropped_nodes, radios, dropped_radios, g_tick - starttick);

			g_last_housekeeping_sec = g_sec;
		}
	}

#ifdef HAVE_HTTPMODE
	if (g_monitor_enabled)
		monitor_stop ();
#endif

#ifdef USE_SQLITE3
	sqlite3_close(db);
#endif

	return 0;
}
