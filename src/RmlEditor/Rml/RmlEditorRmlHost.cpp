#include "RmlEditorRmlHost.h"

#include "../App/RmlEditorLog.h"

#include <commdlg.h>
#include <windowsx.h>

#include <algorithm>
#include <filesystem>
#include <sstream>

namespace
{
	std::string EscapeRmlText(const std::string& Text)
	{
		std::string Result;
		Result.reserve(Text.size());

		for (const char Character : Text)
		{
			switch (Character)
			{
			case '&':
				Result += "&amp;";
				break;

			case '<':
				Result += "&lt;";
				break;

			case '>':
				Result += "&gt;";
				break;

			case '"':
				Result += "&quot;";
				break;

			case '\'':
				Result += "&#39;";
				break;

			case '\t':
				Result += "    ";
				break;

			case '\r':
				break;

			default:
				Result.push_back(Character);
				break;
			}
		}

		return Result;
	}

	std::string EscapeRmlAttribute(const std::string& Text)
	{
		return EscapeRmlText(Text);
	}

	std::string BuildLineNumbers(const std::string& Text)
	{
		int Lines = 1;

		for (const char Character : Text)
		{
			if (Character == '\n')
				++Lines;
		}

		std::ostringstream Stream;

		for (int Line = 1; Line <= Lines; ++Line)
		{
			if (Line > 1)
				Stream << "<br/>";

			Stream << Line;
		}

		return Stream.str();
	}

	std::string BuildSourceRml(const std::string& Text)
	{
		if (Text.empty())
			return "<span class=\"code_dim\">No source loaded.</span>";

		std::string Escaped = EscapeRmlText(Text);
		std::string Result;
		Result.reserve(Escaped.size() + 64);

		for (const char Character : Escaped)
		{
			if (Character == '\n')
				Result += "<br/>";
			else if (Character == ' ')
				Result += "&#160;";
			else
				Result.push_back(Character);
		}

		return Result;
	}

	std::wstring FileNameOnly(const std::wstring& Path)
	{
		return std::filesystem::path(Path).filename().wstring();
	}

	std::string FileNameOnlyUtf8(const std::wstring& Path)
	{
		return RmlDocumentSession::WideToUtf8(FileNameOnly(Path));
	}

	void SetElementText(
		Rml::ElementDocument* Document,
		const char* Id,
		const std::string& Text
	)
	{
		if (!Document || !Id)
			return;

		if (Rml::Element* Element = Document->GetElementById(Id))
			Element->SetInnerRML(EscapeRmlText(Text));
	}

	void SetElementRml(
		Rml::ElementDocument* Document,
		const char* Id,
		const std::string& Rml
	)
	{
		if (!Document || !Id)
			return;

		if (Rml::Element* Element = Document->GetElementById(Id))
			Element->SetInnerRML(Rml);
	}

	void SetElementDisplay(
		Rml::ElementDocument* Document,
		const char* Id,
		const char* Display
	)
	{
		if (!Document || !Id || !Display)
			return;

		if (Rml::Element* Element = Document->GetElementById(Id))
			Element->SetProperty("display", Display);
	}

	void AppendTreeItem(
		std::ostringstream& Stream,
		const char* IconClass,
		const std::string& Name,
		bool Selected
	)
	{
		Stream
			<< "<div class=\"tree_item"
			<< (Selected ? " selected" : "")
			<< "\"><span class=\""
			<< IconClass
			<< "\"></span><span>"
			<< EscapeRmlText(Name)
			<< "</span></div>";
	}

	void AppendTreeGroupStart(
		std::ostringstream& Stream,
		const std::string& Title
	)
	{
		Stream
			<< "<div class=\"tree_group\">"
			<< "<div class=\"tree_group_title\">"
			<< "<span class=\"tree_arrow\">v</span>"
			<< "<span class=\"tree_folder\"></span><span>"
			<< EscapeRmlText(Title)
			<< "</span></div>";
	}

	void AppendTreeGroupEnd(std::ostringstream& Stream)
	{
		Stream << "</div>";
	}

	std::string BuildFileTreeRml(
		const RmlDocumentSession& Session
	)
	{
		if (!Session.HasDocument())
		{
			return
				"<div class=\"tree_group\">"
				"<div class=\"tree_item selected\">"
				"<span class=\"tree_file\"></span>"
				"<span>No document opened</span>"
				"</div>"
				"</div>";
		}

		std::ostringstream Stream;

		AppendTreeGroupStart(Stream, "UI");
		AppendTreeItem(
			Stream,
			"tree_file",
			FileNameOnlyUtf8(Session.GetDocumentPath()),
			true
		);

		for (const std::wstring& StylePath : Session.GetStyleSheetPaths())
		{
			AppendTreeItem(
				Stream,
				"tree_file",
				FileNameOnlyUtf8(StylePath),
				false
			);
		}

		AppendTreeGroupEnd(Stream);

		if (!Session.GetImagePaths().empty())
		{
			AppendTreeGroupStart(Stream, "Images");

			for (const std::wstring& ImagePath : Session.GetImagePaths())
			{
				AppendTreeItem(
					Stream,
					"tree_image",
					FileNameOnlyUtf8(ImagePath),
					false
				);
			}

			AppendTreeGroupEnd(Stream);
		}

		if (!Session.GetFontPaths().empty())
		{
			AppendTreeGroupStart(Stream, "Fonts");

			for (const std::wstring& FontPath : Session.GetFontPaths())
			{
				AppendTreeItem(
					Stream,
					"tree_file",
					FileNameOnlyUtf8(FontPath),
					false
				);
			}

			AppendTreeGroupEnd(Stream);
		}

		if (!Session.GetIncludePaths().empty())
		{
			AppendTreeGroupStart(Stream, "Includes");

			for (const std::wstring& IncludePath : Session.GetIncludePaths())
			{
				AppendTreeItem(
					Stream,
					"tree_file",
					FileNameOnlyUtf8(IncludePath),
					false
				);
			}

			AppendTreeGroupEnd(Stream);
		}

		return Stream.str();
	}
}

RmlEditorRmlHost::RmlEditorRmlHost() = default;

RmlEditorRmlHost::~RmlEditorRmlHost()
{
	Shutdown();
}

std::wstring RmlEditorRmlHost::GetDataRoot()
{
	wchar_t ModulePath[MAX_PATH] = {};

	const DWORD Length = GetModuleFileNameW(
		nullptr,
		ModulePath,
		static_cast<DWORD>(_countof(ModulePath))
	);

	if (Length == 0 || Length >= _countof(ModulePath))
		return L"Data";

	std::wstring Result = ModulePath;

	const size_t Slash = Result.find_last_of(L"\\/");

	if (Slash != std::wstring::npos)
		Result.resize(Slash);
	else
		Result.clear();

	Result += L"\\Data";
	return Result;
}

bool RmlEditorRmlHost::Initialize(
	HWND Window,
	IDirect3DDevice9* Device
)
{
	if (Initialized)
		return true;

	if (!Window || !Device)
	{
		RmlEditorLog::Write(
			"[RmlEditor] Rml host initialization failed: invalid window or device"
		);

		return false;
	}

	WindowHandle = Window;
	DataRoot = GetDataRoot();

	UpdateClientSize();

	SystemInterface =
		std::make_unique<RmlEditorSystemInterface>();

	FileInterface =
		std::make_unique<RmlEditorFileInterface>(
			DataRoot.c_str()
		);

	RenderInterface =
		std::make_unique<RmlEditorRenderDX9>();

	if (!RenderInterface->Initialize(
		Device,
		DataRoot.c_str()
	))
	{
		Shutdown();
		return false;
	}

	Rml::SetSystemInterface(SystemInterface.get());
	Rml::SetFileInterface(FileInterface.get());
	Rml::SetRenderInterface(RenderInterface.get());

	if (!Rml::Initialise())
	{
		RmlEditorLog::Write(
			"[RmlEditor] Rml::Initialise failed"
		);

		Shutdown();
		return false;
	}

	RmlInitialized = true;

	LoadFonts();

	EditorContext = Rml::CreateContext(
		"RmlEditor.EditorContext",
		Rml::Vector2i(
			ClientWidth,
			ClientHeight
		)
	);

	if (!EditorContext)
	{
		RmlEditorLog::Write(
			"[RmlEditor] Failed to create EditorContext"
		);

		Shutdown();
		return false;
	}

	PreviewContext = Rml::CreateContext(
		"RmlEditor.PreviewContext",
		Rml::Vector2i(1920, 1080)
	);

	if (!PreviewContext)
	{
		RmlEditorLog::Write(
			"[RmlEditor] Failed to create PreviewContext"
		);

		Shutdown();
		return false;
	}

	EditorContext->EnableMouseCursor(true);
	PreviewContext->EnableMouseCursor(false);

	if (!PreviewController.Initialize(
		PreviewContext,
		FileInterface.get(),
		RenderInterface.get()
	))
	{
		RmlEditorLog::Write(
			"[RmlEditor] Failed to initialize preview controller"
		);

		Shutdown();
		return false;
	}

	Initialized = true;

	if (!LoadEditorShell())
	{
		Shutdown();
		return false;
	}

	RmlEditorLog::Write(
		"[RmlEditor] Rml host initialized"
	);

	RmlEditorLog::Write(
		"[RmlEditor] EditorContext and PreviewContext created"
	);

	return true;
}

void RmlEditorRmlHost::LoadFonts()
{
	const char* FontPaths[] =
	{
		"Z:/WarZ/External/RmlUI/Fonts/NotoSans-Regular.ttf",
		"Z:/WarZ/External/RmlUI/Fonts/Roboto-Regular.ttf",
		"C:/Windows/Fonts/arial.ttf",
		"C:/Windows/Fonts/consola.ttf"
	};

	bool AnyFontLoaded = false;

	for (const char* FontPath : FontPaths)
	{
		if (Rml::LoadFontFace(FontPath))
		{
			AnyFontLoaded = true;

			RmlEditorLog::Write(
				"[RmlEditor] Font loaded: %s",
				FontPath
			);
		}
	}

	if (!AnyFontLoaded)
	{
		RmlEditorLog::Write(
			"[RmlEditor] Warning: no fonts were loaded"
		);
	}
}

bool RmlEditorRmlHost::LoadEditorShell()
{
	if (!EditorContext)
		return false;

	EditorDocument = EditorContext->LoadDocument(
		"RmlEditor/EditorShell.rml"
	);

	if (!EditorDocument)
	{
		RmlEditorLog::Write(
			"[RmlEditor] Failed to load Data/RmlEditor/EditorShell.rml"
		);

		return false;
	}

	EditorDocument->Show();
	AttachShellController();
	UpdateEditorShellForCurrentDocument();

	RmlEditorLog::Write(
		"[RmlEditor] EditorShell.rml loaded"
	);

	return true;
}

void RmlEditorRmlHost::AttachShellController()
{
	ShellController.SetOpenCallback(
		[this]()
		{
			OpenDocumentFromDialog();
		}
	);

	ShellController.SetReloadCallback(
		[this]()
		{
			ReloadDocument();
		}
	);

	ShellController.Attach(EditorDocument);
}

void RmlEditorRmlHost::Shutdown()
{
	Initialized = false;
	MouseTrackingEnabled = false;
	MouseInPreview = false;
	MouseCapturedByPreview = false;

	ShellController.Detach();
	PreviewController.Shutdown();

	if (EditorContext && EditorDocument)
	{
		EditorContext->UnloadDocument(EditorDocument);
		EditorDocument = nullptr;
	}

	if (PreviewContext)
	{
		const Rml::String ContextName =
			PreviewContext->GetName();

		Rml::RemoveContext(ContextName);
		PreviewContext = nullptr;
	}

	if (EditorContext)
	{
		const Rml::String ContextName =
			EditorContext->GetName();

		Rml::RemoveContext(ContextName);
		EditorContext = nullptr;
	}

	if (RmlInitialized)
	{
		Rml::Shutdown();
		RmlInitialized = false;
	}

	if (RenderInterface)
	{
		RenderInterface->Shutdown();
		RenderInterface.reset();
	}

	FileInterface.reset();
	SystemInterface.reset();

	WindowHandle = nullptr;
	DataRoot.clear();

	RmlEditorLog::Write(
		"[RmlEditor] Rml host shutdown"
	);
}

void RmlEditorRmlHost::UpdateClientSize()
{
	if (!WindowHandle)
	{
		ClientWidth = 1;
		ClientHeight = 1;
		return;
	}

	RECT ClientRectangle{};
	GetClientRect(WindowHandle, &ClientRectangle);

	ClientWidth = std::max(
		1,
		static_cast<int>(
			ClientRectangle.right -
			ClientRectangle.left
		)
	);

	ClientHeight = std::max(
		1,
		static_cast<int>(
			ClientRectangle.bottom -
			ClientRectangle.top
		)
	);

	if (EditorContext)
	{
		EditorContext->SetDimensions(
			Rml::Vector2i(
				ClientWidth,
				ClientHeight
			)
		);
	}
}

void RmlEditorRmlHost::Update(float DeltaSeconds)
{
	(void)DeltaSeconds;

	if (!Initialized)
		return;

	UpdateClientSize();

	if (EditorContext)
		EditorContext->Update();

	UpdatePreviewViewport();
	PreviewController.Update();
}

void RmlEditorRmlHost::Render()
{
	if (!Initialized ||
		!EditorContext ||
		!RenderInterface)
	{
		return;
	}

	RenderInterface->BeginFrame(
		ClientWidth,
		ClientHeight
	);

	EditorContext->Render();
	PreviewController.Render(PreviewViewport);

	// PreviewContext существует отдельно.
	// Его вывод в центральный viewport будет добавлен
	// на следующем этапе через RmlEditorViewport.

	RenderInterface->EndFrame();
}

void RmlEditorRmlHost::UpdatePreviewViewport()
{
	if (!EditorDocument)
	{
		PreviewViewport.Clear();
		return;
	}

	PreviewViewport.SetLogicalSize(1920, 1080);
	PreviewViewport.UpdateFromElement(
		EditorDocument->GetElementById("preview_frame")
	);
}

std::wstring RmlEditorRmlHost::GetOpenDialogInitialDirectory(
	const std::wstring& DataRoot
)
{
	return std::filesystem::path(DataRoot)
		.append(L"Rml")
		.wstring();
}

void RmlEditorRmlHost::OpenDocumentFromDialog()
{
	wchar_t FileName[MAX_PATH] = {};
	const std::wstring InitialDirectory =
		GetOpenDialogInitialDirectory(DataRoot);

	OPENFILENAMEW OpenFileName{};
	OpenFileName.lStructSize = sizeof(OpenFileName);
	OpenFileName.hwndOwner = WindowHandle;
	OpenFileName.lpstrFilter =
		L"RML documents (*.rml)\0*.rml\0All files (*.*)\0*.*\0";
	OpenFileName.lpstrFile = FileName;
	OpenFileName.nMaxFile = static_cast<DWORD>(_countof(FileName));
	OpenFileName.lpstrInitialDir = InitialDirectory.c_str();
	OpenFileName.Flags =
		OFN_FILEMUSTEXIST |
		OFN_PATHMUSTEXIST |
		OFN_NOCHANGEDIR;
	OpenFileName.lpstrDefExt = L"rml";

	if (!GetOpenFileNameW(&OpenFileName))
	{
		const DWORD DialogError = CommDlgExtendedError();

		if (DialogError != 0)
		{
			RmlEditorLog::Write(
				"[RmlEditor] Open dialog failed: %lu",
				DialogError
			);
		}

		return;
	}

	OpenDocument(FileName);
}

bool RmlEditorRmlHost::OpenDocument(
	const std::wstring& FilePath
)
{
	if (!PreviewController.OpenDocument(FilePath))
	{
		UpdateEditorShellStatus(
			"Open failed: " + PreviewController.GetLastError()
		);

		return false;
	}

	UpdateEditorShellForCurrentDocument();
	return true;
}

void RmlEditorRmlHost::ReloadDocument()
{
	if (!PreviewController.HasDocument())
	{
		UpdateEditorShellStatus("No document to reload.");
		return;
	}

	if (!PreviewController.ReloadDocument())
	{
		UpdateEditorShellStatus(
			"Reload failed: " + PreviewController.GetLastError()
		);

		return;
	}

	UpdateEditorShellForCurrentDocument();
}

bool RmlEditorRmlHost::ExecuteShellCommandAt(
	int MouseX,
	int MouseY
)
{
	if (!EditorContext)
		return false;

	Rml::Element* Element =
		EditorContext->GetElementAtPoint(
			Rml::Vector2f(
				static_cast<float>(MouseX),
				static_cast<float>(MouseY)
			)
		);

	while (Element)
	{
		const Rml::String& Id = Element->GetId();

		if (Id == "menu_file" ||
			Id == "toolbar_open" ||
			Id == "preview_open_button")
		{
			OpenDocumentFromDialog();
			return true;
		}

		if (Id == "menu_reload" ||
			Id == "toolbar_reload")
		{
			ReloadDocument();
			return true;
		}

		Element = Element->GetParentNode();
	}

	return false;
}

void RmlEditorRmlHost::UpdateEditorShellStatus(
	const std::string& StatusText
)
{
	SetElementText(EditorDocument, "window_caption", StatusText);
	SetElementText(EditorDocument, "preview_empty_description", StatusText);
	SetElementText(EditorDocument, "rml_save_status", "Read only");
	SetElementText(EditorDocument, "rcss_save_status", "Read only");
}

void RmlEditorRmlHost::UpdateEditorShellForCurrentDocument()
{
	if (!EditorDocument)
		return;

	const RmlDocumentSession& Session =
		PreviewController.GetSession();

	if (!Session.HasDocument())
	{
		SetWindowTextW(
			WindowHandle,
			L"WarZ RML Editor"
		);

		SetElementText(
			EditorDocument,
			"window_caption",
			"No document opened"
		);

		SetElementDisplay(
			EditorDocument,
			"preview_empty",
			"flex"
		);

		SetElementRml(
			EditorDocument,
			"file_tree",
			BuildFileTreeRml(Session)
		);

		return;
	}

	const std::wstring WindowTitle =
		L"WarZ RML Editor - " + Session.GetFileName();

	SetWindowTextW(WindowHandle, WindowTitle.c_str());

	const std::string FileName =
		RmlDocumentSession::WideToUtf8(Session.GetFileName());

	SetElementText(EditorDocument, "window_caption", FileName);

	SetElementDisplay(EditorDocument, "preview_empty", "none");

	SetElementRml(
		EditorDocument,
		"file_tree",
		BuildFileTreeRml(Session)
	);

	SetElementRml(
		EditorDocument,
		"rml_line_numbers",
		BuildLineNumbers(Session.GetRmlSource())
	);

	SetElementRml(
		EditorDocument,
		"rml_code",
		BuildSourceRml(Session.GetRmlSource())
	);

	SetElementRml(
		EditorDocument,
		"rcss_line_numbers",
		BuildLineNumbers(Session.GetRcssSource())
	);

	SetElementRml(
		EditorDocument,
		"rcss_code",
		BuildSourceRml(Session.GetRcssSource())
	);

	SetElementText(EditorDocument, "rml_cursor_status", "Line 1, Col 1");
	SetElementText(EditorDocument, "rcss_cursor_status", "Line 1, Col 1");
	SetElementText(EditorDocument, "rml_save_status", "Read only");
	SetElementText(EditorDocument, "rcss_save_status", "Read only");
}

void RmlEditorRmlHost::OnDeviceLost()
{
	if (RenderInterface)
		RenderInterface->OnDeviceLost();
}

void RmlEditorRmlHost::OnDeviceReset()
{
	UpdateClientSize();

	if (RenderInterface)
	{
		RenderInterface->OnDeviceReset(
			ClientWidth,
			ClientHeight
		);
	}
}

void RmlEditorRmlHost::BeginMouseTracking(HWND Window)
{
	if (MouseTrackingEnabled)
		return;

	TRACKMOUSEEVENT TrackEvent{};

	TrackEvent.cbSize = sizeof(TrackEvent);
	TrackEvent.dwFlags = TME_LEAVE;
	TrackEvent.hwndTrack = Window;

	if (TrackMouseEvent(&TrackEvent))
		MouseTrackingEnabled = true;
}

int RmlEditorRmlHost::GetKeyModifiers()
{
	int Modifiers = 0;

	if (GetKeyState(VK_CONTROL) & 0x8000)
		Modifiers |= Rml::Input::KM_CTRL;

	if (GetKeyState(VK_SHIFT) & 0x8000)
		Modifiers |= Rml::Input::KM_SHIFT;

	if (GetKeyState(VK_MENU) & 0x8000)
		Modifiers |= Rml::Input::KM_ALT;

	if (GetKeyState(VK_CAPITAL) & 0x0001)
		Modifiers |= Rml::Input::KM_CAPSLOCK;

	if (GetKeyState(VK_NUMLOCK) & 0x0001)
		Modifiers |= Rml::Input::KM_NUMLOCK;

	if (GetKeyState(VK_SCROLL) & 0x0001)
		Modifiers |= Rml::Input::KM_SCROLLLOCK;

	return Modifiers;
}

Rml::Input::KeyIdentifier
RmlEditorRmlHost::TranslateKey(WPARAM Key)
{
	if (Key >= 'A' && Key <= 'Z')
	{
		return static_cast<Rml::Input::KeyIdentifier>(
			Rml::Input::KI_A +
			(Key - 'A')
		);
	}

	if (Key >= '0' && Key <= '9')
	{
		return static_cast<Rml::Input::KeyIdentifier>(
			Rml::Input::KI_0 +
			(Key - '0')
		);
	}

	if (Key >= VK_F1 && Key <= VK_F12)
	{
		return static_cast<Rml::Input::KeyIdentifier>(
			Rml::Input::KI_F1 +
			(Key - VK_F1)
		);
	}

	switch (Key)
	{
	case VK_SPACE:
		return Rml::Input::KI_SPACE;

	case VK_BACK:
		return Rml::Input::KI_BACK;

	case VK_TAB:
		return Rml::Input::KI_TAB;

	case VK_RETURN:
		return Rml::Input::KI_RETURN;

	case VK_ESCAPE:
		return Rml::Input::KI_ESCAPE;

	case VK_PRIOR:
		return Rml::Input::KI_PRIOR;

	case VK_NEXT:
		return Rml::Input::KI_NEXT;

	case VK_END:
		return Rml::Input::KI_END;

	case VK_HOME:
		return Rml::Input::KI_HOME;

	case VK_LEFT:
		return Rml::Input::KI_LEFT;

	case VK_UP:
		return Rml::Input::KI_UP;

	case VK_RIGHT:
		return Rml::Input::KI_RIGHT;

	case VK_DOWN:
		return Rml::Input::KI_DOWN;

	case VK_INSERT:
		return Rml::Input::KI_INSERT;

	case VK_DELETE:
		return Rml::Input::KI_DELETE;

	case VK_SHIFT:
		return Rml::Input::KI_LSHIFT;

	case VK_CONTROL:
		return Rml::Input::KI_LCONTROL;

	case VK_MENU:
		return Rml::Input::KI_LMENU;

	case VK_OEM_PLUS:
		return Rml::Input::KI_OEM_PLUS;

	case VK_OEM_MINUS:
		return Rml::Input::KI_OEM_MINUS;

	case VK_OEM_COMMA:
		return Rml::Input::KI_OEM_COMMA;

	case VK_OEM_PERIOD:
		return Rml::Input::KI_OEM_PERIOD;

	default:
		return Rml::Input::KI_UNKNOWN;
	}
}

int RmlEditorRmlHost::TranslateMouseButton(UINT Message)
{
	switch (Message)
	{
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_LBUTTONDBLCLK:
		return 0;

	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_RBUTTONDBLCLK:
		return 1;

	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MBUTTONDBLCLK:
		return 2;

	default:
		return 0;
	}
}

bool RmlEditorRmlHost::ProcessWindowMessage(
	HWND Window,
	UINT Message,
	WPARAM WParam,
	LPARAM LParam,
	LRESULT* Result
)
{
	if (Result)
		*Result = 0;

	if (!Initialized || !EditorContext)
		return false;

	if ((Message == WM_SYSKEYDOWN || Message == WM_SYSKEYUP) &&
		WParam == VK_TAB &&
		(GetKeyState(VK_MENU) & 0x8000))
	{
		return false;
	}

	const bool PreviewInputAvailable =
		PreviewController.HasDocument() &&
		PreviewContext != nullptr &&
		PreviewViewport.IsValid();

	switch (Message)
	{
	case WM_MOUSEMOVE:
	{
		BeginMouseTracking(Window);

		const int MouseX = GET_X_LPARAM(LParam);
		const int MouseY = GET_Y_LPARAM(LParam);

		if (PreviewInputAvailable &&
			(MouseCapturedByPreview ||
				PreviewViewport.ContainsScreenPoint(MouseX, MouseY)))
		{
			if (!MouseInPreview)
				EditorContext->ProcessMouseLeave();

			MouseInPreview = true;

			const Rml::Vector2i PreviewPoint =
				PreviewController.ScreenToPreview(
					PreviewViewport,
					MouseX,
					MouseY
				);

			PreviewContext->ProcessMouseMove(
				PreviewPoint.x,
				PreviewPoint.y,
				GetKeyModifiers()
			);
		}
		else
		{
			if (MouseInPreview && PreviewContext)
				PreviewContext->ProcessMouseLeave();

			MouseInPreview = false;

			EditorContext->ProcessMouseMove(
				MouseX,
				MouseY,
				GetKeyModifiers()
			);
		}

		return true;
	}

	case WM_MOUSELEAVE:
	{
		MouseTrackingEnabled = false;
		MouseInPreview = false;
		MouseCapturedByPreview = false;

		if (PreviewContext)
			PreviewContext->ProcessMouseLeave();

		EditorContext->ProcessMouseLeave();
		return true;
	}

	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDBLCLK:
	{
		const int MouseX = GET_X_LPARAM(LParam);
		const int MouseY = GET_Y_LPARAM(LParam);

		if ((!PreviewInputAvailable ||
				!PreviewViewport.ContainsScreenPoint(MouseX, MouseY)) &&
			ExecuteShellCommandAt(MouseX, MouseY))
		{
			return true;
		}

		SetCapture(Window);

		if (PreviewInputAvailable &&
			PreviewViewport.ContainsScreenPoint(MouseX, MouseY))
		{
			MouseCapturedByPreview = true;
			MouseInPreview = true;

			const Rml::Vector2i PreviewPoint =
				PreviewController.ScreenToPreview(
					PreviewViewport,
					MouseX,
					MouseY
				);

			PreviewContext->ProcessMouseMove(
				PreviewPoint.x,
				PreviewPoint.y,
				GetKeyModifiers()
			);

			PreviewContext->ProcessMouseButtonDown(
				TranslateMouseButton(Message),
				GetKeyModifiers()
			);
		}
		else
		{
			MouseCapturedByPreview = false;

			EditorContext->ProcessMouseButtonDown(
				TranslateMouseButton(Message),
				GetKeyModifiers()
			);
		}

		return true;
	}

	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
	{
		if (GetCapture() == Window)
			ReleaseCapture();

		if (MouseCapturedByPreview &&
			PreviewInputAvailable)
		{
			PreviewContext->ProcessMouseButtonUp(
				TranslateMouseButton(Message),
				GetKeyModifiers()
			);

			MouseCapturedByPreview = false;
		}
		else
		{
			EditorContext->ProcessMouseButtonUp(
				TranslateMouseButton(Message),
				GetKeyModifiers()
			);
		}

		return true;
	}

	case WM_MOUSEWHEEL:
	{
		POINT ClientPoint{};
		ClientPoint.x = GET_X_LPARAM(LParam);
		ClientPoint.y = GET_Y_LPARAM(LParam);
		ScreenToClient(Window, &ClientPoint);

		const short WheelDelta =
			GET_WHEEL_DELTA_WPARAM(WParam);

		const float Delta =
			-static_cast<float>(WheelDelta) /
			static_cast<float>(WHEEL_DELTA);

		if (PreviewInputAvailable &&
			PreviewViewport.ContainsScreenPoint(
				ClientPoint.x,
				ClientPoint.y
			))
		{
			PreviewContext->ProcessMouseWheel(
				Rml::Vector2f(0.0f, Delta),
				GetKeyModifiers()
			);
		}
		else
		{
			EditorContext->ProcessMouseWheel(
				Rml::Vector2f(0.0f, Delta),
				GetKeyModifiers()
			);
		}

		return true;
	}

	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	{
		if (WParam == VK_F5)
		{
			ReloadDocument();
			return true;
		}

		const Rml::Input::KeyIdentifier Key =
			TranslateKey(WParam);

		if (Key != Rml::Input::KI_UNKNOWN)
		{
			if (PreviewInputAvailable &&
				(MouseInPreview || MouseCapturedByPreview))
			{
				PreviewContext->ProcessKeyDown(
					Key,
					GetKeyModifiers()
				);
			}
			else
			{
				EditorContext->ProcessKeyDown(
					Key,
					GetKeyModifiers()
				);
			}

			return true;
		}

		break;
	}

	case WM_KEYUP:
	case WM_SYSKEYUP:
	{
		const Rml::Input::KeyIdentifier Key =
			TranslateKey(WParam);

		if (Key != Rml::Input::KI_UNKNOWN)
		{
			if (PreviewInputAvailable &&
				(MouseInPreview || MouseCapturedByPreview))
			{
				PreviewContext->ProcessKeyUp(
					Key,
					GetKeyModifiers()
				);
			}
			else
			{
				EditorContext->ProcessKeyUp(
					Key,
					GetKeyModifiers()
				);
			}

			return true;
		}

		break;
	}

	case WM_CHAR:
	{
		if (WParam >= 32)
		{
			if (PreviewInputAvailable &&
				(MouseInPreview || MouseCapturedByPreview))
			{
				PreviewContext->ProcessTextInput(
					static_cast<Rml::Character>(WParam)
				);
			}
			else
			{
				EditorContext->ProcessTextInput(
					static_cast<Rml::Character>(WParam)
				);
			}
		}

		return true;
	}

	default:
		break;
	}

	return false;
}

bool RmlEditorRmlHost::IsInitialized() const
{
	return Initialized;
}

Rml::Context* RmlEditorRmlHost::GetEditorContext() const
{
	return EditorContext;
}

Rml::Context* RmlEditorRmlHost::GetPreviewContext() const
{
	return PreviewContext;
}
