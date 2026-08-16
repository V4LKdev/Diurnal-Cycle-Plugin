#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"

class FDiurnalScheduleEditorCommands final : public TCommands<FDiurnalScheduleEditorCommands>
{
public:
	FDiurnalScheduleEditorCommands();
	virtual void RegisterCommands() override;

	TSharedPtr<FUICommandInfo> Validate;
	TSharedPtr<FUICommandInfo> Rename;
	TSharedPtr<FUICommandInfo> Delete;
	TSharedPtr<FUICommandInfo> Duplicate;
	TSharedPtr<FUICommandInfo> MoveUp;
	TSharedPtr<FUICommandInfo> MoveDown;
	TSharedPtr<FUICommandInfo> ClearSelection;
};
