#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/ioctl.h>

#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "anet.h"
#include "StdDataType.h"
#include "PublicFunction.h"
#include "helper.h"

int helperCheckIp() {
	int sock;
	struct sockaddr_in sin;
	struct ifreq ifr;
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == -1) {
		return -1;
	}
	strncpy(ifr.ifr_name, "ppp0", IFNAMSIZ);
	ifr.ifr_name[IFNAMSIZ - 1] = 0;
	if (ioctl(sock, SIOCGIFADDR, &ifr) < 0) {
		close(sock);
		return -1;
	}
	memcpy(&sin, &ifr.ifr_addr, sizeof(sin));
	if (sin.sin_addr.s_addr > 0) {
		close(sock);
		return 1;
	}
	close(sock);
	return 0;
}


void helperPeerStat(int fd, char *info) {
	int peerBuf[128];
	int port = 0;

	memset(peerBuf, 0x00, sizeof(peerBuf));
	anetTcpKeepAlive(NULL, fd);
	anetPeerToString(fd, peerBuf, sizeof(peerBuf), &port);
	asyslog(LOG_INFO, "[%s%s]:%d\n", info, peerBuf, port);
}

int helperKill(char *name, int timeout) {
	pid_t pids[128];
	for (int i = 0; i < timeout; i++) {
		memset(pids, 0x00, sizeof(pids));
		if (prog_find_pid_by_name((signed char *) name, pids) >= 1) {
			char command[64];
			memset(command, 0x00, sizeof(command));
			sprintf(command, "kill %d", pids[0]);
			if (i % 3 == 0) {
				system(command);
				asyslog(LOG_INFO, "正在停止进程[%s],进程号[%d],使用的终止命令[%s]", name,
						pids[0], command);
			}
		} else {
			return 0;
		}
		sleep(1);
	}
	return -1;
}

int helperGetInterFaceIp(char *interface, char *ips) {
	int sock;
	struct sockaddr_in sin;
	struct ifreq ifr;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == -1) {
		return -1;
	}
	strncpy(ifr.ifr_name, interface, IFNAMSIZ);
	ifr.ifr_name[IFNAMSIZ - 1] = 0;
	if (ioctl(sock, SIOCGIFADDR, &ifr) < 0) {
		close(sock);
		return -1;
	}
	memcpy(&sin, &ifr.ifr_addr, sizeof(sin));
	if (sin.sin_addr.s_addr > 0) {
		int ip[4];
		memset(ip, 0x00, sizeof(ip));
		sscanf(inet_ntoa(sin.sin_addr), "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2],
				&ip[3]);
		snprintf(ips, 16, "%d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);
		close(sock);
		return 1;
	}
	close(sock);
	return 0;
}

int helperConnect(char *interface, MASTER_STATION_INFO *info) {
	char boundBuf[32];
	memset(boundBuf, 0x00, sizeof(boundBuf));
	if (helperGetInterFaceIp(interface, boundBuf) == 1) {
		int fd = anetTcpNonBlockBindConnect(NULL, (char *) info->ip, info->port,
				boundBuf);
		if (fd > 0) {
			return fd;
		}
	}
	return -1;

}

int helperCheckConnect(char *interface, int fd) {
	int port = 0;
	char peerBuf[32];
	memset(peerBuf, 0x00, sizeof(peerBuf));
	if (0 == anetPeerToString(fd, peerBuf, sizeof(peerBuf), &port)) {
		return fd;
	}
	return -1;
}

int helperComOpen(int port, int baud, unsigned char par, unsigned char stopb,
		unsigned char bits) {
	static int GlobBand[] = { 300, 600, 1200, 2400, 4800, 7200, 9600, 19200,
			38400, 57600, 115200 };
	static char *GlobCrc[] = { "none", "odd", "even" };
	static int GlobData[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
	static int GlobStop[] = { 0, 1, 2 };
	return OpenCom(port, GlobBand[baud], GlobCrc[par], GlobStop[stopb],
			GlobData[bits]);
}

MASTER_STATION_INFO helperGetNextGPRSIp(){
	static int index = 0;

	CLASS25 *c25 = dbGet("class25");
	MASTER_STATION_INFO *IpPool = c25->master.master;

	MASTER_STATION_INFO res;
	memset(&res, 0x00, sizeof(MASTER_STATION_INFO));
	snprintf((char *) res.ip, sizeof(res.ip), "%d.%d.%d.%d",
			IpPool[index].ip[1], IpPool[index].ip[2], IpPool[index].ip[3],
			IpPool[index].ip[4]);
	res.port = IpPool[index].port;
	index++;
	index %= 2;
	asyslog(LOG_INFO, "客户端[GPRS]尝试链接的IP地址：%s:%d", res.ip, res.port);
	return res;
}

MASTER_STATION_INFO helperGetNextNetIp(){
	static int index = 0;

	CLASS26 *c26 = dbGet("class26");
	MASTER_STATION_INFO *IpPool = c26->master.master;

	MASTER_STATION_INFO res;
	memset(&res, 0x00, sizeof(MASTER_STATION_INFO));
	snprintf((char *) res.ip, sizeof(res.ip), "%d.%d.%d.%d",
			IpPool[index].ip[1], IpPool[index].ip[2], IpPool[index].ip[3],
			IpPool[index].ip[4]);
	res.port = IpPool[index].port;
	index++;
	index %= 2;
	asyslog(LOG_INFO, "客户端[NET]尝试链接的IP地址：%s:%d", res.ip, res.port);
	return res;
}


int helperReadPositionGet(int length) {
	int pos = 1;
	int tmp = length;
	for (int i = 0; i < 5; ++i) {
		tmp /= 10;
		if (tmp == 0) {
			break;
		}
		pos++;
	}
	return pos;
}
