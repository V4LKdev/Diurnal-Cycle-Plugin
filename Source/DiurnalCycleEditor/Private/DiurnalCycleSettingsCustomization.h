#pragma once

#include "IDetailCustomization.h"

class FDiurnalCycleSettingsCustomization final : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
private:
	FReply OpenBrowser();
	FReply RemoveDuplicateSchedules();
	void HandleDefaultSchedulesChanged();
	EVisibility GetDuplicateWarningVisibility() const;
	FText GetDuplicateWarningText() const;
	TWeakPtr<IPropertyUtilities> PropertyUtilities;
};
