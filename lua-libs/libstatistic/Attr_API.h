#ifndef ATTR_API_H
#define ATTR_API_H

#include <stdint.h>
//#include "oi_shm.h"
//���� /mydata/HT_SERVER_IM/Code_newfw/framework/neocomm
//gcc -c Attr_API.c
//ar crv libneocomm.a Attr_API.o
//scp libneocomm.a root@qtest.hellotalk.org:/home/git/HT_GOGO/gotcp/libcomm/libneocomm.a
//
#define ATTR_SHM_ID_HEX  	0x9527  //�����ڴ��id
#define ATTR_SHM_ID_DEC  	38183   //ʮ���Ʊ�ʾ
#define MAX_ATTR_NODE		1000	//�������֧�����Ը���

#define ATTR_SHM_ID_BIGGER_HEX  	0x900866  //�����ڴ��id
#define ATTR_SHM_ID_BIGGER_DEC  	9439334   //ʮ���Ʊ�ʾ 
#define MAX_ATTR_NODE_MAX		256	//ÿ���������ڵ�����
#define MAX_ATTR_NODE_BLOCK		500	//�ֶ��ٸ�����
#define ATTR_NAME_LEN		64		//�������Ƶ���󳤶�

#define VERSION			"1.2(�����ڴ�㵽50000�� )"


enum MONITOR_ATTR_TYPE
{
    EM_TYPE_COUNTER     =   0,      //�ۼ�
    EM_TYPE_AVG         =   1,      //ƽ��
    EM_TYPE_STAT        =   2,      //״̬
};

#pragma pack(1)
typedef struct
{
	uint8_t ucUse;						//�Ƿ�ʹ��
	uint32_t uiAttrID;					//����ID
	char acName[ATTR_NAME_LEN];			//��������
	uint8_t ucType;						//��������
	int iCurValue;						//��ǰ����ֵ
	int iTimes;							//���������ڼ���ƽ��ֵ
} AttrNode;
#pragma pack()

typedef struct
{
	AttrNode astNode[MAX_ATTR_NODE]; 
} AttrList;

//ÿ������� 0 ��ʼʹ�ã��ο�SearchAttrIDForBigger
typedef struct
{
	AttrNode astNode[MAX_ATTR_NODE_BLOCK][MAX_ATTR_NODE_MAX];
} AttrListBigger;

 
//�ص���������0 = �� ������ǰѭ�����������1����������ֵ
//blockIdx �������ǵ�ǰ�ǵڼ������飬
//blockSeqId �����ǵ�ǰ����ĵڼ���Ԫ��
typedef int (*fn_callback_attr_list)(uint32_t blockIdx,uint32_t blockSeqId,uint32_t uiAttr ,const char* acName,uint8_t ucType,int iCurValue,int iTimes); // �ص�����������Ϊ fnc2�������� char *��һ��

int CheckAllBiggerAttr(fn_callback_attr_list callback);
int CheckAllOldAttr(fn_callback_attr_list callback);

//��ʼ���ڴ�
int InitBiggerAttr(int iZero);

//��ʼ���ϵ��ڴ�
int InitOldAttr(int iZero);

int AttrAddBigger(const char* pName,int iValue);
int AttrAddAvgBigger(const char* pName,int iValue);
int AttrSetBigger(const char* pName,int iValue);

int AttrAdd(const char* pName,int iValue);//iValueΪ�ۼ�ֵ
int AttrAddAvg(const char* pName,int iValue);//iValueΪ�ۼ�ֵ
int AttrSet(const char* pName,int iValue);//ֱ�ӽ�iValue���������ڴ�
int GetAttrValue(const char* pName,int *iValue);//�������IDΪattr��ֵ����������ڷ���-1,���ڷ���0������iValue;
void GetAPIversion();

int AttrBiggerChange(const char* pName, int iValue,int iUcType ,char * ReturnMsg,int ReturnMsgSize);

 

#endif
