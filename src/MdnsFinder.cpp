#include "MdnsFinder.hpp"
#include "mdns.h"
#include <regex>

namespace hippo
{
#ifdef _WIN32
#  define _CRT_SECURE_NO_WARNINGS 1
#endif

#include <stdio.h>
#include <errno.h>

#ifdef _WIN32
#  define sleep(x) Sleep(x * 1000)
#else
#include <netdb.h>
#endif

static mdns_string_t
ipv4_address_to_string(char* buffer, size_t capacity, const struct sockaddr_in* addr) {
	char host[NI_MAXHOST] = {0};
	char service[NI_MAXSERV] = {0};
	int ret = getnameinfo((const struct sockaddr*)addr, sizeof(struct sockaddr_in),
	                      host, NI_MAXHOST, service, NI_MAXSERV,
	                      NI_NUMERICSERV | NI_NUMERICHOST);
	size_t len = 0;
	if (ret == 0) {
		if (addr->sin_port != 0)
			len = snprintf(buffer, capacity, "%s:%s", host, service);
		else
			len = snprintf(buffer, capacity, "%s", host);
	}
	if (len >= (int)capacity)
		len = (int)capacity - 1;
	mdns_string_t str = {buffer, len};
	return str;
}

static mdns_string_t
ipv6_address_to_string(char* buffer, size_t capacity, const struct sockaddr_in6* addr) {
	char host[NI_MAXHOST] = {0};
	char service[NI_MAXSERV] = {0};
	int ret = getnameinfo((const struct sockaddr*)addr, sizeof(struct sockaddr_in6),
	                      host, NI_MAXHOST, service, NI_MAXSERV,
	                      NI_NUMERICSERV | NI_NUMERICHOST);
	size_t len = 0;
	if (ret == 0) {
		if (addr->sin6_port != 0)
			len = snprintf(buffer, capacity, "[%s]:%s", host, service);
		else
			len = snprintf(buffer, capacity, "%s", host);
	}
	if (len >= (int)capacity)
		len = (int)capacity - 1;
	mdns_string_t str = {buffer, len};
	return str;
}

static mdns_string_t
ip_address_to_string(char* buffer, size_t capacity, const struct sockaddr* addr) {
	if (addr->sa_family == AF_INET6)
		return ipv6_address_to_string(buffer, capacity, (const struct sockaddr_in6*)addr);
	return ipv4_address_to_string(buffer, capacity, (const struct sockaddr_in*)addr);
}

static int
callback(const struct sockaddr* from,
         mdns_entry_type_t entry, uint16_t type,
         uint16_t rclass, uint32_t ttl,
         const void* data, size_t size, size_t offset, size_t length,
         void* user_data) {

    char addrbuffer[64];
    char namebuffer[256];
    std::list<std::string> *list = (std::list<std::string> *)user_data;

	mdns_string_t fromaddrstr = ip_address_to_string(addrbuffer, sizeof(addrbuffer), from);
	const char* entrytype = (entry == MDNS_ENTRYTYPE_ANSWER) ? "answer" :
	                        ((entry == MDNS_ENTRYTYPE_AUTHORITY) ? "authority" : "additional");
	if (type == MDNS_RECORDTYPE_PTR) {
		mdns_string_t namestr = mdns_record_parse_ptr(data, size, offset, length,
		                                              namebuffer, sizeof(namebuffer));
		printf("%.*s : %s PTR %.*s type %u rclass 0x%x ttl %u length %d\n",
		       MDNS_STRING_FORMAT(fromaddrstr), entrytype,
		       MDNS_STRING_FORMAT(namestr), type, rclass, ttl, (int)length);
	}
	else if (type == MDNS_RECORDTYPE_SRV) {
		mdns_record_srv_t srv = mdns_record_parse_srv(data, size, offset, length,
		                                              namebuffer, sizeof(namebuffer));
		printf("%.*s : %s SRV %.*s priority %d weight %d port %d\n",
		       MDNS_STRING_FORMAT(fromaddrstr), entrytype,
		       MDNS_STRING_FORMAT(srv.name), srv.priority, srv.weight, srv.port);
        {
            char buf[128] = {0};
            const char *p = strchr(fromaddrstr.str, ':');

            strcpy(buf, "TCPIP0::");
            if(p != NULL)
            {
                strncpy(buf+strlen(buf), fromaddrstr.str, p - fromaddrstr.str);
                sprintf(buf+strlen(buf), "::%d::SOCKET", srv.port);
            } else {
                sprintf(buf+strlen(buf), "%s::%d::SOCKET", fromaddrstr.str, srv.port);
            }
            printf("found one:%s\n", buf);
            list->push_back(std::string(buf));
        } 
	}
	else if (type == MDNS_RECORDTYPE_A) {
		struct sockaddr_in addr;
		mdns_record_parse_a(data, size, offset, length, &addr);
		mdns_string_t addrstr = ipv4_address_to_string(namebuffer, sizeof(namebuffer), &addr);
		printf("%.*s : %s A %.*s\n",
		       MDNS_STRING_FORMAT(fromaddrstr), entrytype,
		       MDNS_STRING_FORMAT(addrstr));
	}
	else if (type == MDNS_RECORDTYPE_AAAA) {
		struct sockaddr_in6 addr;
		mdns_record_parse_aaaa(data, size, offset, length, &addr);
		mdns_string_t addrstr = ipv6_address_to_string(namebuffer, sizeof(namebuffer), &addr);
		printf("%.*s : %s AAAA %.*s\n",
		       MDNS_STRING_FORMAT(fromaddrstr), entrytype,
		       MDNS_STRING_FORMAT(addrstr));
	}
	else if (type == MDNS_RECORDTYPE_TXT) {
        mdns_record_txt_t txtbuffer[128];
		size_t parsed = mdns_record_parse_txt(data, size, offset, length,
		                                      txtbuffer, sizeof(txtbuffer) / sizeof(mdns_record_txt_t));
		for (size_t itxt = 0; itxt < parsed; ++itxt) {
			if (txtbuffer[itxt].value.length) {
				printf("%.*s : %s TXT %.*s = %.*s\n",
				       MDNS_STRING_FORMAT(fromaddrstr), entrytype,
				       MDNS_STRING_FORMAT(txtbuffer[itxt].key),
				       MDNS_STRING_FORMAT(txtbuffer[itxt].value));
			}
			else {
				printf("%.*s : %s TXT %.*s\n",
				       MDNS_STRING_FORMAT(fromaddrstr), entrytype,
				       MDNS_STRING_FORMAT(txtbuffer[itxt].key));
			}
		}
	}
	else {
		printf("%.*s : %s type %u rclass 0x%x ttl %u length %d\n",
		       MDNS_STRING_FORMAT(fromaddrstr), entrytype,
		       type, rclass, ttl, (int)length);
	}
	return 0;
}

std::list<std::string> MdnsFinder::availableDevs(const std::string &pattern) const{
    std::list<std::string> list;
	size_t capacity = 2048;
	void* buffer = 0;
	void* user_data = &list;
	size_t records;

#ifdef _WIN32
	WORD versionWanted = MAKEWORD(1, 1);
	WSADATA wsaData;
	WSAStartup(versionWanted, &wsaData);
#endif

	int sock = mdns_socket_open_ipv4();
	if (sock < 0) {
		printf("Failed to open socket:%d: %s\n", sock, strerror(errno));
		return list;
	}

	printf("Opened IPv4 socket for mDNS/DNS-SD\n");

	/*printf("Sending DNS-SD discovery\n");
	if (mdns_discovery_send(sock)) {
		printf("Failed to send DNS-DS discovery: %s\n", strerror(errno));
		goto quit;
	}

	printf("Reading DNS-SD replies\n");
	buffer = malloc(capacity);
	for (int i = 0; i < 10; ++i) {
		records = mdns_discovery_recv(sock, buffer, capacity, callback,
		                              user_data);
		sleep(1);
	}*/

	printf("Sending mDNS query\n");
	buffer = malloc(capacity);
	if (mdns_query_send(sock, MDNS_RECORDTYPE_PTR,
	                    MDNS_STRING_CONST("_scpi-raw._tcp.local."),
	                    buffer, capacity)) {
		printf("Failed to send mDNS query: %s\n", strerror(errno));
		goto quit;
	}

	printf("Reading mDNS replies\n");
	for (int i = 0; i < 10; ++i) {
		records = mdns_query_recv(sock, buffer, capacity, callback, user_data, 1);
		usleep(200);
	}

quit:
	free(buffer);

	mdns_socket_close(sock);
	printf("Closed socket\n");

#ifdef _WIN32
	WSACleanup();
#endif

    {
        std::regex regex(pattern);
        list.remove_if([regex](std::string i){
                        return (!std::regex_match(i, regex)); 
                       }
                       );
        std::cout<<"pattern is:"<<pattern<< " and "<<" size "<<list.size()<<std::endl;
    }

    return list;
}

std::list<std::string> MdnsFinder::availableDevs() const{
    return availableDevs(DEF_PATTERN);
}

#ifdef _WIN32
#include <Iphlpapi.h>
//#pragma comment(lib,"Iphlpapi.lib") 
using namespace std;

std::list<NetcardInfo> availableNetcards() const {
    std::list<NetcardInfo> list;
    NetcardInfo info;
	static const  int ADAPTERNUM  = 10; 

	PIP_ADAPTER_INFO pIpAdapterInfo = new IP_ADAPTER_INFO[ADAPTERNUM];// 10个网卡空间 足够了 
	unsigned long stSize = sizeof(IP_ADAPTER_INFO) * ADAPTERNUM;
	// 获取所有网卡信息，参数二为输入输出参数 
	int nRel = GetAdaptersInfo(pIpAdapterInfo,&stSize);
	// 空间不足
	if (ERROR_BUFFER_OVERFLOW == nRel) {
		// 释放空间
		if(pIpAdapterInfo!=NULL)
			delete[] pIpAdapterInfo;
		return; 
	}
	
	PIP_ADAPTER_INFO cur =   pIpAdapterInfo;
	// 多个网卡 通过链表形式链接起来的 
	while(cur){
		cout<<"网卡描述："<<cur->Description<<endl;
        info.name = std::string(cur->Description);
		switch (cur->Type) {
			case MIB_IF_TYPE_OTHER:
				break;
			case MIB_IF_TYPE_ETHERNET:
				{
					IP_ADDR_STRING *pIpAddrString =&(cur->IpAddressList);
					cout << "IP:" << pIpAddrString->IpAddress.String << endl;
					cout << "子网掩码:" << 	pIpAddrString->IpMask.String <<endl;
                    info.ip = std::string(pIpAddrString->IpAddress.String);
                    info.mask = std::string(pIpAddrString->IpMask.String);
					pIpAddrString =&(cur->GatewayList);
                    info.gateway = std::string(pIpAddrString->IpAddress.String);
				}
				break;
			case MIB_IF_TYPE_TOKENRING:
				break;
			case MIB_IF_TYPE_FDDI:
				break;
			case MIB_IF_TYPE_PPP:
				break;
			case MIB_IF_TYPE_LOOPBACK:
				break;
			case MIB_IF_TYPE_SLIP:
				break;
			default://无线网卡,Unknown type
				{
					IP_ADDR_STRING *pIpAddrString =&(cur->IpAddressList);
					cout << "IP:" << pIpAddrString->IpAddress.String << endl;
					cout << "子网掩码:" << 	pIpAddrString->IpMask.String <<endl;
                    info.ip = std::string(pIpAddrString->IpAddress.String);
                    info.mask = std::string(pIpAddrString->IpMask.String);
					pIpAddrString =&(cur->GatewayList);
                    info.gateway = std::string(pIpAddrString->IpAddress.String);
				}
				break;
		}
	    char hex[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'} ;
		
		// mac 地址一般6个字节 
		// mac 二进制转16进制字符串
		char macStr[18] = {0};//12+5+1
		int k = 0;
		for(int j = 0; j < cur->AddressLength; j++){
			macStr[k++] = hex[(cur->Address[j] & 0xf0) >> 4];
			macStr[k++] = hex[cur->Address[j] & 0x0f];
			macStr[k++] = '-'; 
		} 
		macStr[k-1] = 0;
        info.mac =std::string(macStr);

        if((info.ip.size() > 0) && info.ip != "0.0.0.0"){
            list.push_back(info);
        }
		
		cout<<"MAC:" << macStr << endl; // mac地址 16进制字符串表示 
		cur = cur->Next;
		cout << "--------------------------------------------------" << endl;
	}
	
	// 释放空间
	if(pIpAdapterInfo!=NULL)
		delete[] pIpAdapterInfo;
    return list;
} 
#else
#include <linux/sockios.h> 
#include <linux/ethtool.h> 
#include <net/ethernet.h>
#include <net/if.h> 
#include <net/if_arp.h> 
#include <arpa/inet.h> 
#include <string.h> 
#include <sys/ioctl.h>

static int32_t  network_ip_address(const char *eth, char *ip)
{
	struct ifreq ifr;
	struct sockaddr_in sin;
	struct sockaddr sa;
	int sockfd;

	if(ip == NULL)
	{
		return -1;
	}

	sockfd = socket(AF_INET,SOCK_STREAM,0);
	if(sockfd == -1)
	{
		return -1;
	}

	memset(&ifr, 0, sizeof(struct ifreq));
	strcpy(ifr.ifr_ifrn.ifrn_name, eth);

	ioctl(sockfd, SIOCGIFADDR, &ifr);
	strcpy(ip, inet_ntoa(((struct sockaddr_in*)&(ifr.ifr_addr))->sin_addr));

	close(sockfd);
	return 0;
}

static int32_t  network_net_mask(const char *eth, char *mask)
{
	struct ifreq ifr;
	struct sockaddr_in sin;
	struct sockaddr sa;
	int sockfd;

	if(mask == NULL)
	{
		return -1;
	}

	sockfd = socket(AF_INET,SOCK_STREAM,0);
	if(sockfd == -1)
	{
		return -1;
	}

	memset(&ifr, 0, sizeof(struct ifreq));
	strcpy(ifr.ifr_ifrn.ifrn_name, eth);

	ioctl(sockfd, SIOCGIFNETMASK,&ifr);
	strcpy(mask, inet_ntoa(((struct sockaddr_in*)&(ifr.ifr_netmask))->sin_addr));

	close(sockfd);
	return 0;
}

static int32_t  network_mac(const char *eth, char *mac)
{
	int ret = 0;
	struct ifreq ifr;
	struct sockaddr_in sin;
	struct sockaddr sa;
	int sockfd;

	if(mac == NULL)
	{
		return -1;
	}

	sockfd = socket(AF_INET,SOCK_STREAM,0);
	if(sockfd == -1)
	{
		return -1;
	}

	memset(&ifr, 0, sizeof(struct ifreq));
	strcpy(ifr.ifr_ifrn.ifrn_name, eth);
	if( ioctl(sockfd, SIOCGIFHWADDR, &ifr) >= 0 ) 
    { 
        sprintf( 
            		mac, "%02x:%02x:%02x:%02x:%02x:%02x", 
            		(unsigned char)ifr.ifr_hwaddr.sa_data[0], 
            		(unsigned char)ifr.ifr_hwaddr.sa_data[1], 
            		(unsigned char)ifr.ifr_hwaddr.sa_data[2], 
            		(unsigned char)ifr.ifr_hwaddr.sa_data[3], 
            		(unsigned char)ifr.ifr_hwaddr.sa_data[4], 
            		(unsigned char)ifr.ifr_hwaddr.sa_data[5] 
        	); 
   	} 
    else 
    { 
    	ret = -1; 
    } 

	close(sockfd);
	return ret;
}

static int network_gateway(const char *eth, char *gateway)    
{    
	FILE *fp;    
	char buf[256];
	char iface[16];    
	unsigned long dest_addr, gate_addr;    
	struct in_addr gw_in_addr;	

	gw_in_addr.s_addr = INADDR_NONE;    

	fp = fopen("/proc/net/route", "r");    
	if (fp == NULL)    
	{
	 	 return -1;    
	}

	/*skip title line */    
	fread(buf, sizeof(buf), 1, fp);    
	while (fread(buf, sizeof(buf), 1, fp))
	{
		if (sscanf(buf, "%s\t%lX\t%lX", iface,       &dest_addr, &gate_addr) != 3 ||    
			dest_addr != 0)    
		continue;    

        if(strcmp(eth, iface) != 0) 
		    continue;    


		gw_in_addr.s_addr = (in_addr_t)gate_addr;    
		strcpy(gateway, (char *)inet_ntoa(gw_in_addr));
		break;    
	}
	
	fclose(fp);    
	return 0;    
} 

std::list<NetcardInfo> MdnsFinder::availableNetcards() const{
	FILE *fp;    
	char buf[512];
    std::list<NetcardInfo> list;

	fp = fopen("/proc/net/dev", "r");    
	if (fp == NULL)    
	{
	 	 return list;    
	}

	/*skip title line */    
	fread(buf, sizeof(buf), 1, fp);    
	fread(buf, sizeof(buf), 1, fp);    
	memset(buf, 0, sizeof(buf));

	while (fread(buf, sizeof(buf), 1, fp)) 
	{
		char *pt = strchr(buf, ':');
		if(pt)
		{
            int ret;
			int j = 0;
	        char eth[128];
            std::list<NetcardInfo> list;
            NetcardInfo info;

			for(int i=0; i<pt-buf; i++)	
			{
				if(buf[i] != ' ')
				{
					eth[j++] = buf[i];
				}
			}
            if(strcmp(eth, "lo") == 0) continue;
            info.name = std::string(eth);

            ret = network_ip_address(eth, buf);    
            if(ret < 0) continue;
            info.ip = std::string(buf);

            ret = network_net_mask(eth, buf);    
            if(ret < 0) continue;
            info.mask = std::string(buf);

            ret = network_mac(eth, buf);    
            if(ret < 0) continue;
            info.mask = std::string(buf);

            ret = network_gateway(eth, buf);    
            info.gateway = std::string(buf);

            if((info.ip.size() > 0) && info.ip != "0.0.0.0"){
                list.push_back(info);
            }
        }
		else
		{
            continue;
		}

		memset(buf, 0, sizeof(buf));
	}    

	fclose(fp);    

    return list;
}
#endif

/*------------------------------------------------------------------------------------*/

bool MdnsPort::open(){
    sp = std::make_shared<tcp::socket>(io_context);
    if(sp != nullptr){
        tcp::resolver resolver(io_context);
        std::smatch result;


        if (std::regex_search(url_.cbegin(), url_.cend(), result, std::regex("::([0-9.]{1,})::"))) {
            ip = result[1];
        } else {
            std::cout <<"ip parser error"<<std::endl;
            return false;
        }

        if (std::regex_search(url_.cbegin(), url_.cend(), result, std::regex("::([0-9]{1,})::SOCKET"))) {
            port = result[1];
        } else {
            std::cout <<"port parser error"<<std::endl;
            return false;
        }

        std::cout <<url_<<" ip is "+ip<<" port is "+port<<std::endl;

        asio::connect(*sp, resolver.resolve(ip.c_str(), port.c_str()));
        return true;
    }
    return false;
}

void MdnsPort::close(){
    sp->close();
}

int MdnsPort::write(const char *data, ssize_t len){
    return asio::write(*sp, asio::buffer(data, len));
}

int MdnsPort::read(char *data, ssize_t len){
    return asio::read(*sp, asio::buffer(data, len), asio::transfer_at_least(4));
}

int MdnsPort::setOption(int option, const void *data, ssize_t len){
    return 0;
}

int MdnsPort::getOption(int option, void *data, ssize_t len){
    return 0;
}

}

