#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class FDiurnalScheduleEditorModel;
class IPropertyRowGenerator;
class IPropertyHandle;
class IDetailTreeNode;
class SVerticalBox;
class FScopedTransaction;
class SWindow;
struct FPropertyChangedEvent;

class DIURNALCYCLEEDITOR_API SDiurnalScheduleInspector final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDiurnalScheduleInspector)
		: _ShowCreationActions(true)
	{}
		SLATE_ARGUMENT(TSharedPtr<FDiurnalScheduleEditorModel>, Model)
		SLATE_ARGUMENT(bool, ShowCreationActions)
	SLATE_END_ARGS()
	virtual ~SDiurnalScheduleInspector() override;
	void Construct(const FArguments& Args);
	void Refresh();
private:
	void AddPropertyNode(const TSharedRef<IDetailTreeNode>& Node);
	void AddTimePropertyNode(const TSharedRef<IDetailTreeNode>& Node);
	void AddColorRow();
	void AddRecurrenceRows(const TSharedPtr<IPropertyHandle>& Handle);
	void AddPropertyHandleRow(const FText& Label, const TSharedPtr<IPropertyHandle>& Handle);
	FReply OpenColorPicker();
	FReply HandleColorBlockMouseDown(const FGeometry& Geometry, const FPointerEvent& PointerEvent);
	void ResetColorToAutomatic();
	FLinearColor GetSelectedEditorColor() const;
	bool IsSelectedColorOverridden() const;
	void SetSelectedEditorColor(FLinearColor NewColor);
	void HandleColorPickerCancelled(FLinearColor OriginalColor);
	void HandleColorPickerInteractiveBegin();
	void HandleColorPickerInteractiveEnd();
	void HandleColorPickerClosed(const TSharedRef<SWindow>& Window);
	void HandleObjectPropertyChanged(UObject* Object, FPropertyChangedEvent& Event);
	TSharedPtr<FDiurnalScheduleEditorModel> Model;
	TSharedPtr<IPropertyRowGenerator> RowGenerator;
	TSharedPtr<SVerticalBox> Rows;
	FDelegateHandle ObjectPropertyChangedHandle;
	TUniquePtr<FScopedTransaction> ColorTransaction;
	FLinearColor OriginalStoredColor = FLinearColor::White;
	bool bOriginalColorOverride = false;
	bool bColorPickerCancelled = false;
	bool bPackageWasDirty = false;
	bool bRefreshQueued = false;
	bool bShowCreationActions = true;
};
