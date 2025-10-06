#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

//Kiểm tra xem có phải địa chỉ IP hợp lệ không
int is_valid_ip(const char *ip){
    if (inet_addr(ip) == INADDR_NONE) {
        return 0; // Trả về 0 nếu không hợp lệ
    }
    
    // Check xem có đúng format IPv4 không, logic: check đủ 3 dấu chấm giữa các chữ số
    int dots = 0;
    int len = strlen(ip);
    for(int i = 0; i < len; i++) {
        if(ip[i] == '.') dots++;
        else if(ip[i] < '0' || ip[i] > '9') return 0;
    }

    return (dots == 3);
}

//Kiểm tra xem có phải định dạng IP không: tránh trường hợp lẫn tên miền với địa chỉ IP
int looks_like_ip(const char *str) {
    int len = strlen(str);
    int dots = 0;
    int has_digit = 0;
    
    for(int i = 0; i < len; i++) {
        if(str[i] == '.') dots++;
        else if(str[i] >= '0' && str[i] <= '9') has_digit = 1;
        else return 0; // Gặp ký tự không phải số hoặc dấu chấm thì không phải IP
    }
    
    return (dots > 0 && has_digit); // Phải có ít nhất một dấu chấm và một chữ số
}

//Phân giải tên miền
void resolve_ip_to_domain(const char *ip){
    struct in_addr addr;
    if(!is_valid_ip(ip)){
        printf("Invalid address\n");
        return;
    }
    
    // Chuyển IP string sang struct in_addr
    addr.s_addr = inet_addr(ip);

    struct hostent *host = gethostbyaddr((const char *)&addr, sizeof(addr), AF_INET);
    if (host == NULL) {
        printf("Not found information\n");
        return;
    }

    printf("Official name: %s\n", host->h_name);
    printf("Alias name:\n");
    if (host->h_aliases[0] == NULL) {
        printf("None\n");
    } else {
        for (int i = 0; host->h_aliases[i] != NULL; i++) {
            printf("%s\n", host->h_aliases[i]);
        }
    }
}

//Phân giải tên miền sang IP
void resolve_domain_to_ip(const char *domain){
    struct hostent *host = gethostbyname(domain);
    if (host == NULL) {
        printf("Not found information\n");
        return;
    }
    
    printf("Official IP: %s\n", inet_ntoa(*(struct in_addr *)host->h_addr));
    printf("Alias IP:\n");
    
    // In tất cả các địa chỉ IP 
    for (int i = 1; host->h_addr_list[i] != NULL; i++) {
        printf("%s\n", inet_ntoa(*(struct in_addr *)host->h_addr_list[i]));
    }
}


int main(int argc, char *argv[]){
    if (argc != 2) {
        printf("Usage: %s <domain_or_ip>\n", argv[0]);
        return 1;
    }

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }

    char *input = argv[1];
    // Nếu đúng định dạng IP thì phân giải ngược, nếu không thì phân giải tên miền
    if (is_valid_ip(input)) {
        resolve_ip_to_domain(input);
    } else if (looks_like_ip(input)) {
        printf("Invalid address\n");
    } else {
        resolve_domain_to_ip(input);
    }

    WSACleanup();
    return 0;
}