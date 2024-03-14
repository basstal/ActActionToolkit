#include "ActSliderWidget.h"

#include "PlaySlate.h"
#include "Common/NovaConst.h"
#include "Common/NovaStaticFunction.h"
#include "Common/NovaStruct.h"
// ReSharper disable once CppUnusedIncludeDirective
#include "ITransportControl.h"
#include "NovaAct/ActEventTimeline/Slider/ActSliderViewRangeWidget.h"

#include "LevelEditorViewport.h"
#include "Widgets/Layout/SGridPanel.h"

using namespace NovaConst;

SActSliderWidget::SActSliderWidget()
	: DistanceDragged(0),
	  MouseDragType(ENovaDragType::DRAG_NONE),
	  bPanning(false) {}

SActSliderWidget::~SActSliderWidget()
{
	UE_LOG(LogNovaAct, Log, TEXT("SActSliderWidget::~SActSliderWidget"));
	// NovaDB::Delete("ActEventTimelineArgs/ViewRange");
}

void SActSliderWidget::Construct(const FArguments& InArgs, const TSharedRef<SGridPanel>& InParentGridPanel)
{
	auto DB = GetDataBindingSP(FActEventTimelineArgs, "ActEventTimelineArgs");
	NovaDB::CreateSP("ActEventTimelineArgs/ViewRange", DB->GetData()->ViewRange);

	InParentGridPanel->AddSlot(1, 0, SGridPanel::Layer(10))
	                 .Padding(NovaConst::ResizeBarPadding)
	[
		SNew(SBorder)
		.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
		.BorderBackgroundColor(FLinearColor(.50f, .50f, .50f, 1.0f))
		.Padding(0)
		.Clipping(EWidgetClipping::ClipToBounds)
		[
			AsShared()
		]
	];


	// play range slider
	InParentGridPanel->AddSlot(1, 2, SGridPanel::Layer(10))
	                 .Padding(ResizeBarPadding)
	[
		SNew(SBorder)
		.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
		.BorderBackgroundColor(FLinearColor(0.5f, 0.5f, 0.5f, 1.0f))
		.Clipping(EWidgetClipping::ClipToBounds)
		.Padding(0)
		[
			SNew(SActSliderViewRangeWidget)
		]
	];
}

int32 SActSliderWidget::OnPaint(const FPaintArgs& Args,
                                const FGeometry& AllottedGeometry,
                                const FSlateRect& MyCullingRect,
                                FSlateWindowElementList& OutDrawElements,
                                int32 LayerId,
                                const FWidgetStyle& InWidgetStyle,
                                bool bParentEnabled) const
{
	auto ActEventTimelineArgsDB = GetDataBindingSP(FActEventTimelineArgs, "ActEventTimelineArgs");
	if (!ActEventTimelineArgsDB)
	{
		return LayerId;
	}
	TSharedPtr<FActEventTimelineArgs> ActEventTimelineArgs = ActEventTimelineArgsDB->GetData();
	FActSliderScrubRangeToScreen RangeToScreen(*ActEventTimelineArgs->ViewRange, AllottedGeometry.Size);

	// ** draw tick marks
	FActDrawTickArgs DrawTickArgs;
	{
		DrawTickArgs.AllottedGeometry = AllottedGeometry;
		DrawTickArgs.bOnlyDrawMajorTicks = false;
		DrawTickArgs.TickColor = FLinearColor::White.CopyWithNewOpacity(0.7f);
		DrawTickArgs.CullingRect = MyCullingRect;
		DrawTickArgs.DrawEffects = bParentEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;
		// Draw major ticks under sections
		DrawTickArgs.StartLayer = LayerId;
		// Draw the tick the entire height of the section area
		DrawTickArgs.MajorTickHeight = 9.0f;
		DrawTickArgs.TickOffset = FMath::Abs(AllottedGeometry.Size.Y - DrawTickArgs.MajorTickHeight);
		DrawTickArgs.ActEventTimelineArgs = ActEventTimelineArgs;
	}
	DrawTicks(OutDrawElements, RangeToScreen, DrawTickArgs);

	// Draw the scrub handle
	const FFrameRate TickResolution = ActEventTimelineArgs->TickResolution;
	FQualifiedFrameTime ScrubPosition = FQualifiedFrameTime(*ActEventTimelineArgs->CurrentTime, TickResolution);
	FActSliderScrubberMetrics ScrubberMetrics = NovaStaticFunction::MakePixelScrubberMetrics(TickResolution, ScrubPosition, RangeToScreen);
	const float HandleStart = ScrubberMetrics.HandleRangePx.GetLowerBoundValue();
	const float HandleEnd = ScrubberMetrics.HandleRangePx.GetUpperBoundValue();
	FPaintGeometry PaintGeometry = AllottedGeometry.ToPaintGeometry(FVector2D(HandleStart, 0),
	                                                                FVector2D(HandleEnd - HandleStart, AllottedGeometry.Size.Y));
	FLinearColor ScrubColor = InWidgetStyle.GetColorAndOpacityTint();
	// TODO:Sequencer this color should be specified in the style
	ScrubColor.A *= 0.75f;
	ScrubColor.B *= 0.1f;
	ScrubColor.G *= 0.2f;
	const FSlateBrush* Brush = ScrubberMetrics.Style == EActSliderScrubberStyle::Vanilla ?
		                           FEditorStyle::GetBrush(TEXT("Sequencer.Timeline.VanillaScrubHandleDown")) :
		                           FEditorStyle::GetBrush(TEXT("Sequencer.Timeline.FrameBlockScrubHandleDown"));
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId + 2,
		PaintGeometry,
		Brush,
		DrawTickArgs.DrawEffects,
		ScrubColor
	);

	// Draw the current time next to the scrub handle
	FString FrameString = ActEventTimelineArgs->NumericTypeInterface->ToString(ActEventTimelineArgs->CurrentTime->GetFrame().Value);
	FSlateFontInfo SmallLayoutFont = FCoreStyle::GetDefaultFontStyle("Regular", 10);
	const TSharedRef<FSlateFontMeasure> SlateFontMeasureService = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	FVector2D TextSize = SlateFontMeasureService->Measure(FrameString, SmallLayoutFont);

	// Flip the text position if getting near the end of the view range
	bool bDrawLeft = (AllottedGeometry.Size.X - HandleEnd) < (TextSize.X + 12.0f);
	float TextPosition = bDrawLeft ? (HandleStart - TextSize.X - 2.0f) : (HandleEnd + 2.0f);
	FVector2D TextOffset(TextPosition, 0.0f);
	FSlateDrawElement::MakeText(
		OutDrawElements,
		LayerId + 1,
		DrawTickArgs.AllottedGeometry.ToPaintGeometry(TextOffset, TextSize),
		FrameString,
		SmallLayoutFont,
		DrawTickArgs.DrawEffects,
		FLinearColor::Yellow
	);

	if (MouseDragType == ENovaDragType::DRAG_SETTING_RANGE)
	{
		// ** 按下 Ctrl 的情况下 在 时间轴 上拖拽选择一段时间，可以缩放至这段时间的功能
		FActSliderScrubRangeToScreen MouseDownRange(*ActEventTimelineArgs->ViewRange, MouseDownGeometry.Size);
		FFrameTime DownPosition0Time = NovaStaticFunction::ComputeFrameTimeFromMouse(MouseDownGeometry,
		                                                                             MouseDownPosition[0],
		                                                                             MouseDownRange,
		                                                                             TickResolution);
		FFrameTime DownPosition1Time = NovaStaticFunction::ComputeFrameTimeFromMouse(MouseDownGeometry,
		                                                                             MouseDownPosition[1],
		                                                                             MouseDownRange,
		                                                                             TickResolution);
		float MouseStartPosX = RangeToScreen.InputToLocalX(DownPosition0Time / TickResolution);
		float MouseEndPosX = RangeToScreen.InputToLocalX(DownPosition1Time / TickResolution);
		float RangePosX = MouseStartPosX < MouseEndPosX ? MouseStartPosX : MouseEndPosX;
		float RangeSizeX = FMath::Abs(MouseStartPosX - MouseEndPosX);
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId + 1,
			AllottedGeometry.ToPaintGeometry(FVector2D(RangePosX, 0.0f), FVector2D(RangeSizeX, AllottedGeometry.Size.Y)),
			FEditorStyle::GetBrush(TEXT("Sequencer.Timeline.VanillaScrubHandleUp")),
			DrawTickArgs.DrawEffects,
			MouseStartPosX < MouseEndPosX ? FLinearColor(0.5f, 0.5f, 0.5f) : FLinearColor(0.25f, 0.3f, 0.3f)
		);
	}

	return LayerId + 2;
}


FReply SActSliderWidget::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	MouseDragType = ENovaDragType::DRAG_NONE;
	DistanceDragged = 0;
	MouseDownPosition[0] = MouseDownPosition[1] = MouseEvent.GetScreenSpacePosition();
	MouseDownGeometry = MyGeometry;

	return FReply::Handled().CaptureMouse(AsShared()).PreventThrottling();
}

FReply SActSliderWidget::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	TSharedPtr<FActEventTimelineArgs> ActEventTimelineArgs = GetActEventTimelineArgs();
	const bool bHasMouseCapture = HasMouseCapture();
	UE_LOG(LogNovaAct, Log, TEXT("bHasMouseCapture : %d"), bHasMouseCapture)
	const bool bHandleLeftMouseButton = MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bHasMouseCapture;
	const bool bHandleRightMouseButton = MouseEvent.GetEffectingButton() == EKeys::RightMouseButton && bHasMouseCapture && ActEventTimelineArgs->
		AllowZoom;
	const FActSliderScrubRangeToScreen RangeToScreen(*ActEventTimelineArgs->ViewRange, MyGeometry.Size);
	const FFrameRate TickResolution = ActEventTimelineArgs->TickResolution;
	const FFrameTime MouseTime = NovaStaticFunction::ComputeFrameTimeFromMouse(MyGeometry,
	                                                                           MouseEvent.GetScreenSpacePosition(),
	                                                                           RangeToScreen,
	                                                                           TickResolution);

	if (bHandleRightMouseButton)
	{
		if (!bPanning && DistanceDragged == 0.0f)
		{
			// return unhandled in case our parent wants to use our right mouse button to open a context menu
			return FReply::Unhandled().ReleaseMouseCapture();
		}

		bPanning = false;
		DistanceDragged = 0.f;

		return FReply::Handled().ReleaseMouseCapture();
	}
	else if (bHandleLeftMouseButton)
	{
		if (MouseDragType == ENovaDragType::DRAG_SETTING_RANGE)
		{
			// Zooming
			const FFrameTime MouseDownStart = NovaStaticFunction::ComputeFrameTimeFromMouse(
				MyGeometry,
				MouseDownPosition[0],
				RangeToScreen,
				TickResolution);

			const bool bCanZoomIn = MouseTime > MouseDownStart;
			const bool bCanZoomOut = ViewRangeZoomStack.Num() > 0;
			if (bCanZoomIn || bCanZoomOut)
			{
				if (bCanZoomIn)
				{
					// push the current value onto the stack
					ViewRangeZoomStack.Add(*ActEventTimelineArgs->ViewRange);
					ActEventTimelineArgs->SetViewRangeClamped(MouseDownStart.FrameNumber / TickResolution,
					                                          MouseTime.FrameNumber / TickResolution);
				}
				else
				{
					auto ViewRangePopped = ViewRangeZoomStack.Pop();
					ActEventTimelineArgs->SetViewRangeClamped(ViewRangePopped.GetLowerBoundValue(),
					                                          ViewRangePopped.GetUpperBoundValue());
				}
				// NovaDB::Trigger("ActEventTimelineArgs/ViewRange");
			}
		}
		else
		{
			auto DB = GetDataBinding(ENovaTransportControls, "TransportControlsState");
			DB->SetData(ENovaTransportControls::Pause);
			OnEndScrubberMovement();
			CommitScrubPosition(MouseTime, /*bIsScrubbing=*/false);
		}

		MouseDragType = ENovaDragType::DRAG_NONE;
		DistanceDragged = 0.0f;

		return FReply::Handled().ReleaseMouseCapture();
	}

	return FReply::Unhandled();
}

FReply SActSliderWidget::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	TSharedPtr<FActEventTimelineArgs> ActEventTimelineArgs = GetActEventTimelineArgs();
	bool bHandleLeftMouseButton = MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton);
	bool bHandleRightMouseButton = MouseEvent.IsMouseButtonDown(EKeys::RightMouseButton) && ActEventTimelineArgs->AllowZoom;
	const FFrameRate TickResolution = ActEventTimelineArgs->TickResolution;

	if (bHandleRightMouseButton)
	{
		if (!bPanning)
		{
			DistanceDragged += FMath::Abs(MouseEvent.GetCursorDelta().X);
			if (DistanceDragged > FSlateApplication::Get().GetDragTriggerDistance())
			{
				bPanning = true;
			}
		}
		else
		{
			FActSliderScrubRangeToScreen RangeToScreen(*ActEventTimelineArgs->ViewRange, MyGeometry.Size);
			FVector2D ScreenDelta = MouseEvent.GetCursorDelta();
			FVector2D InputDelta(ScreenDelta.X / RangeToScreen.PixelsPerInput, 0);
			double NewViewOutputMin = ActEventTimelineArgs->ViewRange->GetLowerBoundValue() - InputDelta.X;
			double NewViewOutputMax = ActEventTimelineArgs->ViewRange->GetUpperBoundValue() - InputDelta.X;
			ActEventTimelineArgs->SetViewRangeClamped(NewViewOutputMin, NewViewOutputMax);
			// NovaDB::Trigger("ActEventTimelineArgs/ViewRange");
		}
	}
	else if (bHandleLeftMouseButton)
	{
		FActSliderScrubRangeToScreen RangeToScreen(*ActEventTimelineArgs->ViewRange, MyGeometry.Size);
		DistanceDragged += FMath::Abs(MouseEvent.GetCursorDelta().X);

		if (MouseDragType == ENovaDragType::DRAG_NONE)
		{
			if (DistanceDragged > FSlateApplication::Get().GetDragTriggerDistance())
			{
				if (FSlateApplication::Get().GetModifierKeys().AreModifersDown(EModifierKey::Control))
				{
					MouseDragType = ENovaDragType::DRAG_SETTING_RANGE;
				}
				else
				{
					MouseDragType = ENovaDragType::DRAG_SCRUBBING_TIME;
					auto DB = GetDataBinding(EPlaybackMode::Type, "PreviewInstancePlaybackMode");
					DB->SetData(EPlaybackMode::Stopped);
					OnBeginScrubberMovement();
				}
			}
		}
		else
		{
			FFrameTime ScrubTime = NovaStaticFunction::ComputeFrameTimeFromMouse(MyGeometry,
			                                                                     MouseEvent.GetScreenSpacePosition(),
			                                                                     RangeToScreen,
			                                                                     TickResolution);

			if (MouseDragType == ENovaDragType::DRAG_SCRUBBING_TIME)
			{
				// Delegate responsibility for clamping to the current view range to the client
				CommitScrubPosition(ScrubTime, /*bIsScrubbing=*/true);
			}
			else if (MouseDragType == ENovaDragType::DRAG_SETTING_RANGE)
			{
				MouseDownPosition[1] = MouseEvent.GetScreenSpacePosition();
			}
		}
	}

	if (DistanceDragged != 0.0f && (bHandleLeftMouseButton || bHandleRightMouseButton))
	{
		return FReply::Handled().CaptureMouse(AsShared());
	}


	return FReply::Handled();
}

FReply SActSliderWidget::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	TSharedPtr<FActEventTimelineArgs> ActEventTimelineArgs = GetActEventTimelineArgs();


	TOptional<TRange<float>> NewTargetRange;

	if (ActEventTimelineArgs->AllowZoom && MouseEvent.IsControlDown())
	{
		const float MouseFractionX = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()).X / MyGeometry.GetLocalSize().X;

		const float ZoomDelta = -0.2f * MouseEvent.GetWheelDelta();
		if (ZoomByDelta(ZoomDelta, MouseFractionX))
		{
			return FReply::Handled();
		}
	}
	else if (MouseEvent.IsShiftDown())
	{
		PanByDelta(-MouseEvent.GetWheelDelta());
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

FVector2D SActSliderWidget::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
	return FVector2D(100, 22);
}


void SActSliderWidget::CommitScrubPosition(FFrameTime NewValue, bool bIsScrubbing)
{
	TSharedPtr<FActEventTimelineArgs> ActEventTimelineArgs = GetActEventTimelineArgs();
	// The user can scrub past the viewing range of the time slider controller, so we clamp it to the view range.
	auto ViewRange = *ActEventTimelineArgs->ViewRange;
	const FFrameRate TickResolution = ActEventTimelineArgs->TickResolution;
	const FFrameTime LowerBound = (ViewRange.GetLowerBoundValue() * TickResolution).CeilToFrame();
	const FFrameTime UpperBound = (ViewRange.GetUpperBoundValue() * TickResolution).FloorToFrame();
	NewValue = FMath::Clamp(NewValue, LowerBound, UpperBound);
	OnScrubPositionChanged(NewValue, bIsScrubbing);
}


void SActSliderWidget::SetPlaybackStatus(ENovaPlaybackType InPlaybackStatus)
{
	auto DB = GetDataBindingSP(FActEventTimelineArgs, "ActEventTimelineArgs");
	DB->GetData()->PlaybackStatus = InPlaybackStatus;
	ESequencerState State = ESS_None;
	switch (InPlaybackStatus)
	{
	case ENovaPlaybackType::Playing:
	case ENovaPlaybackType::Recording:
		{
			State = ESS_Playing;
			break;
		}
	case ENovaPlaybackType::Stopped:
	case ENovaPlaybackType::Scrubbing:
	case ENovaPlaybackType::Stepping:
		{
			State = ESS_Paused;
			break;
		}
	default: ;
	}
	for (FLevelEditorViewportClient* LevelVC : GEditor->GetLevelViewportClients())
	{
		if (LevelVC && LevelVC->AllowsCinematicControl())
		{
			LevelVC->ViewState.GetReference()->SetSequencerState(State);
		}
	}
}

void SActSliderWidget::OnBeginScrubberMovement()
{
	// Pause first since there's no explicit evaluation in the stopped state when OnEndScrubberMovement() is called
	SetPlaybackStatus(ENovaPlaybackType::Stopped);
	SetPlaybackStatus(ENovaPlaybackType::Scrubbing);
}

void SActSliderWidget::OnEndScrubberMovement()
{
	SetPlaybackStatus(ENovaPlaybackType::Stopped);
}

void SActSliderWidget::OnScrubPositionChanged(FFrameTime NewScrubPosition, bool bScrubbing)
{
	auto DB = GetDataBindingSP(FActEventTimelineArgs, "ActEventTimelineArgs");
	TSharedPtr<FActEventTimelineArgs> ActEventTimelineArgs = DB->GetData();
	if (ActEventTimelineArgs->PlaybackStatus == ENovaPlaybackType::Scrubbing && !bScrubbing)
	{
		OnEndScrubberMovement();
	}
	const TWeakPtr<SWidget> PreviousFocusedWidget = FSlateApplication::Get().GetKeyboardFocusedWidget();
	// Clear focus before setting time in case there's a key editor value selected that gets committed to a newly selected key on UserMovedFocus
	if (ActEventTimelineArgs->PlaybackStatus == ENovaPlaybackType::Stopped)
	{
		FSlateApplication::Get().ClearKeyboardFocus(EFocusCause::Cleared);
	}
	if (ActEventTimelineArgs->CurrentTime && NewScrubPosition != *ActEventTimelineArgs->CurrentTime)
	{
		*ActEventTimelineArgs->CurrentTime = NewScrubPosition;
		NovaDB::Trigger("ActEventTimelineArgs/CurrentTime");
	}
	if (PreviousFocusedWidget.IsValid())
	{
		FSlateApplication::Get().SetKeyboardFocus(PreviousFocusedWidget.Pin());
	}
}

bool SActSliderWidget::ZoomByDelta(float InDelta, float ZoomBias) const
{
	TSharedPtr<FActEventTimelineArgs> ActEventTimelineArgs = GetActEventTimelineArgs();
	auto LocalViewRange = *ActEventTimelineArgs->ViewRange;
	const double LocalViewRangeMax = LocalViewRange.GetUpperBoundValue();
	const double LocalViewRangeMin = LocalViewRange.GetLowerBoundValue();
	const double OutputViewSize = LocalViewRangeMax - LocalViewRangeMin;
	const double OutputChange = OutputViewSize * InDelta;

	const double NewViewOutputMin = LocalViewRangeMin - (OutputChange * ZoomBias);
	const double NewViewOutputMax = LocalViewRangeMax + (OutputChange * (1.f - ZoomBias));

	if (NewViewOutputMin < NewViewOutputMax)
	{
		ActEventTimelineArgs->SetViewRangeClamped(NewViewOutputMin, NewViewOutputMax);
		// NovaDB::Trigger("ActEventTimelineArgs/ViewRange");
		return true;
	}

	return false;
}

void SActSliderWidget::PanByDelta(float InDelta) const
{
	TSharedPtr<FActEventTimelineArgs> ActEventTimelineArgs = GetActEventTimelineArgs();
	auto LocalViewRange = *ActEventTimelineArgs->ViewRange;
	const double CurrentMin = LocalViewRange.GetLowerBoundValue();
	const double CurrentMax = LocalViewRange.GetUpperBoundValue();
	// Adjust the delta to be a percentage of the current range
	InDelta *= 0.1f * (CurrentMax - CurrentMin);
	const double NewViewOutputMin = CurrentMin + InDelta;
	const double NewViewOutputMax = CurrentMax + InDelta;
	ActEventTimelineArgs->SetViewRangeClamped(NewViewOutputMin, NewViewOutputMax);
	// NovaDB::Trigger("ActEventTimelineArgs/ViewRange");
}

void SActSliderWidget::DrawTicks(FSlateWindowElementList& OutDrawElements,
                                 const FActSliderScrubRangeToScreen& RangeToScreen,
                                 const FActDrawTickArgs& InArgs)
{
	if (!InArgs.ActEventTimelineArgs)
	{
		UE_LOG(LogNovaAct, Warning, TEXT("FActDrawTickArgs.ActEventTimelineArgs not assigned!"));
		return;
	}
	const FPaintGeometry PaintGeometry = InArgs.AllottedGeometry.ToPaintGeometry();
	const FSlateFontInfo SmallLayoutFont = FCoreStyle::GetDefaultFontStyle("Regular", 8);

	double MajorGridStep = 0.0;
	int32 MinorDivisions = 0;
	if (!NovaStaticFunction::GetGridMetrics(InArgs.ActEventTimelineArgs.ToSharedRef(),
	                                        InArgs.AllottedGeometry.Size.X,
	                                        MajorGridStep,
	                                        MinorDivisions))
	{
		return;
	}

	if (InArgs.bOnlyDrawMajorTicks)
	{
		MinorDivisions = 0;
	}

	TArray<FVector2D> LinePoints;
	LinePoints.SetNumUninitialized(2);

	const double FirstMajorLine = FMath::FloorToDouble(RangeToScreen.ViewStart / MajorGridStep) * MajorGridStep;
	const double LastMajorLine = FMath::CeilToDouble(RangeToScreen.ViewEnd / MajorGridStep) * MajorGridStep;
	const FFrameRate TickResolution = InArgs.ActEventTimelineArgs->TickResolution;
	const FFrameNumber FrameNumber = ConvertFrameTime(*InArgs.ActEventTimelineArgs->CurrentTime, TickResolution, TickResolution).FloorToFrame();
	const float FlooredScrubPx = RangeToScreen.InputToLocalX(FrameNumber / TickResolution);
	for (double CurrentMajorLine = FirstMajorLine; CurrentMajorLine < LastMajorLine; CurrentMajorLine += MajorGridStep)
	{
		const float MajorLinePx = RangeToScreen.InputToLocalX(CurrentMajorLine);

		LinePoints[0] = FVector2D(MajorLinePx, InArgs.TickOffset);
		LinePoints[1] = FVector2D(MajorLinePx, InArgs.TickOffset + InArgs.MajorTickHeight);

		// Draw each tick mark
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			InArgs.StartLayer,
			PaintGeometry,
			LinePoints,
			InArgs.DrawEffects,
			InArgs.TickColor,
			false
		);

		if (!InArgs.bOnlyDrawMajorTicks && !FMath::IsNearlyEqual(MajorLinePx, FlooredScrubPx, 3.f))
		{
			FString FrameString = InArgs.ActEventTimelineArgs->NumericTypeInterface ?
				                      InArgs.ActEventTimelineArgs->NumericTypeInterface->ToString(
					                      (CurrentMajorLine * TickResolution).RoundToFrame().Value) :
				                      FString();

			// Space the text between the tick mark but slightly above
			FVector2D TextOffset(MajorLinePx + 5.f, FMath::Abs(InArgs.AllottedGeometry.Size.Y - (InArgs.MajorTickHeight + 3.f)));
			FSlateDrawElement::MakeText(
				OutDrawElements,
				InArgs.StartLayer + 1,
				InArgs.AllottedGeometry.ToPaintGeometry(TextOffset, InArgs.AllottedGeometry.Size),
				FrameString,
				SmallLayoutFont,
				InArgs.DrawEffects,
				InArgs.TickColor * 0.65f
			);
		}

		for (int32 Step = 1; Step < MinorDivisions; ++Step)
		{
			// Compute the size of each tick mark.  If we are half way between to visible values display a slightly larger tick mark
			const float MinorTickHeight = ((MinorDivisions % 2 == 0) && (Step % (MinorDivisions / 2)) == 0) ? 6.0f : 2.0f;
			const float MinorLinePx = RangeToScreen.InputToLocalX(CurrentMajorLine + Step * MajorGridStep / MinorDivisions);

			LinePoints[0] = FVector2D(MinorLinePx, FMath::Abs(InArgs.AllottedGeometry.Size.Y - MinorTickHeight));
			LinePoints[1] = FVector2D(MinorLinePx, LinePoints[0].Y + MinorTickHeight);

			// Draw each sub mark
			FSlateDrawElement::MakeLines(
				OutDrawElements,
				InArgs.StartLayer,
				PaintGeometry,
				LinePoints,
				InArgs.DrawEffects,
				InArgs.TickColor,
				false
			);
		}
	}
}

TSharedRef<FActEventTimelineArgs> SActSliderWidget::GetActEventTimelineArgs() const
{
	auto ActEventTimelineArgsDB = GetDataBindingSP(FActEventTimelineArgs, "ActEventTimelineArgs");
	TSharedPtr<FActEventTimelineArgs> ActEventTimelineArgs = ActEventTimelineArgsDB->GetData();
	return ActEventTimelineArgs.ToSharedRef();
}
