#pragma once

#include <RmlUi/Core.h>
#include <RmlUi/Core/EventListener.h>

#include <d3d9.h>
#include <windows.h>

#include <memory>

enum class ERmlFrontEndResult
{
	None = 0,
	JoinGame,
	Exit
};

class RmlFrontEndContext final
{
public:
	RmlFrontEndContext();
	~RmlFrontEndContext();

	bool Init(
		HWND WindowHandle,
		IDirect3DDevice9* Device
	);

	void Shutdown();

	void Update();
	void Render();

	bool ProcessWin32Message(
		HWND WindowHandle,
		UINT Message,
		WPARAM WParam,
		LPARAM LParam,
		LRESULT* OutResult
	);

	bool IsInitialized() const;

	ERmlFrontEndResult ConsumeResult();

	void ShowLoginMessage(
		const wchar_t* Message
	);

	void ShowMainMenuMessage(
		const wchar_t* Message
	);

	void RefreshProfile();

private:
	enum class EScreen
	{
		Login = 0,
		MainMenu
	};

	enum EAsyncOperation : LONG
	{
		AsyncOperation_None = 0,
		AsyncOperation_Login,
		AsyncOperation_Profile
	};

	enum EAsyncResult : LONG
	{
		AsyncResult_Idle = 0,
		AsyncResult_Working,
		AsyncResult_Success,
		AsyncResult_Timeout,
		AsyncResult_Error,
		AsyncResult_BadPassword,
		AsyncResult_Frozen
	};

	class FClickListener final :
		public Rml::EventListener
	{
	public:
		explicit FClickListener(
			RmlFrontEndContext* InOwner
		);

		void ProcessEvent(
			Rml::Event& Event
		) override;

		void OnDetach(
			Rml::Element* Element
		) override;

	private:
		RmlFrontEndContext* Owner = nullptr;
	};

private:
	bool LoadDocuments();

	void UnloadDocuments();

	void AttachEvents();
	void DetachEvents();

	void RefreshDimensions();

	void ShowLogin();
	void ShowMainMenu();

	void HandleClick(
		Rml::Element* Element
	);

	void RequestLogin();

	void BeginProfileLoad();

	bool StartAsyncOperation(
		EAsyncOperation Operation
	);

	void PollAsyncOperation();

	void StopAsyncOperation();

	static unsigned int WINAPI AsyncThreadEntry(
		void* Parameter
	);

	unsigned int RunAsyncOperation();

	void HandleLoginResult(
		EAsyncResult Result
	);

	void HandleProfileResult(
		EAsyncResult Result
	);

	void BuildMainMenu();

	void SelectCharacter(
		int CharacterIndex
	);

	void RefreshCharacterSelection();

	void RequestQuickJoin();

	void SetLoginControlsEnabled(
		bool bEnabled
	);

	void SetMainMenuControlsEnabled(
		bool bEnabled
	);

	void SetElementEnabled(
		Rml::ElementDocument* Document,
		const char* ElementId,
		bool bEnabled
	);

	void SetElementText(
		Rml::ElementDocument* Document,
		const char* ElementId,
		const Rml::String& Text
	);

	Rml::String GetInputValue(
		Rml::ElementDocument* Document,
		const char* ElementId
	) const;

	void SetInputValue(
		Rml::ElementDocument* Document,
		const char* ElementId,
		const Rml::String& Value
	);

	void SetLoginStatus(
		const Rml::String& Text
	);

	void SetMainMenuStatus(
		const Rml::String& Text
	);

	bool IsBusy() const;

	static Rml::String WideToUtf8(
		const wchar_t* Text
	);

	static Rml::String EscapeRmlText(
		const Rml::String& Text
	);

private:
	HWND Hwnd = nullptr;

	Rml::Context* Context = nullptr;

	Rml::ElementDocument* LoginDocument = nullptr;
	Rml::ElementDocument* MainMenuDocument = nullptr;

	std::unique_ptr<FClickListener> ClickListener;

	HANDLE WorkerThread = nullptr;

	volatile LONG AsyncOperation = AsyncOperation_None;
	volatile LONG AsyncResult = AsyncResult_Idle;

	char LoginUser[256]{};
	char LoginPassword[256]{};

	int Width = 1;
	int Height = 1;

	int SelectedCharacterIndex = -1;

	EScreen CurrentScreen = EScreen::Login;
	ERmlFrontEndResult PendingResult = ERmlFrontEndResult::None;

	bool bInitialized = false;
	bool bRuntimeAcquired = false;
	bool bProfileLoaded = false;
};