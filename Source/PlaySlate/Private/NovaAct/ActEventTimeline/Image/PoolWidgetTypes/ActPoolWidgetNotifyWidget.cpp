#include "ActPoolWidgetNotifyWidget.h"

#include "EditorStyleSet.h"
#include "StatusBarSubsystem.h"
#include "NovaAct/ActEventTimeline/Image/Subs/ActNotifyPoolEditorLaneWidget.h"
#include "NovaAct/ActEventTimeline/Image/Subs/NovaActUICommandInfo.h"
#include "Animation/EditorNotifyObject.h"
#include "Common/NovaConst.h"
#include "Common/NovaDataBinding.h"
#include "NovaAct/NovaActEditor.h"
#include "NovaAct/ActEventTimeline/Image/ImageTrackTypes/ActImageTrackBase.h"
#include "NovaAct/ActEventTimeline/Image/ImageTrackTypes/ActImageTrackNotify.h"
#include "NovaAct/ActEventTimeline/Image/Subs/ActNotifyPoolNotifyNodeWidget.h"
#include "NovaAct/ActEventTimeline/Operation/ActTrackAreaSlotDragDrop.h"

#define LOCTEXT_NAMESPACE "NovaAct"

using namespace NovaConst;

SActPoolWidgetNotifyWidget::~SActPoolWidgetNotifyWidget()
{
	// NovaDB::Delete("ActPoolNotify");
}

void SActPoolWidgetNotifyWidget::Construct(const FArguments& InArgs, const TSharedRef<FActImageTrackNotify>& InActImageTrackNotify)
{
	TSharedPtr<SActPoolWidgetNotifyWidget> Pool = SharedThis(this);
	NovaDB::CreateSP("ActPoolNotify", Pool);
	ActImageTrackNotify = InActImageTrackNotify;

	bIsSelecting = false;
	bIsUpdating = false;

	FNovaActNotifiesPanelCommands::Register();

	auto HostAppDB = GetDataBindingSP(FNovaActEditor, "NovaActEditor");
	TSharedRef<FUICommandList> CommandList = HostAppDB->GetData()->GetToolkitCommands();
	const FNovaActNotifiesPanelCommands& Commands = FNovaActNotifiesPanelCommands::Get();
	CommandList->MapAction(
		Commands.DeleteNotify,
		FExecuteAction::CreateSP(this, &SActPoolWidgetNotifyWidget::OnDeletePressed));

	auto DB = GetDataBinding(UAnimSequenceBase**, "ActAnimation/AnimSequence");
	UAnimSequenceBase* AnimSequenceBase = *(DB->GetData());
	AnimSequenceBase->RegisterOnNotifyChanged(
		UAnimSequenceBase::FOnNotifyChanged::CreateSP(this, &SActPoolWidgetNotifyWidget::RefreshNotifyTracks));

	if (GEditor)
	{
		GEditor->RegisterForUndo(this);
	}

	this->ChildSlot
	[
		SAssignNew(PanelArea, SBorder)
		.Visibility(EVisibility::SelfHitTestInvisible)
		.AddMetaData<FTagMetaData>(TEXT("AnimNotify.Notify"))
		.BorderImage(FEditorStyle::GetBrush("NoBorder"))
		.Padding(0.0f)
		.ColorAndOpacity(FLinearColor::White)
	];

	FDelegateHandle _;
	DataBindingSPBindRaw(IActImageTrackBase, "ActImageTrack/Refresh", this, &SActPoolWidgetNotifyWidget::OnLaneContentRefresh, _)
}

FVector2D SActPoolWidgetNotifyWidget::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
	return FVector2D(100.0f, ActImageTrackNotify->ActImageTrackArgs->Height);
}

void SActPoolWidgetNotifyWidget::OnDeletePressed()
{
	// If there's no focus on the panel it's likely the user is not editing notifies
	// so don't delete anything when the key is pressed.
	if (HasKeyboardFocus() || HasFocusedDescendants())
	{
		DeleteSelectedNodeObjects();
	}
}

void SActPoolWidgetNotifyWidget::DeleteSelectedNodeObjects() {}

void SActPoolWidgetNotifyWidget::RefreshNotifyTracks()
{
	auto DB = GetDataBinding(UAnimSequenceBase**, "ActAnimation/AnimSequence");
	if (!DB)
	{
		return;
	}
	UAnimSequenceBase* AnimSequenceBase = *(DB->GetData());
	if (AnimSequenceBase)
	{
		PanelArea->SetContent(
			SAssignNew(NotifySlots, SVerticalBox)
		);

		NotifyLanes.Empty();
		NotifyEditorTracks.Empty();

		for (int32 TrackIndex = 0; TrackIndex < AnimSequenceBase->AnimNotifyTracks.Num(); TrackIndex++)
		{
			TSharedRef<SActNotifyPoolEditorLaneWidget> EditorLaneWidget =
				SNew(SActNotifyPoolEditorLaneWidget)
				.LaneIndex(TrackIndex);
			NotifySlots->AddSlot()
			           .AutoHeight()
			           .VAlign(VAlign_Center)
			[
				EditorLaneWidget
			];

			NotifyLanes.Add(EditorLaneWidget->NotifyTrack);
			NotifyEditorTracks.Add(EditorLaneWidget);
		}
	}

	// Signal selection change to refresh details panel
	OnTrackSelectionChanged();
}


void SActPoolWidgetNotifyWidget::OnTrackSelectionChanged() {}

void SActPoolWidgetNotifyWidget::OnLaneContentRefresh(TSharedPtr<IActImageTrackBase> InActImageTrack)
{
	if (InActImageTrack != ActImageTrackNotify)
	{
		return;
	}
	if (!bIsUpdating)
	{
		auto DB = GetDataBinding(UAnimSequenceBase**, "ActAnimation/AnimSequence");
		if (!DB)
		{
			return;
		}
		TGuardValue<bool> ScopeGuard(bIsUpdating, true);

		UAnimSequenceBase* AnimSequenceBase = *(DB->GetData());
		if (AnimSequenceBase)
		{
			AnimSequenceBase->RefreshCacheData();
		}

		RefreshNotifyTracks();
	}
}


FReply SActPoolWidgetNotifyWidget::OnNotifyNodeDragStarted(const TSharedRef<SActNotifyPoolNotifyNodeWidget>& InNotifyNode,
                                                           const FPointerEvent& InMouseEvent,
                                                           const bool bDragOnMarker)
{
	SelectNotifyNode(InNotifyNode, InMouseEvent.IsShiftDown());

	// If we're dragging one of the direction markers we don't need to call any further as we don't want the drag drop op
	if (!bDragOnMarker)
	{
		TSharedRef<SOverlay> NodeDragDecoratorOverlay = SNew(SOverlay);
		TSharedRef<SBorder> NodeDragDecorator = SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				NodeDragDecoratorOverlay
			];

		TArray<TSharedPtr<SActNotifyPoolNotifyNodeWidget>> NotifyNodes;
		for (TSharedPtr<SActNotifyPoolLaneWidget> Lane : NotifyLanes)
		{
			Lane->DisconnectSelectedNodesForDrag(NotifyNodes);
		}
		FBox2D OverlayBounds = FBox2D(ForceInitToZero);
		for (const TSharedPtr<SActNotifyPoolNotifyNodeWidget>& NotifyNode : NotifyNodes)
		{
			OverlayBounds += FBox2D(NotifyNode->ScreenPosition, NotifyNode->ScreenPosition + FVector2D(NotifyNode->DurationSizeX, 0.0f));
		}
		FVector2D OverlayOrigin = OverlayBounds.Min;
		FVector2D OverlayExtents = OverlayBounds.GetSize();
		for (const TSharedPtr<SActNotifyPoolNotifyNodeWidget>& NotifyNode : NotifyNodes)
		{
			FVector2D OffsetFromMin(NotifyNode->ScreenPosition - OverlayOrigin);
			NodeDragDecoratorOverlay->AddSlot()
			                        .Padding(FMargin(OffsetFromMin.X, OffsetFromMin.Y, 0.0f, 0.0f))
			[
				NotifyNode.ToSharedRef()
			];
		}

		return FReply::Handled().BeginDragDrop(FActTrackAreaSlotDragDrop::New(SharedThis(this),
		                                                                      NotifyNodes,
		                                                                      NodeDragDecorator,
		                                                                      InMouseEvent.GetScreenSpacePosition(),
		                                                                      OverlayOrigin,
		                                                                      OverlayExtents,
		                                                                      CurrentDragXPosition));
	}
	else
	{
		// Capture the mouse in the node
		return FReply::Handled().CaptureMouse(InNotifyNode).UseHighPrecisionMouseMovement(InNotifyNode);
	}
}


void SActPoolWidgetNotifyWidget::SelectNotifyNode(const TSharedRef<SActNotifyPoolNotifyNodeWidget>& InNotifyNode,
                                                  bool Append)
{
	// Deselect all other notifies if necessary.
	if (!Append)
	{
		DeselectAllNotifies();
	}
	if (!SelectedNotifyNodes.Contains(InNotifyNode))
	{
		SelectedNotifyNodes.Add(InNotifyNode);
	}
}


void SActPoolWidgetNotifyWidget::DeselectAllNotifies()
{
	if (!bIsSelecting)
	{
		TGuardValue<bool> GuardValue(bIsSelecting, true);

		SelectedNotifyNodes.Empty();
	}
}

void SActPoolWidgetNotifyWidget::OnNotifyNodesBeingDraggedStatusBarMessage()
{
	if (!StatusBarMessageHandle.IsValid())
	{
		if (UStatusBarSubsystem* StatusBarSubsystem = GEditor->GetEditorSubsystem<UStatusBarSubsystem>())
		{
			StatusBarMessageHandle = StatusBarSubsystem->PushStatusBarMessage(AnimationEditorStatusBarName,
			                                                                  LOCTEXT("AutoscrubNotify",
			                                                                          "Hold SHIFT while dragging a notify to auto scrub the timeline."));
		}
	}
}

void SActPoolWidgetNotifyWidget::OnNotifyStateBeingDraggedStatusBarMessage()
{
	if (!StatusBarMessageHandle.IsValid())
	{
		if (UStatusBarSubsystem* StatusBarSubsystem = GEditor->GetEditorSubsystem<UStatusBarSubsystem>())
		{
			StatusBarMessageHandle = StatusBarSubsystem->PushStatusBarMessage(AnimationEditorStatusBarName,
			                                                                  LOCTEXT("AutoscrubNotifyStateHandle",
			                                                                          "Hold SHIFT while dragging a notify state Begin or End handle to auto scrub the timeline."));
		}
	}
}

void SActPoolWidgetNotifyWidget::ToggleNotifyNodeSelectStatus(const TSharedRef<SActNotifyPoolNotifyNodeWidget>& InNotifyNode)
{
	bool bSelected = SelectedNotifyNodes.Contains(InNotifyNode);
	if (bSelected)
	{
		SelectedNotifyNodes.Remove(InNotifyNode);
	}
	else
	{
		SelectedNotifyNodes.Add(InNotifyNode);
	}
}

bool SActPoolWidgetNotifyWidget::IsNotifyNodeSelected(const TSharedRef<SActNotifyPoolNotifyNodeWidget const>& InNotifyNode) const
{
	return SelectedNotifyNodes.Contains(InNotifyNode);
}

bool SActPoolWidgetNotifyWidget::IsSingleNotifyNodeSelected()
{
	return SelectedNotifyNodes.Num() == 1;
}

#undef LOCTEXT_NAMESPACE
