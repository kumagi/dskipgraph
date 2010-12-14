#include "get_interface.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/un.h> // bzero
#include <netinet/tcp.h>
#include <sys/ioctl.h> // ioctl
#include <net/if.h> // ifreq ifr IFNAMSIZ
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
uint32_t get_myip_interface(const char* const name){
	struct ifreq ifr;
	struct sockaddr_in sin;
	int fd=socket(AF_INET, SOCK_STREAM,0);
	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_addr.sa_family = AF_INET;
	
	memset(&ifr, 0, sizeof(ifr));
	strcpy(ifr.ifr_name, name);
	if(ioctl(fd, SIOCGIFADDR, &ifr) < 0){
		char buff[250];
		sprintf(buff,"cannot use <%s> SIOCGIFADDR",ifr.ifr_name);
		perror(buff);exit(1);
	}
	if(ioctl(fd, SIOCGIFADDR, &ifr) < 0){
		perror("ioctl(SIOCGIFADDR)");exit(1);
	}
	if(ifr.ifr_addr.sa_family == AF_INET){
		memcpy(&sin, &(ifr.ifr_addr), sizeof(struct sockaddr_in));
	}else{
		exit(1);
	}
	close(fd);
	return sin.sin_addr.s_addr;
}

char* ntoa(int ip){
	struct sockaddr_in tmpAddr;
	tmpAddr.sin_addr.s_addr = ip;
	return inet_ntoa(tmpAddr.sin_addr);
}
