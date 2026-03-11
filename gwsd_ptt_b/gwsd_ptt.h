//
// Created by hejingsheng on 2024/5/8.
//

#ifndef GWSD_GWSD_PTT_H
#define GWSD_GWSD_PTT_H

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_NAME_LENGTH (64)

#define GW_PTT_AUDIO_SAMPLERATE            (8000)
#define GW_PTT_AUDIO_BITS                  (16)
#define GW_PTT_AUDIO_CHANNELS              (1)
#define GW_PTT_AUDIO_DURATION_MS_PER_FRAME (20)
#define GW_PTT_AUDIO_BYTES_PER_FRAME       ((GW_PTT_AUDIO_SAMPLERATE/(1000/GW_PTT_AUDIO_DURATION_MS_PER_FRAME)*GW_PTT_AUDIO_BITS/8)*GW_PTT_AUDIO_CHANNELS)
#define GW_PTT_ENCODE_LEVEL_LOW            0
#define GW_PTT_ENCODE_LEVEL_MID            1
#define GW_PTT_ENCODE_LEVEL_HIGH           2

enum {
    GW_PTT_SPEAK_START = 0,
    GW_PTT_SPEAK_END = 1,
};

enum {
    GW_PTT_DUPLEX_ACTION_CREATE = 20,  // start call
    GW_PTT_DUPLEX_ACTION_ACCEPT = 21,  // accept call
    GW_PTT_DUPLEX_ACTION_HANGUP = 25,  // hangup call
};

enum {
    GW_PTT_EVENT_ERROR = -1,
    GW_PTT_EVENT_LOGIN = 0,
    GW_PTT_EVENT_QUERY_GROUP = 1,
    GW_PTT_EVENT_JOIN_GROUP = 2,
    GW_PTT_EVENT_QUERY_MEMBER = 3,
    GW_PTT_EVENT_REQUEST_MIC = 4,
    GW_PTT_EVENT_TMP_GROUP_ACTIVE = 5,
    GW_PTT_EVENT_TMP_GROUP_PASSIVE = 6,
    GW_PTT_EVENT_DUPLEX = 7,
    GW_PTT_EVENT_LOGOUT = 8,
    GW_PTT_EVENT_KICKOUT = 9,
    GW_PTT_EVENT_CURRENT_GROUP = 10,
    GW_PTT_EVENT_SPEAK = 11,
    GW_PTT_EVENT_PLAY_DATA = 12,
    GW_PTT_EVENT_LOSTMIC = 13,
    GW_PTT_EVENT_TEXT_RECV = 14,
    GW_PTT_EVENT_QUERY_DISPATCH = 16,
    GW_PTT_EVENT_PAGE_QUERY = 17,
};

enum {
    GW_PTT_DUPLEX_STATUS_START = 10,
    GW_PTT_DUPLEX_STATUS_INVITE = GW_PTT_DUPLEX_STATUS_START,
    GW_PTT_DUPLEX_STATUS_ACCEPTED = 11,
    GW_PTT_DUPLEX_STATUS_END = 15,
    GW_PTT_DUPLEX_STATUS_BUSY = 16,
    GW_PTT_DUPLEX_STATUS_OTHER_INVITE = 17,
};

enum {
    GW_PTT_MSG_RECV_TYPE_USER = 0,
    GW_PTT_MSG_RECV_TYPE_DISPATCH = 1,
    GW_PTT_MSG_RECV_TYPE_GROUP = 3,
};

enum {
	GW_PTT_MSG_TYPE_NOTICE = 1100,
    GW_PTT_MSG_TYPE_TEXT = 1103,
    GW_PTT_MSG_TYPE_PHOTO = 1104,
    GW_PTT_MSG_TYPE_VIDEO = 1105,
    GW_PTT_MSG_TYPE_VOICE = 1106,

    GW_PTT_MSG_TYPE_PATROL_PUSH = 1310,
    GW_PTT_MSG_TYPE_PATROL_CHANGE = 1311,
    GW_PTT_MSG_TYPE_PATROL_DELETE = 1313,
};

enum {
    GW_PTT_PATROL_RESULT_PUNCH = 0,
    GW_PTT_PATROL_RESULT_NORMAL = 1,
    GW_PTT_PATROL_RESULT_EXCE = 2,
};

enum {
    GW_PTT_FILE_TYPE_IMAGE = 0,
    GW_PTT_FILE_TYPE_VOICE = 1,
    GW_PTT_FILE_TYPE_VIDEO = 2,
};

enum {
    GW_MSG_STATUS_ERROR = -1,
    GW_MSG_STATUS_SUCC = 0,
    GW_MSG_STATUS_DATA = 1,
	GW_MSG_STATUS_WEATHER = 2,
	GW_MSG_STATUS_ADDRESS = 3,
    GW_MSG_STATUS_PATROL = 4,
    GW_MSG_STATUS_SELF_DATA = 5,
    GW_MSG_STATUS_FILE_UPLOAD = 6,

    GW_MSG_STATUS_CREATE_GROUP = 7,
    GW_MSG_STATUS_DELETE_GROUP = 8,
    GW_MSG_STATUS_DELETE_GRPMEM = 9,
    GW_MSG_STATUS_GENERAL_TOKEN = 10,
    GW_MSG_STATUS_ENTER_GRP_TOKEN = 11,
};

enum {
	GW_PTT_LOC_TYPE_GPS = 0,
	GW_PTT_LOC_TYPE_CELL = 1,
	GW_PTT_LOC_TYPE_WIFI = 2,
};

enum {
	GW_PTT_NETMODE_2G = 2,
	GW_PTT_NETMODE_3G = 3,
	GW_PTT_NETMODE_4G = 4,
};

typedef struct
{
    int uid;
    int priority;
    int change;
    char name[MAX_NAME_LENGTH];
}GWPttSpeaker;

typedef struct
{
	double lat;
	double lon;
}GWLocGps;

typedef struct
{
	unsigned int cellid;
	unsigned int lac_or_tac;
	int mode;
	char mcc[4];
	char mnc[4];
}GWLocCell;

typedef struct
{
	char *bssid;
	int signal;
	char *ssid;
}GWLocWifi;

typedef struct
{
	union {
		GWLocGps gps;
		GWLocCell cell;
		GWLocWifi wifi;
	}location;
	int type;
}GWPttLocation;

typedef struct
{
    char opttype;
    char num;
    char type;
    char onoff;
    unsigned int token;
    unsigned int gids[8];
    unsigned int uids[8];
}GWGroupOperate;

typedef void (*GWPttEvent)(int event, char *data, int data1);
typedef void (*GWMsgEvent)(int status, char *msg, int length);
typedef void (*GWVideoEvent)(char *data, int length);

int gwPttInit(GWPttEvent event, GWMsgEvent event1, GWVideoEvent event2, char externalCodec, int encodeLevel, int frameSize, int frameNum, char softAEC);

char *gwPttGetVersion(void);

int gwPttConfigServer(int type, char *host, int port);

int gwPttSaveVoice(int open, char *path);

int gwPttNetCheck(int type, char *host, int port);

int gwPttLogin(const char *account, const char *pass, const char *imei, const char *iccid);

int gwPttQueryGroup(void);

int gwPttJoinGroup(int gid, int type);

int gwPttGetLastGid(void);

int gwPttQueryMember(int gid, int type);

int gwPttQueryDispatcher(int gid, int type);

int gwPttSpeakStartToDmr(char dmrtype, int dmrid, long long ms);

int gwPttSpeak(int action, long long ms);

int gwPttHeart(int battery, const char *net);

int gwPttQueryByPage(int queryType, int userType, int id, int pageSize, int pageNum);

int gwPttLogout(void);

int gwPttDuplexCall(int uid, int action);

int gwPttTempGroup(int *uids, int num);

int gwPttRegMsg(int groups[], int type[], int num, char tls, char offline);

int gwPttSendVideoMsg(char *sid, const char *data);

int gwPttSendMsg(int sid, const char *snm, int type, int id, char *rnm, int msgType, const char *content, const char *thumbUrl, long long ms, char offline);

int gwPttSosMsg(int sid, const char *snm, int id, long long ms, double lat, double lon, char hasLoc);

int gwPttSendSelf(int rid, int type, char *data, int len, char offline, char old);

int gwPttSendText(int rid, int type, char *data, int len);

int gwPttAddDelGroupOfflineMsg(int group, int type, int act);

int gwPttExitMsg(void);

int gwPttReportLocation(GWPttLocation *location, char **out, long long time);

int gwPttGetLocation(GWPttLocation *location, char **out, long long time);

int gwPttUploadLocation(char *data, char **response);

int gwPttGetWeather(char *param, char **response);

int gwPttUploadPatrol(char *data, char **response);

int gwPttUploadFile(char *data, int type, char **response);

int gwPttChannal(char *acc, int type, int mask, char *acc1, char **response);

int gwPttGroupOperate(GWGroupOperate groupOperate, char **response);

unsigned int gwPttGetTime(void);

int gwPttRecordDataReady(int len, long long ms);

void gwPttUpdateLeftVoicePacket(int num);

int gwPttVoiceAttachBuffer(int type, char *pcm, int size);

void gwSaveFarVoice(char *pcm, int len);

void gwProcessNearVoice(char *nearPcm, int length);

int gwPttConfigAgeGain(int db);

int gwPttConfigAgeBalance(int balanceLevel);

#ifdef __cplusplus
}
#endif

#endif //GWSD_GWSD_PTT_H
