//
// Created by hejingsheng on 2024/12/4.
//
#include "GWLog.h"
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <windows.h>

#define TAG  "GWAPP->"

int gwlog_level = GW_LOG_LEVEL_INFO;

static uint64_t get_current_time_ms(void)
{
	FILETIME ft;
	ULARGE_INTEGER uli;
	static LARGE_INTEGER perf_freq = { 0 };
	static BOOL has_perf_counter = FALSE;
	static BOOL initialized = FALSE;

	if (!initialized) {
		has_perf_counter = QueryPerformanceFrequency(&perf_freq);
		initialized = TRUE;
	}

	if (has_perf_counter && perf_freq.QuadPart) {
		// 使用高精度性能计数器
		LARGE_INTEGER counter;
		QueryPerformanceCounter(&counter);
		return (counter.QuadPart * 1000) / perf_freq.QuadPart;
	}
	else {
		// 使用系统时间
		GetSystemTimeAsFileTime(&ft);
		uli.LowPart = ft.dwLowDateTime;
		uli.HighPart = ft.dwHighDateTime;
		// FILETIME 是从 1601-01-01 开始的 100-ns 间隔
		// 转换为从 1970-01-01 开始的毫秒数
		uli.QuadPart -= 116444736000000000ULL; // 从 1601 调整到 1970
		return uli.QuadPart / 10000; // 100-ns 转换为 ms
	}
}

void print_log(const char *fmt,...)
{
    static char buf[256];
    va_list argp;
    memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf)-1, "%llu:%s", get_current_time_ms(), TAG);
    va_start(argp, fmt);
    vsnprintf(buf+ strlen(buf), sizeof(buf)-1-strlen(buf), fmt, argp);
    va_end(argp);
	printf("%s\n", buf);
}
