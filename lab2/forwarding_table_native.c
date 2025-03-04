#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/socket.h>
#include <errno.h>

#define BUFFER_SIZE 8192

// Utility function to create a Netlink socket
int create_netlink_socket() {
    int sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (sock < 0) {
        perror("Failed to create Netlink socket");
        exit(EXIT_FAILURE);
    }
    return sock;
}

// Utility function to send Netlink messages
int send_netlink_message(int sock, struct nlmsghdr *nlh) {
    struct sockaddr_nl addr = {0};
    addr.nl_family = AF_NETLINK;

    struct iovec iov = {nlh, nlh->nlmsg_len};
    struct msghdr msg = {&addr, sizeof(addr), &iov, 1, NULL, 0, 0};

    if (sendmsg(sock, &msg, 0) < 0) {
        perror("Failed to send Netlink message");
        return -1;
    }
    return 0;
}

// Utility function to receive Netlink messages
int receive_netlink_message(int sock, char *buffer) {
    struct sockaddr_nl addr;
    struct iovec iov = {buffer, BUFFER_SIZE};
    struct msghdr msg = {&addr, sizeof(addr), &iov, 1, NULL, 0, 0};

    int len = recvmsg(sock, &msg, 0);
    if (len < 0) {
        perror("Failed to receive Netlink message");
        return -1;
    }
    return len;
}

// Add a route entry
void add_route(const char *prefix, const char *next_hop) {
    int sock = create_netlink_socket();

    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    struct nlmsghdr *nlh = (struct nlmsghdr *)buffer;
    struct rtmsg *rtm = (struct rtmsg *)(buffer + sizeof(struct nlmsghdr));
    
    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
    nlh->nlmsg_type = RTM_NEWROUTE;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_REPLACE;
    nlh->nlmsg_seq = 1;
    
    rtm->rtm_family = AF_INET; // IPv4
    rtm->rtm_table = RT_TABLE_MAIN;
    rtm->rtm_protocol = RTPROT_BOOT;
    rtm->rtm_scope = RT_SCOPE_UNIVERSE;
    rtm->rtm_type = RTN_UNICAST;

    // Add destination prefix
    struct rtattr *rta_dst = (struct rtattr *)(buffer + NLMSG_ALIGN(nlh->nlmsg_len));
    rta_dst->rta_type = RTA_DST;
    
    struct in_addr dst_addr;
    inet_pton(AF_INET, prefix, &dst_addr);
    
    memcpy(RTA_DATA(rta_dst), &dst_addr, sizeof(dst_addr));
    
}

// Delete a route entry
void remove_route(const char *prefix) {
}

// Flush all routes
void flush_routes() {
}

int main() {
}
