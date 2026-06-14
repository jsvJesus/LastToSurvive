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
#include <string>

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
static bool gDiscordUseIpc = true;
static float gDiscordNextStatusLog = 0.0f;
static float gDiscordNextIpcConnect = 0.0f;
static __int64 gDiscordStartTime = 0;

#if defined(_WIN64)
static Discord_Client gDiscordClient;
static HANDLE gDiscordIpcPipe = INVALID_HANDLE_VALUE;
static int gDiscordIpcNonce = 1;
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
static std::string DiscordPresence_JsonEscape(const char* str)
{
	std::string out;
	if(!str)
		return out;

	for(const unsigned char* p = (const unsigned char*)str; *p; ++p)
	{
		switch(*p)
		{
		case '\\': out += "\\\\"; break;
		case '"': out += "\\\""; break;
		case '\b': out += "\\b"; break;
		case '\f': out += "\\f"; break;
		case '\n': out += "\\n"; break;
		case '\r': out += "\\r"; break;
		case '\t': out += "\\t"; break;
		default:
			if(*p < 32)
			{
				char tmp[8];
				sprintf(tmp, "\\u%04x", (unsigned int)*p);
				out += tmp;
			}
			else
			{
				out += (char)*p;
			}
			break;
		}
	}

	return out;
}

static bool DiscordPresence_IpcWrite(uint32_t op, const std::string& json)
{
	if(gDiscordIpcPipe == INVALID_HANDLE_VALUE)
		return false;

	uint32_t header[2] = { op, (uint32_t)json.size() };
	DWORD written = 0;

	if(!WriteFile(gDiscordIpcPipe, header, sizeof(header), &written, NULL) || written != sizeof(header))
		return false;

	if(!json.empty() && (!WriteFile(gDiscordIpcPipe, json.c_str(), (DWORD)json.size(), &written, NULL) || written != json.size()))
		return false;

	return true;
}

static void DiscordPresence_IpcClose()
{
	if(gDiscordIpcPipe != INVALID_HANDLE_VALUE)
	{
		CloseHandle(gDiscordIpcPipe);
		gDiscordIpcPipe = INVALID_HANDLE_VALUE;
	}
	gDiscordReady = false;
}

static bool DiscordPresence_IpcConnect()
{
	if(gDiscordIpcPipe != INVALID_HANDLE_VALUE)
		return true;

	if(r3dGetTime() < gDiscordNextIpcConnect)
		return false;

	gDiscordNextIpcConnect = r3dGetTime() + 5.0f;

	for(int i = 0; i < 10; ++i)
	{
		char pipeName[64];
		sprintf(pipeName, "\\\\.\\pipe\\discord-ipc-%d", i);

		HANDLE pipe = CreateFileA(pipeName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		if(pipe == INVALID_HANDLE_VALUE)
			continue;

		gDiscordIpcPipe = pipe;

		std::string handshake = "{\"v\":1,\"client_id\":\"";
		handshake += DiscordPresence_JsonEscape(d_discord_app_id ? d_discord_app_id->GetString() : DISCORD_DEFAULT_APP_ID);
		handshake += "\"}";

		if(!DiscordPresence_IpcWrite(0, handshake))
		{
			DiscordPresence_IpcClose();
			continue;
		}

		gDiscordReady = true;
		gDiscordPresenceDirty = true;
		r3dOutToLog("DiscordPresence: connected via local IPC %s\n", pipeName);
		return true;
	}

	return false;
}

static bool DiscordPresence_IpcSendActivity(bool clearPresence)
{
	if(!DiscordPresence_IpcConnect())
		return false;

	char nonce[64];
	sprintf(nonce, "lts-%d", gDiscordIpcNonce++);

	std::string json = "{\"cmd\":\"SET_ACTIVITY\",\"args\":{\"pid\":";
	char pid[32];
	sprintf(pid, "%u", (unsigned int)GetCurrentProcessId());
	json += pid;
	json += ",\"activity\":";

	if(clearPresence)
	{
		json += "null";
	}
	else
	{
		char startTime[64];
		sprintf(startTime, "%I64d", gDiscordStartTime);

		json += "{\"type\":0";
		json += ",\"name\":\"Eclipse Studio\"";
		json += ",\"details\":\"" + DiscordPresence_JsonEscape(gDiscordDetails) + "\"";
		json += ",\"state\":\"" + DiscordPresence_JsonEscape(gDiscordState) + "\"";
		json += ",\"timestamps\":{\"start\":";
		json += startTime;
		json += "}";
		json += ",\"assets\":{\"large_image\":\"";
		json += DiscordPresence_JsonEscape(DISCORD_LARGE_IMAGE_KEY);
		json += "\",\"large_text\":\"";
		json += DiscordPresence_JsonEscape(gDiscordLargeText);
		json += "\",\"small_text\":\"";
		json += DiscordPresence_JsonEscape(gDiscordSmallText);
		json += "\"}";
		json += "}";
	}

	json += "},\"nonce\":\"";
	json += nonce;
	json += "\"}";

	if(!DiscordPresence_IpcWrite(1, json))
	{
		r3dOutToLog("DiscordPresence: IPC write failed, reconnecting\n");
		DiscordPresence_IpcClose();
		gDiscordPresenceDirty = true;
		return false;
	}

	if(!clearPresence)
		r3dOutToLog("DiscordPresence: IPC activity sent: %s | %s | image=%s\n", gDiscordDetails, gDiscordState, DISCORD_LARGE_IMAGE_KEY);

	return true;
}

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
		if(errorDetail == 4004)
		{
			gDiscordUseIpc = true;
			gDiscordPresenceDirty = true;
			r3dOutToLog("DiscordPresence: Social SDK auth failed, switching to local IPC\n");
		}
	}
}

static void DiscordPresence_Send()
{
	if(!gDiscordStarted || !gDiscordPresenceDirty)
		return;

	if(gDiscordUseIpc)
	{
		if(DiscordPresence_IpcSendActivity(false))
			gDiscordPresenceDirty = false;
		return;
	}

	if(!gDiscordReady)
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

	// Rich Presence only needs Discord desktop IPC. This avoids the Social SDK
	// gateway auth flow, which can reject non-social apps with close code 4004.
	gDiscordUseIpc = true;
	gDiscordStarted = true;
	gDiscordPresenceDirty = true;
	DiscordPresence_IpcConnect();
	r3dOutToLog("DiscordPresence: using local IPC appId=%I64u\n", appId);
	return;

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

	if(gDiscordUseIpc)
	{
		DiscordPresence_IpcSendActivity(true);
		DiscordPresence_IpcClose();
	}
	else
	{
		Discord_Client_ClearRichPresence(&gDiscordClient);
		Discord_Client_Disconnect(&gDiscordClient);
		for(int i = 0; i < 8; ++i)
		{
			Discord_RunCallbacks();
			Sleep(10);
		}
		Discord_Client_Drop(&gDiscordClient);
		Discord_ResetCallbacks();
	}

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

	if(!gDiscordUseIpc)
		Discord_RunCallbacks();

	if(!gDiscordReady && r3dGetTime() >= gDiscordNextStatusLog)
	{
		gDiscordNextStatusLog = r3dGetTime() + 5.0f;
		if(gDiscordUseIpc)
			r3dOutToLog("DiscordPresence: waiting for Discord IPC\n");
		else
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
