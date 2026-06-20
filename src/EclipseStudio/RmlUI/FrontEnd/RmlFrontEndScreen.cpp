#include "r3dPCH.h"
#include "r3d.h"

#include "RmlFrontEndScreen.h"

#include "r3dDebug.h"

RmlFrontEndScreen::FClickListener::FClickListener(
	RmlFrontEndScreen* InOwner
)
	: Owner(InOwner)
{
}

void RmlFrontEndScreen::FClickListener::ProcessEvent(
	Rml::Event& Event
)
{
	if (!Owner)
		return;

	Owner->ProcessClick(
		Event.GetTargetElement()
	);
}

void RmlFrontEndScreen::FClickListener::OnDetach(
	Rml::Element* Element
)
{
	(void)Element;
}

RmlFrontEndScreen::RmlFrontEndScreen()
{
}

RmlFrontEndScreen::~RmlFrontEndScreen()
{
	Unload();
}

bool RmlFrontEndScreen::Load(
	Rml::Context* InContext,
	const char* PrimaryDocumentPath,
	const char* FallbackDocumentPath
)
{
	if (Document)
		return true;

	if (!InContext || !PrimaryDocumentPath)
		return false;

	Context = InContext;

	Document =
		Context->LoadDocument(
			PrimaryDocumentPath
		);

	if (!Document && FallbackDocumentPath)
	{
		r3dOutToLog(
			"[RmlUI][FrontEnd][Screen] Failed to load %s, trying fallback %s\n",
			PrimaryDocumentPath,
			FallbackDocumentPath
		);

		Document =
			Context->LoadDocument(
				FallbackDocumentPath
			);
	}

	if (!Document)
	{
		r3dOutToLog(
			"[RmlUI][FrontEnd][Screen] Failed to load document %s\n",
			PrimaryDocumentPath
		);

		Context = nullptr;
		return false;
	}

	ClickListener =
		std::make_unique<FClickListener>(
			this
		);

	Document->AddEventListener(
		"click",
		ClickListener.get()
	);

	Document->Hide();

	OnDocumentLoaded();

	return true;
}

void RmlFrontEndScreen::Unload()
{
	OnDocumentUnloaded();

	if (
		Document &&
		ClickListener
	)
	{
		Document->RemoveEventListener(
			"click",
			ClickListener.get()
		);
	}

	ClickListener.reset();

	if (
		Context &&
		Document
	)
	{
		Context->UnloadDocument(
			Document
		);
	}

	Document = nullptr;
	Context = nullptr;
}

void RmlFrontEndScreen::Show()
{
	if (Document)
		Document->Show();
}

void RmlFrontEndScreen::Hide()
{
	if (Document)
		Document->Hide();
}

bool RmlFrontEndScreen::IsLoaded() const
{
	return Document != nullptr;
}

Rml::ElementDocument* RmlFrontEndScreen::GetDocument() const
{
	return Document;
}

void RmlFrontEndScreen::SetClickHandler(
	const std::function<void(const Rml::String&)>& InClickHandler
)
{
	ClickHandler = InClickHandler;
}

void RmlFrontEndScreen::OnDocumentLoaded()
{
}

void RmlFrontEndScreen::OnDocumentUnloaded()
{
}

bool RmlFrontEndScreen::HandleClickId(
	const Rml::String& Id
)
{
	if (ClickHandler)
	{
		ClickHandler(
			Id
		);

		return true;
	}

	return false;
}

void RmlFrontEndScreen::ProcessClick(
	Rml::Element* Element
)
{
	if (!Element || !Document)
		return;

	Rml::Element* Current =
		Element;

	while (Current)
	{
		const Rml::String& Id =
			Current->GetId();

		if (!Id.empty())
		{
			if (HandleClickId(Id))
				return;
		}

		if (Current == Document)
			break;

		Current =
			Current->GetParentNode();
	}
}

void RmlFrontEndScreen::SetElementText(
	const char* ElementId,
	const Rml::String& Text
)
{
	if (!Document || !ElementId)
		return;

	Rml::Element* Element =
		Document->GetElementById(
			ElementId
		);

	if (!Element)
		return;

	Element->SetInnerRML(
		Text
	);
}

void RmlFrontEndScreen::SetElementClass(
	const char* ElementId,
	const char* ClassName,
	bool bEnabled
)
{
	if (!Document || !ElementId || !ClassName)
		return;

	Rml::Element* Element =
		Document->GetElementById(
			ElementId
		);

	if (!Element)
		return;

	Element->SetClass(
		ClassName,
		bEnabled
	);
}

void RmlFrontEndScreen::SetElementProperty(
	const char* ElementId,
	const char* PropertyName,
	const Rml::String& Value
)
{
	if (!Document || !ElementId || !PropertyName)
		return;

	Rml::Element* Element =
		Document->GetElementById(
			ElementId
		);

	if (!Element)
		return;

	Element->SetProperty(
		PropertyName,
		Value
	);
}

void RmlFrontEndScreen::SetElementAttribute(
	const char* ElementId,
	const char* AttributeName,
	const Rml::String& Value
)
{
	if (!Document || !ElementId || !AttributeName)
		return;

	Rml::Element* Element =
		Document->GetElementById(
			ElementId
		);

	if (!Element)
		return;

	Element->SetAttribute(
		AttributeName,
		Value
	);
}