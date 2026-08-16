#pragma once

#include "CoreMinimal.h"

class FSlateStyleSet;
class ISlateStyle;

class FDiurnalCycleEditorStyle final
{
public:
	static void Initialize();
	static void Shutdown();
	static FName GetStyleSetName();
	static const ISlateStyle& Get();
private:
	static TSharedPtr<FSlateStyleSet> StyleInstance;
};
