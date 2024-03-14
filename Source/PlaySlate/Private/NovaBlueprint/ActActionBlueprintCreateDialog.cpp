#include "ActActionBlueprintCreateDialog.h"

#include "NovaBlueprint/Assets/ActActionLogic.h"
#include "NovaBlueprint/Assets/AssetTypeActions/ActActionBlueprintFactory.h"

#include "ClassViewerModule.h"
#include "Editor.h"
#include "EditorStyleSet.h"
#include "SlateOptMacros.h"
#include "Widgets/Layout/SUniformGridPanel.h"

#define LOCTEXT_NAMESPACE "NovaAct"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SActActionBlueprintCreateDialog::Construct(const FArguments& InArgs)
{
	bOKClicked = false;
	ParentClass = UActActionLogic::StaticClass();

	ChildSlot
	[
		SNew(SBorder).Visibility(EVisibility::Visible).BorderImage(FEditorStyle::GetBrush("Menu.Background"))
		[
			SNew(SBox).Visibility(EVisibility::Visible).WidthOverride(500.0f)
			[
				SNew(SVerticalBox)

				// + ??
				+ SVerticalBox::Slot().FillHeight(1)
				[
					SNew(SBorder).BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder")).Content()
					[
						SAssignNew(ParentClassContainer, SVerticalBox)
					]

				]

				// OK/Cancel buttons
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(8)
				[
					SNew(SUniformGridPanel).SlotPadding(FEditorStyle::GetMargin("StandardDialog.SlotPadding")).MinDesiredSlotWidth(FEditorStyle::GetFloat("StandardDialog.MinDesiredSlotWidth")).MinDesiredSlotHeight(FEditorStyle::GetFloat("StandardDialog.MinDesiredSlotHeight"))
					+ SUniformGridPanel::Slot(0, 0)
					[
						// OK button
						SNew(SButton).HAlign(HAlign_Center).ContentPadding(FEditorStyle::GetMargin("StandardDialog.ContentPadding")).OnClicked(this, &SActActionBlueprintCreateDialog::OKClicked).Text(LOCTEXT("CreateActionBlueprintOK", "OK"))

					]
					+ SUniformGridPanel::Slot(1, 0)
					[
						// Cancel button
						SNew(SButton).HAlign(HAlign_Center).ContentPadding(FEditorStyle::GetMargin("StandardDialog.ContentPadding")).OnClicked(this, &SActActionBlueprintCreateDialog::CancelClicked).Text(LOCTEXT("CreateActionBlueprintCancel", "Cancel"))

					]
				]
			]
		]
	];
	MakeParentClassPicker();
}


FReply SActActionBlueprintCreateDialog::CancelClicked()
{
	CloseDialog();
	return FReply::Handled();
}

void SActActionBlueprintCreateDialog::MakeParentClassPicker()
{
	FClassViewerModule& ClassViewerModule = FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer");

	FClassViewerInitializationOptions Options;
	Options.Mode = EClassViewerMode::ClassPicker;
	Options.bIsBlueprintBaseOnly = true;

	TSharedPtr<FActionBlueprintParentFilter> Filter = MakeShareable(new FActionBlueprintParentFilter);
	Filter->AllowedChildrenOfClasses.Add(UActActionLogic::StaticClass());
	Options.ClassFilter = Filter;

	ParentClassContainer->ClearChildren();
	ParentClassContainer->AddSlot().AutoHeight()
	[
		SNew(STextBlock).Text(LOCTEXT("ParentClass", "Parent Class.")).ShadowOffset(FVector2D(1.0f, 1.0f))

	];
	ParentClassContainer->AddSlot()
	[
		ClassViewerModule.CreateClassViewer(Options, FOnClassPicked::CreateSP(this, &SActActionBlueprintCreateDialog::OnClassPicked))

	];
}

void SActActionBlueprintCreateDialog::OnClassPicked(UClass* ChosenClass)
{
	ParentClass = ChosenClass;
}

bool SActActionBlueprintCreateDialog::ConfigureProperties(TWeakObjectPtr<UActActionBlueprintFactory> InActionFactory)
{
	ActionFactory = InActionFactory;

	const TSharedRef<SWindow> Window = SNew(SWindow).Title(LOCTEXT("CreateActionBlueprintOptions", "Create Action Blueprint")).ClientSize(FVector2D(400, 700)).SupportsMinimize(false).SupportsMaximize(false)
	[
		AsShared()

	];
	PickerWindow = Window;
	GEditor->EditorAddModalWindow(Window);
	ActionFactory.Reset();

	return bOKClicked;
}

FReply SActActionBlueprintCreateDialog::OKClicked()
{
	if (ActionFactory.IsValid())
	{
		ActionFactory->BlueprintType = BPTYPE_Normal;
		ActionFactory->ParentClass = ParentClass.Get();
	}

	CloseDialog(true);
	return FReply::Handled();
}

void SActActionBlueprintCreateDialog::CloseDialog(bool bWasPicked)
{
	bOKClicked = bWasPicked;
	if (PickerWindow.IsValid())
	{
		PickerWindow.Pin()->RequestDestroyWindow();
	}
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

#undef LOCTEXT_NAMESPACE
