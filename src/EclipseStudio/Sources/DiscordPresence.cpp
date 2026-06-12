#include "r3dPCH.h"
#include "r3d.h"

#include "DiscordPresence.h"

#include "../../Eternity/SF/Console/Config.h"

#if defined(_WIN64)
#define DISCORD_API __declspec(dllimport)
#include "../../../External/discord_social_sdk/include/cdiscord.h"
#endif

#include <stdlib.h>
#include <time.h>

static const char* DISCORD_DEFAULT_APP_ID = "1515111956263211049";
static const char* DISCORD_LARGE_IMAGE_KEY = "lts_logo";

static char gDiscordDetails[128] = "In Studio";
static char gDiscordState[128] = "Main Menu";
static char gDiscordLargeText[128] = "Eclipse Studio";
static char gDiscordSmallText[128] = "WarZ Editor";
static bool gDiscordPresenceDirty = false;
static bool gDiscordReady = false;
static bool gDiscordStarted = false;
static bool gDiscordLoggedDisabled = false;
static float gDiscordNextStatusLog = 0.0f;
static __int64 gDiscordStartTime = 0;

#if defined(_WIN64)
static Discord_Client gDiscordClient;
#endif

static void DiscordPresence_Copy(char* dst, size_t dstSize, const char* src)
{
	if(!dst || dstSize == 0)
		return;

	if(!src || !*src)
		src = "Unknown";

	r3dscpy(dst, src);
	dst[dstSize - 1] = 0;
}

static const char* DiscordPresence_CleanMapName(const char* mapName)
{
	if(!mapName || !*mapName)
		return "No map selected";

	if(!_strnicmp(mapName, "Levels\\", 7) || !_strnicmp(mapName, "Levels/", 7))
		mapName += 7;

	return mapName;
}

static uint64_t DiscordPresence_GetAppId()
{
	const char* appId = d_discord_app_id ? d_discord_app_id->GetString() : DISCORD_DEFAULT_APP_ID;
	if(!appId || !*appId)
		appId = DISCORD_DEFAULT_APP_ID;

	return _strtoui64(appId, NULL, 10);
}

#if defined(_WIN64)
static Discord_String DiscordPresence_String(const char* str)
{
	Discord_String s;
	if(!str)
		str = "";

	s.ptr = (uint8_t*)str;
	s.size = strlen(str);
	return s;
}

static void DiscordPresence_UpdateFinished(Discord_ClientResult* result, void* userData)
{
	(void)userData;

	if(!result)
		return;

	if(!Discord_ClientResult_Successful(result))
	{
		r3dOutToLog("DiscordPresence: UpdateRichPresence failed, code=%d\n", Discord_ClientResult_ErrorCode(result));
	}
	else
	{
		r3dOutToLog("DiscordPresence: UpdateRichPresence sent: %s | %s | image=%s\n", gDiscordDetails, gDiscordState, DISCORD_LARGE_IMAGE_KEY);
	}
}

static void DiscordPresence_Log(Discord_String message, Discord_LoggingSeverity severity, void* userData)
{
	(void)userData;

	if(message.ptr && message.size)
		r3dOutToLog("DiscordPresence SDK[%d]: %.*s\n", (int)severity, (int)message.size, (const char*)message.ptr);

	Discord_Free(message.ptr);
}

static void DiscordPresence_StatusChanged(Discord_Client_Status status, Discord_Client_Error error, int32_t errorDetail, void* userData)
{
	(void)userData;

	gDiscordReady = status == Discord_Client_Status_Ready;
	if(gDiscordReady)
	{
		gDiscordPresenceDirty = true;
		r3dOutToLog("DiscordPresence: connected\n");
	}
	else if(error != Discord_Client_Error_None)
	{
		r3dOutToLog("DiscordPresence: status=%d error=%d detail=%d\n", (int)status, (int)error, (int)errorDetail);
	}
}

static void DiscordPresence_Send()
{
	if(!gDiscordStarted || !gDiscordPresenceDirty)
		return;

	Discord_Activity activity;
	Discord_Activity_Init(&activity);

	uint64_t appId = DiscordPresence_GetAppId();
	Discord_Activity_SetApplicationId(&activity, &appId);

	Discord_String name = DiscordPresence_String("Eclipse Studio");
	Discord_Activity_SetName(&activity, name);
	Discord_Activity_SetType(&activity, Discord_ActivityTypes_Playing);
	Discord_Activity_SetSupportedPlatforms(&activity, Discord_ActivityGamePlatforms_Desktop);

	Discord_String details = DiscordPresence_String(gDiscordDetails);
	Discord_String state = DiscordPresence_String(gDiscordState);
	Discord_Activity_SetDetails(&activity, &details);
	Discord_Activity_SetState(&activity, &state);

	Discord_ActivityAssets assets;
	Discord_ActivityAssets_Init(&assets);
	Discord_String largeImage = DiscordPresence_String(DISCORD_LARGE_IMAGE_KEY);
	Discord_String largeText = DiscordPresence_String(gDiscordLargeText);
	Discord_String smallText = DiscordPresence_String(gDiscordSmallText);
	Discord_ActivityAssets_SetLargeImage(&assets, &largeImage);
	Discord_ActivityAssets_SetLargeText(&assets, &largeText);
	Discord_ActivityAssets_SetSmallText(&assets, &smallText);
	Discord_Activity_SetAssets(&activity, &assets);

	Discord_ActivityTimestamps timestamps;
	Discord_ActivityTimestamps_Init(&timestamps);
	Discord_ActivityTimestamps_SetStart(&timestamps, (uint64_t)gDiscordStartTime);
	Discord_Activity_SetTimestamps(&activity, &timestamps);

	Discord_Client_UpdateRichPresence(&gDiscordClient, &activity, DiscordPresence_UpdateFinished, NULL, NULL);

	Discord_ActivityTimestamps_Drop(&timestamps);
	Discord_ActivityAssets_Drop(&assets);
	Discord_Activity_Drop(&activity);

	gDiscordPresenceDirty = false;
}
#endif

void DiscordPresence_Init()
{
#if defined(_WIN64)
	if(gDiscordStarted)
		return;

	if(d_discord_presence && !d_discord_presence->GetBool())
	{
		if(!gDiscordLoggedDisabled)
		{
			r3dOutToLog("DiscordPresence: disabled by d_discord_presence\n");
			gDiscordLoggedDisabled = true;
		}
		return;
	}

	const uint64_t appId = DiscordPresence_GetAppId();
	if(appId == 0)
	{
		r3dOutToLog("DiscordPresence: disabled, invalid d_discord_app_id\n");
		return;
	}

	gDiscordStartTime = _time64(NULL);
	Discord_SetFreeThreaded();
	Discord_Client_Init(&gDiscordClient);
	Discord_Client_SetApplicationId(&gDiscordClient, appId);
	Discord_Client_SetGameWindowPid(&gDiscordClient, (int32_t)GetCurrentProcessId());
	Discord_String logDir = DiscordPresence_String(".");
	Discord_Client_SetLogDir(&gDiscordClient, logDir, Discord_LoggingSeverity_Info);
	Discord_Client_AddLogCallback(&gDiscordClient, DiscordPresence_Log, NULL, NULL, Discord_LoggingSeverity_Info);
	Discord_Client_SetStatusChangedCallback(&gDiscordClient, DiscordPresence_StatusChanged, NULL, NULL);
	Discord_Client_Connect(&gDiscordClient);

	gDiscordStarted = true;
	gDiscordPresenceDirty = true;
	r3dOutToLog("DiscordPresence: connecting appId=%I64u\n", appId);
#endif
}

void DiscordPresence_Shutdown()
{
#if defined(_WIN64)
	if(!gDiscordStarted)
		return;

	Discord_Client_ClearRichPresence(&gDiscordClient);
	Discord_Client_Disconnect(&gDiscordClient);
	Discord_Client_Drop(&gDiscordClient);
	Discord_ResetCallbacks();

	gDiscordStarted = false;
	gDiscordReady = false;
	gDiscordPresenceDirty = false;
#endif
}

void DiscordPresence_Tick()
{
#if defined(_WIN64)
	if(!gDiscordStarted)
		return;

	Discord_RunCallbacks();
	if(!gDiscordReady && r3dGetTime() >= gDiscordNextStatusLog)
	{
		gDiscordNextStatusLog = r3dGetTime() + 5.0f;
		r3dOutToLog("DiscordPresence: waiting, status=%d\n", (int)Discord_Client_GetStatus(&gDiscordClient));
	}
	DiscordPresence_Send();
#endif
}

void DiscordPresence_SetMenu()
{
	DiscordPresence_Copy(gDiscordDetails, sizeof(gDiscordDetails), "In Studio");
	DiscordPresence_Copy(gDiscordState, sizeof(gDiscordState), "Main Menu");
	DiscordPresence_Copy(gDiscordLargeText, sizeof(gDiscordLargeText), "Eclipse Studio");
	DiscordPresence_Copy(gDiscordSmallText, sizeof(gDiscordSmallText), "Menu");
	gDiscordPresenceDirty = true;
	DiscordPresence_Tick();
}

void DiscordPresence_SetGame(const char* serverName, const char* mapName)
{
	if(!serverName || !*serverName)
		serverName = gDiscordSmallText[0] ? gDiscordSmallText : "Game";

	char state[128];
	sprintf(state, "%s - %s", serverName, DiscordPresence_CleanMapName(mapName));

	DiscordPresence_Copy(gDiscordDetails, sizeof(gDiscordDetails), "Playing WarZ");
	DiscordPresence_Copy(gDiscordState, sizeof(gDiscordState), state);
	DiscordPresence_Copy(gDiscordLargeText, sizeof(gDiscordLargeText), DiscordPresence_CleanMapName(mapName));
	DiscordPresence_Copy(gDiscordSmallText, sizeof(gDiscordSmallText), serverName);
	gDiscordPresenceDirty = true;
	DiscordPresence_Tick();
}

void DiscordPresence_SetEditor(const char* editorName, const char* mapName)
{
	char details[128];
	sprintf(details, "%s", editorName && *editorName ? editorName : "Editor");

	DiscordPresence_Copy(gDiscordDetails, sizeof(gDiscordDetails), details);
	DiscordPresence_Copy(gDiscordState, sizeof(gDiscordState), DiscordPresence_CleanMapName(mapName));
	DiscordPresence_Copy(gDiscordLargeText, sizeof(gDiscordLargeText), DiscordPresence_CleanMapName(mapName));
	DiscordPresence_Copy(gDiscordSmallText, sizeof(gDiscordSmallText), editorName);
	gDiscordPresenceDirty = true;
	DiscordPresence_Tick();
}
