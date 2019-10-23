#ifndef ATTR_API_H
#define ATTR_API_H

#include <stdint.h>
//#include "oi_shm.h"
//编译 /mydata/HT_SERVER_IM/Code_newfw/framework/neocomm
//gcc -c Attr_API.c
//ar crv libneocomm.a Attr_API.o
//scp libneocomm.a root@qtest.hellotalk.org:/home/git/HT_GOGO/gotcp/libcomm/libneocomm.a
//
#define ATTR_SHM_ID_HEX  	0x9527  //共享内存的id
#define ATTR_SHM_ID_DEC  	38183   //十进制表示
#define MAX_ATTR_NODE		1000	//单机最大支持属性个数

#define ATTR_SHM_ID_BIGGER_HEX  	0x900866  //共享内存的id
#define ATTR_SHM_ID_BIGGER_DEC  	9439334   //十进制表示 
#define MAX_ATTR_NODE_MAX		256	//每个区间最大节点数量
#define MAX_ATTR_NODE_BLOCK		500	//分多少个区间
#define ATTR_NAME_LEN		64		//属性名称的最大长度

#define VERSION			"1.2(增加内存点到50000个 )"


enum MONITOR_ATTR_TYPE
{
    EM_TYPE_COUNTER     =   0,      //累加
    EM_TYPE_AVG         =   1,      //平均
    EM_TYPE_STAT        =   2,      //状态
};

#pragma pack(1)
typedef struct
{
	uint8_t ucUse;						//是否使用
	uint32_t uiAttrID;					//属性ID
	char acName[ATTR_NAME_LEN];			//属性名称
	uint8_t ucType;						//属性类型
	int iCurValue;						//当前计数值
	int iTimes;							//次数，用于计算平均值
} AttrNode;
#pragma pack()

typedef struct
{
	AttrNode astNode[MAX_ATTR_NODE]; 
} AttrList;

//每个区间从 0 开始使用，参看SearchAttrIDForBigger
typedef struct
{
	AttrNode astNode[MAX_ATTR_NODE_BLOCK][MAX_ATTR_NODE_MAX];
} AttrListBigger;

 
//回调函数返回0 = 则 结束当前循环，如果返回1，则清理数值
//blockIdx 描述的是当前是第几个区块，
//blockSeqId 描述是当前区块的第几个元素
typedef int (*fn_callback_attr_list)(uint32_t blockIdx,uint32_t blockSeqId,uint32_t uiAttr ,const char* acName,uint8_t ucType,int iCurValue,int iTimes); // 回调函数的名称为 fnc2，参数是 char *是一个

int CheckAllBiggerAttr(fn_callback_attr_list callback);
int CheckAllOldAttr(fn_callback_attr_list callback);

//初始化内存
int InitBiggerAttr(int iZero);

//初始化老的内存
int InitOldAttr(int iZero);

int AttrAddBigger(const char* pName,int iValue);
int AttrAddAvgBigger(const char* pName,int iValue);
int AttrSetBigger(const char* pName,int iValue);

int AttrAdd(const char* pName,int iValue);//iValue为累加值
int AttrAddAvg(const char* pName,int iValue);//iValue为累加值
int AttrSet(const char* pName,int iValue);//直接将iValue赋给共享内存
int GetAttrValue(const char* pName,int *iValue);//获得属性ID为attr的值，如果不存在返回-1,存在返回0并附给iValue;
void GetAPIversion();

int AttrBiggerChange(const char* pName, int iValue,int iUcType ,char * ReturnMsg,int ReturnMsgSize);

 

#endif
