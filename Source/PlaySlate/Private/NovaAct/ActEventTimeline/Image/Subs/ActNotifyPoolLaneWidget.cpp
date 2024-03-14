#include "ActNotifyPoolLaneWidget.h"

#include "ActNotifyPoolNotifyNodeWidget.h"
#include "AssetSelection.h"
#include "BlueprintActionDatabase.h"
#include "EditorStyleSet.h"
#include "IEditableSkeleton.h"
#include "ISkeletonEditorModule.h"
#include "SCurveEditor.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Common/NovaConst.h"
#include "Common/NovaDataBinding.h"
#include "Common/NovaStruct.h"
#include "NovaAct/NovaActEditor.h"
#include "NovaAct/ActEventTimeline/Image/PoolWidgetTypes/ActPoolWidgetNotifyWidget.h"
#include "NovaAct/ActEventTimeline/Operation/ActNotifyStateClassFilter.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/STextEntryPopup.h"

#define LOCTEXT_NAMESPACE "NovaAct"
using namespace NovaStruct;
using namespace NovaConst;

void SActNotifyPoolLaneWidget::Construct(const FArguments& InArgs)
{
	SetClipping(EWidgetClipping::ClipToBounds);
	LaneIndex = InArgs._LaneIndex;
	NovaDB::CreateSP<FActCreateNewNotify>("CreateNewNotify", nullptr);
	ChildSlot
	[
		SAssignNew(TrackBorder, SBorder)
			.Visibility(EVisibility::SelfHitTestInvisible)
			.BorderImage(FEditorStyle::GetBrush("NoBorder"))
			.Padding(FMargin(0.f, 0.f))
	];

	Update();
}

int32 SActNotifyPoolLaneWidget::OnPaint(const FPaintArgs& Args,
                                        const FGeometry& AllottedGeometry,
                                        const FSlateRect& MyCullingRect,
                                        FSlateWindowElementList& OutDrawElements,
                                        int32 LayerId,
                                        const FWidgetStyle& InWidgetStyle,
                                        bool bParentEnabled) const
{
	auto AnimSequenceDB = GetDataBinding(UAnimSequenceBase**, "ActAnimation/AnimSequence");
	if (!AnimSequenceDB)
	{
		return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId + 1, InWidgetStyle, bParentEnabled);
	}

	bool bAnyDraggedNodes = false;
	float AllottedGeometrySizeX = AllottedGeometry.Size.X;
	for (int32 Index = 0; Index < NotifyNodes.Num(); ++Index)
	{
		TSharedPtr<SActNotifyPoolNotifyNodeWidget> NotifyWidget = NotifyNodes[Index];
		if (NotifyWidget->bBeingDragged)
		{
			bAnyDraggedNodes = true;
		}
		else
		{
			NotifyWidget->UpdateSizeAndPosition(AllottedGeometrySizeX);
		}
	}
	UAnimSequenceBase* AnimSequence = *(AnimSequenceDB->GetData());

	if (LaneIndex < AnimSequence->AnimNotifyTracks.Num())
	{
		float LocalSizeY = AllottedGeometry.GetLocalSize().Y;
		// UE_LOG(LogNovaAct, Log, TEXT("LaneIndex %d : LocalSizeY %f"), LaneIndex, LocalSizeY);
		// Draw track bottom border
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId + 1,
			AllottedGeometry.ToPaintGeometry(),
			TArray<FVector2D>({
				FVector2D(0.0f, LocalSizeY),
				FVector2D(AllottedGeometry.GetLocalSize().X, LocalSizeY)
			}),
			ESlateDrawEffect::None,
			FLinearColor::White.CopyWithNewOpacity(0.3f)
		);
	}

	auto PoolNotifyDB = GetDataBindingSP(SActPoolWidgetNotifyWidget, "ActPoolNotify");
	if (bAnyDraggedNodes && PoolNotifyDB)
	{
		TSharedPtr<SActPoolWidgetNotifyWidget> ActPoolWidgetNotifyWidget = PoolNotifyDB->GetData();
		float Value = ActPoolWidgetNotifyWidget->CurrentDragXPosition;
		if (Value >= 0.0f)
		{
			float XPos = Value;
			TArray<FVector2D> LinePoints;
			LinePoints.Add(FVector2D(XPos, 0.f));
			LinePoints.Add(FVector2D(XPos, AllottedGeometry.Size.Y));

			FSlateDrawElement::MakeLines(
				OutDrawElements,
				LayerId + 2,
				AllottedGeometry.ToPaintGeometry(),
				LinePoints,
				ESlateDrawEffect::None,
				FLinearColor(1.0f, 0.5f, 0.0f)
			);
		}
	}

	return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId + 2, InWidgetStyle, bParentEnabled);
}

void SActNotifyPoolLaneWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	auto DB = GetDataBindingSP(FActEventTimelineArgs, "ActEventTimelineArgs");
	if (!DB)
	{
		return;
	}
	TSharedPtr<FActEventTimelineArgs> ActEventTimelineArgs = DB->GetData();
	CachedScaleInfo = MakeShareable(new FTrackScaleInfo(ActEventTimelineArgs->ViewRange->GetLowerBoundValue(),
	                                                    ActEventTimelineArgs->ViewRange->GetUpperBoundValue(),
	                                                    0,
	                                                    0,
	                                                    AllottedGeometry.Size));
	CachedAllottedGeometrySizeScaled = AllottedGeometry.Size * AllottedGeometry.Scale;
}

FReply SActNotifyPoolLaneWidget::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	FVector2D CursorPos = MouseEvent.GetScreenSpacePosition();
	CursorPos = MyGeometry.AbsoluteToLocal(CursorPos);
	TSharedPtr<SActNotifyPoolNotifyNodeWidget> HitNotifyNode = GetHitNotifyNode(CursorPos);

	if (HitNotifyNode)
	{
		if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
		{
			// Hit a node, record the mouse position for use later so we can know when / where a
			// drag happened on the node handles if necessary.
			HitNotifyNode->LastMouseDownPosition = CursorPos;

			return FReply::Handled().DetectDrag(HitNotifyNode.ToSharedRef(), EKeys::LeftMouseButton);
		}
		else if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
		{
			// Hit a node, return handled so we can pop a context menu on mouse up
			return FReply::Handled();
		}
	}

	return FReply::Unhandled();
}

FReply SActNotifyPoolLaneWidget::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	auto DB = GetDataBinding(UAnimSequence**, "ActAnimation/AnimSequence");
	if (!DB)
	{
		return FReply::Unhandled();
	}
	bool bLeftMouseButton = MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton;
	bool bRightMouseButton = MouseEvent.GetEffectingButton() == EKeys::RightMouseButton;
	bool bShift = MouseEvent.IsShiftDown();
	bool bCtrl = MouseEvent.IsControlDown();

	FVector2D MouseAbsolutePosition = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	UAnimSequence* AnimSequence = *DB->GetData();
	LastClickedTime = FMath::Clamp<float>(CachedScaleInfo->LocalXToInput(MouseAbsolutePosition.X), 0.0f, AnimSequence->GetPlayLength());
	TSharedPtr<SActNotifyPoolNotifyNodeWidget> HitNotifyNode = GetHitNotifyNode(MouseAbsolutePosition);

	if (bRightMouseButton)
	{
		TSharedPtr<SWidget> WidgetToFocus = SummonContextMenu(MouseEvent, HitNotifyNode);

		return (WidgetToFocus.IsValid()) ?
			       FReply::Handled().ReleaseMouseCapture().SetUserFocus(WidgetToFocus.ToSharedRef(), EFocusCause::SetDirectly) :
			       FReply::Handled().ReleaseMouseCapture();
	}
	else if (bLeftMouseButton)
	{
		auto PoolNotifyDB = GetDataBindingSP(SActPoolWidgetNotifyWidget, "ActPoolNotify");
		if (!PoolNotifyDB)
		{
			return FReply::Unhandled();
		}
		TSharedPtr<SActPoolWidgetNotifyWidget> ActPoolWidgetNotifyWidget = PoolNotifyDB->GetData();
		if (!HitNotifyNode)
		{
			// Clicked in empty space, clear selection
			ActPoolWidgetNotifyWidget->DeselectAllNotifies();
		}
		else if (bCtrl)
		{
			ActPoolWidgetNotifyWidget->ToggleNotifyNodeSelectStatus(HitNotifyNode.ToSharedRef());
		}
		else
		{
			ActPoolWidgetNotifyWidget->SelectNotifyNode(HitNotifyNode.ToSharedRef(), bShift);
		}

		return FReply::Handled();
	}

	return FReply::Unhandled();
}

FVector2D SActNotifyPoolLaneWidget::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
	return FVector2D(100, NotifyNodes.IsEmpty() ? NotifyHeight : NotifyNodes[0]->WidgetSize.Y);
}

void SActNotifyPoolLaneWidget::Update()
{
	NotifyNodes.Empty();

	TrackBorder->SetContent(
		SAssignNew(NodeSlots, SOverlay)
	);

	auto DB = GetDataBinding(UAnimSequence**, "ActAnimation/AnimSequence");
	UAnimSequence* AnimSequence = *(DB->GetData());
	FAnimNotifyTrack& AnimNotifyTrack = AnimSequence->AnimNotifyTracks[LaneIndex];
	TArray<FAnimNotifyEvent*>& Notifies = AnimNotifyTrack.Notifies;
	if (Notifies.Num() > 0)
	{
		for (int32 NotifyIndex = 0; NotifyIndex < Notifies.Num(); ++NotifyIndex)
		{
			FAnimNotifyEvent* AnimNotifyEvent = Notifies[NotifyIndex];

			TSharedPtr<SActNotifyPoolNotifyNodeWidget> AnimNotifyNode = nullptr;

			AnimNotifyNode = SNew(SActNotifyPoolNotifyNodeWidget, SharedThis(this))
				.AnimNotifyEvent(AnimNotifyEvent);

			NodeSlots->AddSlot()
			         .Padding(TAttribute<FMargin>::Create(TAttribute<FMargin>::FGetter::CreateSP(this,
			                                                                                     &SActNotifyPoolLaneWidget::GetNotifyTrackPadding,
			                                                                                     NotifyIndex)))
			[
				AnimNotifyNode.ToSharedRef()
			];

			NotifyNodes.Add(AnimNotifyNode);
		}
	}
}

// Returns the padding needed to render the notify in the correct track position
FMargin SActNotifyPoolLaneWidget::GetNotifyTrackPadding(int32 NotifyIndex) const
{
	float LeftMargin = NotifyNodes[NotifyIndex]->GetWidgetPosition().X;
	float RightMargin = GetCachedGeometry().GetLocalSize().X - NotifyNodes[NotifyIndex]->GetWidgetPosition().X -
		NotifyNodes[NotifyIndex]->WidgetSize.X;
	return FMargin(LeftMargin, 0, RightMargin, 0);
}

FText SActNotifyPoolLaneWidget::GetNodeTooltip()
{
	return FText();
}

FName SActNotifyPoolLaneWidget::GetName()
{
	return NAME_None;
}


FLinearColor SActNotifyPoolLaneWidget::GetEditorColor()
{
	return FLinearColor(1, 1, 0.5f);
}

void SActNotifyPoolLaneWidget::DisconnectSelectedNodesForDrag(TArray<TSharedPtr<SActNotifyPoolNotifyNodeWidget>>& DragNodes)
{
	for (TSharedPtr<SActNotifyPoolNotifyNodeWidget> NotifyNode : NotifyNodes)
	{
		if (NotifyNode->IsSelected())
		{
			NodeSlots->RemoveSlot(NotifyNode.ToSharedRef());
			DragNodes.Add(NotifyNode);
		}
	}
}

TSharedPtr<SActNotifyPoolNotifyNodeWidget> SActNotifyPoolLaneWidget::GetHitNotifyNode(const FVector2D& CursorPosition)
{
	//Run through from 'top most' Notify to bottom
	for (int32 Index = NotifyNodes.Num() - 1; Index >= 0; --Index)
	{
		TSharedPtr<SActNotifyPoolNotifyNodeWidget> NotifyNode = NotifyNodes[Index];
		FVector2D Position = NotifyNode->GetWidgetPosition();
		FVector2D Size = NotifyNode->WidgetSize;
		if (CursorPosition >= Position && CursorPosition <= (Position + Size))
		{
			return NotifyNode;
		}
	}

	return nullptr;
}


TSharedPtr<SWidget> SActNotifyPoolLaneWidget::SummonContextMenu(const FPointerEvent& MouseEvent,
                                                                const TSharedPtr<SActNotifyPoolNotifyNodeWidget> NotifyNode)
{
	auto DB = GetDataBindingSP(FNovaActEditor, "NovaActEditor");
	auto PoolNotifyDB = GetDataBindingSP(SActPoolWidgetNotifyWidget, "ActPoolNotify");
	auto AnimSequenceDB = GetDataBinding(UAnimSequence**, "ActAnimation/AnimSequence");
	if (!DB || !PoolNotifyDB || !AnimSequenceDB)
	{
		return SNullWidget::NullWidget;
	}
	TSharedPtr<SActPoolWidgetNotifyWidget> ActPoolWidgetNotifyWidget = PoolNotifyDB->GetData();
	TSharedPtr<FNovaActEditor> NovaActEditor = DB->GetData();
	FMenuBuilder MenuBuilder(true, NovaActEditor->GetToolkitCommands());
	FUIAction NewAction;

	FAnimNotifyEvent* NotifyEvent = NotifyNode ? NotifyNode->AnimNotifyEvent : nullptr;

	UAnimSequence* AnimSequence = *AnimSequenceDB->GetData();
	// DataBindingSPBindRaw(FActCreateNewNotify, "CreateNewNotify", this, &SActNotifyPoolLaneWidget::OnCreateNewNotify, OnCreateNewNotifyBinding)

	MenuBuilder.BeginSection("AnimNotify", LOCTEXT("NotifyHeading", "Notify"));
	{
		if (NotifyNode)
		{
			if (!NotifyNode->IsSelected())
			{
				ActPoolWidgetNotifyWidget->SelectNotifyNode(NotifyNode.ToSharedRef(), MouseEvent.IsControlDown());
			}

			if (ActPoolWidgetNotifyWidget->IsSingleNotifyNodeSelected())
			{
				auto CommitBeginTime = [NotifyEvent, AnimSequenceDB](float InValue, ETextCommit::Type InCommitType, bool bValueAsFrame)
				{
					if (InCommitType == ETextCommit::OnEnter && NotifyEvent && AnimSequenceDB)
					{
						UAnimSequence* InnerAnimSequence = *AnimSequenceDB->GetData();
						float NewTime = FMath::Clamp(bValueAsFrame ? InnerAnimSequence->GetTimeAtFrame(InValue) : InValue,
						                             0.0f,
						                             InnerAnimSequence->GetPlayLength() - NotifyEvent->GetDuration());
						NotifyEvent->SetTime(NewTime);

						NotifyEvent->RefreshTriggerOffset(InnerAnimSequence->CalculateOffsetForNotify(NotifyEvent->GetTime()));
						if (NotifyEvent->GetDuration() > 0.0f)
						{
							NotifyEvent->RefreshEndTriggerOffset(
								InnerAnimSequence->CalculateOffsetForNotify(NotifyEvent->GetTime() + NotifyEvent->GetDuration()));
						}
						NovaDB::Trigger("ActImageTrack/Refresh");

						FSlateApplication::Get().DismissAllMenus();
					}
				};
				// Add item to directly set notify time
				TSharedRef<SWidget> TimeWidget =
					SNew(SBox)
					.HAlign(HAlign_Right)
					.ToolTipText(LOCTEXT("SetTimeToolTip", "Set the time of this notify directly"))
					[
						SNew(SBox)
						.Padding(FMargin(4.0f, 0.0f, 0.0f, 0.0f))
						.WidthOverride(100.0f)
						[
							SNew(SNumericEntryBox<float>)
							.Font(FEditorStyle::GetFontStyle(TEXT("MenuItem.Font")))
							.MinValue(0.0f)
							.MaxValue(AnimSequence->GetPlayLength())
							.Value(NotifyEvent->GetTime())
							.AllowSpin(false)
							.OnValueCommitted_Lambda([CommitBeginTime](float InValue, ETextCommit::Type InCommitType)
							{
								CommitBeginTime(InValue, InCommitType, false);
							})
						]
					];

				MenuBuilder.AddWidget(TimeWidget, LOCTEXT("TimeMenuText", "Notify Begin Time"));

				// Add item to directly set notify frame
				TSharedRef<SWidget> FrameWidget =
					SNew(SBox)
					.HAlign(HAlign_Right)
					.ToolTipText(LOCTEXT("SetFrameToolTip", "Set the frame of this notify directly"))
					[
						SNew(SBox)
						.Padding(FMargin(4.0f, 0.0f, 0.0f, 0.0f))
						.WidthOverride(100.0f)
						[
							SNew(SNumericEntryBox<int32>)
							.Font(FEditorStyle::GetFontStyle(TEXT("MenuItem.Font")))
							.MinValue(0)
							.MaxValue(AnimSequence->GetNumberOfSampledKeys())
							.Value(AnimSequence->GetFrameAtTime(NotifyEvent->GetTime()))
							.AllowSpin(false)
							.OnValueCommitted_Lambda([CommitBeginTime](int32 InValue, ETextCommit::Type InCommitType)
							{
								CommitBeginTime(InValue, InCommitType, true);
							})
						]
					];

				MenuBuilder.AddWidget(FrameWidget, LOCTEXT("FrameMenuText", "Notify Frame"));

				if (NotifyEvent)
				{
					// add menu to get threshold weight for triggering this notify
					TSharedRef<SWidget> ThresholdWeightWidget =
						SNew(SBox)
						.HAlign(HAlign_Right)
						.ToolTipText(LOCTEXT("MinTriggerWeightToolTip", "The minimum weight to trigger this notify"))
						[
							SNew(SBox)
							.Padding(FMargin(4.0f, 0.0f, 0.0f, 0.0f))
							.WidthOverride(100.0f)
							[
								SNew(SNumericEntryBox<float>)
								.Font(FEditorStyle::GetFontStyle(TEXT("MenuItem.Font")))
								.MinValue(0.0f)
								.MaxValue(1.0f)
								.Value(NotifyEvent->TriggerWeightThreshold)
								.AllowSpin(false)
								.OnValueCommitted_Lambda([this, NotifyEvent](float InValue, ETextCommit::Type InCommitType)
								{
									if (InCommitType == ETextCommit::OnEnter && NotifyEvent)
									{
										float NewWeight = FMath::Max(InValue, ZERO_ANIMWEIGHT_THRESH);
										NotifyEvent->TriggerWeightThreshold = NewWeight;

										FSlateApplication::Get().DismissAllMenus();
									}
								})
							]
						];

					auto CommitEndTime = [NotifyEvent, AnimSequenceDB](float InValue, ETextCommit::Type InCommitType, bool bValueAsFrame)
					{
						if (InCommitType == ETextCommit::OnEnter && NotifyEvent && AnimSequenceDB)
						{
							UAnimSequence* AnimSequence = *AnimSequenceDB->GetData();
							float NewDuration = FMath::Max(bValueAsFrame ? AnimSequence->GetTimeAtFrame(InValue) : InValue, MinimumStateDuration);
							float MaxDuration = AnimSequence->GetPlayLength() - NotifyEvent->GetTime();
							NewDuration = FMath::Min(NewDuration, MaxDuration);
							NotifyEvent->SetDuration(NewDuration);

							FSlateApplication::Get().DismissAllMenus();
						}
					};
					MenuBuilder.AddWidget(ThresholdWeightWidget, LOCTEXT("MinTriggerWeight", "Min Trigger Weight"));

					// Add menu for changing duration if this is an AnimNotifyState
					if (NotifyEvent->NotifyStateClass)
					{
						TSharedRef<SWidget> NotifyStateDurationWidget =
							SNew(SBox)
							.HAlign(HAlign_Right)
							.ToolTipText(LOCTEXT("SetAnimStateDuration_ToolTip", "The duration of this Anim Notify State"))
							[
								SNew(SBox)
								.Padding(FMargin(4.0f, 0.0f, 0.0f, 0.0f))
								.WidthOverride(100.0f)
								[
									SNew(SNumericEntryBox<float>)
									.Font(FEditorStyle::GetFontStyle(TEXT("MenuItem.Font")))
									.MinValue(MinimumStateDuration)
									.MinSliderValue(MinimumStateDuration)
									.MaxSliderValue(100.0f)
									.Value(NotifyEvent->GetDuration())
									.AllowSpin(false)
									.OnValueCommitted_Lambda([CommitEndTime](float InValue, ETextCommit::Type InCommitType)
									{
										CommitEndTime(InValue, InCommitType, false);
									})
								]
							];

						MenuBuilder.AddWidget(NotifyStateDurationWidget, LOCTEXT("SetAnimStateDuration", "Anim Notify State Duration"));

						TSharedRef<SWidget> NotifyStateDurationFramesWidget =
							SNew(SBox)
							.HAlign(HAlign_Right)
							.ToolTipText(LOCTEXT("SetAnimStateDurationFrames_ToolTip", "The duration of this Anim Notify State in frames"))
							[
								SNew(SBox)
								.Padding(FMargin(4.0f, 0.0f, 0.0f, 0.0f))
								.WidthOverride(100.0f)
								[
									SNew(SNumericEntryBox<int32>)
									.Font(FEditorStyle::GetFontStyle(TEXT("MenuItem.Font")))
									.MinValue(1)
									.MinSliderValue(1)
									.MaxSliderValue(AnimSequence->GetNumberOfSampledKeys())
									.Value(AnimSequence->GetFrameAtTime(NotifyEvent->GetDuration()))
									.AllowSpin(false)
									.OnValueCommitted_Lambda([CommitEndTime](int32 InValue, ETextCommit::Type InCommitType)
									{
										CommitEndTime(InValue, InCommitType, true);
									})
								]
							];

						MenuBuilder.AddWidget(NotifyStateDurationFramesWidget, LOCTEXT("SetAnimStateDurationFrames", "Anim Notify State Frames"));
					}
				}
			}
		}
		else
		{
			MenuBuilder.AddSubMenu(
				NSLOCTEXT("NewNotifySubMenu", "NewNotifySubMenuAddNotify", "Add Notify..."),
				NSLOCTEXT("NewNotifySubMenu", "NewNotifySubMenuAddNotifyToolTip", "Add AnimNotifyEvent"),
				FNewMenuDelegate::CreateRaw(this, &SActNotifyPoolLaneWidget::FillNewNotifyMenu, false));

			MenuBuilder.AddSubMenu(
				NSLOCTEXT("NewNotifySubMenu", "NewNotifySubMenuAddNotifyState", "Add Notify State..."),
				NSLOCTEXT("NewNotifySubMenu", "NewNotifySubMenuAddNotifyStateToolTip", "Add AnimNotifyState"),
				FNewMenuDelegate::CreateRaw(this, &SActNotifyPoolLaneWidget::FillNewNotifyStateMenu, false));
		}
	}
	MenuBuilder.EndSection();
	//AnimNotify

	NewAction.CanExecuteAction = 0;

	FWidgetPath WidgetPath = MouseEvent.GetEventPath() != nullptr ? *MouseEvent.GetEventPath() : FWidgetPath();

	// Display the newly built menu
	FSlateApplication::Get().PushMenu(SharedThis(this),
	                                  WidgetPath,
	                                  MenuBuilder.MakeWidget(),
	                                  MouseEvent.GetScreenSpacePosition(),
	                                  FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu));

	return TSharedPtr<SWidget>();
}

void SActNotifyPoolLaneWidget::FillNewNotifyStateMenu(FMenuBuilder& MenuBuilder, bool bIsReplaceWithMenu /* = false */)
{
	auto AnimSequenceDB = GetDataBinding(UAnimSequence**, "ActAnimation/AnimSequence");
	if (!AnimSequenceDB)
	{
		return;
	}
	UAnimSequence* AnimSequence = *AnimSequenceDB->GetData();
	NotifyStateFilter::MakeNewNotifyPicker<UAnimNotifyState>(MenuBuilder, false, AnimSequence, this->SharedThis(this));
}

void SActNotifyPoolLaneWidget::FillNewNotifyMenu(FMenuBuilder& MenuBuilder, bool bIsReplaceWithMenu /* = false */)
{
	auto AnimSequenceDB = GetDataBinding(UAnimSequence**, "ActAnimation/AnimSequence");
	if (!AnimSequenceDB)
	{
		return;
	}
	UAnimSequence* AnimSequence = *AnimSequenceDB->GetData();
	// now add custom anim notifiers
	USkeleton* SeqSkeleton = AnimSequence->GetSkeleton();
	if (SeqSkeleton)
	{
		MenuBuilder.BeginSection("AnimNotifySkeletonSubMenu", LOCTEXT("NewNotifySubMenu_Skeleton", "Skeleton Notifies"));
		{
			if (!bIsReplaceWithMenu)
			{
				FUIAction UIAction;
				UIAction.ExecuteAction.BindSP(
					this,
					&SActNotifyPoolLaneWidget::OnNewNotifyClicked);
				MenuBuilder.AddMenuEntry(LOCTEXT("NewNotify", "New Notify..."),
				                         LOCTEXT("NewNotifyToolTip", "Create a new animation notify on the skeleton"),
				                         FSlateIcon(),
				                         UIAction);
			}

			MenuBuilder.AddSubMenu(
				LOCTEXT("NewNotifySubMenu_Skeleton", "Skeleton Notifies"),
				LOCTEXT("NewNotifySubMenu_Skeleton_Tooltip", "Choose from custom notifies on the skeleton"),
				FNewMenuDelegate::CreateLambda([this, SeqSkeleton](FMenuBuilder& InSubMenuBuilder)
				{
					ISkeletonEditorModule& SkeletonEditorModule = FModuleManager::LoadModuleChecked<ISkeletonEditorModule>("SkeletonEditor");
					TSharedRef<IEditableSkeleton> EditableSkeleton = SkeletonEditorModule.CreateEditableSkeleton(SeqSkeleton);

					InSubMenuBuilder.AddWidget(
						SNew(SBox)
						.MinDesiredWidth(300.0f)
						.MaxDesiredHeight(400.0f),
						FText(),
						true,
						false
					);
				}));
		}
		MenuBuilder.EndSection();
	}

	NotifyStateFilter::MakeNewNotifyPicker<UAnimNotify>(MenuBuilder, bIsReplaceWithMenu, AnimSequence, this->SharedThis(this));
}


void SActNotifyPoolLaneWidget::OnNewNotifyClicked()
{
	// Show dialog to enter new track name
	TSharedRef<STextEntryPopup> TextEntry =
		SNew(STextEntryPopup)
		.Label(LOCTEXT("NewNotifyLabel", "Notify Name"))
		.OnTextCommitted(this, &SActNotifyPoolLaneWidget::AddNewNotify);

	// Show dialog to enter new event name
	FSlateApplication::Get().PushMenu(
		AsShared(),
		// Menu being summoned from a menu that is closing: Parent widget should be k2 not the menu that's open or it will be closed when the menu is dismissed
		FWidgetPath(),
		TextEntry,
		FSlateApplication::Get().GetCursorPos(),
		FPopupTransitionEffect(FPopupTransitionEffect::TypeInPopup)
	);
}


void SActNotifyPoolLaneWidget::AddNewNotify(const FText& NewNotifyName, ETextCommit::Type CommitInfo)
{
	auto AnimSequenceDB = GetDataBinding(UAnimSequence**, "ActAnimation/AnimSequence");
	if (!AnimSequenceDB)
	{
		return;
	}
	UAnimSequence* AnimSequence = *AnimSequenceDB->GetData();
	USkeleton* Skeleton = AnimSequence->GetSkeleton();
	if ((CommitInfo == ETextCommit::OnEnter) && Skeleton)
	{
		const FScopedTransaction Transaction(LOCTEXT("AddNewNotifyEvent", "Add New Anim Notify"));
		FName NewName = FName(*NewNotifyName.ToString());

		ISkeletonEditorModule& SkeletonEditorModule = FModuleManager::LoadModuleChecked<ISkeletonEditorModule>("SkeletonEditor");
		TSharedRef<IEditableSkeleton> EditableSkeleton = SkeletonEditorModule.CreateEditableSkeleton(Skeleton);

		EditableSkeleton->AddNotify(NewName);

		FBlueprintActionDatabase::Get().RefreshAssetActions(Skeleton);

		auto DB = GetDataBindingSP(FActCreateNewNotify, "CreateNewNotify");
		TSharedPtr<FActCreateNewNotify> CreateNewNotify = MakeShareable(new FActCreateNewNotify());
		{
			CreateNewNotify->NotifyClass = nullptr;
			CreateNewNotify->NewNotifyName = NewNotifyName.ToString();
		}
		DB->SetData(CreateNewNotify);
		OnCreateNewNotify(CreateNewNotify);
	}

	FSlateApplication::Get().DismissAllMenus();
}


void SActNotifyPoolLaneWidget::OnCreateNewNotify(TSharedPtr<FActCreateNewNotify> InActCreateNewNotify)
{
	if (!InActCreateNewNotify)
	{
		return;
	}
	auto DB = GetDataBinding(UAnimSequence**, "ActAnimation/AnimSequence");
	if (!DB)
	{
		return;
	}
	const FScopedTransaction Transaction(LOCTEXT("AddNotifyEvent", "Add Anim Notify"));
	UAnimSequence* AnimSequence = *DB->GetData();
	AnimSequence->Modify();
	FAnimNotifyEvent _;
	CreateNewNotify(InActCreateNewNotify->NewNotifyName, InActCreateNewNotify->NotifyClass, LastClickedTime, _);
	NovaDB::Trigger("ActImageTrack/Refresh");
}


bool SActNotifyPoolLaneWidget::CreateNewNotify(FString NewNotifyName, UClass* NotifyClass, float StartTime, FAnimNotifyEvent& OutAnimNotifyEvent)
{
	auto DB = GetDataBinding(UAnimSequence**, "ActAnimation/AnimSequence");
	if (!DB)
	{
		return false;
	}
	UAnimSequence* AnimSequence = *DB->GetData();
	// Insert a new notify record and spawn the new notify object
	int32 NewNotifyIndex = AnimSequence->Notifies.Add(FAnimNotifyEvent());
	OutAnimNotifyEvent = AnimSequence->Notifies[NewNotifyIndex];
	OutAnimNotifyEvent.NotifyName = FName(*NewNotifyName);
	OutAnimNotifyEvent.Guid = FGuid::NewGuid();

	OutAnimNotifyEvent.Link(AnimSequence, StartTime);
	OutAnimNotifyEvent.TriggerTimeOffset = GetTriggerTimeOffsetForType(AnimSequence->CalculateOffsetForNotify(StartTime));
	OutAnimNotifyEvent.TrackIndex = LaneIndex;

	if (NotifyClass)
	{
		class UObject* AnimNotifyClass = NewObject<UObject>(AnimSequence, NotifyClass, NAME_None, RF_Transactional);
		OutAnimNotifyEvent.NotifyStateClass = Cast<UAnimNotifyState>(AnimNotifyClass);
		OutAnimNotifyEvent.Notify = Cast<UAnimNotify>(AnimNotifyClass);

		if (OutAnimNotifyEvent.NotifyStateClass)
		{
			// Set default duration to 1 frame for AnimNotifyState.
			OutAnimNotifyEvent.SetDuration(MinimumStateDuration);
			OutAnimNotifyEvent.EndLink.Link(AnimSequence, OutAnimNotifyEvent.EndLink.GetTime());
			OutAnimNotifyEvent.TriggerWeightThreshold = OutAnimNotifyEvent.NotifyStateClass->GetDefaultTriggerWeightThreshold();
		}
		else if (OutAnimNotifyEvent.Notify)
		{
			OutAnimNotifyEvent.TriggerWeightThreshold = OutAnimNotifyEvent.Notify->GetDefaultTriggerWeightThreshold();
		}
	}
	else
	{
		OutAnimNotifyEvent.Notify = NULL;
		OutAnimNotifyEvent.NotifyStateClass = NULL;
	}

	if (OutAnimNotifyEvent.Notify)
	{
		TArray<FAssetData> SelectedAssets;
		AssetSelectionUtils::GetSelectedAssets(SelectedAssets);

		for (TFieldIterator<FObjectProperty> PropIt(OutAnimNotifyEvent.Notify->GetClass()); PropIt; ++PropIt)
		{
			if (PropIt->GetBoolMetaData(TEXT("ExposeOnSpawn")))
			{
				FObjectProperty* Property = *PropIt;
				const FAssetData* Asset = SelectedAssets.FindByPredicate([Property](const FAssetData& Other)
				{
					return Other.GetAsset()->IsA(Property->PropertyClass);
				});

				if (Asset)
				{
					uint8* Offset = (*PropIt)->ContainerPtrToValuePtr<uint8>(OutAnimNotifyEvent.Notify);
					(*PropIt)->ImportText_Direct(*Asset->GetAsset()->GetPathName(), Offset, OutAnimNotifyEvent.Notify, 0);
					break;
				}
			}
		}

		OutAnimNotifyEvent.Notify->OnAnimNotifyCreatedInEditor(OutAnimNotifyEvent);
	}
	else if (OutAnimNotifyEvent.NotifyStateClass)
	{
		OutAnimNotifyEvent.NotifyStateClass->OnAnimNotifyCreatedInEditor(OutAnimNotifyEvent);
	}

	AnimSequence->PostEditChange();
	AnimSequence->MarkPackageDirty();

	return true;
}

#undef LOCTEXT_NAMESPACE
