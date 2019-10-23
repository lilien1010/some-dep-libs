/*
	Author: Lien

	Date：2018-10-26

	是为了给共享内存里面写入一个

*/

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <stdint.h>
 
#include<ctype.h>  

#include "Attr_API.h"
#include "lib_hash.h"
#include "oi_shm.h"


#include <lua.h>
#include <lualib.h>
#include <lauxlib.h> 

#if LUA_VERSION_NUM >= 502

#define luaL_register(L,n,f) luaL_newlib(L,f)

#endif

#define true 	1
#define false 	0

/*
 Wid1 高位
 Wid2 Wid低位
 ModN 要取模的数
*/
static int mm_mod_wid(lua_State *L)
{
	long Wid1 	= 	luaL_checklong(L, 1);
	long Wid2 	= 	luaL_checklong(L, 2);
	int ModN 	= 	luaL_checkint(L, 3);

	uint64_t Wid =  Wid1 | Wid2<<32;

	int ret = (int)(Wid%ModN);

	lua_pushinteger(L,ret);
	return 1;
}

/*
 传输字符串的Wid到里面取模
*/
static int mm_mod_str_wid(lua_State *L)
{
	const char *StrWid 	= 	luaL_checkstring(L, 1);
	if( StrWid== NULL ){ 
		lua_pushnil(L); 
		return 1;
	}
	int ModN 	= 	luaL_checkint(L, 2);
	
	long int longWid 	= atol(StrWid);

	uint64_t Wid = (uint64_t)longWid;

	int ret = (int)(Wid%ModN);

	lua_pushinteger(L,ret);
	return 1;
}

/* Lua函数返回 nil和错误描述 
	
	metric 描述的指标，
	project_name 描述业务
	val 描述要增加指标
	is_url 描述是不是url的采集

	如果 is_url == 1 会对url 根据 斜杠做截取，如果一个 维度（两个斜杠中间）有两个以上的数字，就忽略该维度，同事把 指标转小写
*/
static int mm_base(lua_State *L,int uType,int is_url)
{
	const char *metric 	= 	luaL_checkstring(L, 1);
	const char *project_name 	= 	luaL_checkstring(L, 2);
	if( metric == NULL || project_name== NULL ){ 
		lua_pushnil(L);
		lua_pushstring(L,"param nil error");
		return 2;
	}

	int val 	= 	luaL_checkint(L, 3);

	if(val == 0) {
		lua_pushnil(L);
		lua_pushstring(L,"val is zero");
		return 2;	
	}

	size_t 		metric_len 	=	lua_objlen (L,1);
	size_t 		project_name_len 	=	lua_objlen (L,2);
	
	if(project_name_len == 0 || metric_len==0 ){ 
		lua_pushnil(L);
		lua_pushstring(L,"param length error");
		return 2;
	}

 
 	char acName[ATTR_NAME_LEN] = {0};

 	int writesize =  snprintf(acName,ATTR_NAME_LEN-1,"%s/%s", project_name,metric);
	

	if (writesize >= ATTR_NAME_LEN-1) {
		writesize = ATTR_NAME_LEN-1;
	}

	
	/*记录是否要从尾巴后面去掉 斜杠，避免尾巴后面有多个/的问题 */
	int allow_drop_last_slash = 1; 
 
	int  slash_count 		= 	-1;  /*总共找到的斜杠的个数*/ 
 
	int  num_show_count 	=	0;  /*用斜杠描述一个维度，如果出现一次变化就*/
	int i=0;
	/*全部转成小写,同时过滤里面的数字 */
	for (i = writesize;i >= 0 ;i--){
		
		if (acName[i] == '\0') {
			continue;
		}

		acName[i] = tolower(acName[i]);

		if (is_url == 1) {
				if (acName[i] == '?') {
					/*问号出现 就清理标记符 */
					acName[i] 		= '\0';
					allow_drop_last_slash = 1;
					writesize 		= i;

					slash_count 	=	0;
					num_show_count 	=	0; 
				}else{
					/*如果找到了/ 尾巴后面要去掉，如果不是 /  就终止，如果找到问号 又开始终止 */
					if (allow_drop_last_slash == 1 && acName[i] == '/') {
						acName[i] 		= 	'\0';
						writesize 		= 	i;
						slash_count 	= 	0; 
						num_show_count 	=	0;
					}else{ 
						/*如果不是,或者已经终止了（找到别的符号了） 斜杠就终止 */
						allow_drop_last_slash = 0;

						/*维度切换*/
						if ( acName[i] == '/') {

							slash_count++; 
							/*如果当前维度里面有两个数字，就去掉当前维度*/
							if (num_show_count >= 2) {
								acName[i] 	= '\0';
								writesize 	= i;
								allow_drop_last_slash = 1;
								slash_count 	=	0;
							}
							/*归零数字计数器*/
							num_show_count 		=	0;
							 
						}
					}

					/*统计出现过数字的频率*/
					if( acName[i] >='0' && acName[i] <= '9' ){
						num_show_count++;
					}

			}
		}
 
	} 

	 

	/*记录错误返回值*/
	char ErrorMsg[ATTR_NAME_LEN+2] = {0};

	int ret = AttrBiggerChange(acName, val,uType,ErrorMsg,ATTR_NAME_LEN);
 	 
 	lua_pushinteger(L,ret);

 	if (ret == 0 && ErrorMsg[0] == '\0') {
 		// lua_pushnil(L);
 		lua_pushstring(L,acName);
 	}else{
 		lua_pushstring(L,ErrorMsg);
 	}

 	/* lua_pushinteger(L,metric_len); */
 	/* lua_pushinteger(L,project_name_len); */

	return 2;
}

/* Lua函数返回 nil和错误描述 
	
	metric 描述的指标，
	project_name 描述业务
	val 描述要增加指标
*/
static int mm_add_metric(lua_State *L)
{
	return mm_base(L,EM_TYPE_COUNTER,0); 
}

/**统计平均时间比较好**/
static int mm_avg_metric(lua_State *L)
{  
	return mm_base(L,EM_TYPE_AVG,0); 
}

/**同来清空时间**/
static int mm_set_metric(lua_State *L)
{  
	return mm_base(L,EM_TYPE_COUNTER,0); 
}

/*****下面是专门处理有URL的情况********/
static int mm_add_url_metric(lua_State *L)
{
	return mm_base(L,EM_TYPE_COUNTER,1); 
}

/**统计平均时间比较好**/
static int mm_avg_url_metric(lua_State *L)
{  
	return mm_base(L,EM_TYPE_AVG,1); 
}

/**同来清空时间**/
static int mm_set_url_metric(lua_State *L)
{  
	return mm_base(L,EM_TYPE_COUNTER,1); 
}


/* 编译命令

cd /home/git/webapi/lua/lib/libstatistic
gcc  xstatistic.c Attr_API.c oi_shm.c lib_hash.c -fPIC -shared -o xstatistic.so -Wall -I/usr/local/openresty/luajit/include/luajit-2.1 -L/usr/local/openresty/luajit/lib/  -lluajit-5.1

*/

/*
  都会接收三个参数，描述了 指标，项目（或者说os类型，如果第二个参数是ios android，）
  avg_url_metric 会对url 根据 斜杠做截取，如果一个 维度（两个斜杠中间）有两个以上的数字，就忽略该维度
*/
 
LUALIB_API int luaopen_lib_xstatistic(lua_State * L)
{
	static const luaL_Reg api[] = {
		{ "add_url_metric", mm_add_url_metric },
		{ "avg_url_metric", mm_avg_url_metric },
		{ "set_url_metric", mm_set_url_metric },


		{ "add_metric", mm_add_metric },
		{ "avg_metric", mm_avg_metric },
		{ "set_metric", mm_set_metric },
		{ "mod_wid", mm_mod_wid },
		{ "mod_str_wid", mm_mod_str_wid },
		{ 0, 0 },
	};

	luaL_register(L, "xstatistic", api); 
	return 1;
}
