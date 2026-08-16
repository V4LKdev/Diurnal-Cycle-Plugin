#include "DiurnalScheduleFactory.h"

#include "DiurnalSchedule.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DiurnalScheduleFactory)

UDiurnalScheduleFactory::UDiurnalScheduleFactory()
{
	SupportedClass = UDiurnalSchedule::StaticClass();
	bCreateNew = true;
	bEditorImport = false;
	bEditAfterNew = true;
}

UObject* UDiurnalScheduleFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject*, FFeedbackContext*)
{
	UDiurnalSchedule* Schedule = NewObject<UDiurnalSchedule>(InParent, Class, Name, Flags | RF_Transactional);
	Schedule->RepairEntries();
	return Schedule;
}

FName UDiurnalScheduleFactory::GetNewAssetThumbnailOverride() const { return TEXT("ClassThumbnail.DiurnalSchedule"); }
FName UDiurnalScheduleFactory::GetNewAssetIconOverride() const { return TEXT("ClassIcon.DiurnalSchedule"); }
FString UDiurnalScheduleFactory::GetDefaultNewAssetName() const { return TEXT("DS_Schedule"); }
