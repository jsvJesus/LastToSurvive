#pragma once

#include <RmlUi/Core.h>
#include <RmlUi/Core/EventListener.h>

#include <d3d9.h>
#include <windows.h>

#include <memory>

class RmlFrontEndCharacterPreview;

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
	void PrepareRender();

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
		MainMenu,
		CharacterCreate,
		Skills,
		Shop
	};

	enum EAsyncOperation : LONG
	{
		AsyncOperation_None = 0,
		AsyncOperation_Login,
		AsyncOperation_Profile,
		AsyncOperation_RenameCharacter,
		AsyncOperation_CreateCharacter
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
	bool EnsureCharacterPreview();

	void ShowLogin();
	void ShowMainMenu();
	void ShowCharacterCreate();
	void ShowSkills();
	void ShowShop();

	void HandleClick(
		Rml::Element* Element
	);

	void RequestLogin();
	void RequestRenameCharacter();
	void RequestCreateCharacter();

	void ResetCharacterCreate();

	void AdjustCharacterAppearance(
		const Rml::String& ControlId
	);

	void RefreshCharacterCreateAppearance();

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

	void HandleRenameCharacterResult(
		EAsyncResult Result
	);

	void HandleCreateCharacterResult(
		EAsyncResult Result
	);

	void BuildMainMenu();

	void BuildSkills();

	void SelectSkillNode(
		const Rml::String& SkillNodeId
	);

	void RefreshSkillSelection();

	void RequestLearnSelectedSkill();

	void SetSkillsControlsEnabled(
		bool bEnabled
	);

	void SetSkillsStatus(
		const Rml::String& Text
	);

	void BuildShop();

	void SelectShopItem(
		const Rml::String& ShopItemId
	);

	void RefreshShopSelection();

	void SetShopControlsEnabled(
		bool bEnabled
	);

	void SetShopStatus(
		const Rml::String& Text
	);

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

	void SetCharacterCreateControlsEnabled(
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

	void SetCharacterCreateStatus(
		const Rml::String& Text
	);

	bool IsBusy() const;

	static Rml::String WideToUtf8(
		const wchar_t* Text
	);

	static Rml::String EscapeRmlText(
		const Rml::String& Text
	);

	enum class EPreviewDragMode
	{
		None = 0,
		Rotate,
		Move
	};

	bool IsElementOrChildOfId(
		Rml::Element* Element,
		const char* ParentId
	) const;

	bool IsPointerOverMainMenuElement(
		const char* ElementId
	) const;

	void CancelPreviewDrag();

	void SetElementProperty(
		Rml::ElementDocument* Document,
		const char* ElementId,
		const char* PropertyName,
		const Rml::String& Value
	);

	void SetElementClass(
		Rml::ElementDocument* Document,
		const char* ElementId,
		const char* ClassName,
		bool bEnabled
	);

	void SetElementPercent(
		Rml::ElementDocument* Document,
		const char* ElementId,
		float Percent
	);

private:
	HWND Hwnd = nullptr;

	Rml::Context* Context = nullptr;

	Rml::ElementDocument* LoginDocument = nullptr;
	Rml::ElementDocument* MainMenuDocument = nullptr;
	Rml::ElementDocument* CharacterCreateDocument = nullptr;
	Rml::ElementDocument* SkillsDocument = nullptr;
	Rml::ElementDocument* ShopDocument = nullptr;

	std::unique_ptr<FClickListener> ClickListener;
	std::unique_ptr<RmlFrontEndCharacterPreview> CharacterPreview;

	HANDLE WorkerThread = nullptr;

	volatile LONG AsyncOperation = AsyncOperation_None;
	volatile LONG AsyncResult = AsyncResult_Idle;
	volatile LONG AsyncApiCode = 0;

	char LoginUser[256]{};
	char LoginPassword[256]{};
	char RenameGamertag[64]{};
	char CreateGamertag[64]{};

	int CreateHeroItemID = 20201;
	int CreateHardcore = 0;

	int CreateHeadIndex = 0;
	int CreateBodyIndex = 0;
	int CreateLegsIndex = 0;

	int Width = 1;
	int Height = 1;

	int SelectedCharacterIndex = -1;

	Rml::String SelectedSkillElementId = "skill_node_vitality_1";
	int SelectedSkillBackendId = 0;

	Rml::String SelectedShopItemElementId = "shop_item_0";
	int SelectedShopBackendItemId = 0;

	EScreen CurrentScreen = EScreen::Login;
	ERmlFrontEndResult PendingResult = ERmlFrontEndResult::None;

	bool bInitialized = false;
	bool bRuntimeAcquired = false;
	bool bProfileLoaded = false;

	EPreviewDragMode PreviewDragMode =
		EPreviewDragMode::None;

	POINT PreviewDragLastPoint{};
};