#include "ScheduleEditor/DiurnalScheduleEditorToolkit.h"

#include "DiurnalSchedule.h"
#include "DiurnalCycleEditorStyle.h"
#include "ScheduleEditor/DiurnalScheduleEditorCommands.h"
#include "ScheduleEditor/DiurnalScheduleEditorModel.h"
#include "ScheduleEditor/Widgets/SDiurnalScheduleWorkspace.h"
#include "ScheduleEditor/Widgets/SDiurnalScheduleInspector.h"
#include "Editor.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Framework/Docking/TabManager.h"
#include "Misc/DataValidation.h"
#include "Logging/MessageLog.h"
#include "Logging/TokenizedMessage.h"
#include "Styling/AppStyle.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "DiurnalScheduleEditorToolkit"

namespace { const FName ScheduleTab(TEXT("DiurnalScheduleEditor.Schedule")); }

FDiurnalScheduleEditorToolkit::~FDiurnalScheduleEditorToolkit()
{
	if (GEditor) GEditor->UnregisterForUndo(this);
}

void FDiurnalScheduleEditorToolkit::InitScheduleEditor(EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& Host, UDiurnalSchedule* Schedule)
{
	check(IsValid(Schedule)); ScheduleAsset = Schedule; Model = MakeShared<FDiurnalScheduleEditorModel>(Schedule);
	const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("Standalone_DiurnalScheduleEditor_Layout_v1")
		->AddArea(FTabManager::NewPrimaryArea()->SetOrientation(Orient_Vertical)->Split(FTabManager::NewStack()->AddTab(ScheduleTab, ETabState::OpenedTab)->SetHideTabWell(true)));
	InitAssetEditor(Mode, Host, TEXT("DiurnalScheduleEditorApp"), Layout, true, true, Schedule);
	BindCommands();
	ExtendToolbar();
	RegenerateMenusAndToolbars();
	if (GEditor) GEditor->RegisterForUndo(this);
}

void FDiurnalScheduleEditorToolkit::FocusEntry(
	const EDiurnalScheduleSelectionType Type,
	const FGuid EntryId)
{
	FocusWindow(ScheduleAsset.Get());
	if (Workspace)
	{
		Workspace->FocusEntry(Type, EntryId);
	}
}

void FDiurnalScheduleEditorToolkit::BindCommands()
{
	const FDiurnalScheduleEditorCommands& Commands = FDiurnalScheduleEditorCommands::Get();
	const TWeakPtr<FDiurnalScheduleEditorModel> WeakModel = Model;
	GetToolkitCommands()->MapAction(Commands.Validate, FExecuteAction::CreateSP(this, &FDiurnalScheduleEditorToolkit::ValidateSchedule));
	GetToolkitCommands()->MapAction(Commands.Rename,
		FExecuteAction::CreateLambda([this] { if (Workspace) Workspace->RequestRenameSelected(); }),
		FCanExecuteAction::CreateSP(this, &FDiurnalScheduleEditorToolkit::HasSelection));
	GetToolkitCommands()->MapAction(Commands.Delete,
		FExecuteAction::CreateLambda([WeakModel] { if (const TSharedPtr<FDiurnalScheduleEditorModel> Pinned = WeakModel.Pin()) Pinned->DeleteSelected(); }),
		FCanExecuteAction::CreateSP(this, &FDiurnalScheduleEditorToolkit::HasSelection));
	GetToolkitCommands()->MapAction(Commands.Duplicate,
		FExecuteAction::CreateLambda([this, WeakModel]
		{
			if (const TSharedPtr<FDiurnalScheduleEditorModel> Pinned = WeakModel.Pin(); Pinned && Pinned->DuplicateSelected() && Workspace) Workspace->RequestRenameSelected();
		}),
		FCanExecuteAction::CreateSP(this, &FDiurnalScheduleEditorToolkit::HasSelection));
	GetToolkitCommands()->MapAction(Commands.MoveUp,
		FExecuteAction::CreateLambda([WeakModel] { if (const TSharedPtr<FDiurnalScheduleEditorModel> Pinned = WeakModel.Pin()) Pinned->MoveSelected(-1); }),
		FCanExecuteAction::CreateLambda([WeakModel] { const TSharedPtr<FDiurnalScheduleEditorModel> Pinned = WeakModel.Pin(); return Pinned && Pinned->CanMoveSelected(-1); }));
	GetToolkitCommands()->MapAction(Commands.MoveDown,
		FExecuteAction::CreateLambda([WeakModel] { if (const TSharedPtr<FDiurnalScheduleEditorModel> Pinned = WeakModel.Pin()) Pinned->MoveSelected(1); }),
		FCanExecuteAction::CreateLambda([WeakModel] { const TSharedPtr<FDiurnalScheduleEditorModel> Pinned = WeakModel.Pin(); return Pinned && Pinned->CanMoveSelected(1); }));
	GetToolkitCommands()->MapAction(Commands.ClearSelection,
		FExecuteAction::CreateLambda([this, WeakModel]
		{
			if (Workspace) Workspace->ClearSelectionForCurrentView();
			else if (const TSharedPtr<FDiurnalScheduleEditorModel> Pinned = WeakModel.Pin()) Pinned->ClearAllSelection();
		}));
}

void FDiurnalScheduleEditorToolkit::ExtendToolbar()
{
	TSharedPtr<FExtender> Extender = MakeShared<FExtender>();
	Extender->AddToolBarExtension(TEXT("Asset"), EExtensionHook::After, GetToolkitCommands(), FToolBarExtensionDelegate::CreateSP(this, &FDiurnalScheduleEditorToolkit::FillToolbar));
	AddToolbarExtender(Extender);
}

void FDiurnalScheduleEditorToolkit::FillToolbar(FToolBarBuilder& ToolbarBuilder)
{
	ToolbarBuilder.BeginSection(TEXT("DiurnalScheduleAuthoring"));
	ToolbarBuilder.AddComboButton(
		FUIAction(),
		FOnGetContent::CreateSP(this, &FDiurnalScheduleEditorToolkit::BuildAddMenu),
		LOCTEXT("Add", "Add"),
		LOCTEXT("AddTooltip", "Add an instantaneous Event or a duration-based Time Range."),
		FSlateIcon(FDiurnalCycleEditorStyle::GetStyleSetName(), TEXT("DiurnalCycle.Toolbar.Schedule")));
	ToolbarBuilder.AddToolBarButton(
		FDiurnalScheduleEditorCommands::Get().Validate,
		NAME_None,
		TAttribute<FText>(),
		TAttribute<FText>(),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Icons.Validate")));
	ToolbarBuilder.EndSection();
}

TSharedRef<SWidget> FDiurnalScheduleEditorToolkit::BuildAddMenu()
{
	FMenuBuilder Menu(true, GetToolkitCommands());
	Menu.BeginSection(TEXT("AddScheduleEntry"), LOCTEXT("AddSection", "Schedule Entry"));
	Menu.AddMenuEntry(
		LOCTEXT("AddEvent", "Event"),
		LOCTEXT("AddEventTooltip", "Add an instantaneous event. Choose Repeating or One-off recurrence in the inspector."),
		FSlateIcon(FDiurnalCycleEditorStyle::GetStyleSetName(), FDiurnalCycleEditorStyle::GetOccurrenceIconName(false)),
		FUIAction(FExecuteAction::CreateSP(this, &FDiurnalScheduleEditorToolkit::AddEvent)));
	Menu.AddMenuEntry(
		LOCTEXT("AddTimeRange", "Time Range"),
		LOCTEXT("AddTimeRangeTooltip", "Add a recurring duration with start and end times."),
		FSlateIcon(FDiurnalCycleEditorStyle::GetStyleSetName(), FName("DiurnalCycle.Entry.Range")),
		FUIAction(FExecuteAction::CreateSP(this, &FDiurnalScheduleEditorToolkit::AddRange)));
	Menu.EndSection();
	return Menu.MakeWidget();
}

void FDiurnalScheduleEditorToolkit::AddEvent()
{
	if (Model->AddRepeatingEvent().IsValid() && Workspace) Workspace->RequestRenameSelected();
}

void FDiurnalScheduleEditorToolkit::AddRange()
{
	if (Model->AddRange().IsValid() && Workspace) Workspace->RequestRenameSelected();
}

void FDiurnalScheduleEditorToolkit::ValidateSchedule()
{
	const UDiurnalSchedule* Asset = ScheduleAsset.Get();
	if (!Asset) return;
	TArray<FDiurnalScheduleValidationIssue> Issues;
	Asset->GetValidationIssues(Issues);
	int32 ErrorCount = 0;
	for (const FDiurnalScheduleValidationIssue& Issue : Issues)
	{
		ErrorCount += Issue.Severity == EDiurnalScheduleIssueSeverity::Error ? 1 : 0;
	}
	const int32 WarningCount = Issues.Num() - ErrorCount;
	const bool bValid = ErrorCount == 0;

	FMessageLog ValidationLog(TEXT("DiurnalScheduleValidation"));
	ValidationLog.NewPage(FText::Format(LOCTEXT("ValidationPage", "Validate {0}"), FText::FromString(Asset->GetName())));
	for (const FDiurnalScheduleValidationIssue& Issue : Issues)
	{
		const EDiurnalScheduleSelectionType IssueType = Issue.EntryType == EDiurnalScheduleIssueEntryType::Event
			? EDiurnalScheduleSelectionType::Event
			: Issue.EntryType == EDiurnalScheduleIssueEntryType::Range
				? EDiurnalScheduleSelectionType::Range
				: EDiurnalScheduleSelectionType::None;

		const TSharedRef<FTokenizedMessage> Message = FTokenizedMessage::Create(
			Issue.Severity == EDiurnalScheduleIssueSeverity::Error ? EMessageSeverity::Error : EMessageSeverity::Warning);
		const TSharedRef<FTextToken> TextToken = FTextToken::Create(Issue.Message);
		if (IssueType != EDiurnalScheduleSelectionType::None && Issue.EntryId.IsValid())
		{
			TextToken->OnMessageTokenActivated(FOnMessageTokenActivated::CreateSP(
				this,
				&FDiurnalScheduleEditorToolkit::HandleValidationToken,
				IssueType,
				Issue.EntryId));
		}
		Message->AddToken(TextToken);
		ValidationLog.AddMessage(Message);
	}
	if (!bValid || WarningCount > 0)
	{
		ValidationLog.Open(EMessageSeverity::Info, true);
	}

	FNotificationInfo Info(bValid && WarningCount == 0
		? LOCTEXT("Valid", "Schedule is valid.")
		: FText::Format(LOCTEXT("Invalid", "Schedule validation found {0} errors and {1} warnings."), ErrorCount, WarningCount));
	Info.ExpireDuration = 5.0f;
	Info.bUseSuccessFailIcons = true;
	if (const TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info))
	{
		Notification->SetCompletionState(bValid ? SNotificationItem::CS_Success : SNotificationItem::CS_Fail);
	}
}

void FDiurnalScheduleEditorToolkit::HandleValidationToken(
	const TSharedRef<IMessageToken>&,
	const EDiurnalScheduleSelectionType Type,
	const FGuid EntryId)
{
	if (Model) Model->SelectEntry(Type, EntryId);
}

bool FDiurnalScheduleEditorToolkit::HasSelection() const
{
	return Model && Model->GetSelectionType() != EDiurnalScheduleSelectionType::None;
}

void FDiurnalScheduleEditorToolkit::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("Workspace", "Day/Night Cycle Schedule"));
	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);
	InTabManager->RegisterTabSpawner(ScheduleTab, FOnSpawnTab::CreateSP(this, &FDiurnalScheduleEditorToolkit::SpawnScheduleTab))
		.SetDisplayName(LOCTEXT("Tab", "Schedule")).SetIcon(FSlateIcon(FDiurnalCycleEditorStyle::GetStyleSetName(), "DiurnalCycle.Toolbar.Schedule")).SetGroup(WorkspaceMenuCategory.ToSharedRef());
}

void FDiurnalScheduleEditorToolkit::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager); InTabManager->UnregisterTabSpawner(ScheduleTab);
}

FName FDiurnalScheduleEditorToolkit::GetToolkitFName() const { return TEXT("DiurnalScheduleEditor"); }
FText FDiurnalScheduleEditorToolkit::GetBaseToolkitName() const { return LOCTEXT("BaseName", "Day/Night Cycle Schedule"); }
FText FDiurnalScheduleEditorToolkit::GetToolkitName() const { return ScheduleAsset.IsValid() ? FText::Format(LOCTEXT("Name", "Schedule: {0}"), FText::FromString(ScheduleAsset->GetName())) : GetBaseToolkitName(); }
FLinearColor FDiurnalScheduleEditorToolkit::GetWorldCentricTabColorScale() const { return FLinearColor(0.33f, 0.78f, 0.91f, 0.5f); }
FString FDiurnalScheduleEditorToolkit::GetWorldCentricTabPrefix() const { return TEXT("Schedule "); }
void FDiurnalScheduleEditorToolkit::PostUndo(bool bSuccess) { if (bSuccess && Model) Model->HandleUndoRedo(); }
void FDiurnalScheduleEditorToolkit::PostRedo(bool bSuccess) { PostUndo(bSuccess); }

TSharedRef<SDockTab> FDiurnalScheduleEditorToolkit::SpawnScheduleTab(const FSpawnTabArgs&)
{
	return SNew(SDockTab).Label(LOCTEXT("ScheduleLabel", "Schedule"))
	[
		SNew(SSplitter)
		+ SSplitter::Slot().Value(0.72f)[SAssignNew(Workspace, SDiurnalScheduleWorkspace).Model(Model).Commands(GetToolkitCommands())]
		+ SSplitter::Slot().Value(0.28f)[SAssignNew(Inspector, SDiurnalScheduleInspector).Model(Model)]
	];
}

#undef LOCTEXT_NAMESPACE
