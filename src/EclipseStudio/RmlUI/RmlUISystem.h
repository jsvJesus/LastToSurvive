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
	using FCharacterCallback = std::function<void(const char* Action, const char* Value)>;

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

	void SetCharacterCallback(FCharacterCallback Callback);

	bool LoadCharacterEditor();
	void ShowCharacterEditor();
	void HideCharacterEditor();

	bool IsCharacterEditorReady() const;
	bool IsCharacterEditorVisible() const;

	void ToggleDebugger();
	void SetDebuggerVisible(bool bVisible);
	bool IsDebuggerVisible() const;
	void ReloadVisibleDocuments();
	void ReloadStyleSheets();
	bool IsLiveEditorVisible() const;

	void SetCharacterMode(int ModeIndex);
	void SetCharacterSelectedState(int StateIndex);
	void SetCharacterSelectedDirection(int DirectionIndex);

	void SetCharacterToggle(
		const char* ButtonId,
		const char* ValueId,
		bool bEnabled
	);

	void SetCharacterText(
		const char* ElementId,
		const char* Text
	);

	void SetCharacterVisible(
		const char* ElementId,
		bool bVisible
	);

	void SetCharacterAnimationList(
		const char** Names,
		int Count,
		int SelectedIndex
	);

	void SetCharacterAnimationStack(
		const char** Names,
		const char** Data,
		int Count
	);

	void SetCharacterAnimationInfo(
		float Length,
		int Frames,
		int Tracks,
		float FrameRate,
		const char* AnimationName,
		const char* AnimationFile
	);

	float GetCharacterInputValue(
		const char* ElementId,
		float DefaultValue
	) const;

	void SetCharacterInputValue(
		const char* ElementId,
		const char* ValueElementId,
		float Value,
		const char* Format
	);

	void SetCharacterInputRange(
		const char* ElementId,
		float Minimum,
		float Maximum,
		float Step
	);

	void SetCharacterEquipmentCategory(
		int CategoryIndex
	);

	void SetCharacterEquipmentSelected(
		const char* ItemName
	);

	void SetCharacterEquipmentList(
		const char** ItemNames,
		int ItemCount,
		int SelectedIndex
	);

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

	class FCharacterClickListener final :
	public Rml::EventListener
	{
	public:
		explicit FCharacterClickListener(
			RmlUISystem* InOwner
		);

		void ProcessEvent(
			Rml::Event& Event
		) override;

		void OnDetach(
			Rml::Element* Element
		) override;

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
	Rml::ElementDocument* CharacterEditorDocument = nullptr;
	Rml::ElementDocument* LiveEditorDocument = nullptr;
	Rml::Element* SelectedLiveElement = nullptr;

	std::unique_ptr<FAppSelectClickListener> AppSelectClickListener;
	std::unique_ptr<FAppMainClickListener> AppMainClickListener;
	std::unique_ptr<FCharacterClickListener> CharacterClickListener;

	FAppSelectCallback AppSelectCallback;
	FAppMainCallback AppMainCallback;
	FCharacterCallback CharacterCallback;

	bool bInitialized = false;
	bool bAppSelectVisible = false;
	bool bLoadingScreenVisible = false;
	bool bAppMainVisible = false;
	bool bCoreInitializedHere = false;
	bool bCharacterEditorVisible = false;
	bool bDebuggerInitialized = false;
	bool bLiveEditorVisible = false;

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
	void SelectLiveElementAt(int X, int Y);
	void SelectLiveElement(Rml::Element* Element);
	void RefreshLiveEditorFields();
	void UpdateLiveEditorHighlight();
	void ApplyLiveEditorChanges();
	Rml::String GetLiveEditorControlValue(const char* ElementId) const;
	void SetLiveEditorControlValue(const char* ElementId, const Rml::String& Value);
	void SetLiveEditorStatus(const char* Text);

	static int GetKeyModifiers();
	static Rml::Input::KeyIdentifier TranslateKey(WPARAM WParam);
	static int TranslateMouseButton(UINT Message);
	static std::wstring GetStudioDataRoot();

	void AttachCharacterEvents();
	void DetachCharacterEvents();
};
