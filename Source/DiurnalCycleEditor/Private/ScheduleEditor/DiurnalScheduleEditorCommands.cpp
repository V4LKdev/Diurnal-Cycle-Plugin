#include "ScheduleEditor/DiurnalScheduleEditorCommands.h"

#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "DiurnalScheduleEditorCommands"

FDiurnalScheduleEditorCommands::FDiurnalScheduleEditorCommands()
	: TCommands<FDiurnalScheduleEditorCommands>(
		TEXT("DiurnalScheduleEditor"),
		LOCTEXT("Context", "Day/Night Cycle Schedule Editor"),
		NAME_None,
		FAppStyle::GetAppStyleSetName())
{
}

void FDiurnalScheduleEditorCommands::RegisterCommands()
{
	UI_COMMAND(Validate, "Validate", "Validate this schedule asset.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(Rename, "Rename", "Rename the selected schedule entry without changing its tags or identity.", EUserInterfaceActionType::Button, FInputChord(EKeys::F2));
	UI_COMMAND(Delete, "Delete", "Delete the selected schedule entry.", EUserInterfaceActionType::Button, FInputChord(EKeys::Delete));
	UI_COMMAND(Duplicate, "Duplicate", "Duplicate the selected schedule entry.", EUserInterfaceActionType::Button, FInputChord(EKeys::D, EModifierKey::Control));
	UI_COMMAND(MoveUp, "Move Up", "Move the selected entry earlier in its authored array. Available only while List sorting is Manual Order.", EUserInterfaceActionType::Button, FInputChord(EKeys::Up, EModifierKey::Alt));
	UI_COMMAND(MoveDown, "Move Down", "Move the selected entry later in its authored array. Available only while List sorting is Manual Order.", EUserInterfaceActionType::Button, FInputChord(EKeys::Down, EModifierKey::Alt));
	UI_COMMAND(ClearSelection, "Clear Selection", "Clear the active view's selection.", EUserInterfaceActionType::Button, FInputChord(EKeys::Escape));
}

#undef LOCTEXT_NAMESPACE
