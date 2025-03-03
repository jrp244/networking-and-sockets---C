#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <time.h>

#define ETH_P_CUSTOM 0x0800 // IPv4 EtherType
#define BUFFER_SIZE 1500

// Function prototypes
void send_icmp_echo(int sockfd, const char *src_ip, const char *dst_ip,
                    const unsigned char *src_mac, const unsigned char *dst_mac,
                    int id, int seq, const char *interface);
unsigned short checksum(void *b, int len);

unsigned short checksum(void *b, int len) {
    unsigned short *buf = b;
    unsigned int sum = 0;
    unsigned short result;

    for (sum = 0; len > 1; len -= 2) {
        sum += *buf++;
    }
    if (len == 1) {
        sum += *(unsigned char *)buf;
    }
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

void send_icmp_echo(int sockfd, const char *src_ip, const char *dst_ip,
                    const unsigned char *src_mac, const unsigned char *dst_mac,
                    int id, int seq, const char *interface) {
    unsigned char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    // Construct Ethernet header
    struct ether_header *eth_hdr = (struct ether_header *)buffer;
    memcpy(eth_hdr->ether_shost, src_mac, ETH_ALEN);
    memcpy(eth_hdr->ether_dhost, dst_mac, ETH_ALEN);
    eth_hdr->ether_type = htons(ETH_P_CUSTOM);

    // Construct IP header
    struct iphdr *ip_hdr = (struct iphdr *)(buffer + sizeof(struct ether_header));
    ip_hdr->version = 4;
    ip_hdr->ihl = 5;
    ip_hdr->tos = 0;
    ip_hdr->tot_len = htons(sizeof(struct iphdr) + sizeof(struct icmphdr) + 10); // Payload size: 10 bytes
    ip_hdr->id = htons(54321);
    ip_hdr->frag_off = 0;
    ip_hdr->ttl = 64;
    ip_hdr->protocol = IPPROTO_ICMP;
    ip_hdr->saddr = inet_addr(src_ip);
    ip_hdr->daddr = inet_addr(dst_ip);
    ip_hdr->check = checksum(ip_hdr, sizeof(struct iphdr));

    // Construct ICMP header
    struct icmphdr *icmp_hdr = (struct icmphdr *)(buffer + sizeof(struct ether_header) + sizeof(struct iphdr));
    icmp_hdr->type = ICMP_ECHO;
    icmp_hdr->code = 0;
    icmp_hdr->un.echo.id = htons(id);
    icmp_hdr->un.echo.sequence = htons(seq);
    icmp_hdr->checksum = 0;

    // Add payload
    unsigned char *payload = buffer + sizeof(struct ether_header) + sizeof(struct iphdr) + sizeof(struct icmphdr);
    memcpy(payload, "0123456789", 10);

    // Calculate ICMP checksum
    icmp_hdr->checksum = checksum(icmp_hdr, sizeof(struct icmphdr) + 10);

    // Send frame
    struct sockaddr_ll socket_address;
    memset(&socket_address, 0, sizeof(socket_address));
    
    socket_address.sll_ifindex = if_nametoindex(interface); // Interface index
    socket_address.sll_halen = ETH_ALEN;                   // MAC address length
    memcpy(socket_address.sll_addr, dst_mac, ETH_ALEN);     // Destination MAC address

    if (sendto(sockfd, buffer,
               sizeof(struct ether_header) + ntohs(ip_hdr->tot_len),
               0,
               (struct sockaddr *)&socket_address,
               sizeof(socket_address)) < 0) {
        perror("sendto");
        exit(EXIT_FAILURE);
    }
}

int main() {
    int sockfd;
    
    // Create raw socket
    if ((sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Example parameters for HostA sending to HostC
    const char *src_ip = "10.0.0.1";
    const char *dst_ip = "10.0.0.3";
    
    unsigned char src_mac[6] = {0x00, 0x00, 0x00, 0xaa, 0xaa, 0xaa};
    unsigned char dst_mac[6] = {0x00, 0x00, 0x00, 0xcc, 0xcc, 0xcc};
    
    send_icmp_echo(sockfd, src_ip, dst_ip,
                   src_mac, dst_mac,
                   1 /* ID */, 
                   1 /* Sequence Number */, 
                   "eth0" /* Interface */);

    close(sockfd);
    
    return 0;
}
