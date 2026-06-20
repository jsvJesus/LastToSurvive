#ifndef LOADINGSCREEN_H
#define LOADINGSCREEN_H

#include "r3d.h"

#include "multiplayer/ClientGameLogic.h"

class LoadingScreen
{
private:
	r3dTexture* m_pBackgroundTex;
	bool m_RenderingDisabled;

	wchar_t* m_MapName;
	wchar_t* m_MapDesc;
	wchar_t* m_TipOfTheDay;

	void ApplyStoredLoadingText();

public:
	explicit LoadingScreen(const char* movieName);
	~LoadingScreen();

	bool Initialize();
	int Update();

	void SetLoadingTexture(const char* ImagePath);
	void SetData(
		const char* ImagePath,
		const wchar_t* Name,
		const wchar_t* Message,
		const wchar_t* tip_of_the_day
	);

	void SetProgress(float progress);
	void SetRenderingDisabled(bool disabled)
	{
		m_RenderingDisabled = disabled;
	}
};

void StartLoadingScreen();
void DisableLoadingRendering();
void StopLoadingScreen();
void SetLoadingTexture(const char* ImagePath);
void SetLoadingProgress(float progress);
void AdvanceLoadingProgress(float add);
float GetLoadingProgress();
void SetLoadingPhase(const char* Phase);

int DoLoadingScreen(
	volatile LONG* loading,
	const wchar_t* LevelName,
	const wchar_t* LevelDescription,
	const char* LevelFolder,
	float TimeOut,
	int gameMode
);

template <typename T>
int DoConnectScreen(
	T* Logic,
	bool (T::*CheckFunc)(),
	const wchar_t* Message,
	float TimeOut
);

int DoConnectScreen(
	volatile LONG* loading,
	const wchar_t* Message,
	float TimeOut
);

#define PROGRESS_LOAD_LEVEL_START 0.033f
#define PROGRESS_LOAD_LEVEL_END 0.8f
#define PLAYER_CACHE_INIT_END 0.85f

#endif