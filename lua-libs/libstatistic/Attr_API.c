#include <stdio.h>
#include <string.h>
#include "Attr_API.h"
#include "lib_hash.h"
#include "oi_shm.h"

static AttrList *pstAttr=NULL;

//支持5w指标的指针 二维
static AttrListBigger *pstAttrBigger=NULL;

void GetAPIversion()
{
	printf("version:%s\n",VERSION);
}

//支持分区的指标搜索，blockIndex 分区下标
int SearchAttrIDForBigger(AttrListBigger *pstAttrBigger,uint32_t uiAttr,int blockIndex,int *piPos)
{
	int i = 0;
	*piPos = 0;
 
	while(pstAttrBigger->astNode[blockIndex][i].ucUse)
	{
		if(pstAttrBigger->astNode[blockIndex][i].uiAttrID == uiAttr)
		{
			*piPos=i;
			return 1;
		}
		++i;
		if(i >= MAX_ATTR_NODE_MAX)
		{
			return -1;
		}
	}
	
	*piPos = i;
	
	return 0;
}

//一维的列表搜索，老的
int SearchAttrID(AttrList *pstAttr,uint32_t uiAttr,int *piPos)
{
	int i = 0;
	*piPos = 0;
	while(pstAttr->astNode[i].ucUse)
	{
		if(pstAttr->astNode[i].uiAttrID == uiAttr)
		{
			*piPos=i;
			return 1;
		}
		++i;
		if(i >= MAX_ATTR_NODE)
		{
			return -1;
		}
	}
	
	*piPos = i;
	
	return 0;
}


//功能和原来的一样，只是使用了一块新的 更大的内存，支持30000个指标
//iUcType ,0 表示添加，1=表示设置平均，2=表示设置
int AttrBiggerChange(const char* pName, int iValue,int iUcType ,char * ReturnMsg,int ReturnMsgSize)
{
	if(!pName)
	{
		return -1;
	}
	int iLen = strlen(pName);
	if(iLen <= 0 )
    {   
        return -2; 
    } 
	
	int iAttrPos = 0;
	int shang 	= iLen+pName[0]+pName[iLen-1];
	//通过首尾字母和长度 构造不一样的因子，增加熵的已一致性，避免熵一样，都落在统一block
	uint32_t uiAttr = fnv_32_str((char*)pName, shang);
 
	if (!pstAttrBigger)
	{
		//变更函数不在遍历，一定要初始化内存。
		if(GetShm2((void**)&pstAttrBigger, ATTR_SHM_ID_BIGGER_DEC, sizeof(AttrListBigger), 0666) < 0)
		{
			if( ReturnMsg != NULL && ReturnMsgSize > 0 ){
				 snprintf(ReturnMsg,ReturnMsgSize,"uiAttr=%u,iUcType=%d  can not get shm %s",uiAttr,iUcType,pName);
			}

			return -1;
		}else{
			if( ReturnMsg != NULL && ReturnMsgSize > 0 ){
				snprintf(ReturnMsg,ReturnMsgSize,"create success\n");
			}
		}
	}
	
	int blockIndex 	=	uiAttr % MAX_ATTR_NODE_BLOCK;
	int iRet = SearchAttrIDForBigger(pstAttrBigger,uiAttr,blockIndex,&iAttrPos);
	if(iRet < 0)
	{
		if( ReturnMsg != NULL && ReturnMsgSize > 0 ){
			snprintf(ReturnMsg,ReturnMsgSize,"uiAttr=%u,iUcType=%d out of memery when %s\n",uiAttr,iUcType,pName);
		}
		return -1;
	}
	else if(iRet > 0)//使用老的节点，修改变量
	{	
		if ( EM_TYPE_AVG==iUcType ){  
			pstAttrBigger->astNode[blockIndex][iAttrPos].iCurValue += iValue;
			pstAttrBigger->astNode[blockIndex][iAttrPos].iTimes += 1;   
		}else if (EM_TYPE_STAT==iUcType) {
			pstAttrBigger->astNode[blockIndex][iAttrPos].iCurValue = iValue;  
		}else{ 
			pstAttrBigger->astNode[blockIndex][iAttrPos].iCurValue += iValue; 
		}

		pstAttrBigger->astNode[blockIndex][iAttrPos].ucType = iUcType;
		pstAttrBigger->astNode[blockIndex][iAttrPos].uiAttrID = uiAttr;
		
		if(uiAttr != fnv_32_str(pstAttrBigger->astNode[blockIndex][iAttrPos].acName, shang))//如果数据一致，重新覆盖一份
		{
			int iCopyLen = iLen >= ATTR_NAME_LEN ? ATTR_NAME_LEN - 1 : iLen;
			strncpy(pstAttrBigger->astNode[blockIndex][iAttrPos].acName, pName, iCopyLen);

			if( ReturnMsg != NULL && ReturnMsgSize > 0 ){
				snprintf(ReturnMsg,ReturnMsgSize,"%s covered %s",pName,pstAttrBigger->astNode[blockIndex][iAttrPos].acName);
			}

			pstAttrBigger->astNode[blockIndex][iAttrPos].acName[iCopyLen] = '\0';
		}
	}
	else //新建节点
	{

		if ( EM_TYPE_AVG==iUcType  ){  
			pstAttrBigger->astNode[blockIndex][iAttrPos].iTimes = 1;

		}else if (EM_TYPE_STAT  == iUcType ) { 
			pstAttrBigger->astNode[blockIndex][iAttrPos].iTimes = 1;  
		}else{ 
			pstAttrBigger->astNode[blockIndex][iAttrPos].iTimes = 0;
		}
		/*多进程使用时这里会有互斥问题, 所以每次添加时覆盖一遍可以自动修正*/ 
		pstAttrBigger->astNode[blockIndex][iAttrPos].ucUse = 1; 
		pstAttrBigger->astNode[blockIndex][iAttrPos].uiAttrID = uiAttr; 
		pstAttrBigger->astNode[blockIndex][iAttrPos].iCurValue = iValue;
		pstAttrBigger->astNode[blockIndex][iAttrPos].ucType = iUcType;

		int iCopyLen = iLen >= ATTR_NAME_LEN ? ATTR_NAME_LEN - 1 : iLen;
		strncpy(pstAttrBigger->astNode[blockIndex][iAttrPos].acName, pName, iCopyLen);
		pstAttrBigger->astNode[blockIndex][iAttrPos].acName[iCopyLen] = '\0';
	}
	
	return 0;
}


//遍历所有指标，通过回调函数去梳理，回调函数 在Go里面实现了。
int CheckAllBiggerAttr(fn_callback_attr_list callback)   {

	if (!pstAttrBigger){
		return -1;
	}
	uint32_t i = 0;
	uint32_t k = 0;
	for(;i < MAX_ATTR_NODE_BLOCK ; ++i)
	{ 
		k=0;
		for (; k<MAX_ATTR_NODE_MAX ;++k)
		{
			if(pstAttrBigger->astNode[i][k].ucUse==1){ 
				AttrNode *pstNode;
				pstNode = &pstAttrBigger->astNode[i][k];
				int ret = callback(i,k,pstNode->uiAttrID ,(const char* )pstNode->acName,pstNode->ucType,pstNode->iCurValue,pstNode->iTimes);
				if (ret == 0 ){
					return 0;
				}else if (ret ==1){ 
					pstNode->iCurValue = 0;
					pstNode->iTimes = 0;
				}else{
					
				}
			}else{
				//定位到当前区域的未可用区域，终止对该区域的遍历
				break;
			}
	 

		}
	}
	
	return 1;
}


//遍历所有老的指标，通过回调函数去遍历
int CheckAllOldAttr(fn_callback_attr_list callback)   {

	if (!pstAttr){
		return -1;
	}
	uint32_t i = 0; 

	for(;i < MAX_ATTR_NODE && pstAttr->astNode[i].ucUse; ++i)
	{

			AttrNode *pstNode;
			pstNode = & pstAttr->astNode[i];
			int ret = callback(0,i,pstNode->uiAttrID ,(const char* )pstNode->acName,pstNode->ucType,pstNode->iCurValue,pstNode->iTimes);
			if (ret == 0){
				return 0;
			}else if (ret ==1 ){ 
				pstNode->iCurValue = 0;
				pstNode->iTimes = 0;
			}else{

			}
	}
	return 1;
}

//功能和原来的一样，只是使用了一块新的 更大的内存，支持30000个指标
int AttrAddBigger(const char* pName, int iValue)
{
	return AttrBiggerChange(pName,iValue,EM_TYPE_COUNTER,NULL,0);
}

//功能和原来的一样，只是使用了一块新的 更大的内存，支持30000个指标
int AttrAddAvgBigger(const char* pName, int iValue)
{
	return AttrBiggerChange(pName,iValue,EM_TYPE_AVG,NULL,0);
}
//功能和原来的一样，只是使用了一块新的 更大的内存，支持30000个指标
int AttrSetBigger(const char* pName, int iValue)
{
	return AttrBiggerChange(pName,iValue,EM_TYPE_STAT,NULL,0);
}

//共享内从初始化
int InitBiggerAttr(int iZero){
		
		/*加载本机的共享内存，地址是固定的，定义在Attr_API.h里面*/
	pstAttrBigger = (AttrListBigger*)GetShmEx(ATTR_SHM_ID_BIGGER_DEC, sizeof(AttrListBigger), 0666);
	if(!pstAttrBigger)
	{
		pstAttrBigger = (AttrListBigger*)GetShmEx(ATTR_SHM_ID_BIGGER_DEC, sizeof(AttrListBigger), 0666 | IPC_CREAT);
        if(!pstAttrBigger)
        {  
			return -1;
        }
		else
		{
			bzero(pstAttrBigger, sizeof(AttrListBigger));
			return 2;
		}
	}else{
		//由外层决定，如果内存存在的情况下是否清空，清空可以清理一些不用的指标
		if(iZero==1){
			bzero(pstAttrBigger, sizeof(AttrListBigger));
		} 
		return 1;
	}
}


//初始化老的内存
int InitOldAttr(int iZero){
		
	/*加载本机的共享内存，地址是固定的，定义在Attr_API.h里面*/
	pstAttr = (AttrList*)GetShmEx(ATTR_SHM_ID_DEC, sizeof(AttrList), 0666);
	if(!pstAttr)
	{
		pstAttr = (AttrList*)GetShmEx(ATTR_SHM_ID_DEC, sizeof(AttrList), 0666 | IPC_CREAT);
        if(!pstAttr)
        {
             
			return -1;
        }
		else
		{
			bzero(pstAttr, sizeof(AttrList));
			return 2;
		}
	}else{
				//由外层决定，如果内存存在的情况下是否清空，清空可以清理一些不用的指标
		if(iZero==1){
			bzero(pstAttr, sizeof(AttrList));
		} 
		return 1;
	}
 
}

//加载数值 
int GetAttrValuetBigger(const char* pName, int *iValue)
{
	if(!pName)
	{
		return -1;
	}
	int iLen = strlen(pName);
	if(iLen <= 0)
    {   
        return -2; 
    } 
	 
	int iAttrPos = 0;
	//通过收尾字母和长度 构造不一样的 因子
	uint32_t uiAttr = fnv_32_str((char*)pName, iLen+pName[0]+pName[iLen-1]);

	int blockIndex 	=	uiAttr % MAX_ATTR_NODE_BLOCK;
	if (!pstAttrBigger)
	{
		if(GetShm2((void**)&pstAttrBigger, ATTR_SHM_ID_BIGGER_DEC, sizeof(AttrListBigger), 0666) < 0)
		{
			printf("GetAttrValuetBigger can not get shm ...continue!\n");
			return -1;
		}else{
			printf("GetAttrValuetBigger create success\n");
		}
	}
	
	int iRet = SearchAttrIDForBigger(pstAttrBigger, uiAttr,blockIndex, &iAttrPos);
	
	if(!iRet)//返回0 attr不存在，返回1 attr存在
	{
		return -1;
	}
	else if(iRet==-1)
	{
		return -1;
	}
	else
	{
		*iValue=pstAttrBigger->astNode[blockIndex][iAttrPos].iCurValue;
		return 0;
	}
}


int AttrAdd(const char* pName, int iValue)
{
	if(!pName)
	{
		return -1;
	}
	int iLen = strlen(pName);
	if(iLen <= 0 )
    {   
        return -2; 
    } 
	
	int iAttrPos = 0;
	uint32_t uiAttr = fnv_32_str((char*)pName, 0);
	if (!pstAttr)
	{
		if(GetShm2((void**)&pstAttr, ATTR_SHM_ID_DEC, sizeof(AttrList), 0666) < 0)
		{
			printf("AttrAdd can not get shm ...continue!\n");
			return -1;
		}
	}
	
	int iRet = SearchAttrID(pstAttr,uiAttr,&iAttrPos);
	if(iRet < 0)
	{ 
		printf("AttrAdd out of memery %s\n",pName);
		return -1;
	}
	else if(iRet > 0)//返回0 attr不存在，返回1 attr存在
	{
		pstAttr->astNode[iAttrPos].iCurValue += iValue;
		pstAttr->astNode[iAttrPos].uiAttrID = uiAttr;
		pstAttr->astNode[iAttrPos].ucType = EM_TYPE_COUNTER;
		if(uiAttr != fnv_32_str(pstAttr->astNode[iAttrPos].acName, 0))//如果数据一致，重新覆盖一份
		{
			int iCopyLen = iLen >= ATTR_NAME_LEN ? ATTR_NAME_LEN - 1 : iLen;
			strncpy(pstAttr->astNode[iAttrPos].acName, pName, iCopyLen);
			pstAttr->astNode[iAttrPos].acName[iCopyLen] = '\0';
		}
	}
	else
	{
		/*多进程使用时这里会有互斥问题, 所以每次添加时覆盖一遍可以自动修正*/
		pstAttr->astNode[iAttrPos].ucUse = 1;
		pstAttr->astNode[iAttrPos].uiAttrID = uiAttr;
		pstAttr->astNode[iAttrPos].ucType = EM_TYPE_COUNTER;
		pstAttr->astNode[iAttrPos].iCurValue = iValue;
		pstAttr->astNode[iAttrPos].iTimes = 0;
		int iCopyLen = iLen >= ATTR_NAME_LEN ? ATTR_NAME_LEN - 1 : iLen;
		strncpy(pstAttr->astNode[iAttrPos].acName, pName, iCopyLen);
		pstAttr->astNode[iAttrPos].acName[iCopyLen] = '\0';
	}
	
	return 0;
}

int AttrAddAvg(const char* pName, int iValue)
{
	if(!pName)
	{
		return -1;
	}
	int iLen = strlen(pName);
	if(iLen <= 0)
    {   
        return -2; 
    } 
	
	int iAttrPos = 0;
	uint32_t uiAttr = fnv_32_str((char*)pName, 0);
	if (!pstAttr)
	{
		if(GetShm2((void**)&pstAttr, ATTR_SHM_ID_DEC, sizeof(AttrList), 0666) < 0)
		{
			printf("pName can not get shm ...continue!\n");
			return -1;
		}
	}
	
	int iRet = SearchAttrID(pstAttr,uiAttr,&iAttrPos);
	if(iRet < 0)
	{
		printf("pName out of memery");
		return -1;
	}
	else if(iRet > 0)//返回0 attr不存在，返回1 attr存在
	{
		pstAttr->astNode[iAttrPos].iCurValue += iValue;
		pstAttr->astNode[iAttrPos].iTimes += 1;
		pstAttr->astNode[iAttrPos].uiAttrID = uiAttr;
		pstAttr->astNode[iAttrPos].ucType = EM_TYPE_AVG;
		if(uiAttr != fnv_32_str(pstAttr->astNode[iAttrPos].acName, 0))//如果数据一致，重新覆盖一份
		{
			int iCopyLen = iLen >= ATTR_NAME_LEN ? ATTR_NAME_LEN - 1 : iLen;
			strncpy(pstAttr->astNode[iAttrPos].acName, pName, iCopyLen);
			pstAttr->astNode[iAttrPos].acName[iCopyLen] = '\0';
		}
	}
	else
	{
		pstAttr->astNode[iAttrPos].ucUse = 1;
		pstAttr->astNode[iAttrPos].uiAttrID = uiAttr;
		pstAttr->astNode[iAttrPos].ucType = EM_TYPE_AVG;
		pstAttr->astNode[iAttrPos].iCurValue = iValue;
		pstAttr->astNode[iAttrPos].iTimes = 1;
		int iCopyLen = iLen >= ATTR_NAME_LEN ? ATTR_NAME_LEN - 1 : iLen;
		strncpy(pstAttr->astNode[iAttrPos].acName, pName, iCopyLen);
		pstAttr->astNode[iAttrPos].acName[iCopyLen] = '\0';
	}
	
	return 0;
}


int AttrSet(const char* pName, int iValue)
{
	if(!pName)
	{
		return -1;
	}
	int iLen = strlen(pName);
	if(iLen <= 0 )
    {   
        return -2; 
    } 
	
	int iAttrPos = 0;
	uint32_t uiAttr = fnv_32_str((char*)pName, 0);
	if (!pstAttr)
	{
		if(GetShm2((void**)&pstAttr, ATTR_SHM_ID_DEC, sizeof(AttrList), 0666) < 0)
		{
			printf("can not get shm ...continue!\n");
			return -1;
		}
	}
	
	int iRet = SearchAttrID(pstAttr, uiAttr, &iAttrPos);
	
	if(iRet == 0)//返回0 attr不存在，返回1 attr存在
	{
		pstAttr->astNode[iAttrPos].ucUse = 1;
		pstAttr->astNode[iAttrPos].uiAttrID = uiAttr;
		pstAttr->astNode[iAttrPos].ucType = EM_TYPE_STAT;
		pstAttr->astNode[iAttrPos].iCurValue = iValue;
		pstAttr->astNode[iAttrPos].iTimes = 1;
		int iCopyLen = iLen >= ATTR_NAME_LEN ? ATTR_NAME_LEN - 1 : iLen;
		strncpy(pstAttr->astNode[iAttrPos].acName, pName, iCopyLen);
		pstAttr->astNode[iAttrPos].acName[iCopyLen] = '\0';
	}
	else if(iRet < 0)
	{
		printf("out of memery");
		return -1;
	}
	else
	{
		pstAttr->astNode[iAttrPos].iCurValue = iValue;
		pstAttr->astNode[iAttrPos].uiAttrID = uiAttr;
		pstAttr->astNode[iAttrPos].ucType = EM_TYPE_STAT;
		if(uiAttr != fnv_32_str(pstAttr->astNode[iAttrPos].acName, 0))//如果数据一致，重新覆盖一份
		{
			int iCopyLen = iLen >= ATTR_NAME_LEN ? ATTR_NAME_LEN - 1 : iLen;
			strncpy(pstAttr->astNode[iAttrPos].acName, pName, iCopyLen);
			pstAttr->astNode[iAttrPos].acName[iCopyLen] = '\0';
		}
	}
	return 0;
}

int GetAttrValue(const char* pName, int *iValue)
{
	if(!pName)
	{
		return -1;
	}
	int iLen = strlen(pName);
	if(iLen <= 0)
    {   
        return -2; 
    } 
	
	int iAttrPos = 0;
	uint32_t uiAttr = fnv_32_str((char*)pName, 0);
	
	if (!pstAttr)
	{
		if(GetShm2((void**)&pstAttr, ATTR_SHM_ID_DEC, sizeof(AttrList), 0666) < 0)
		{
			printf("can not get shm ...continue!\n");
			return -1;
		}
	}
	
	int iRet = SearchAttrID(pstAttr, uiAttr, &iAttrPos);
	
	if(!iRet)//返回0 attr不存在，返回1 attr存在
	{
		return -1;
	}
	else if(iRet==-1)
	{
		return -1;
	}
	else
	{
		*iValue=pstAttr->astNode[iAttrPos].iCurValue;
		return 0;
	}
}
