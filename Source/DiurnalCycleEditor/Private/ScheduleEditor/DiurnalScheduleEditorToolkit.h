#pragma once

#include "CoreMinimal.h"
#include "EditorUndoClient.h"
#include "Toolkits/AssetEditorToolkit.h"

class FDiurnalScheduleEditorModel;
class UDiurnalSchedule;
class SDiurnalScheduleInspector;
class SDiurnalScheduleWorkspace;
class FToolBarBuilder;
class IMessageToken;
enum class EDiurnalScheduleSelectionType : uint8;

class FDiurnalScheduleEditorToolkit final : public FAssetEditorToolkit, public FEditorUndoClient
{
public:
	virtual ~FDiurnalScheduleEditorToolkit() override;
	void InitScheduleEditor(EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& Host, UDiurnalSchedule* Schedule);
	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& TabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& TabManager) override;
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FText GetToolkitName() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override;
	void FocusEntry(EDiurnalScheduleSelectionType Type, FGuid EntryId);
private:
	void BindCommands();
	void ExtendToolbar();
	void FillToolbar(FToolBarBuilder& ToolbarBuilder);
	TSharedRef<SWidget> BuildAddMenu();
	void AddEvent();
	void AddRange();
	void ValidateSchedule();
	void HandleValidationToken(const TSharedRef<IMessageToken>& Token, EDiurnalScheduleSelectionType Type, FGuid EntryId);
	bool HasSelection() const;
	TSharedRef<SDockTab> SpawnScheduleTab(const FSpawnTabArgs& Args);
	TWeakObjectPtr<UDiurnalSchedule> ScheduleAsset;
	TSharedPtr<FDiurnalScheduleEditorModel> Model;
	TSharedPtr<SDiurnalScheduleWorkspace> Workspace;
	TSharedPtr<SDiurnalScheduleInspector> Inspector;
};
