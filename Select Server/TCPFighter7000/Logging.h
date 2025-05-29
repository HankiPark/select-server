#ifndef __LOGGING__
#define __LOGGING__

#define LOG_LEVEL_DEBUG		0
#define LOG_LEVEL_ERROR		1
#define LOG_LEVEL_SYSTEM	2




#define LOG(LogLevel, fmt, ...)							\
do {													\
		if (logLevel <= LogLevel)						\
		{												\
			wsprintf(logBuffer, fmt, ##__VA_ARGS__);	\
			Logging(logBuffer, LogLevel);				\
		}												\
} while (0)												\

#define GAMELOG(fmt, ...)								\
do {													\
		wsprintf(logBuffer, fmt, ##__VA_ARGS__);		\
		GameLogging(logBuffer);							\
} while (0)												\


void Logging(WCHAR * str, int logLevel);

void GameLogging(WCHAR * str);

#endif