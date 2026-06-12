#pragma once

void DiscordPresence_Init();
void DiscordPresence_Shutdown();
void DiscordPresence_Tick();

void DiscordPresence_SetMenu();
void DiscordPresence_SetGame(const char* serverName, const char* mapName);
void DiscordPresence_SetEditor(const char* editorName, const char* mapName);

