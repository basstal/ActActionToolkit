#pragma once
#include "ClassViewerFilter.h"
#include "ClassViewerModule.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Common/NovaDataBinding.h"
#include "Common/NovaStruct.h"

#define LOCTEXT_NAMESPACE "NovaAct"

using namespace NovaStruct;
class SActNotifyPoolLaneWidget;

namespace NotifyStateFilter
{
	template <typename NotifyTypeClass>
	class FActNotifyStateClassFilter : public IClassViewerFilter
	{
	public:
		FActNotifyStateClassFilter(UAnimSequenceBase* InSequence);

		//~Begin IClassViewerFilter interface
		virtual bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions,
		                            const UClass* InClass,
		                            TSharedRef<FClassViewerFilterFuncs> InFilterFuncs) override;


		virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions,
		                                    const TSharedRef<const IUnloadedBlueprintData> InUnloadedClassData,
		                                    TSharedRef<FClassViewerFilterFuncs> InFilterFuncs) override;
		//~End IClassViewerFilter interface

		/** Sequence referenced by outer panel */
		UAnimSequenceBase* Sequence;
	};

	template <typename NotifyTypeClass>
	FActNotifyStateClassFilter<NotifyTypeClass>::FActNotifyStateClassFilter(UAnimSequenceBase* InSequence)
		: Sequence(InSequence)
	{
	}

	template <typename NotifyTypeClass>
	bool FActNotifyStateClassFilter<NotifyTypeClass>::IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions,
	                                                                 const UClass* InClass,
	                                                                 TSharedRef<FClassViewerFilterFuncs> InFilterFuncs)
	{
		const bool bChildOfObjectClass = InClass->IsChildOf(NotifyTypeClass::StaticClass());
		const bool bMatchesFlags = !InClass->HasAnyClassFlags(CLASS_Hidden | CLASS_HideDropDown | CLASS_Deprecated | CLASS_Abstract);
		return bChildOfObjectClass && bMatchesFlags && CastChecked<NotifyTypeClass>(InClass->ClassDefaultObject)->CanBePlaced(Sequence);
	}

	template <typename NotifyTypeClass>
	bool FActNotifyStateClassFilter<NotifyTypeClass>::IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions,
	                                                                         const TSharedRef<const IUnloadedBlueprintData> InUnloadedClassData,
	                                                                         TSharedRef<FClassViewerFilterFuncs> InFilterFuncs)
	{
		const bool bChildOfObjectClass = InUnloadedClassData->IsChildOf(NotifyTypeClass::StaticClass());
		const bool bMatchesFlags = !InUnloadedClassData->HasAnyClassFlags(
			CLASS_Hidden | CLASS_HideDropDown | CLASS_Deprecated | CLASS_Abstract);
		bool bValidToPlace = false;
		if (bChildOfObjectClass)
		{
			if (const UClass* NativeBaseClass = InUnloadedClassData->GetNativeParent())
			{
				bValidToPlace = CastChecked<NotifyTypeClass>(NativeBaseClass->ClassDefaultObject)->CanBePlaced(Sequence);
			}
		}

		return bChildOfObjectClass && bMatchesFlags && bValidToPlace;
	}

	static FString MakeBlueprintNotifyName(const FString& InNotifyClassName)
	{
		FString DefaultNotifyName = InNotifyClassName;
		DefaultNotifyName = DefaultNotifyName.Replace(TEXT("AnimNotify_"), TEXT(""), ESearchCase::CaseSensitive);
		DefaultNotifyName = DefaultNotifyName.Replace(TEXT("AnimNotifyState_"), TEXT(""), ESearchCase::CaseSensitive);

		return DefaultNotifyName;
	}

	template <typename NotifyTypeClass>
	static void MakeNewNotifyPicker(FMenuBuilder& MenuBuilder,
	                                bool bIsReplaceWithMenu /* = false */,
	                                UAnimSequenceBase* AnimSequence, TSharedRef<SActNotifyPoolLaneWidget> PoolLaneWidget)
	{
		FText TypeName = NotifyTypeClass::StaticClass() == UAnimNotify::StaticClass() ?
			                 LOCTEXT("AnimNotifyName", "anim notify") :
			                 LOCTEXT("AnimNotifyStateName", "anim notify state");
		FText SectionHeaderFormat = bIsReplaceWithMenu ?
			                            LOCTEXT("ReplaceWithAnExistingAnimNotify", "Replace with an existing {0}") :
			                            LOCTEXT("AddsAnExistingAnimNotify", "Add an existing {0}");

		// MenuBuilder always has a search widget added to it by default, hence if larger then 1 then something else has been added to it
		if (MenuBuilder.GetMultiBox()->GetBlocks().Num() > 1)
		{
			MenuBuilder.AddMenuSeparator();
		}

		FClassViewerInitializationOptions InitOptions;
		InitOptions.Mode = EClassViewerMode::ClassPicker;
		InitOptions.bShowObjectRootClass = false;
		InitOptions.bShowUnloadedBlueprints = true;
		InitOptions.bShowNoneOption = false;
		InitOptions.bEnableClassDynamicLoading = true;
		InitOptions.bExpandRootNodes = true;
		InitOptions.NameTypeToDisplay = EClassViewerNameTypeToDisplay::DisplayName;
		// Add a notify picker
		InitOptions.ClassFilters.Add(MakeShared<FActNotifyStateClassFilter<NotifyTypeClass>>(AnimSequence));
		InitOptions.bShowBackgroundBorder = false;

		FClassViewerModule& ClassViewerModule = FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer");
		MenuBuilder.AddWidget(
			SNew(SBox)
			.MinDesiredWidth(300.0f)
			.MaxDesiredHeight(400.0f)
			[
				ClassViewerModule.CreateClassViewer(InitOptions,
				                                    FOnClassPicked::CreateLambda([bIsReplaceWithMenu, PoolLaneWidget](UClass* InClass)
					                                    {
						                                    FSlateApplication::Get().DismissAllMenus();
						                                    if (bIsReplaceWithMenu)
						                                    {
							                                    // ** TODO: 待绑定
							                                    NovaDB::Trigger("NotifyPicker/ReplaceSelectedWithNotify");
							                                    // ReplaceSelectedWithNotify(MakeBlueprintNotifyName(InClass->GetName()), InClass);
						                                    }
						                                    else
						                                    {
							                                    auto DB = GetDataBindingSP(FActCreateNewNotify, "CreateNewNotify");
							                                    if (DB)
							                                    {
								                                    TSharedPtr<FActCreateNewNotify> NewNotify = MakeShareable(
									                                    new FActCreateNewNotify());
								                                    {
									                                    NewNotify->NewNotifyName = MakeBlueprintNotifyName(InClass->GetName());
									                                    NewNotify->NotifyClass = InClass;
								                                    }
								                                    DB->SetData(NewNotify);
							                                    	PoolLaneWidget->OnCreateNewNotify(NewNotify);
							                                    }
						                                    }
					                                    }
				                                    ))
			],
			FText(),
			true,
			false);
	}
}


#undef LOCTEXT_NAMESPACE
