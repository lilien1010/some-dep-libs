 
#ifndef __TEACRYPOT_H__
#define __TEACRYPOT_H__


void tinyEncrypt ( const uint32_t * plain, const uint32_t * key, uint32_t *crypt, unsigned int power);
void tinyDecrypt ( const unsigned int * crypt, const unsigned int * key, unsigned int *plain, unsigned int power);

int xTEAEncryptWithKey(const char *plain, uint32_t plain_len, const unsigned char key[16], char *crypt, uint32_t * crypt_len);
int xTEADecryptWithKey(const char *crypt, uint32_t crypt_len, const unsigned char key[16], char *plain, uint32_t * plain_len);



#endif

 