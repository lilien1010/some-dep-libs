

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <stdint.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h> 

#if LUA_VERSION_NUM >= 502

#define luaL_register(L,n,f) luaL_newlib(L,f)

#endif

#define true 	1
#define false 	0

#define MAX_ENC_LEN	1048576
#define TEA_DELTA	0x9E3779B9
#define TEA_SUM		0xE3779B90

void static tinyEncrypt ( const uint32_t * plain, const uint32_t * key, uint32_t *crypt, unsigned int power)
{
    uint32_t y,z,a,b,c,d;
    uint32_t sum = 0;
    unsigned int i;
    unsigned int rounds = 1<<power;

    y = plain[0];
    z = plain[1];
    a = key[0];
    b = key[1];
    c = key[2];
    d = key[3];

    for (i = 0; i < rounds; i++) {
        sum += TEA_DELTA;
        y += ((z << 4) + a) ^ (z + sum) ^ ((z >> 5) + b);
        z += ((y << 4) + c) ^ (y + sum) ^ ((y >> 5) + d);
    }

    crypt[0] = y;
    crypt[1] = z;
}

void static tinyDecrypt ( const unsigned int * crypt, const unsigned int * key, unsigned int *plain, unsigned int power)
{
    unsigned int y,z,a,b,c,d;
    unsigned int rounds = 1<<power;
    unsigned int sum = TEA_DELTA<<power;
    unsigned int i;

    y = crypt[0];
    z = crypt[1];
    a = key[0];
    b = key[1];
    c = key[2];
    d = key[3];

    for (i = 0; i < rounds; i++) {
        z -= ((y << 4) + c) ^ (y + sum) ^ ((y >> 5) + d);
        y -= ((z << 4) + a) ^ (z + sum) ^ ((z >> 5) + b);
        sum -= TEA_DELTA;
    }

    plain[0] = y;
    plain[1] = z;
}

/*================================================================
*
* �� �� ����xTEADecryptWithKey
** Decrypt the cipher text to plain text with the key

* �� ����
*
* const unsigned long *crypt [IN] : the Cipher text
* DWORD dwCryptLen[IN]: cipher text length
* const unsigned long *theKey [IN] : the key
* DWORD dwKeyLen[IN]: key length
* unsigned long *plain [[IN,OUT]] : the pointer to plain text(net order unsigned char)
* DWORD * pdwPlainLen[IN,OUT]: Valid plain text length
*
* �� �� ֵ��int-	SUCCESS:true
*							Fail:NULL
*
*
================================================================*/
int static xTEADecryptWithKey(const char *crypt, uint32_t crypt_len, const unsigned char key[16], char *plain, uint32_t * plain_len)
{
	if(crypt == NULL || plain == NULL)
		return 1;
	const uint32_t *tkey   = (const uint32_t *)key;
	const uint32_t *tcrypt = (const uint32_t *)crypt;

	if( crypt_len<1 || crypt_len%8 )
		return 2;
	int alloLen =	crypt_len/4+16;
	int *tplain = (int*)malloc(sizeof(int)*alloLen);
	memset(tplain,0,sizeof(int)*alloLen);
	/*int *tplain = (int*)plain;*/

	uint32_t  length = crypt_len;
	uint32_t pre_plain[2] = {0,0};
	uint32_t p_buf[2] = {0};
	uint32_t c_buf[2] = {0};

	int padLength = 0;
	int i = 0;

	//Decrypt the first 8 unsigned chars(64 bits)
	tinyDecrypt(tcrypt, tkey, p_buf, 4);

	memcpy(pre_plain, p_buf, 8);
	memcpy(tplain, p_buf, 8);

	//Decrype with TEA and interlace algorithm
	for (i = 2; i < (int)length/4; i+=2) {
		c_buf[0] = *(tcrypt+i) ^ pre_plain[0];
		c_buf[1] = *(tcrypt+i+1) ^ pre_plain[1];
		tinyDecrypt((const uint32_t *)c_buf, tkey, p_buf, 4);
		memcpy(pre_plain, p_buf, 8);
		*(tplain+i) = p_buf[0] ^ *(tcrypt+i-2);
		*(tplain+i+1) = p_buf[1] ^ *(tcrypt+i-1);
	}

	//check the last 7 unsigned chars is 0x00
	if ( tplain[length/4-1] || (tplain[length/4-2]&0xffffff00))
	{
		/**/free(tplain);
		tplain = NULL;
		return 3;
	}

	padLength = *((unsigned char *)tplain) & 0x07;

	length = (length / 4 + 1)*4 - (padLength+3);  //add by zyf
	//Remove padding data
	memcpy(tplain,(unsigned char*)tplain+padLength+3,length);

	*plain_len = crypt_len - (padLength+3) -7;/*(pad 7 unsigned chars 0x00 at the end)*/

	/**/memcpy(plain,tplain,*plain_len);


	/**/free(tplain); tplain = NULL;

	return 0;

}
/*================================================================
*
* �� �� ����xTEAEncryptWithKey
** Encrypt the plain text to cipher text with the key

* �� ����
*
* const unsigned long *plain [IN] : the plain text
* DWORD dwPlainLen[IN]: plain text length
* const unsigned long *theKey [IN] : the key
* DWORD dwKeyLen[IN]: key length
* unsigned long *crypt [[IN,OUT]] : the pointer to cipher text(net order unsigned char)
* DWORD * pdwCryptLen[IN,OUT]: Valid cipher text length
*
* �� �� ֵ��int-	SUCCESS:true
*							Fail:NULL
*
*
================================================================*/

int static xTEAEncryptWithKey(const char *plain, uint32_t plain_len, const unsigned char key[16], char *crypt, uint32_t * crypt_len )
{
	if(plain == NULL || crypt == NULL)
		return 1;
	const unsigned char pad[9] = {0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad,0xad};

	uint32_t *tkey = (uint32_t *)key;
	uint32_t *tplain = (uint32_t *)plain;

	if ( plain_len<1 )
	{
		return 2;
	}

	uint32_t pre_plain[2] = {0,0};
	uint32_t pre_crypt[2] = {0,0};
	uint32_t p_buf[2] = {0};
	uint32_t c_buf[2] = {0};

	int padLength = 0;
	int i = 0;

	// padding data
	padLength = (plain_len+10)%8;//at least pad 2 unsigned chars
	padLength = padLength ? 8 - padLength : 0;//total pad length -2

	int length = padLength+3+plain_len+7;
	*crypt_len = length;

	/**/int alloLen 	=	length/4+16;
	
	int *tcrypt = (int*)malloc(sizeof(int)*alloLen);
	memset(tcrypt,0,sizeof(int)*alloLen);
	
	/*
		int *tcrypt =(int*)plain;  
		memset(tcrypt,0,sizeof(int)*alloLen);
	*/

	*((unsigned char*)tcrypt) = 0xa8 | (unsigned char)padLength;//first pad unsigned char: total padding unsigned chars - 2 or 0xa8
	memcpy ( (unsigned char*)tcrypt+1, (unsigned char*)pad, padLength+2);//add other padding data
	memcpy ( (unsigned char*)tcrypt+padLength+3, (unsigned char*)tplain, plain_len);//add plain data
	memset ( (unsigned char*)tcrypt+padLength+3+plain_len, 0, 7);  //pad 7 0x00 at the end

	//Interlace algorithm(��֯�㷨)
	for (i = 0; i < length/4; i+=2) {
		p_buf[0] = *(tcrypt+i) ^ pre_crypt[0];
		p_buf[1] = *(tcrypt+i+1) ^ pre_crypt[1];
		tinyEncrypt( p_buf, tkey, c_buf, 4);
		*(tcrypt+i) = c_buf[0] ^ pre_plain[0];
		*(tcrypt+i+1) = c_buf[1] ^ pre_plain[1];
		memcpy(pre_crypt, tcrypt+i, 8);
		memcpy(pre_plain, p_buf, 8);
	}

	/**/
	memcpy(crypt,tcrypt,length);

	free(tcrypt);
	tcrypt = NULL;
	
	
	return 0;

}

static int mm_encrpyt(lua_State *L)
{
	const char *data 	= 	luaL_checkstring(L, 1);
	const char *key 	= 	luaL_checkstring(L, 2);
	size_t 		data_len 	=	lua_objlen (L,1);
	size_t 		key_len 	=	lua_objlen (L,2);
	uint32_t   crypt_len =	0;
	if(key_len != 16  ){
		lua_pushnil(L);
		lua_pushstring(L,"key length error"); 
		return 2;
	}
	
	char * _enc  	=	NULL;
	int  need_free 	=	0;
	
	if( data_len>=MAX_ENC_LEN){
		/*lua_pushnil(L);
		lua_pushstring(L,"data length error"); 
		return 2;*/ 
		_enc = (char*)malloc(data_len+1);
		memset(_enc,0,data_len+1);
		need_free	=	1;
	}else{
		
		char buf[MAX_ENC_LEN]={0};
		_enc	=	buf;
	}
	 
	
	int ret 	=	xTEAEncryptWithKey(data,(uint32_t)data_len,(const unsigned char *)key,(char *)_enc,&crypt_len);
	
	if( ret == 0  ){
		lua_pushlstring(L,_enc,crypt_len);
		if(need_free==1 && _enc){
				free(_enc);_enc=NULL;
		}
		return 1;
	} 
	if(need_free==1 && _enc){
			free(_enc);_enc=NULL;
	}
	
	lua_pushnil(L);
	lua_pushstring(L,"fail to encrypt");
	lua_pushnumber(L,ret);
	
	return 3;
}



static int mm_decrpyt(lua_State *L)
{
	const char *data 	= 	luaL_checkstring(L, 1);
	const char *key 	= 	luaL_checkstring(L, 2);
	size_t 		data_len 	=	lua_objlen (L,1);
	size_t 		key_len 	=	lua_objlen (L,2);
	
	if(key_len != 16 ){ 
		lua_pushnil(L);
		lua_pushstring(L,"key length error");
		return 2;
	}
	
 	char * _enc  	=	NULL;
	int  need_free 	=	0;
	if( data_len>=MAX_ENC_LEN){
		/*lua_pushnil(L);
		lua_pushstring(L,"data length error"); 
		return 2;*/ 
		_enc = (char*)malloc(data_len+1);
		memset(_enc,0,data_len+1);
		need_free	=	1;
	}else{
		
		char buf[MAX_ENC_LEN]={0};
		_enc	=	buf;
	}
	 
	
	uint32_t   crypt_len =	0;
	 
	int ret 	=	xTEADecryptWithKey(data,(uint32_t)data_len,(const unsigned char *)key,_enc,&crypt_len);
	
	if( ret == 0  ){
		lua_pushlstring(L,_enc,crypt_len);
		if(need_free==1 && _enc){
				free(_enc);_enc=NULL;
		}
		return 1;
	}
	if(need_free==1 && _enc){
			free(_enc);_enc=NULL;
	}
	

 	char acName[256] = {0};

 	int writesize =  snprintf(acName,256-1,"fail to decrpyt,error code=%d",ret);
	 
	lua_pushnil(L); 
	lua_pushlstring(L,acName,writesize);  

	return 2;
}
/*
root@qcloud210:/home/git/webapi/lua/lib/libteacrypto# 
gcc xteacrypt.c -fPIC -shared -o xteacrypt.so -Wall -I/usr/local/openresty/luajit/include/luajit-2.1 -L/usr/local/openresty/luajit/lib/  -lluajit-5.1 */
 
LUALIB_API int luaopen_lib_xteacrypt(lua_State * L)
{
	static const luaL_Reg api[] = {
		{ "encrypt", mm_encrpyt },
		{ "decrypt", mm_decrpyt },
		{ 0, 0 },
	};

	luaL_register(L, "xteacrypt", api); 
	return 1;
}
