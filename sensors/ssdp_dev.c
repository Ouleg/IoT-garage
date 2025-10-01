// sensors/ssdp_dev.c (shared)
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "config.h"
#include "protocol.h"

static int sock = -1;
static struct sockaddr_in maddr;

static void ssdp_send_notify(const char* dev_id, const char* st_urn)
{
    char buf[512];
    char myip[] = "192.168.1.33"; // za demo; po želji autodetekcija
    char usn[128]; snprintf(usn, sizeof usn, "uuid:%s::%s", dev_id, st_urn);
    char loc[128]; snprintf(loc, sizeof loc, DEV_HTTP_LOCATION_FMT, myip, 8080, dev_id);

    int n = snprintf(buf, sizeof buf,
        "NOTIFY * HTTP/1.1\r\n"
        "HOST: %s:%d\r\n"
        "NT: %s\r\n"
        "NTS: ssdp:alive\r\n"
        "USN: %s\r\n"
        "LOCATION: %s\r\n"
        "CACHE-CONTROL: max-age=%d\r\n"
        "SERVER: esp32/1.0 ssdp/1.0\r\n"
        "\r\n", SSDP_ADDR, SSDP_PORT, st_urn, usn, loc, SSDP_MAX_AGE);

    sendto(sock, buf, n, 0, (struct sockaddr*)&maddr, sizeof maddr);
}

static void ssdp_answer_msearch(const char* dev_id, const char* st_urn,
                                const struct sockaddr_in* src)
{
    char buf[512];
    char myip[] = "192.168.1.33";
    char usn[128]; snprintf(usn, sizeof usn, "uuid:%s::%s", dev_id, st_urn);
    char loc[128]; snprintf(loc, sizeof loc, DEV_HTTP_LOCATION_FMT, myip, 8080, dev_id);

    int n = snprintf(buf, sizeof buf,
        "HTTP/1.1 200 OK\r\n"
        "CACHE-CONTROL: max-age=%d\r\n"
        "ST: %s\r\n"
        "USN: %s\r\n"
        "EXT:\r\n"
        "SERVER: esp32/1.0 ssdp/1.0\r\n"
        "LOCATION: %s\r\n"
        "\r\n", SSDP_MAX_AGE, st_urn, usn, loc);

    sendto(sock, buf, n, 0, (const struct sockaddr*)src, sizeof *src);
}

void ssdp_dev_loop(const char* dev_id, const char* st_urn)
{
    if (sock < 0) {
        sock = socket(AF_INET, SOCK_DGRAM, 0);
        int yes=1; setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
        memset(&maddr,0,sizeof maddr);
        maddr.sin_family = AF_INET;
        maddr.sin_port = htons(SSDP_PORT);
        inet_pton(AF_INET, SSDP_ADDR, &maddr.sin_addr);
        // join nije striktno nužan za slanje; za prijem M-SEARCH možeš bind+recvfrom na 1900
    }

    static time_t last=0;
    time_t now=time(NULL);
    if (now-last >= SSDP_MAX_AGE/2) { // povremeno obnavljaj NOTIFY alive
        ssdp_send_notify(dev_id, st_urn);
        last = now;
    }

    // Minimalni prijem M-SEARCH (ostavljeno za vežbu: bind na 1900 i filtriranje MAN/ST)
    // ... recvfrom(...) pa ssdp_answer_msearch(dev_id, st_urn, &src) ...
}
