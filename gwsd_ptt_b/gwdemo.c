// gwdemo.c : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>

#define VERSION "v1.0.1"
#define AUTHOR  "Jack He"
#define DATE    "2026/03/09"

extern void pttClientStart(const char *account, const char *password, const char *dns, int port, int msgport);
extern void pttClientQueryGroup();
extern void pttClientQueryMember(int gid);
extern void pttClientSpeakStart();
extern void pttClientSpeakEnd();
extern void pttClientJoinGroup(int gid);
extern void pttClientOffline();

void GetCurrentTimeString(char* buffer, int bufferSize) {
	time_t now;
	struct tm *local;

	time(&now);
	local = localtime(&now);

	strftime(buffer, bufferSize, "%Y-%m-%d %H:%M:%S", local);
}

void showMenuScreen(char *account, char *dns, int voiceport, int msgport) 
{
	char timeStr[64];
	GetCurrentTimeString(timeStr, sizeof(timeStr));
	//system("cls");

	printf("╔═══════════════════════════════════════════════════════╗\n");
	printf("║              GWSD Intercom System Platform            ║\n");
	printf("╠═══════════════════════════════════════════════════════╣\n");
	printf("║ Version: %s              Developer: %s       ║\n", VERSION, AUTHOR);
	printf("╠═══════════════════════════════════════════════════════╣\n");
	printf("║ Function Options:                                      ║\n");
	printf("║  ┌─────────────────────────────────────────────┐      ║\n");
	printf("║  │  0. System Help                            │      ║\n");
	printf("║  │  1. Query Groups                           │      ║\n");
	printf("║  │  2. Query Members                          │      ║\n");
	printf("║  │  3. Start Speaking                         │      ║\n");
	printf("║  │  4. Stop Speaking                          │      ║\n");
	printf("║  │  5. Join Group                             │      ║\n");
	printf("║  │  6. Logout                                 │      ║\n");
	printf("║  └─────────────────────────────────────────────┘      ║\n");
	printf("╠═══════════════════════════════════════════════════════╣\n");
	printf("║ Current User: %-10s | Time: %-21s ║\n", account, timeStr);
	printf("║ Server Address: %s:%d,%d ║\n", dns, voiceport, msgport);
	printf("╠═══════════════════════════════════════════════════════╣\n");
	printf("║ Please select operation (0-6) >                      ║\n");
	printf("╚═══════════════════════════════════════════════════════╝\n");
	printf(" > ");
}

void showWelcomeScreen() 
{
	system("cls");

	printf("\n\n\n");
	printf("              ╔══════════════════════════════════════╗\n");
	printf("              ║                                      ║\n");
	printf("              ║    Welcome to GWSD Intercom System   ║\n");
	printf("              ║                                      ║\n");
	printf("              ║          Developer: %s          ║\n", AUTHOR);
	printf("              ║          Version: %s             ║\n", VERSION);
	printf("              ║           Year: %s           ║\n", DATE);
	printf("              ║                                      ║\n");
	printf("              ╚══════════════════════════════════════╝\n");

	Sleep(2000);  // Show for 2 seconds

	// Loading animation
	printf("\n\n              System initializing");
	for (int i = 0; i < 3; i++) {
		printf(".");
		Sleep(500);
	}
	printf("\n\n");
}

void showGoodbyeScreen() 
{
	system("cls");

	printf("\n\n\n");
	printf("              ╔══════════════════════════════════════╗\n");
	printf("              ║                                      ║\n");
	printf("              ║    Thank you for using GWSD System   ║\n");
	printf("              ║                                      ║\n");
	printf("              ║          Developer: %s          ║\n", AUTHOR);
	printf("              ║          Goodbye!                    ║\n");
	printf("              ║                                      ║\n");
	printf("              ╚══════════════════════════════════════╝\n");
}

static int ValidatePort(int port) 
{
	return (port >= 1024 && port <= 65535);
}

// 验证DNS格式（简单验证）
static int ValidateDNS(const char* dns) 
{
	return (strlen(dns) > 0 && strlen(dns) < 256);
}

// 验证IMEI格式（简单验证）
static int ValidateAccountAndPassword(const char* imei) 
{
	return (strlen(imei) > 0 && strlen(imei) < 32);
}

void showConfigScreen(char* dns, int* voicePort, int* msgPort, char* account, char* password) {
	int valid = 0;

	do {
		system("cls");

		printf("╔═══════════════════════════════════════════════════════╗\n");
		printf("║           GWSD System Configuration                   ║\n");
		printf("╠═══════════════════════════════════════════════════════╣\n");
		printf("║                                                       ║\n");
		printf("║  Please enter the required configuration:             ║\n");
		printf("║                                                       ║\n");
		printf("║  ┌─────────────────────────────────────────────┐      ║\n");
		printf("║  │                                             │      ║\n");
		printf("║  │  DNS Server Address:                       │      ║\n");
		printf("║  │  (Required for connection)                 │      ║\n");
		printf("║  │  (Default: chn-gwsd-access.hawk-sight.com) │      ║\n");
		printf("║  │                                             │      ║\n");
		printf("║  │  > ");
		fflush(stdout);
		scanf("%s", dns);

		// 添加默认值提示
		if (strlen(dns) == 0) {
			strcpy(dns, "chn-gwsd-access.hawk-sight.com");
		}

		printf("║  │                                             │      ║\n");
		printf("║  │                                             │      ║\n");
		printf("║  │  Voice Server Port:                         │      ║\n");
		printf("║  │  (1024-65535, Default: 23003)               │      ║\n");
		printf("║  │                                             │      ║\n");
		printf("║  │  > ");
		fflush(stdout);
		scanf("%d", voicePort);

		// 验证端口
		if (*voicePort <= 0) {
			*voicePort = 23003; // 默认端口
		}

		printf("║  │                                             │      ║\n");
		printf("║  │                                             │      ║\n");
		printf("║  │  Msg Server Port:                           │      ║\n");
		printf("║  │  (1024-65535, Default: 51883)               │      ║\n");
		printf("║  │                                             │      ║\n");
		printf("║  │  > ");
		fflush(stdout);
		scanf("%d", msgPort);

		// 验证端口
		if (*msgPort <= 0) {
			*msgPort = 23003; // 默认端口
		}

		printf("║  │                                             │      ║\n");
		printf("║  │                                             │      ║\n");
		printf("║  │  Device Account:                            │      ║\n");
		printf("║  │  (Unique device ID, e.g., test01)           │      ║\n");
		printf("║  │                                             │      ║\n");
		printf("║  │  > ");
		fflush(stdout);
		scanf("%s", account);

		printf("║  │                                             │      ║\n");
		printf("║  │                                             │      ║\n");
		printf("║  │  Device Pasword:                            │      ║\n");
		printf("║  │  (Unique device ID, e.g., 111111)           │      ║\n");
		printf("║  │                                             │      ║\n");
		printf("║  │  > ");
		fflush(stdout);
		scanf("%s", password);

		printf("║  │                                             │      ║\n");
		printf("║  └─────────────────────────────────────────────┘      ║\n");
		printf("║                                                       ║\n");

		// 验证输入
		valid = ValidateDNS(dns) && ValidatePort(*voicePort) && ValidatePort(*msgPort) && ValidateAccountAndPassword(account) && ValidateAccountAndPassword(password);

		if (!valid) {
			printf("║  Invalid input detected. Please check:            ║\n");
			if (!ValidateDNS(dns)) printf("║     DNS address is required                        ║\n");
			if (!ValidatePort(*voicePort)) printf("║     Voice Port must be between 1024-65535                ║\n");
			if (!ValidatePort(*msgPort)) printf("║     Msg Port must be between 1024-65535                ║\n");
			if (!ValidateAccountAndPassword(account)) printf("║     Device account is required                          ║\n");
			if (!ValidateAccountAndPassword(password)) printf("║     Device password is required                          ║\n");
			printf("║                                                       ║\n");
			printf("║  Press Enter to try again...                         ║\n");
			printf("║                                                       ║\n");
			getchar();
			getchar();
		}

	} while (!valid);

	// 显示配置确认
	system("cls");
	printf("╔═══════════════════════════════════════════════════════╗\n");
	printf("║           Configuration Summary                      ║\n");
	printf("╠═══════════════════════════════════════════════════════╣\n");
	printf("║                                                       ║\n");
	printf("║  Your configuration has been saved:                  ║\n");
	printf("║                                                       ║\n");
	printf("║  ┌─────────────────────────────────────────────┐      ║\n");
	printf("║  │                                             │      ║\n");
	printf("║  │  DNS Server:   %-25s       │      ║\n", dns);
	printf("║  │  Voice Server Port:  %-25d │      ║\n", *voicePort);
	printf("║  │  Msg Server Port:  %-25d   │      ║\n", *msgPort);
	printf("║  │  Device Account: %-25s     │      ║\n", account);
	printf("║  │  Device Password: %-25s    │      ║\n", password);
	printf("║  │                                             │      ║\n");
	printf("║  └─────────────────────────────────────────────┘      ║\n");
	printf("║                                                       ║\n");
	printf("║  Press Enter to start the system...                   ║\n");
	printf("║                                                       ║\n");
	printf("╚═══════════════════════════════════════════════════════╝\n");
	getchar();
	getchar();
}

void showMemberPageConfigScreen(int *gid)
{
	system("cls");

	printf("┌───────────────────────────────────────────────────────┐\n");
	printf("│              Advanced Member Query                     │\n");
	printf("├───────────────────────────────────────────────────────┤\n");
	printf("│                                                       │\n");
	printf("│  Enter query parameters (Format: gid)                 │\n");
	printf("│  Example: 12, for 12 group                            │\n");
	printf("│                                                       │\n");
	printf("│  Parameter details:                                   │\n");
	printf("│    Group ID:      Specific group id                   │\n");
	printf("│                                                       │\n");
	printf("│                                                       │\n");
	printf("│  Input format: [gid]                                  │\n");
	printf("│                                                       │\n");
	printf("│  > ");
	fflush(stdout);
	scanf("%d", gid);
	printf("│                                                       │\n");
	printf("│  Parameters accepted:                                 │\n");
	printf("│    Group ID:      %-4d                           │\n",*gid);
	printf("│                                                       │\n");
	printf("└───────────────────────────────────────────────────────┘\n");
}

void showGroupConfigScreen(int *gid)
{
	system("cls");

	printf("┌───────────────────────────────────────────────────────┐\n");
	printf("│              Group Selection                          │\n");
	printf("├───────────────────────────────────────────────────────┤\n");
	printf("│                                                       │\n");
	printf("│  Enter Group ID:                                      │\n");
	printf("│                                                       │\n");
	printf("│  >0 = Specific Group                                  │\n");
	printf("│  Examples: 10001, 10002, 20003, 10008                 │\n");
	printf("│                                                       │\n");
	printf("│  > ");
	fflush(stdout);
	scanf("%d", gid);
	printf("│                                                       │\n");
	printf("│  Group selected:                                      │\n");
	printf("│    Group ID: %-4d                              │\n", *gid);
	printf("│                                                       │\n");
	printf("└───────────────────────────────────────────────────────┘\n");
}

int main()
{
	int cmd;
	int gid;
	int token, type;
	char account[33];
	char password[33];
	char dns[256];
	int voiceport;
	int msgport;
	showWelcomeScreen();
	showConfigScreen(dns, &voiceport, &msgport, account, password);
	pttClientStart(account, password, dns, voiceport, msgport);
	Sleep(1000);
	while (1)
	{
		showMenuScreen(account, dns, voiceport, msgport);
		scanf("%d", &cmd);
		switch (cmd)
		{
		case 1:
			pttClientQueryGroup();
			break;
		case 2:
			showMemberPageConfigScreen(&gid);
			pttClientQueryMember(gid);
			break;
		case 3:
			pttClientSpeakStart();
			break;
		case 4:
			pttClientSpeakEnd();
			break;
		case 5:
			showGroupConfigScreen(&gid);
			pttClientJoinGroup(gid);
			break;
		case 6:
			pttClientOffline();
			Sleep(5000);
			showGoodbyeScreen();
			ExitProcess(0);
			break;
		default:
			break;
		}
	}
	return 0;
}

