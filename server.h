#ifndef _SERVER_H
#define _SERVER_H

#define _CRT_SECURE_NO_WARNINGS

#define DMR_VERSION 1
#define DMR_RELEASE "0.1.0"
#define OB_VERSION 0
#define OB_RELEASE "3 Beta"
#ifdef HAVE_APRS
#define APRS_VERSION 0
#define APRS_RELEASE "2 Alpha"
#endif
#ifdef HAVE_SMS
#define SMS_VERSION 0
#define SMS_RELEASE "2 Alpha"
#endif

#ifndef WIN32
#define LINUX
#endif

#ifdef WIN32
#pragma warning (disable : 4786)
#pragma warning (disable : 4018)
#pragma warning (disable : 4244)	
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>

#include "stdio.h"
#include "stdlib.h"
#include "time.h"
#include "errno.h"
#include "assert.h"
#include <iostream>
#include <iterator>
#include <string>
#include <vector>
#include <array>
#include <map>
#include <cstdint>
#include <cstdint>

#ifdef WIN32
typedef unsigned char byte;
typedef byte BYTE;
typedef unsigned short word;
typedef word WORD;
typedef unsigned long dword;
typedef dword DWORD;
#else
typedef uint8_t  byte;
typedef uint8_t  BYTE;
typedef uint16_t word;
typedef uint16_t WORD;
typedef uint32_t dword;
typedef uint32_t DWORD;
#endif

typedef char const *PCSTR;

#ifdef WIN32
#include "winsock2.h"
#include "ws2tcpip.h"
#include "process.h"
#include <io.h>

#ifdef VS12
#include "pthread.h"
#endif

typedef unsigned __int64 u64;

#define getinaddr(ADDR) ((ADDR).sin_addr.S_un.S_addr)
#define PTHREAD_PROC(NAME) unsigned _stdcall NAME (void *threadcookie)
#ifdef MINGW
	typedef HANDLE pthread_t;
	typedef int pthread_attr_t;
#endif
typedef SOCKET sock_t;
typedef unsigned (_stdcall *PTHREADPROC)(void *);
typedef int socklen_t;
int pthread_create (pthread_t *, const pthread_attr_t *, PTHREADPROC, void *);
#define GetInetError() ((int)GetLastError())
#define SetInetError(E) (SetLastError(E))
#define CLOSESOCKET closesocket
#define CLOSESOCK(s) closesocket(s)
#define SOCK_INVALID INVALID_SOCKET

#ifdef VS12
#define eq(A,B) _stricmp((A),(B))==0
#else
#define eq(A,B) stricmp((A),(B))==0
#endif

#ifdef VS12
#define snprintf(buf,len, format,...) _snprintf_s(buf, len,len, format, __VA_ARGS__)
#endif
#else

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <memory.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netdb.h>

typedef int sock_t;
typedef unsigned long long u64;

dword GetTickCount();

#define getinaddr(ADDR) ((ADDR).sin_addr.s_addr)
#define PTHREAD_PROC(NAME) void * NAME (void *threadcookie)
#define GetInetError() ((int)errno)
#define SetInetError(E) (errno = (E))
#define CLOSESOCKET close
#define CLOSESOCK(s) close(s)
#define SOCK_INVALID (-1)

#define Sleep(MS)	do { \
						if(MS) \
							usleep((MS) * 1000); \
						else \
							sched_yield();} \
					while(0)

#define eq(A,B) (strcasecmp((A),(B))==0)
#endif

#ifdef USE_SQLITE3
#include <sqlite3.h>
#endif

#ifdef USE_OPENSSL
#include <openssl/hmac.h>
#endif

enum LogColorLevel {
	LOG_RED,
	LOG_GREEN,
	LOG_YELLOW,
	LOG_BLUE,
	LOG_PURPLE,
	LOG_CYAN,
	LOG_WHITE
};

#define DMRD_TOTAL_NO_HMAC   55
#define DMRD_BLOCK_LEN       51
#define DMRD_TOTAL_WITH_HMAC 75

#define LOW_DMRID 1000000
#define HIGH_DMRID 8000000
#define MAX_TALK_GROUPS 100000000
#define UNSUBSCRIBE_ALL_TG 4000
#define MAX_BANNED_SIZE 16384

#define MAX_PASSWORD_SIZE 128
#define DEFAULT_HOUSEKEEPING_MINUTES 1
#define DEFAULT_PORT 62031

#define inrange(V,L,H) ((V) >= (L) && (V) <= (H))

#define NODEID(SLOTID) ((SLOTID) & 0x7FFFFFFF)
#define SLOTID(NODEID,SLOT) ((NODEID) | ((SLOT) ? 0x80000000 : 0))
#define SLOT(SLOTID) (((SLOTID) & 0x80000000) ? 1 : 0)

#define inet_ntoa __use_my_inet_ntoa__

#define SHA256_BLOCK_SIZE 32

#define ROTLEFT(a,b) (((a) << (b)) | ((a) >> (32-(b))))
#define ROTRIGHT(a,b) (((a) >> (b)) | ((a) << (32-(b))))

#define CH(x,y,z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTRIGHT(x,2) ^ ROTRIGHT(x,13) ^ ROTRIGHT(x,22))
#define EP1(x) (ROTRIGHT(x,6) ^ ROTRIGHT(x,11) ^ ROTRIGHT(x,25))
#define SIG0(x) (ROTRIGHT(x,7) ^ ROTRIGHT(x,18) ^ ((x) >> 3))
#define SIG1(x) (ROTRIGHT(x,17) ^ ROTRIGHT(x,19) ^ ((x) >> 10))

void init_process();
int open_udp (int port);
bool IsOptionPresent (int argc, char **argv, PCSTR arg);
byte * make_sha256_hash (void const *pSrc, int nSize, byte *dest, void const *pSalt, int nSaltSize);
bool select_rx (int sock, int wait_secs);
PCSTR skipspaces (PCSTR p, bool bSkipTabs=true, bool bSkipCtrl=false);
void trim (std::string &s);

struct ob_peer {
    sockaddr_in addr;
    int         sock;
	int         local_port;
    bool        enabled;
    dword       network_id;
    char        pass[MAX_PASSWORD_SIZE];
    bool        force_slot1;
    bool        permit_all;
    char        permit_tgs[512];
    dword       last_rx_sec, last_tx_sec, last_ping_sec;
    dword       stream_ring[256];
    byte        ring_ix;
    bool        enhanced;
    bool        relax_checks;
    bool        hblink_compat;
	char  		target_host[256];
	char        alias_name[128];
	int   		target_port;
	dword 		last_resolve_sec;
	int   		resolve_interval;
};

#ifdef HAVE_APRS
struct aprs_client {
    int   sock;
    bool  enabled;
    char  server_host[256];
    int   server_port;
    char  callsign[32];
    char  passcode[16];
    char  filter[128];
    int   keepalive_secs;
    int   reconnect_secs; 
    dword last_io_sec;
    dword last_try_sec;
};
#endif

template <class X> void swap (X &a, X &b) {
	X temp;
	temp = a;
	a = b;
	b = temp;
}

class memfile
{
public:
	byte				*m_pData;
	dword				m_nSize;
	dword				m_nAlloc;
	dword				m_nPos;
	dword				m_nAllocSize;

private:

	void Init() {
		m_pData = NULL;
		m_nSize = 0;
		m_nAlloc = 0;
		m_nPos = 0;
	}

public:
	memfile(int nAllocSize=65536) {
		Init();

		m_nAllocSize = nAllocSize;
	}

	virtual ~memfile() {
		Close();
	}

	virtual bool IsOpen() const {return true;}

	virtual bool Open_(PCSTR pPath=NULL, int nFlags=0, int nCreateMode=0660) {
		Close();

		return true;
	}

	virtual bool Creat(PCSTR pPath=NULL, int nCreateMode=0) {
		return Open_ (NULL);
	}

	virtual bool Close() {
		if (m_pData) 
			free(m_pData);
			
		Init();

		return true;
	}

	virtual dword GetSize () const {
		return m_nSize;
	}

	virtual dword GetPos() const {
		return m_nPos;
	}

	void SetSize (int nSize) {
		if (nSize == 0) {
			Creat();
		} else {
			m_pData = (byte*) realloc (m_pData, nSize);

			if (nSize && !m_pData)
				throw (int) ENOMEM;

			m_nSize = m_nAlloc = nSize;
			m_nPos = 0;
		}
	}

	virtual dword Seek (dword nPos, int nFrom=SEEK_SET) {
		switch (nFrom) {
			case SEEK_SET:
				m_nPos = (int) nPos;
				break;

			case SEEK_END:
				m_nPos = (int) nPos + m_nSize;
				break;

			case SEEK_CUR:
				m_nPos += (int) nPos;
				break;

			default:
				throw (int) EINVAL;
				break;
		}

		return m_nPos;
	}

	virtual int Read (void *buf, int nCount) {
		if (nCount < 1)
			return 0;

		if (!buf)
			throw (int) EINVAL;

		if (m_nPos < 0)
			throw (int) EINVAL;

		int i;

		for (i=0; i < nCount && m_nPos < m_nSize; i++) {
			((byte*)buf)[i] = m_pData[m_nPos++];
		}

		return i;
	}

	virtual int Write (void const *buf, int nCount) {
		if (nCount < 1)
			return 0;

		if (!buf)
			throw (int) EINVAL;

		if (m_nPos < 0)
			throw (int) EINVAL;

		int i;

		for (i=0; i < nCount; i++) {
			while (!m_pData || m_nPos >= m_nAlloc) {

				m_nAlloc = m_nPos + m_nAllocSize;

				m_pData = (byte*) realloc (m_pData, m_nAlloc);

				if (!m_pData) {
					Init();

					throw (int) ENOMEM;
				}
			}

			m_pData[m_nPos++] = ((byte*)buf)[i];

			if (m_nPos > m_nSize)
				m_nSize = m_nPos;
		}

		return i;
	}

	virtual void Unlock () {
	}

};

typedef std::map <std::string, std::string> STRINGMAP;

typedef STRINGMAP::iterator STRINGMAP_ITERATOR;

class config_file
{
public:
	STRINGMAP values;
	
	bool load (PCSTR path) {
		FILE *f = fopen (path, "r");

		if (!f)
			return false;

		std::string section;

		char temp[1000];

		while (fgets(temp, sizeof(temp), f)) {
			PCSTR p = skipspaces(temp);

			if (*p == '#' || *p == '\r' || *p == '\n')
				continue;

			if (*p == '[') {
				p = skipspaces(p+1);
				section = "";

				while (*p && *p != ']' && *p != '#' && *p != '\r' && *p != '\n')
					section += *p++;

				trim(section);
			} else {
				std::string name, val;

				while (*p && *p != '=' && *p != '#' && *p != '\r' && *p != '\n')
					name += *p++;

				trim(name);

				if (*p == '=') {
					p = skipspaces(p+1);

					while (*p && *p != '#' && *p != '\r' && *p != '\n')
						val += *p++;

					trim(val);

					if (eq(val.c_str(), "false") || eq(val.c_str(), "off") || eq(val.c_str(), "no") || eq(val.c_str(), "disable") || eq(val.c_str(), "disabled"))
						val = "0";
					else if (eq(val.c_str(), "true") || eq(val.c_str(), "on") || eq(val.c_str(), "yes") || eq(val.c_str(), "enable") || eq(val.c_str(), "enabled"))
						val = "1";
				}

				std::string key;

				key = section;
				key += "|";
				key += name;

				values[key] = val;
			}
		}

		fclose (f);

		return true;
	}	

	void dump() {
		for (STRINGMAP_ITERATOR it = values.begin(); it != values.end(); it++) {
			std::string const &key = (*it).first;
			std::string &value = (*it).second;

			printf ("%s = %s\n", (PCSTR)key.c_str(), (PCSTR)value.c_str());
		}
	}

	bool getvalue (PCSTR section, PCSTR name, std::string &ret) {
		std::string key;

		key = section;
		key += "|";
		key += name;

		if (values.find(key) == values.end())
			return false;

		ret = values[key];

		return true;
	}

	std::string getstring (PCSTR section, PCSTR name, PCSTR Default="") {
		std::string val;
		
		if (getvalue (section, name, val))
			return val;

		return Default;
	}

	int getint (PCSTR section, PCSTR name, int Default=0) {
		std::string val;
		
		if (getvalue (section, name, val))
			return atoi(val.c_str());

		return Default;
	}
};

extern ob_peer g_obp;
extern std::vector<ob_peer> g_obp_peers;

bool obp_fetch_check(int timeout_ms);
void obp_init();
bool resolve_hostname_ipv4(PCSTR host, in_addr* out);
bool obp_resolve_now();
void obp_set_target(PCSTR host, int port);
void obp_housekeeping();
void obp_handle_rx();
void obp_load_extra_from_config(class config_file& c);
void obp_open_all();
void obp_housekeeping_all();
void obp_handle_rx_all();
void obp_forward_dmrd(const byte* pk, int sz, int origin_tag);

extern int  g_auth_enabled;
extern char g_auth_file[260];
extern int  g_auth_reload_secs;
extern int  g_auth_unknown_default;

void auth_load_initial();
void auth_housekeeping();

const char* auth_lookup_pass(dword dmrid);

#ifdef HAVE_APRS
extern aprs_client g_aprs;

bool aprs_init_from_config();
void aprs_housekeeping();
void aprs_send_heard(dword dmrid, dword tg, dword nodeid);
bool aprs_load_idmap(const char* path);
const char* aprs_lookup_callsign(dword dmrid);
#endif

#ifdef HAVE_SMS
struct sms_settings {
    bool  enabled;
    char  udphost[256];
    int   udpport;
    bool  allow_private;
    bool  permit_all;
    char  permit_tgs[512];
    int   max_frames;
    int   max_seconds;
};

struct sms_buf {
    memfile *buf;
    dword    streamid;
    dword    start_sec;
    int      frames;
    sms_buf() : buf(NULL), streamid(0), start_sec(0), frames(0) {}
};

extern sms_settings g_sms;

bool sms_tg_permitted(dword tg);
void sms_reset(struct sms_buf &sb);
void sms_append(struct sms_buf &sb, const byte *block51);
void sms_emit_udp(dword radioid, dword dest, bool is_private, struct sms_buf &sb);
void sms_housekeeping();
bool sms_init_from_config(class config_file &c);
#endif

#ifdef HAVE_HTTPMODE
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
#endif

#endif
