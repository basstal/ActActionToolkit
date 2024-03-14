#include "ActNotifyPoolNotifyNodeWidget.h"

#include "ActNotifyPoolLaneWidget.h"
#include "SCurveEditor.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Common/NovaEnum.h"
#include "Common/NovaConst.h"
#include "Common/NovaDataBinding.h"
#include "Common/NovaStaticFunction.h"
#include "Common/NovaStruct.h"
#include "Fonts/FontMeasure.h"
#include "NovaAct/ActEventTimeline/Image/PoolWidgetTypes/ActPoolWidgetNotifyWidget.h"

#define LOCTEXT_NAMESPACE "NovaAct"
using namespace NovaStruct;
using namespace NovaConst;

void SActNotifyPoolNotifyNodeWidget::Construct(const FArguments& InArgs,
                                               const TSharedRef<SActNotifyPoolLaneWidget>& InActNotifiesPanelLaneWidget)
{
	AnimNotifyEvent = InArgs._AnimNotifyEvent;
	check(AnimNotifyEvent)
	if (AnimNotifyEvent->Notify)
	{
		CachedNotifyName = FName(*AnimNotifyEvent->Notify->GetNotifyName());
	}
	else if (AnimNotifyEvent->NotifyStateClass)
	{
		CachedNotifyName = FName(*AnimNotifyEvent->NotifyStateClass->GetNotifyName());
	}
	else
	{
		CachedNotifyName = AnimNotifyEvent->NotifyName;
	}

	if (AnimNotifyEvent->Notify)
	{
		NotifyEditorColor = AnimNotifyEvent->Notify->GetEditorColor();
	}
	else if (AnimNotifyEvent->NotifyStateClass)
	{
		NotifyEditorColor = AnimNotifyEvent->NotifyStateClass->GetEditorColor();
	}
	else
	{
		NotifyEditorColor = FLinearColor(1, 1, 0.5f);
	}
	NotifyEditorColor.A = 0.67f;

	WeakActNotifyPoolLaneWidget = InActNotifiesPanelLaneWidget;
	bBeingDragged = false;
	CurrentDragHandle = ENovaNotifyStateHandleHit::None;
	DragMarkerTransactionIdx = INDEX_NONE;
	bDrawTooltipToRight = true;
	PanTrackRequest = InArgs._PanTrackRequest;
	OnSelectionChanged = InArgs._OnSelectionChanged;
	OnUpdatePanel = InArgs._OnUpdatePanel;
	OnSnapPosition = InArgs._OnSnapPosition;

	SetClipping(EWidgetClipping::ClipToBounds);
	SetToolTipText(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateLambda([this]()
	{
		auto DB = GetDataBinding(UAnimSequenceBase**, "ActAnimation/AnimSequence");
		if (!DB)
		{
			return FText::GetEmpty();
		}
		FText ToolTipText = NovaStaticFunction::MakeTooltipFromTime(*DB->GetData(), AnimNotifyEvent->GetTime(), AnimNotifyEvent->GetDuration());

		if (AnimNotifyEvent->IsBranchingPoint())
		{
			ToolTipText = FText::Format(LOCTEXT("AnimNotify_ToolTipBranchingPoint", "{0} (BranchingPoint)"), ToolTipText);
		}

		UObject* NotifyToDisplayClassOf = AnimNotifyEvent->Notify;
		if (!NotifyToDisplayClassOf)
		{
			NotifyToDisplayClassOf = AnimNotifyEvent->NotifyStateClass;
		}
		if (NotifyToDisplayClassOf)
		{
			ToolTipText = FText::Format(
				LOCTEXT("AnimNotify_ToolTipNotifyClass", "{0}\nClass: {1}"),
				ToolTipText,
				NotifyToDisplayClassOf->GetClass()->GetDisplayNameText());
		}

		return ToolTipText;
	})));
}

int32 SActNotifyPoolNotifyNodeWidget::OnPaint(const FPaintArgs& Args,
                                              const FGeometry& AllottedGeometry,
                                              const FSlateRect& MyCullingRect,
                                              FSlateWindowElementList& OutDrawElements,
                                              int32 LayerId,
                                              const FWidgetStyle& InWidgetStyle,
                                              bool bParentEnabled) const
{
	auto DB = GetDataBinding(UAnimSequence**, "ActAnimation/AnimSequence");
	if (!DB)
	{
		return LayerId;
	}
	const FSlateBrush* StyleInfo = FEditorStyle::GetBrush(TEXT("SpecialEditableTextImageNormal"));

	FLinearColor BoxColor = IsSelected() ? FEditorStyle::GetSlateColor("SelectionColor").GetSpecifiedColor() : NotifyEditorColor;
	float HalfScrubHandleWidth = ScrubHandleSize.X / 2.0f;
	// Show duration of AnimNotifyState
	if (DurationSizeX > 0.f)
	{
		FVector2D DurationBoxSize = FVector2D(DurationSizeX, TextSize.Y + TextBorderSize.Y * 2.f);
		FVector2D DurationBoxPosition = FVector2D(NotifyScrubHandleCentre, (NotifyHeight - TextSize.Y) * 0.5f);
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(DurationBoxPosition, DurationBoxSize),
			StyleInfo,
			ESlateDrawEffect::None,
			BoxColor);

		DrawScrubHandle(DurationBoxPosition.X + DurationBoxSize.X,
		                OutDrawElements,
		                LayerId + 2,
		                AllottedGeometry,
		                MyCullingRect,
		                NotifyEditorColor);

		// Render offsets if necessary
		// Do we have an offset to render?
		if (AnimNotifyEvent && AnimNotifyEvent->EndTriggerTimeOffset != 0.f)
		{
			float EndTime = AnimNotifyEvent->GetTime() + AnimNotifyEvent->GetDuration();
			//Don't render offset when we are at the end of the sequence, doesnt help the user
			UAnimSequence* AnimSequence = *(DB->GetData());
			if (EndTime != AnimSequence->GetPlayLength())
			{
				float HandleCentre = DurationSizeX + (ScrubHandleSize.X - 2.0f);
				DrawHandleOffset(AnimNotifyEvent->EndTriggerTimeOffset,
				                 HandleCentre,
				                 OutDrawElements,
				                 LayerId + 1,
				                 AllottedGeometry,
				                 MyCullingRect,
				                 NotifyEditorColor);
			}
		}
	}

	// Branching point
	bool bDrawBranchingPoint = AnimNotifyEvent && AnimNotifyEvent->IsBranchingPoint();

	// Background
	FVector2D LabelSize = TextSize + TextBorderSize * 2.f;
	LabelSize.X += HalfScrubHandleWidth + (bDrawBranchingPoint ? (BranchingPointIconSize.X + TextBorderSize.X * 2.f) : 0.f);

	FVector2D LabelPosition(bDrawTooltipToRight ? NotifyScrubHandleCentre : NotifyScrubHandleCentre - LabelSize.X,
	                        (NotifyHeight - TextSize.Y) * 0.5f);

	if (DurationSizeX == 0.f)
	{
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(LabelPosition, LabelSize),
			StyleInfo,
			ESlateDrawEffect::None,
			BoxColor);
	}

	// Text
	FVector2D TextPosition = LabelPosition + TextBorderSize;
	if (bDrawTooltipToRight)
	{
		TextPosition.X += HalfScrubHandleWidth;
	}

	float ValueX = DurationSizeX - ScrubHandleSize.X - (bDrawBranchingPoint ? BranchingPointIconSize.X : 0);
	FVector2D DrawTextSize((DurationSizeX > 0.0f ? FMath::Min(ValueX, TextSize.X) : TextSize.X), TextSize.Y);

	if (bDrawBranchingPoint)
	{
		TextPosition.X += BranchingPointIconSize.X;
	}

	FPaintGeometry TextGeometry = AllottedGeometry.ToPaintGeometry(TextPosition, DrawTextSize);
	OutDrawElements.PushClip(FSlateClippingZone(TextGeometry));

	FSlateDrawElement::MakeText(
		OutDrawElements,
		LayerId + 3,
		TextGeometry,
		GetNotifyText(),
		Font,
		ESlateDrawEffect::None,
		FLinearColor::Black
	);

	OutDrawElements.PopClip();

	// Draw Branching Point
	if (bDrawBranchingPoint)
	{
		FVector2D BranchPointIconPos = LabelPosition + TextBorderSize;
		if (bDrawTooltipToRight)
		{
			BranchPointIconPos.X += HalfScrubHandleWidth;
		}
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId + 4,
			AllottedGeometry.ToPaintGeometry(BranchPointIconPos, BranchingPointIconSize),
			FEditorStyle::GetBrush(TEXT("AnimNotifyEditor.BranchingPoint")),
			ESlateDrawEffect::None,
			FLinearColor::White
		);
	}

	DrawScrubHandle(NotifyScrubHandleCentre, OutDrawElements, LayerId + 2, AllottedGeometry, MyCullingRect, NotifyEditorColor);

	//Do we have an offset to render?
	if (AnimNotifyEvent && AnimNotifyEvent->TriggerTimeOffset != 0.f)
	{
		float NotifyTime = AnimNotifyEvent->GetTime();
		UAnimSequence* AnimSequence = *(DB->GetData());
		//Don't render offset when we are at the start/end of the sequence, doesn't help the user
		if (NotifyTime != 0.f && NotifyTime != AnimSequence->GetPlayLength())
		{
			DrawHandleOffset(AnimNotifyEvent->TriggerTimeOffset,
			                 NotifyScrubHandleCentre,
			                 OutDrawElements,
			                 LayerId + 1,
			                 AllottedGeometry,
			                 MyCullingRect,
			                 NotifyEditorColor);
		}
	}

	return LayerId + 3;
}

FReply SActNotifyPoolNotifyNodeWidget::OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	auto PoolNotifyDB = GetDataBindingSP(SActPoolWidgetNotifyWidget, "ActPoolNotify");
	if (!PoolNotifyDB)
	{
		return FReply::Unhandled();
	}
	// Whether the drag has hit a duration marker
	bool bDragOnMarker = false;
	bBeingDragged = true;

	if (DurationSizeX > 0.0f)
	{
		// This is a state node, check for a drag on the markers before movement. Use last screen space position before the drag started
		// as using the last position in the mouse event gives us a mouse position after the drag was started.
		ENovaNotifyStateHandleHit MarkerHit = DurationHandleHitTest(LastMouseDownPosition);
		if (MarkerHit == ENovaNotifyStateHandleHit::Start || MarkerHit == ENovaNotifyStateHandleHit::End)
		{
			bDragOnMarker = true;
			bBeingDragged = false;
			CurrentDragHandle = MarkerHit;

			// Modify the owning sequence as we're now dragging the marker and begin a transaction
			check(DragMarkerTransactionIdx == INDEX_NONE);
			DragMarkerTransactionIdx = GEditor->BeginTransaction(NSLOCTEXT("AnimNotifyNode", "StateNodeDragTransation", "Drag State Node Marker"));
			auto DB = GetDataBinding(UAnimSequence**, "ActAnimation/AnimSequence");
			(*DB->GetData())->Modify();
		}
	}

	return PoolNotifyDB->GetData()->OnNotifyNodeDragStarted(SharedThis(this),
	                                                        MouseEvent,
	                                                        bDragOnMarker);
}


FCursorReply SActNotifyPoolNotifyNodeWidget::OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const
{
	// Show resize cursor if the cursor is hovering over either of the scrub handles of a notify state node
	if (IsHovered() && DurationSizeX > 0.0f)
	{
		FVector2D RelMouseLocation = MyGeometry.AbsoluteToLocal(CursorEvent.GetScreenSpacePosition());

		const float HandleHalfWidth = ScrubHandleSize.X / 2.0f;
		const float DistFromFirstHandle = FMath::Abs(RelMouseLocation.X - NotifyScrubHandleCentre);
		const float DistFromSecondHandle = FMath::Abs(RelMouseLocation.X - (NotifyScrubHandleCentre + DurationSizeX));

		if (DistFromFirstHandle < HandleHalfWidth || DistFromSecondHandle < HandleHalfWidth || CurrentDragHandle != ENovaNotifyStateHandleHit::None)
		{
			return FCursorReply::Cursor(EMouseCursor::ResizeLeftRight);
		}
	}
	return FCursorReply::Unhandled();
}

FReply SActNotifyPoolNotifyNodeWidget::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	auto DB = GetDataBindingSP(FActEventTimelineArgs, "ActEventTimelineArgs");
	auto AnimSequenceDB = GetDataBinding(UAnimSequence**, "ActAnimation/AnimSequence");
	if (!AnimSequenceDB || !DB)
	{
		return FReply::Unhandled();
	}
	// Don't do scrub handle dragging if we haven't captured the mouse.
	if (!HasMouseCapture())
	{
		return FReply::Unhandled();
	}

	if (CurrentDragHandle == ENovaNotifyStateHandleHit::None)
	{
		// We've had focus taken away - release the mouse
		FSlateApplication::Get().ReleaseAllPointerCapture();
		return FReply::Unhandled();
	}
	TSharedPtr<FActEventTimelineArgs> ActEventTimelineArgs = DB->GetData();
	FTrackScaleInfo ScaleInfo = *WeakActNotifyPoolLaneWidget.Pin()->CachedScaleInfo;

	const FGeometry& CachedLaneGeometry = WeakActNotifyPoolLaneWidget.Pin()->GetCachedGeometry();
	const FVector2D& CachedAllottedGeometrySizeScaled = WeakActNotifyPoolLaneWidget.Pin()->CachedAllottedGeometrySizeScaled;
	float XPositionInTrack = MyGeometry.AbsolutePosition.X - CachedLaneGeometry.AbsolutePosition.X;
	float TrackScreenSpaceXPosition = MyGeometry.AbsolutePosition.X - XPositionInTrack;
	float TrackScreenSpaceOrigin = CachedLaneGeometry.LocalToAbsolute(FVector2D(ScaleInfo.InputToLocalX(0.0f), 0.0f)).X;
	float PlayLength = (*AnimSequenceDB->GetData())->GetPlayLength();
	float TrackScreenSpaceLimit = CachedLaneGeometry.LocalToAbsolute(FVector2D(ScaleInfo.InputToLocalX(PlayLength), 0.0f)).X;
	auto GetScrubHandleSnapPosition = [ScaleInfo](float& NotifyInputX)
	{
		const float MaxSnapDist = 5.0f;

		if (!FSlateApplication::Get().GetModifierKeys().IsControlDown())
		{
			if (NovaStaticFunction::OnSnapTime(NotifyInputX, MaxSnapDist / ScaleInfo.PixelsPerInput, TArrayView<const FName>()))
			{
				return NotifyInputX;
			}
		}
		return -1.0f;
	};
	if (CurrentDragHandle == ENovaNotifyStateHandleHit::Start)
	{
		// Check track bounds
		float OldDisplayTime = AnimNotifyEvent->GetTime();
		if (MouseEvent.GetScreenSpacePosition().X >= TrackScreenSpaceXPosition &&
			MouseEvent.GetScreenSpacePosition().X <= TrackScreenSpaceXPosition + CachedAllottedGeometrySizeScaled.X)
		{
			float NewDisplayTime = ScaleInfo.LocalXToInput((MouseEvent.GetScreenSpacePosition() - MyGeometry.AbsolutePosition + XPositionInTrack).X);
			float NewDuration = AnimNotifyEvent->GetDuration() + OldDisplayTime - NewDisplayTime;
			// Check to make sure the duration is not less than the minimum allowed
			if (NewDuration < MinimumStateDuration)
			{
				NewDisplayTime -= MinimumStateDuration - NewDuration;
			}
			AnimNotifyEvent->SetTime(FMath::Max(0.0f, NewDisplayTime));
			AnimNotifyEvent->SetDuration(AnimNotifyEvent->GetDuration() + OldDisplayTime - AnimNotifyEvent->GetTime());
		}
		else if (AnimNotifyEvent->GetDuration() > MinimumStateDuration)
		{
			HandleOverflowPan(MouseEvent.GetScreenSpacePosition(),
			                  TrackScreenSpaceXPosition,
			                  TrackScreenSpaceOrigin,
			                  TrackScreenSpaceLimit);
			// Update scale info to the new view inputs after panning
			ScaleInfo.ViewMinInput = ActEventTimelineArgs->ViewRange->GetLowerBoundValue();
			ScaleInfo.ViewMaxInput = ActEventTimelineArgs->ViewRange->GetUpperBoundValue();

			float NewDisplayTime = FMath::Max(0.0f,
			                                  ScaleInfo.LocalXToInput(
				                                  (MouseEvent.GetScreenSpacePosition() - MyGeometry.AbsolutePosition + XPositionInTrack).X));
			AnimNotifyEvent->SetTime(FMath::Max(0.0f, NewDisplayTime));
			AnimNotifyEvent->SetDuration(AnimNotifyEvent->GetDuration() + OldDisplayTime - AnimNotifyEvent->GetTime());

			// Adjust in case we went under the minimum
			if (AnimNotifyEvent->GetDuration() < MinimumStateDuration)
			{
				AnimNotifyEvent->SetTime(AnimNotifyEvent->GetTime() + AnimNotifyEvent->GetDuration() - MinimumStateDuration);
				AnimNotifyEvent->SetDuration(MinimumStateDuration);
			}
		}

		// Now we know where the marker should be, look for possible snaps on montage marker bars
		if (AnimNotifyEvent)
		{
			float InputStartTime = AnimNotifyEvent->GetTime();
			float MarkerSnap = GetScrubHandleSnapPosition(InputStartTime);
			if (MarkerSnap != -1.0f)
			{
				// We're near to a snap bar
				EAnimEventTriggerOffsets::Type Offset = (MarkerSnap < InputStartTime) ?
					                                        EAnimEventTriggerOffsets::OffsetAfter :
					                                        EAnimEventTriggerOffsets::OffsetBefore;
				AnimNotifyEvent->TriggerTimeOffset = GetTriggerTimeOffsetForType(Offset);

				// Adjust our start marker
				OldDisplayTime = AnimNotifyEvent->GetTime();
				AnimNotifyEvent->SetTime(MarkerSnap);
				AnimNotifyEvent->SetDuration(AnimNotifyEvent->GetDuration() + OldDisplayTime - AnimNotifyEvent->GetTime());
			}
			else
			{
				AnimNotifyEvent->TriggerTimeOffset = GetTriggerTimeOffsetForType(EAnimEventTriggerOffsets::NoOffset);
			}
		}
	}
	else
	{
		if (MouseEvent.GetScreenSpacePosition().X >= TrackScreenSpaceXPosition && MouseEvent.GetScreenSpacePosition().X <= TrackScreenSpaceXPosition +
			CachedAllottedGeometrySizeScaled.X)
		{
			float NewDuration = ScaleInfo.LocalXToInput((MouseEvent.GetScreenSpacePosition() - MyGeometry.AbsolutePosition + XPositionInTrack).X) -
				AnimNotifyEvent->GetTime();

			AnimNotifyEvent->SetDuration(FMath::Max(NewDuration, MinimumStateDuration));
		}
		else if (AnimNotifyEvent->GetDuration() > MinimumStateDuration)
		{
			HandleOverflowPan(MouseEvent.GetScreenSpacePosition(),
			                  TrackScreenSpaceXPosition,
			                  TrackScreenSpaceOrigin,
			                  TrackScreenSpaceLimit);

			// Update scale info to the new view inputs after panning
			ScaleInfo.ViewMinInput = ActEventTimelineArgs->ViewRange->GetLowerBoundValue();
			ScaleInfo.ViewMaxInput = ActEventTimelineArgs->ViewRange->GetUpperBoundValue();

			float NewDuration = ScaleInfo.LocalXToInput((MouseEvent.GetScreenSpacePosition() - MyGeometry.AbsolutePosition + XPositionInTrack).X) -
				AnimNotifyEvent->GetTime();
			AnimNotifyEvent->SetDuration(FMath::Max(NewDuration, MinimumStateDuration));
		}

		if (AnimNotifyEvent->GetTime() + AnimNotifyEvent->GetDuration() > PlayLength)
		{
			AnimNotifyEvent->SetDuration(PlayLength - AnimNotifyEvent->GetTime());
		}

		// Now we know where the scrub handle should be, look for possible snaps on montage marker bars
		if (AnimNotifyEvent)
		{
			float InputEndTime = AnimNotifyEvent->GetTime() + AnimNotifyEvent->GetDuration();
			float MarkerSnap = GetScrubHandleSnapPosition(InputEndTime);
			if (MarkerSnap != -1.0f)
			{
				// We're near to a snap bar
				EAnimEventTriggerOffsets::Type Offset = (MarkerSnap < InputEndTime) ?
					                                        EAnimEventTriggerOffsets::OffsetAfter :
					                                        EAnimEventTriggerOffsets::OffsetBefore;
				AnimNotifyEvent->EndTriggerTimeOffset = GetTriggerTimeOffsetForType(Offset);

				// Adjust our end marker
				AnimNotifyEvent->SetDuration(MarkerSnap - AnimNotifyEvent->GetTime());
			}
			else
			{
				AnimNotifyEvent->EndTriggerTimeOffset = GetTriggerTimeOffsetForType(EAnimEventTriggerOffsets::NoOffset);
			}
		}
	}
	if (MouseEvent.IsShiftDown())
	{
		float Time = AnimNotifyEvent->GetTime();
		if (CurrentDragHandle == ENovaNotifyStateHandleHit::End)
		{
			Time += AnimNotifyEvent->GetDuration();
		}
		FFrameTime FrameTime = FFrameTime::FromDecimal(Time * (double)ActEventTimelineArgs->TickResolution.AsDecimal());
		*ActEventTimelineArgs->CurrentTime = FrameTime;
		NovaDB::Trigger("ActEventTimelineArgs/CurrentTime");
	}
	auto PoolNotifyDB = GetDataBindingSP(SActPoolWidgetNotifyWidget, "ActPoolNotify");
	if (PoolNotifyDB)
	{
		PoolNotifyDB->GetData()->OnNotifyStateBeingDraggedStatusBarMessage();
	}

	return FReply::Handled();
}

FReply SActNotifyPoolNotifyNodeWidget::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	bool bLeftButton = MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton;

	if (bLeftButton && CurrentDragHandle != ENovaNotifyStateHandleHit::None)
	{
		// Clear the drag marker and give the mouse back
		CurrentDragHandle = ENovaNotifyStateHandleHit::None;

		// Signal selection changing so details panels get updated
		OnSelectionChanged.ExecuteIfBound();

		// End drag transaction before handing mouse back
		check(DragMarkerTransactionIdx != INDEX_NONE);
		GEditor->EndTransaction();
		DragMarkerTransactionIdx = INDEX_NONE;

		auto DB = GetDataBinding(UAnimSequence**, "ActAnimation/AnimSequence");
		if (DB)
		{
			UAnimSequence* AnimSequence = *DB->GetData();
			AnimSequence->PostEditChange();
			AnimSequence->MarkPackageDirty();
		}

		OnUpdatePanel.ExecuteIfBound();

		return FReply::Handled().ReleaseMouseCapture();
	}

	return FReply::Unhandled();
}


FVector2D SActNotifyPoolNotifyNodeWidget::ComputeDesiredSize(float) const
{
	return WidgetSize;
}

void SActNotifyPoolNotifyNodeWidget::OnFocusLost(const FFocusEvent& InFocusEvent)
{
	if (CurrentDragHandle != ENovaNotifyStateHandleHit::None)
	{
		// Lost focus while dragging a state node, clear the drag and end the current transaction
		CurrentDragHandle = ENovaNotifyStateHandleHit::None;

		check(DragMarkerTransactionIdx != INDEX_NONE);
		GEditor->EndTransaction();
		DragMarkerTransactionIdx = INDEX_NONE;
	}
}

void SActNotifyPoolNotifyNodeWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	ScreenPosition = FVector2D(AllottedGeometry.AbsolutePosition);
}

FText SActNotifyPoolNotifyNodeWidget::GetNotifyText() const
{
	// Combine comment from notify struct and from function on object
	return FText::FromName(CachedNotifyName);
}

void SActNotifyPoolNotifyNodeWidget::UpdateSizeAndPosition(float AllottedGeometrySizeX)
{
	auto DB = GetDataBindingSP(TRange<double>, "ActEventTimelineArgs/ViewRange");
	if (!DB)
	{
		return;
	}
	TSharedPtr<TRange<double>> ViewRange = DB->GetData();
	FTrackScaleInfo ScaleInfo = *WeakActNotifyPoolLaneWidget.Pin()->CachedScaleInfo;
	// Cache the geometry information, the allotted geometry is the same size as the track.
	NotifyTimePositionX = ScaleInfo.InputToLocalX(AnimNotifyEvent->GetTime());
	DurationSizeX = ScaleInfo.PixelsPerInput * AnimNotifyEvent->GetDuration();
	const TSharedRef<FSlateFontMeasure> FontMeasureService = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	TextSize = FontMeasureService->Measure(GetNotifyText(), Font);
	float LabelWidth = TextSize.X + (TextBorderSize.X * 2.f) + (ScrubHandleSize.X / 2.f);

	bool bDrawBranchingPoint = AnimNotifyEvent->IsBranchingPoint();
	if (bDrawBranchingPoint)
	{
		BranchingPointIconSize = FVector2D(TextSize.Y, TextSize.Y);
		LabelWidth += BranchingPointIconSize.X + TextBorderSize.X * 2.f;
	}

	//Calculate scrub handle box size (the notional box around the scrub handle and the alignment marker)
	float NotifyHandleBoxWidth = FMath::Max(ScrubHandleSize.X, AlignmentMarkerSize.X * 2);
	// Work out where we will have to draw the tool tip
	float LeftEdgeToNotify = NotifyTimePositionX;
	float RightEdgeToNotify = AllottedGeometrySizeX - NotifyTimePositionX;
	bDrawTooltipToRight = DurationSizeX > 0.0f || ((RightEdgeToNotify > LabelWidth) || (RightEdgeToNotify > LeftEdgeToNotify));

	// Calculate widget width/position based on where we are drawing the tool tip
	WidgetX = bDrawTooltipToRight ? (NotifyTimePositionX - (NotifyHandleBoxWidth / 2.f)) : (NotifyTimePositionX - LabelWidth);
	WidgetSize = bDrawTooltipToRight ?
		             FVector2D((DurationSizeX > 0.0f ? DurationSizeX : FMath::Max(LabelWidth, DurationSizeX)),
		                       NotifyHeight) :
		             FVector2D((LabelWidth + DurationSizeX), NotifyHeight);
	WidgetSize.X += NotifyHandleBoxWidth;
	// Widget position of the notify marker
	NotifyScrubHandleCentre = bDrawTooltipToRight ? NotifyHandleBoxWidth / 2.f : LabelWidth;
}

void SActNotifyPoolNotifyNodeWidget::DrawScrubHandle(float ScrubHandleCentre,
                                                     FSlateWindowElementList& OutDrawElements,
                                                     int32 ScrubHandleID,
                                                     const FGeometry& AllottedGeometry,
                                                     const FSlateRect& MyCullingRect,
                                                     FLinearColor NodeColour) const
{
	FVector2D ScrubHandlePosition(ScrubHandleCentre - ScrubHandleSize.X / 2.0f, (NotifyHeight - ScrubHandleSize.Y) / 2.f);
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		ScrubHandleID,
		AllottedGeometry.ToPaintGeometry(ScrubHandlePosition, ScrubHandleSize),
		FEditorStyle::GetBrush(TEXT("Sequencer.KeyDiamond")),
		ESlateDrawEffect::None,
		NodeColour
	);

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		ScrubHandleID,
		AllottedGeometry.ToPaintGeometry(ScrubHandlePosition, ScrubHandleSize),
		FEditorStyle::GetBrush(TEXT("Sequencer.KeyDiamondBorder")),
		ESlateDrawEffect::None,
		IsSelected() ? FEditorStyle::GetSlateColor("SelectionColor").GetSpecifiedColor() : FLinearColor::Black
	);
}


void SActNotifyPoolNotifyNodeWidget::DrawHandleOffset(const float& Offset,
                                                      const float& HandleCentre,
                                                      FSlateWindowElementList& OutDrawElements,
                                                      int32 MarkerLayer,
                                                      const FGeometry& AllottedGeometry,
                                                      const FSlateRect& MyCullingRect,
                                                      FLinearColor NodeColor) const
{
	FVector2D MarkerPosition;
	FVector2D MarkerSize = AlignmentMarkerSize;

	if (Offset < 0.f)
	{
		MarkerPosition.Set(HandleCentre - AlignmentMarkerSize.X, (NotifyHeight - AlignmentMarkerSize.Y) / 2.f);
	}
	else
	{
		MarkerPosition.Set(HandleCentre + AlignmentMarkerSize.X, (NotifyHeight - AlignmentMarkerSize.Y) / 2.f);
		MarkerSize.X = -AlignmentMarkerSize.X;
	}

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		MarkerLayer,
		AllottedGeometry.ToPaintGeometry(MarkerPosition, MarkerSize),
		FEditorStyle::GetBrush(TEXT("Sequencer.Timeline.NotifyAlignmentMarker")),
		ESlateDrawEffect::None,
		NodeColor
	);
}


ENovaNotifyStateHandleHit SActNotifyPoolNotifyNodeWidget::DurationHandleHitTest(const FVector2D& CursorTrackPosition) const
{
	ENovaNotifyStateHandleHit MarkerHit = ENovaNotifyStateHandleHit::None;

	// Make sure this node has a duration box (meaning it is a state node)
	if (DurationSizeX > 0.0f)
	{
		// Test for mouse inside duration box with handles included
		float ScrubHandleHalfWidth = ScrubHandleSize.X / 2.0f;

		// Position and size of the notify node including the scrub handles
		FVector2D NotifyNodePosition(NotifyScrubHandleCentre - ScrubHandleHalfWidth, 0.0f);
		FVector2D NotifyNodeSize(DurationSizeX + ScrubHandleHalfWidth * 2.0f, NotifyHeight);

		FVector2D MouseRelativePosition(CursorTrackPosition - FVector2D(WidgetX, NotifyHeightOffset));

		if (MouseRelativePosition > NotifyNodePosition && MouseRelativePosition < (NotifyNodePosition + NotifyNodeSize))
		{
			// Definitely inside the duration box, need to see which handle we hit if any
			if (MouseRelativePosition.X <= (NotifyNodePosition.X + ScrubHandleSize.X))
			{
				// Left Handle
				MarkerHit = ENovaNotifyStateHandleHit::Start;
			}
			else if (MouseRelativePosition.X >= (NotifyNodePosition.X + NotifyNodeSize.X - ScrubHandleSize.X))
			{
				// Right Handle
				MarkerHit = ENovaNotifyStateHandleHit::End;
			}
		}
	}

	return MarkerHit;
}

/** @return the Node's position within the track */
FVector2D SActNotifyPoolNotifyNodeWidget::GetWidgetPosition() const
{
	return FVector2D(WidgetX, NotifyHeightOffset);
}


float SActNotifyPoolNotifyNodeWidget::HandleOverflowPan(const FVector2D& ScreenCursorPos,
                                                        float TrackScreenSpaceXPosition,
                                                        float TrackScreenSpaceMin,
                                                        float TrackScreenSpaceMax)
{
	float Overflow = 0.0f;
	const FVector2D& CachedAllottedGeometrySizeScaled = WeakActNotifyPoolLaneWidget.Pin()->CachedAllottedGeometrySizeScaled;
	if (ScreenCursorPos.X < TrackScreenSpaceXPosition && TrackScreenSpaceXPosition > TrackScreenSpaceMin - 10.0f)
	{
		// Overflow left edge
		Overflow = FMath::Min(ScreenCursorPos.X - TrackScreenSpaceXPosition, -10.0f);
	}
	else if (ScreenCursorPos.X > CachedAllottedGeometrySizeScaled.X &&
		(TrackScreenSpaceXPosition + CachedAllottedGeometrySizeScaled.X) < TrackScreenSpaceMax + 10.0f)
	{
		// Overflow right edge
		Overflow = FMath::Max(ScreenCursorPos.X - (TrackScreenSpaceXPosition + CachedAllottedGeometrySizeScaled.X), 10.0f);
	}

	NovaStaticFunction::OnViewRangePan(Overflow, CachedAllottedGeometrySizeScaled);
	// NovaDB::Trigger("ActEventTimelineArgs/ViewRange");
	return Overflow;
}


FVector2D SActNotifyPoolNotifyNodeWidget::GetNotifyPositionOffset() const
{
	return GetNotifyPosition() - GetWidgetPosition();
}

FVector2D SActNotifyPoolNotifyNodeWidget::GetNotifyPosition() const
{
	return FVector2D(NotifyTimePositionX, NotifyHeightOffset);
}

bool SActNotifyPoolNotifyNodeWidget::IsSelected() const
{
	auto DB = GetDataBindingSP(SActPoolWidgetNotifyWidget, "ActPoolNotify");
	if (DB)
	{
		return DB->GetData()->IsNotifyNodeSelected(SharedThis(this));
	}
	return false;
}

#undef LOCTEXT_NAMESPACE
