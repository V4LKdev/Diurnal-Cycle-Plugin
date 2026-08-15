// Fill out your copyright notice in the Description page of Project Settings.


#include "DiurnalCycleGameplayTags.h"


namespace DiurnalCycle::TimeEvent
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		DailyExample,
		"DiurnalCycle.TimeEvent.DailyExample",
		"Default event that occurs daily");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		DatedExample,
		"DiurnalCycle.TimeEvent.DatedExample",
		"Default event that occurs on a specific day");
}

namespace DiurnalCycle::TimeRange
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		DayTime,
		"DiurnalCycle.TimeRange.DayTime",
		"Default daytime range");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(
		NightTime,
		"DiurnalCycle.TimeRange.NightTime",
		"Default nighttime range");
}
