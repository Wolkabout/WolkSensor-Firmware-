#ifndef INET_H_
#define INET_H_

uint32_t inet_aton(const char *cp);
uint32_t inet_pton_ipv4(const char *src, unsigned char *dst);

char *inet_ntoa(uint32_t ip_address);


#endif /* INET_H_ */