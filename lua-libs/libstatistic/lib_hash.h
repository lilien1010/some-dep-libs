#if !defined(__FNV_H__)
#define __FNV_H__


#ifdef  __cplusplus
extern "C" {
#endif


/* * 32 bit FNV-0 hash type */
//typedef unsigned long Fnv32_t;
typedef unsigned int Fnv32_t;

#define FNV1_32_INIT ((Fnv32_t)0x811c9dc5)
extern Fnv32_t fnv_32_buf(void *buf, size_t len, Fnv32_t hashval);
extern Fnv32_t fnv_32_str(char *buf, Fnv32_t hashval);


#ifdef  __cplusplus
}
#endif


#endif
