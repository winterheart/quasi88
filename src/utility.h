#ifndef UTILITY_H
#define UTILITY_H

void sjis2euc(char *euc_p, const char *sjis_p);
void euc2sjis(char *sjis_p, const char *euc_p);
int euclen(const char *euc_p);
int my_strcmp(const char *s, const char *d);
void my_strncpy(char *s, const char *ct, unsigned long n);
void my_strncat(char *s, const char *ct, unsigned long n);
char *my_strtok(char *dst, char *src);

#endif // UTILITY_H
