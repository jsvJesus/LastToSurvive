#pragma once

#include "RmlRenderDX9.h"
#include "RmlSystemInterface.h"
#include "RmlFileInterface.h"

#include <RmlUi/Core.h>
#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/ElementDocument.h>

#include <windows.h>
#include <functional>
#include <memory>

class RmlUISystem final
{
public:
	using FAppSelectCallback = std::function<void(const char* ModeId)>;
	using FAppMainCallback = std::function<void(const char* Action, const char* Value)>;

	RmlUISystem();
	~RmlUISystem();

	bool Init(HWND InHwnd, IDirect3DDevice9* InDevice, bool bLoadAppSelectOnInit = true);
	void Shutdown();

	void Update(float DeltaSeconds);
	void Render();

	bool ProcessWin32Message(HWND Hwnd, UINT Message, WPARAM WParam, LPARAM LParam, LRESULT* OutResult);

	void OnDeviceLost();
	void OnDeviceReset();

	void SetAppSelectCallback(FAppSelectCallback Callback);

	bool LoadAppSelect();
	void ShowAppSelect();
	void HideAppSelect();

	bool LoadLoadingScreen();
	void ShowLoadingScreen();
	void HideLoadingScreen();
	void SetLoadingScreenData(const wchar_t* Name, const wchar_t* Description, const wchar_t* Tip);
	void SetLoadingScreenProgress(float Progress);

	bool IsInitialized() const;
	bool IsAppSelectReady() const;
	bool IsAppSelectVisible() const;
	bool IsLoadingScreenReady() const;
	bool IsLoadingScreenVisible() const;

	void SetAppMainCallback(FAppMainCallback Callback);

	bool LoadAppMain();
	void ShowAppMain();
	void HideAppMain();

	void SetAppMainTab(int TabIndex);
	void SetAppMainMaps(const char** Names, int Count);
	void SetAppMainScrollInfo(int FirstIndex, int VisibleCount, int TotalCount);
	void SetAppMainSelectedLevel(const char* Name);
	Rml::String GetAppMainCreateLevelName() const;

	bool IsAppMainReady() const;
	bool IsAppMainVisible() const;

	void SetAppMainCreateOptions(
		bool bHaveTerrain,
		bool bTerrainV2,
		int TerrainSizeIndex,
		int SplatSizeIndex,
		float CellSize,
		float Height
	);

	bool GetAppMainCreateData(
		char* OutName,
		int OutNameSize,
		bool& bOutHaveTerrain,
		bool& bOutTerrainV2,
		int& OutTerrainSizeIndex,
		int& OutSplatSizeIndex,
		float& OutCellSize,
		float& OutHeight
	) const;

private:
	class FAppSelectClickListener final : public Rml::EventListener
	{
	public:
		explicit FAppSelectClickListener(RmlUISystem* InOwner);

		void ProcessEvent(Rml::Event& Event) override;
		void OnDetach(Rml::Element* Element) override;

	private:
		RmlUISystem* Owner = nullptr;
	};

	class FAppMainClickListener final : public Rml::EventListener
	{
	public:
		explicit FAppMainClickListener(RmlUISystem* InOwner);

		void ProcessEvent(Rml::Event& Event) override;
		void OnDetach(Rml::Element* Element) override;

	private:
		RmlUISystem* Owner = nullptr;
	};

	HWND Hwnd = nullptr;

	std::unique_ptr<RmlSystemInterface> SystemInterface;
	std::unique_ptr<RmlFileInterface> FileInterface;
	std::unique_ptr<RmlRenderDX9> RenderInterface;

	Rml::Context* Context = nullptr;
	Rml::ElementDocument* AppSelectDocument = nullptr;
	Rml::ElementDocument* LoadingScreenDocument = nullptr;
	Rml::ElementDocument* AppMainDocument = nullptr;

	std::unique_ptr<FAppSelectClickListener> AppSelectClickListener;
	std::unique_ptr<FAppMainClickListener> AppMainClickListener;

	FAppSelectCallback AppSelectCallback;
	FAppMainCallback AppMainCallback;

	bool bInitialized = false;
	bool bAppSelectVisible = false;
	bool bLoadingScreenVisible = false;
	bool bAppMainVisible = false;
	bool bCoreInitializedHere = false;

	float LoadingProgressTarget = 0.0f;
	float LoadingProgressVisual = 0.0f;

	void ApplyLoadingScreenProgress(float Progress);

	int ClientWidth = 1;
	int ClientHeight = 1;

	void UpdateClientSize();
	void AttachAppSelectEvents();
	void DetachAppSelectEvents();
	void AttachAppMainEvents();
	void DetachAppMainEvents();

	static int GetKeyModifiers();
	static Rml::Input::KeyIdentifier TranslateKey(WPARAM WParam);
	static int TranslateMouseButton(UINT Message);
	static std::wstring GetStudioDataRoot();
};
