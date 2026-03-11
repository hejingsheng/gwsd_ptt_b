/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "pt.h"
#include "cJSON.h"
#include "GWLog.h"
#include "GWPttEngine.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "gwsd.lib")

#define WM_USER_MESSAGE (WM_USER + 100)

#define TIME_1_MICROSECOND  (1)
#define TIME_1_MILLSECOND   (1000 * TIME_1_MICROSECOND)
#define TIME_1_SECOND       (1000 * TIME_1_MILLSECOND)

#define PTT_CLIENT_DEFAULT_HEART_PERIOD   10

#define PTT_THREAD_POC_CMD_EVENT_TYPE       0
#define PTT_THREAD_TIMER_EVENT_TYPE         1
#define PTT_THREAD_NET_FOUND_EVENT_TYPE     2
#define PTT_THREAD_NET_READY_EVENT_TYPE     3
#define PTT_THREAD_LOGIN_EVENT_TYPE         4
#define PTT_THREAD_REGMSG_EVENT_TYPE        5
#define PTT_THREAD_IDLE_EVENT_TYPE          6
#define PTT_THREAD_OFFLINE_EVENT_TYPE       7

typedef enum {
	PTT_CLIENT_STATUS_NONE = -1,
	PTT_CLIENT_STATUS_NETSCAN = 0,
	PTT_CLIENT_STATUS_NETCONN = 1,
	PTT_CLIENT_STATUS_LOGIN = 2,
	PTT_CLIENT_STATUS_REGMSG = 3,
	PTT_CLIENT_STATUS_JOINGRP = 4,
	PTT_CLIENT_STATUS_IDLE = 5,
	PTT_CLIENT_STATUS_OFFLINE = 6,
}PTT_CLIENT_STATUS_ENUM;

#define CHECK_JSON_ITEM_TYPE(item, targettype) \
	if ((item) == NULL || (item)->type != targettype) \
	{                                                   \
        printf("json item type error"); \
		goto error;                             \
	}

typedef struct
{
	struct pt ptt_pt;
	PTT_CLIENT_STATUS_ENUM status; //-1 none 0 start wifi scan 1 start wifi connect 2 start login 3 start join group  4 enter idle 5 offline
	int net_search_timeout;
	int net_ready_timeout;
	int login_timeout;
	int joingrp_timeout;
	int regmsg_timeout;
	int heart_timeout;
	int heart_period;
	char account[33];
	char password[33];
	char newAccount[33];
	char newPassword[33];
	char agentPassword[33];
	char name[64];
	uint32_t uid;
	int defaultGrpId;
	int currentGrpId;
	int currentGrpPri;
	int currentGrpType;
	int lastGrpPri;
	int gps;
	int gps_period;
	int message;
	int call;
	int video;
	int silent;
	char msgRegSucc;
	char currentGrpName[64];
	//DataList *grouplist;
	char serverIp[33];
	char platformDns[33];
	int platformVoicePort;
	int platformMsgPort;
	signed char local_zone;
	char local_zone_dec;
	int mode;
	char binding;
	char locinfo[256];
	int reportLoc;
	char weatherinfo[256];
	char battery;
	char net[10];
}PttClient;

struct Group
{
	char name[64];
	uint32_t gid;
	int type;
};

struct Member
{
	char *name;
	uint32_t gid;
	uint32_t uid;
};

struct Wifi
{
	char ssid[33];
	char pass[33];
	unsigned char auth;
	unsigned char rssi;
	unsigned char available;
};

typedef struct
{
	int type;
	void *data;
	int len;
}PttThreadMsg;

typedef struct
{
	char *cmd;
	int len;
}AtThreadMsg;

static PttClient *pttClient = NULL;
static DWORD ptt_thread_id = 0;

static void sendMsgToPttMainThread(int type, int len)
{
	PostThreadMessage(ptt_thread_id, WM_USER_MESSAGE, type, len);
}

static void offline()
{
	pttClient->uid = 0;
	pttClient->defaultGrpId = 0;
	pttClient->currentGrpId = 0;
	//datalist_clear(pttClient->grouplist);
	sendMsgToPttMainThread(PTT_THREAD_OFFLINE_EVENT_TYPE, 0);
}

// SDK io thread
static void onGWPttEvent(int event, char *data, int data1)
{
	cJSON *root = NULL;
	int ret;
	if (event == GW_PTT_EVENT_LOGIN)
	{
		cJSON *item;
		root = cJSON_Parse(data);
		ret = cJSON_GetObjectItem(root, "result")->valueint;
		if (ret == 0)
		{
			pttClient->uid = cJSON_GetObjectItem(root, "uid")->valueint;
			pttClient->defaultGrpId = cJSON_GetObjectItem(root, "defaultGid")->valueint;
			item = cJSON_GetObjectItem(root, "gps");
			if (item != NULL)
			{
				pttClient->gps = item->valueint;
				if (pttClient->gps)
				{
					item = cJSON_GetObjectItem(root, "gps_period");
					pttClient->gps_period = item->valueint;
				}
			}
			else
			{
				pttClient->gps = 0;
			}
			item = cJSON_GetObjectItem(root, "message");
			if (item != NULL)
			{
				pttClient->message = item->valueint;
			}
			else
			{
				pttClient->message = 0;
			}
			item = cJSON_GetObjectItem(root, "call");
			if (item != NULL)
			{
				pttClient->call = item->valueint;
			}
			else
			{
				pttClient->call = 0;
			}
			item = cJSON_GetObjectItem(root, "video");
			if (item != NULL)
			{
				pttClient->video = item->valueint;
			}
			else
			{
				pttClient->video = 0;
			}
			strcpy(pttClient->name, cJSON_GetObjectItem(root, "name")->valuestring);
			pttHeart(100, "net");
			sendMsgToPttMainThread(PTT_THREAD_LOGIN_EVENT_TYPE, 0);
		}
		else if (ret == 1)
		{
			// remote change account
			strcpy(pttClient->newAccount, cJSON_GetObjectItem(root, "newAccount")->valuestring);
			strcpy(pttClient->newPassword, cJSON_GetObjectItem(root, "newPassword")->valuestring);
			strcpy(pttClient->agentPassword, cJSON_GetObjectItem(root, "agentPassword")->valuestring);
		}
		else
		{
			pttClient->uid = 0;
			memset(pttClient->name, 0, sizeof(pttClient->name));
		}
	}
	else if (event == GW_PTT_EVENT_QUERY_GROUP)
	{
		root = cJSON_Parse(data);
		ret = cJSON_GetObjectItem(root, "result")->valueint;
		if (ret == 0)
		{
			int i;
			char groupnum[16];
			int size;
			int joinGrpId = 0;
			int joinGrpType = 0;
			//int *gids;
			//int *types;
			cJSON *groups = cJSON_GetObjectItem(root, "groups");
			//if (pttClient->grouplist && pttClient->grouplist->count)
			//{
			//    datalist_clear(pttClient->grouplist);
			//}
			size = cJSON_GetArraySize(groups);
			//gids = (int*) malloc(sizeof(int)*size);
			//types = (int*) malloc(sizeof(int)*size);
			if (size > 0)
			{
				cJSON *item = cJSON_GetArrayItem(groups, 0);
				joinGrpId = cJSON_GetObjectItem(item, "gid")->valueint;
				joinGrpType = cJSON_GetObjectItem(item, "type")->valueint;
				for (i = 0; i < size; i++)
				{
					struct Group group;
					cJSON *item = cJSON_GetArrayItem(groups, i);
					strcpy(group.name, cJSON_GetObjectItem(item, "name")->valuestring);
					group.gid = cJSON_GetObjectItem(item, "gid")->valueint;
					group.type = cJSON_GetObjectItem(item, "type")->valueint;
					//datalist_push_back(pttClient->grouplist, &group, sizeof(struct Group));
					//if (gids != NULL && types != NULL)
					//{
					//    gids[i] = group.gid;
					//    types[i] = group.type;
					//}
					if (group.gid == pttClient->defaultGrpId)
					{
						joinGrpId = group.gid;
						joinGrpType = group.type;
					}
				}
				if (pttClient->currentGrpId == 0)
				{
					pttJoinGroup(joinGrpId, joinGrpType);
				}
			}
			else
			{
				GWLOG_PRINT(GW_LOG_LEVEL_WARN, "no group");
			}
		}
		else
		{
			GWLOG_PRINT(GW_LOG_LEVEL_ERROR, "query group error");
		}
	}
	else if (event == GW_PTT_EVENT_JOIN_GROUP)
	{
		//Node *node;
		root = cJSON_Parse(data);
		ret = cJSON_GetObjectItem(root, "result")->valueint;
		if (ret == 0)
		{
			cJSON *item;
			int creater;
			pttClient->currentGrpPri = cJSON_GetObjectItem(root, "priority")->valueint;
			pttClient->currentGrpId = cJSON_GetObjectItem(root, "gid")->valueint;
			creater = cJSON_GetObjectItem(root, "creater")->valueint;
			if (creater == pttClient->uid)
			{
				pttClient->currentGrpType = 8;
			}
			else
			{
				pttClient->currentGrpType = 0;
			}
			item = cJSON_GetObjectItem(root, "gname");
			if (item != NULL)
			{
				strcpy(pttClient->currentGrpName, item->valuestring);
			}
			else
			{
				strcpy(pttClient->currentGrpName, "null");
			}
			sendMsgToPttMainThread(PTT_THREAD_IDLE_EVENT_TYPE, 0);
		}
		else
		{
			GWLOG_PRINT(GW_LOG_LEVEL_ERROR, "join group error");
		}
	}
	else if (event == GW_PTT_EVENT_QUERY_MEMBER || event == GW_PTT_EVENT_QUERY_DISPATCH)
	{
		root = cJSON_Parse(data);
		ret = cJSON_GetObjectItem(root, "result")->valueint;
		if (ret == 0)
		{
			int i;
			char membernum[16];
			cJSON *members = cJSON_GetObjectItem(root, "members");
			for (i = 0; i < cJSON_GetArraySize(members); i++)
			{
				struct Member member;
				int status;
				cJSON *item = cJSON_GetArrayItem(members, i);
				member.name = cJSON_GetObjectItem(item, "name")->valuestring;
				member.gid = cJSON_GetObjectItem(item, "gid")->valueint;
				member.uid = cJSON_GetObjectItem(item, "uid")->valueint;
				if (event == GW_PTT_EVENT_QUERY_DISPATCH)
				{
					status = 4;
				}
				else
				{
					if (member.gid == pttClient->currentGrpId)
					{
						status = 3;
					}
					else
					{
						status = 2;
					}
				}
			}
			snprintf(membernum, sizeof(membernum) - 1, "%d", cJSON_GetArraySize(members));
		}
		else
		{
		}
	}
	else if (event == GW_PTT_EVENT_PAGE_QUERY)
	{
		root = cJSON_Parse(data);
		ret = cJSON_GetObjectItem(root, "result")->valueint;
		if (ret == 0)
		{
			int i;
			int count = cJSON_GetObjectItem(root, "count")->valueint;
			int allcount = cJSON_GetObjectItem(root, "allcount")->valueint;
			int type = cJSON_GetObjectItem(root, "type")->valueint;
			cJSON *pages = cJSON_GetObjectItem(root, "page");
			for (i = 0; i < cJSON_GetArraySize(pages); i++)
			{
				if (type == 0)
				{
					struct Group group;
					cJSON *item = cJSON_GetArrayItem(pages, i);
					strcpy(group.name, cJSON_GetObjectItem(item, "name")->valuestring);
					group.gid = cJSON_GetObjectItem(item, "id")->valueint;
					group.type = cJSON_GetObjectItem(item, "type")->valueint;
				}
				else
				{
					struct Member member;
					int status;
					cJSON *item = cJSON_GetArrayItem(pages, i);
					member.name = cJSON_GetObjectItem(item, "name")->valuestring;
					member.uid = cJSON_GetObjectItem(item, "id")->valueint;
					status = cJSON_GetObjectItem(item, "type")->valueint;
				}
			}
		}
		else
		{
			GWLOG_PRINT(GW_LOG_LEVEL_INFO, "page query fail");
		}
	}
	else if (event == GW_PTT_EVENT_TMP_GROUP_ACTIVE)
	{
		root = cJSON_Parse(data);
		ret = cJSON_GetObjectItem(root, "result")->valueint;
		if (ret == 0)
		{
			pttClient->lastGrpPri = pttClient->currentGrpPri;
			pttClient->currentGrpPri = 0;
		}
	}
	else if (event == GW_PTT_EVENT_TMP_GROUP_PASSIVE)
	{
		int status = 0;
		char *name = NULL;
		cJSON *item;
		root = cJSON_Parse(data);
		ret = cJSON_GetObjectItem(root, "result")->valueint;
		if (ret == 0)
		{
			item = cJSON_GetObjectItem(root, "name");
			if (item != NULL)
			{
				// arrive
				name = item->valuestring;
				status = 2;
				pttClient->lastGrpPri = pttClient->currentGrpPri;
				pttClient->currentGrpPri = 0;
			}
			else
			{
				// release
				status = 1;
			}
		}
		else
		{
			status = -1;
			name = NULL;
		}
	}
	else if (event == GW_PTT_EVENT_LOSTMIC)
	{
		
	}
	else if (event == GW_PTT_EVENT_SPEAK)
	{
		int speakerid = 0;
		int speakerpri;
		char *speakernm = NULL;
		int status;
		root = cJSON_Parse(data);
		speakerid = cJSON_GetObjectItem(root, "uid")->valueint;
		if (speakerid > 0)
		{
			speakernm = cJSON_GetObjectItem(root, "name")->valuestring;
			speakerpri = cJSON_GetObjectItem(root, "priority")->valueint;
			if (speakerpri >= pttClient->currentGrpPri)
			{
				status = 0;
			}
			else
			{
				status = 1;
			}
		}
		else
		{
			status = 1;
		}
	}
	else if (event == GW_PTT_EVENT_CURRENT_GROUP)
	{
		root = cJSON_Parse(data);
		ret = cJSON_GetObjectItem(root, "result")->valueint;
		if (ret == 0)
		{
			//Node *node;
			int creater;
			//char findInGrpList = 0;
			struct Group group;
			group.gid = cJSON_GetObjectItem(root, "gid")->valueint;
			strcpy(group.name, cJSON_GetObjectItem(root, "name")->valuestring);
			creater = cJSON_GetObjectItem(root, "creater")->valueint;
			if (creater == pttClient->uid)
			{
				// my grp
				group.type = 8;
			}
			else
			{
				// not my grp
				group.type = 0;
			}
			if (cJSON_GetObjectItem(root, "reason") != NULL)
			{
				char *reason = cJSON_GetObjectItem(root, "reason")->valuestring;
				if (strcmp(reason, "return") == 0)
				{
					GWLOG_PRINT(GW_LOG_LEVEL_INFO, "reset group");
					pttClient->currentGrpPri = pttClient->lastGrpPri;
				}
				else
				{
					pttClient->lastGrpPri = pttClient->currentGrpPri;
					pttClient->currentGrpPri = 0;
				}
			}
			else
			{
				pttClient->lastGrpPri = pttClient->currentGrpPri;
				pttClient->currentGrpPri = 0;
			}
			pttClient->currentGrpId = group.gid;
			strcpy(pttClient->currentGrpName, group.name);
			pttClient->currentGrpType = group.type;
			sendMsgToPttMainThread(PTT_THREAD_IDLE_EVENT_TYPE, 0);
		}
	}
	else if (event == GW_PTT_EVENT_DUPLEX)
	{
		char *name = NULL;
		int uid;
		int status;
		root = cJSON_Parse(data);
		ret = cJSON_GetObjectItem(root, "result")->valueint;
		if (ret == 0)
		{
			status = cJSON_GetObjectItem(root, "status")->valueint;
			if (status == GW_PTT_DUPLEX_STATUS_INVITE)
			{
				cJSON* name_item = cJSON_GetObjectItem(root, "name");
				if (name_item != NULL && name_item->type == cJSON_String)
				{
					name = name_item->valuestring;
				}
				else
				{
					name = NULL;  // ?á???2?????¨2?¨°¨¤¨¤D¨a??¨a?¨?¨o?à¨|¨¨?aNULL
				}
				cJSON* uid_item = cJSON_GetObjectItem(root, "uid");
				if (uid_item != NULL && uid_item->type == cJSON_Number)
				{
					uid = uid_item->valueint;
				}
				else
				{
					uid = 0;      // ??¨¨??|ì?¨°??¨a?¨???
				}
			}
			else
			{
				name = NULL;
				uid = 0;
			}
		}
		else
		{
			status = -1;
			uid = -1;
		}
		GWLOG_PRINT(GW_LOG_LEVEL_INFO, "GW_PTT_EVENT_DUPLEX GWBuildDuplexCallReport after");
	}
	else if (event == GW_PTT_EVENT_LOGOUT || event == GW_PTT_EVENT_KICKOUT || event == GW_PTT_EVENT_ERROR)
	{
		offline();
	}
	if (root)
	{
		cJSON_Delete(root);
	}
}

static void onGWMsgEvent(int status, char *msg, int length)
{
	cJSON *root = NULL;
	cJSON *data = NULL;
	cJSON *item = NULL;
	int response_status;
	if (status == GW_MSG_STATUS_SUCC || status == GW_MSG_STATUS_ERROR)
	{
		pttClient->msgRegSucc = (status == GW_MSG_STATUS_SUCC) ? 1 : 0;
		sendMsgToPttMainThread(PTT_THREAD_REGMSG_EVENT_TYPE, 0);
		return;
	}
	if (status == GW_MSG_STATUS_ADDRESS)
	{
		root = cJSON_Parse(msg);
		if (root != NULL)
		{
			item = cJSON_GetObjectItem(root, "status");
			CHECK_JSON_ITEM_TYPE(item, cJSON_Number);
			response_status = item->valueint;
			if (response_status == 200)
			{
				data = cJSON_GetObjectItem(root, "data");
				CHECK_JSON_ITEM_TYPE(data, cJSON_Object);
				item = cJSON_GetObjectItem(data, "ads");
				CHECK_JSON_ITEM_TYPE(item, cJSON_String);
				if (item->valuestring != NULL)
				{
					snprintf(pttClient->locinfo, sizeof(pttClient->locinfo), "ads:%s", item->valuestring);
					if (pttClient->reportLoc == 1)
					{
						
					}
				}
			}
			else
			{
				GWLOG_PRINT(GW_LOG_LEVEL_ERROR, "address response error %d", response_status);
			}
			pttClient->reportLoc = 0;
		}
	}
	else if (status == GW_MSG_STATUS_WEATHER)
	{
		root = cJSON_Parse(msg);
		if (root != NULL)
		{
			item = cJSON_GetObjectItem(root, "status");
			CHECK_JSON_ITEM_TYPE(item, cJSON_Number);
			response_status = item->valueint;
			if (response_status == 200)
			{
				char *weath = NULL;
				char *tempe = NULL;
				data = cJSON_GetObjectItem(root, "data");
				CHECK_JSON_ITEM_TYPE(data, cJSON_Object);
				item = cJSON_GetObjectItem(data, "weath");
				CHECK_JSON_ITEM_TYPE(item, cJSON_String);
				if (item->valuestring != NULL)
				{
					weath = item->valuestring;
				}
				item = cJSON_GetObjectItem(data, "lh");
				CHECK_JSON_ITEM_TYPE(item, cJSON_String);
				if (item->valuestring != NULL)
				{
					tempe = item->valuestring;
				}
				snprintf(pttClient->weatherinfo, sizeof(pttClient->weatherinfo), "%s,%s", weath, tempe);
			}
			else
			{
				GWLOG_PRINT(GW_LOG_LEVEL_ERROR, "weather response error %d", response_status);
			}
		}
	}
error:
	if (root)
	{
		cJSON_Delete(root);
	}
}

static int configAddress(PttClient *client)
{
	char serverIp[33];
	const char* hostname = client->platformDns;
	printf("Resolving %s using gethostbyname()...\n", hostname);
	struct hostent* host = gethostbyname(hostname);
	if (host == NULL) {
		printf("gethostbyname failed: %d\n", WSAGetLastError());
		return -1;
	}
	printf("Official name: %s\n", host->h_name);
	printf("Address type: %s\n", host->h_addrtype == AF_INET ? "IPv4" : "IPv6");
	printf("Address length: %d bytes\n", host->h_length);

	// 打印所有IP地址
	char ip[64];
	for (int i = 0; host->h_addr_list[i] != NULL; i++) {
		struct in_addr addr;
		memcpy(&addr, host->h_addr_list[i], sizeof(addr));
		inet_ntop(AF_INET, &addr, ip, sizeof(ip));
		printf("IP %d: %s\n", i + 1, ip);
		strcpy(serverIp, ip);
	}

	GWLOG_PRINT(GW_LOG_LEVEL_INFO, "server ip %s", serverIp);
	pttConfigServer(0, serverIp, client->platformVoicePort);
	pttConfigServer(1, serverIp, client->platformMsgPort);
	//pttConfigServer(0, serverIp, 50001);
	return 0;
}

static int netIsConnected()
{
	// check current network status 
	// net ready return 1
	// net error retrun 0
	return 1;
}

static char pttClientMainLoop(PttClient *client, int ms)
{
	int ret;
	struct pt *pt = &client->ptt_pt;

	if (ms != 0)
	{
		if (client->net_search_timeout > ms)
		{
			client->net_search_timeout -= ms;
		}
		else
		{
			client->net_search_timeout = 0;
		}
		if (client->net_ready_timeout > ms)
		{
			client->net_ready_timeout -= ms;
		}
		else
		{
			client->net_ready_timeout = 0;
		}
		if (client->login_timeout > ms)
		{
			client->login_timeout -= ms;
		}
		else
		{
			client->login_timeout = 0;
		}
		if (client->regmsg_timeout > ms)
		{
			client->regmsg_timeout -= ms;
		}
		else
		{
			client->regmsg_timeout = 0;
		}
		if (client->joingrp_timeout > ms)
		{
			client->joingrp_timeout -= ms;
		}
		else
		{
			client->joingrp_timeout = 0;
		}
		if (client->heart_timeout > ms)
		{
			client->heart_timeout -= ms;
		}
		else
		{
			client->heart_timeout = 0;
		}
	}

	if (client->status == PTT_CLIENT_STATUS_OFFLINE)
	{
		GWLOG_PRINT(GW_LOG_LEVEL_INFO, "offline...");
		PT_INIT(pt);
	}

	PT_BEGIN(pt);

	PT_WAIT_UNTIL(pt, client->status == PTT_CLIENT_STATUS_LOGIN);

LOGIN:
	GWLOG_PRINT(GW_LOG_LEVEL_INFO, "net connect success start logining...");
	ret = configAddress(client);
	if (ret < 0)
	{
		GWLOG_PRINT(GW_LOG_LEVEL_ERROR, "domain or ip is error");
		PT_EXIT(pt);
	}
	pttLogin(pttClient->account, pttClient->password, "12345", "54321");

	client->login_timeout = 30 * TIME_1_SECOND;
	PT_WAIT_UNTIL(pt, (client->status == PTT_CLIENT_STATUS_REGMSG) || (client->login_timeout == 0));
	if (client->login_timeout == 0)
	{
		GWLOG_PRINT(GW_LOG_LEVEL_ERROR, "login fail...");
		goto LOGIN;
	}
	GWLOG_PRINT(GW_LOG_LEVEL_INFO, "login success start reg msg...");

REGMSG:
	if (client->message == 1)
	{
		pttRegOfflineMsg(NULL, NULL, 0, 0);
	}
	else
	{
		client->status = PTT_CLIENT_STATUS_JOINGRP;
		GWLOG_PRINT(GW_LOG_LEVEL_INFO, "account not have msg func..");
	}
	client->regmsg_timeout = 30 * TIME_1_SECOND;
	PT_WAIT_UNTIL(pt, (client->status == PTT_CLIENT_STATUS_JOINGRP) || (client->regmsg_timeout == 0));
	if ((client->message == 1 && client->msgRegSucc == 0) || (client->regmsg_timeout == 0))
	{
		GWLOG_PRINT(GW_LOG_LEVEL_ERROR, "reg msg fail or timeout...");
		client->status = PTT_CLIENT_STATUS_REGMSG;
		goto REGMSG;
	}
	GWLOG_PRINT(GW_LOG_LEVEL_ERROR, "reg msg success start join group...");

JOINGRP:
	if (pttClient->defaultGrpId != 0)
	{
		pttJoinGroup(pttClient->defaultGrpId, 0);
	}
	else
	{
		GWLOG_PRINT(GW_LOG_LEVEL_WARN, "no default group please query group and select join one");
		client->status = PTT_CLIENT_STATUS_IDLE;
	}
	client->joingrp_timeout = 30 * TIME_1_SECOND;
	PT_WAIT_UNTIL(pt, (client->status == PTT_CLIENT_STATUS_IDLE) || (client->joingrp_timeout == 0));
	if (client->joingrp_timeout == 0)
	{
		GWLOG_PRINT(GW_LOG_LEVEL_ERROR, "join group fail...");
		goto JOINGRP;
	}
	GWLOG_PRINT(GW_LOG_LEVEL_INFO, "join group success...");
	while (1)
	{
		pttHeart(client->battery, client->net);
		GWLOG_PRINT(GW_LOG_LEVEL_INFO, "send heart...");
		client->heart_timeout = client->heart_period * TIME_1_SECOND;
		PT_WAIT_UNTIL(pt, (client->heart_timeout == 0));
	}
	PT_END(pt);
}

static VOID CALLBACK timerCallback(PVOID lpParam, BOOLEAN timerOrWaitFired)
{
	// timer callback
	sendMsgToPttMainThread(PTT_THREAD_TIMER_EVENT_TYPE, 0);
}

static void pttClientMainThread(void *param)
{
	MSG msg;
	PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);
	while (1)
	{
		if (GetMessage(&msg, NULL, 0, 0))
		{
			if (msg.message == WM_USER_MESSAGE)
			{
				switch (msg.wParam)
				{
				case PTT_THREAD_TIMER_EVENT_TYPE:
					pttClientMainLoop(pttClient, TIME_1_SECOND);
					break;
				case PTT_THREAD_NET_READY_EVENT_TYPE:
					pttClient->status = PTT_CLIENT_STATUS_LOGIN;
					pttClientMainLoop(pttClient, 0);
					break;
				case PTT_THREAD_LOGIN_EVENT_TYPE:
					pttClient->status = PTT_CLIENT_STATUS_REGMSG;
					pttClientMainLoop(pttClient, 0);
					break;
				case PTT_THREAD_REGMSG_EVENT_TYPE:
					pttClient->status = PTT_CLIENT_STATUS_JOINGRP;
					pttClientMainLoop(pttClient, 0);
					break;
				case PTT_THREAD_IDLE_EVENT_TYPE:
					pttClient->status = PTT_CLIENT_STATUS_IDLE;
					pttClientMainLoop(pttClient, 0);
					//updatePeriod(pttClient->heart_period*TIME_1_SECOND);
					break;
				case PTT_THREAD_OFFLINE_EVENT_TYPE:
					pttClient->status = PTT_CLIENT_STATUS_OFFLINE;
					pttClientMainLoop(pttClient, 0);
					//updatePeriod(100*TIME_1_MILLSECOND);
					break;
				default:
					break;
				}
			}
		}
		else
		{
			continue;
		}
	}
}

static void windows_init_winsock(void)
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		printf("WSAStartup failed\n");
		return;
	}
	sendMsgToPttMainThread(PTT_THREAD_NET_READY_EVENT_TYPE, 0);
}

static void initTimer()
{
	HANDLE g_hTimerQueue = NULL;
	HANDLE g_hTimer = NULL;
	g_hTimerQueue = CreateTimerQueue();
	if (!g_hTimerQueue) {
		printf("创建定时器队列失败: %lu\n", GetLastError());
		return;
	}

	// 创建定时器
	if (!CreateTimerQueueTimer(
		&g_hTimer,          // 定时器句柄
		g_hTimerQueue,      // 定时器队列
		timerCallback,      // 回调函数
		NULL,               // 回调参数
		TIME_1_SECOND / 1000,               // 延迟时间（毫秒）
		TIME_1_SECOND / 1000,               // 间隔时间（毫秒）
		WT_EXECUTEDEFAULT   // 执行标志
	)) {
		printf("创建定时器失败: %lu\n", GetLastError());
		DeleteTimerQueue(g_hTimerQueue);
		return;
	}
}

// simulate audio device
static DWORD record_thread_id = 0;
static char speaking = 0;
void *record_thread(void *param)
{
	FILE *fp = fopen("speaker.pcm", "rb");
	if (fp == NULL)
	{
		return NULL;
	}
	int size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	while (speaking)
	{
		char pcm[320];
		int size;
		size = fread(pcm, 1, 320, fp);
		if (size <= 0)
		{
			break;
		}
		pttOnPcmData(pcm, 320);
		Sleep(10);
	}
	fclose(fp);
	return NULL;
}

static void startRecord()
{
	// start record non block
	speaking = 1;
	CreateThread(
		NULL,                   // 默认安全属性
		3072,             // 堆栈大小
		(LPTHREAD_START_ROUTINE)record_thread,   // 线程入口函数
		NULL,                   // 参数
		0,                      // 创建标志，0表示立即运行
		&record_thread_id);
}

static void stopRecord()
{
	// stop record non block
	speaking = 0;
}

static void startPlay()
{
	// start play non block
}

static void stopPlay()
{
	// stop play non block
}

void write_to_file(char *data, int len)
{
	FILE *fp = fopen("play.pcm", "ab");

	fwrite(data, 1, len, fp);

	fclose(fp);
}

static void playbackPlayData(char *pcm, int len)
{
	// write wav data to play device
	printf("recv play data %d\n", len);
	write_to_file(pcm, len);

}

static void muteSpeaker(char mute)
{

}

static void muteRecorder(char mute)
{
}

static GWPttAudioModule pttAudioDevice = {
		.startPlay = startPlay,
		.stopPlay = stopPlay,
		.playData = playbackPlayData,
		.startRecord = startRecord,
		.stopRecord = stopRecord,
		.muteSpeaker = muteSpeaker,
		.muteRecorder = muteRecorder,
};

void pttClientStart(const char *account, const char *password, const char *dns, int port, int msgport)
{
	HANDLE th = NULL;

	printf("GWPttClient Start...\n");

	initTimer();

	char *version = NULL;
	version = pttGetVersion();
	printf("gw ptt version %s\n", version);

	pttInit(onGWPttEvent, onGWMsgEvent, &pttAudioDevice, 0, GW_PTT_ENCODE_LEVEL_HIGH, 0, 0, 0);

	pttClient = (PttClient*)malloc(sizeof(PttClient));
	assert(pttClient != NULL);
	memset(pttClient, 0, sizeof(PttClient));
	pttClient->status = PTT_CLIENT_STATUS_NONE;
	pttClient->heart_period = PTT_CLIENT_DEFAULT_HEART_PERIOD;
	pttClient->local_zone = 8; // beijing time
	pttClient->local_zone_dec = 0;
	strcpy(pttClient->account, account);
	strcpy(pttClient->password, password);
	strcpy(pttClient->platformDns, dns);
	pttClient->platformVoicePort = port;
	pttClient->platformMsgPort = msgport;
	pttClient->battery = 100;
	strcpy(pttClient->net, "net");

	th = CreateThread(
		NULL,                   // 默认安全属性
		5 * 1024,             // 堆栈大小
		(LPTHREAD_START_ROUTINE)pttClientMainThread,   // 线程入口函数
		NULL,                   // 参数
		0,                      // 创建标志，0表示立即运行
		&ptt_thread_id              // 线程ID
	);
	assert(th != NULL);

	Sleep(1000);
	windows_init_winsock();
}

void pttClientQueryGroup()
{
	pttQueryGroup();
}

void pttClientQueryMember(int gid)
{
	pttQueryMember(gid, 0);
}

void pttClientSpeakStart()
{
	pttSpeak(GW_PTT_SPEAK_START, 0);
}

void pttClientSpeakEnd()
{
	pttSpeak(GW_PTT_SPEAK_END, 0);
}

void pttClientJoinGroup(int gid)
{
	pttJoinGroup(gid, 16);
}

void pttClientOffline()
{
	pttLogout();
}

