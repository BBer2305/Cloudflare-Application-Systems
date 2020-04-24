#include <chrono>
#include <fcntl.h>
#include <iostream>
#include <netdb.h>
#include <netinet/ip_icmp.h>
#include <strings.h>
#include <unistd.h>

using namespace std;
using std::chrono::system_clock;

struct packet {
    struct icmphdr head;
    char msg[64-sizeof(struct icmphdr)];
};

short checksum(void *b, int len) { //checksum
    short *buf =(short *) b;
    int sum=0;

    for (sum = 0; len > 1; len -= 2 ) {
        sum += *buf++;
    }
    if (len == 1 ) {
        sum += *(char*)buf;
    }
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    return ~sum;
}

int ping(char *address) {
    const int val=255;
    int cnt=1;
    struct packet pkt;
    struct sockaddr_in r_addr, addr_ping;

    int pid = getpid();
    struct protoent *proto = getprotobyname("ICMP");
    struct hostent *host = gethostbyname(address);

    bzero(&addr_ping, sizeof(addr_ping));
    addr_ping.sin_addr.s_addr = *(long*)host->h_addr;
    addr_ping.sin_family = host->h_addrtype;
    addr_ping.sin_port = 0;

    struct sockaddr_in *addr = &addr_ping;

    int sd = socket(PF_INET, SOCK_RAW, proto->p_proto);
    if (sd < 0) { //check if socket can be established
        perror("socket");
        return 1;
    }
    else if (setsockopt(sd, SOL_IP, IP_TTL, &val, sizeof(val)) != 0) {
        perror("Set TTL option");
        return 1;
    }
    else if (fcntl(sd, F_SETFL, O_NONBLOCK) != 0 ) {
        perror("Request nonblocking I/O");
        return 1;
    }
    
    int sent = 0, received = 0, i = 0;
    while (true) { //infinite loop: receive packet, send packet, print results
        int len=sizeof(r_addr);

        system_clock::time_point start = system_clock::now();
        if (!(recvfrom(sd, &pkt, sizeof(pkt), 0, (struct sockaddr*)&r_addr, (socklen_t*)&len) > 0)) {
            received++;
        }
        system_clock::time_point end = system_clock::now();
        auto latency = end-start;

        bzero(&pkt, sizeof(pkt));
        pkt.head.type = ICMP_ECHO;
        pkt.head.un.echo.id = pid;
        for (i = 0; i < sizeof(pkt.msg)-1; i++) {
            pkt.msg[i] = i + '0';
        }
        pkt.msg[i] = 0;
        pkt.head.checksum = checksum(&pkt, sizeof(pkt));
        pkt.head.un.echo.sequence = cnt++;

        if (sendto(sd, &pkt, sizeof(pkt), 0, (struct sockaddr*)addr, sizeof(*addr)) < 0 ) {
            perror("sendto");
            return 1;
        }
        else {
            sent++;
        }
        
        printf("%s%f%s%d%s%d%s%d%s\n", "Latency: ", ((double)latency.count() / 1000000), " seconds, Packets sent: ", sent, ", Packets received: ", received, ", Packets lost: ", sent-received, ".");
        usleep(500000);

    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Error: Exactly One Argument Should Be Provided.\n");
        return 0;
    }

    ping(argv[1]);
    return 0;
}