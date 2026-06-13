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

	HWND Hwnd = nullptr;

	std::unique_ptr<RmlSystemInterface> SystemInterface;
	std::unique_ptr<RmlFileInterface> FileInterface;
	std::unique_ptr<RmlRenderDX9> RenderInterface;

	Rml::Context* Context = nullptr;
	Rml::ElementDocument* AppSelectDocument = nullptr;
	Rml::ElementDocument* LoadingScreenDocument = nullptr;

	std::unique_ptr<FAppSelectClickListener> AppSelectClickListener;

	FAppSelectCallback AppSelectCallback;

	bool bInitialized = false;
	bool bAppSelectVisible = false;
	bool bLoadingScreenVisible = false;
	bool bCoreInitializedHere = false;

	float LoadingProgressTarget = 0.0f;
	float LoadingProgressVisual = 0.0f;

	void ApplyLoadingScreenProgress(float Progress);

	int ClientWidth = 1;
	int ClientHeight = 1;

	void UpdateClientSize();
	void AttachAppSelectEvents();
	void DetachAppSelectEvents();

	static int GetKeyModifiers();
	static Rml::Input::KeyIdentifier TranslateKey(WPARAM WParam);
	static int TranslateMouseButton(UINT Message);
	static std::wstring GetStudioDataRoot();
};
