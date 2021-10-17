﻿#include "ActActionTimeSliderWidget.h"

#include "NovaSequenceEditor/Controllers/Sequence/TimeSlider/ActActionTimeSliderController.h"

#include "Fonts/FontMeasure.h"
#include "Utils/ActActionStaticUtil.h"

void SActActionTimeSliderWidget::Construct(const FArguments& InArgs, const TSharedRef<FActActionTimeSliderController>& InTimeSliderController)
{
	TimeSliderController = InTimeSliderController;
}


int32 SActActionTimeSliderWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	ActActionSequence::FActActionTimeSliderArgs TimeSliderArgs = TimeSliderController.Pin()->GetTimeSliderArgs();
	const TRange<double> LocalViewRange = TimeSliderArgs.ViewRange.Get();
	const float LocalViewRangeMin = LocalViewRange.GetLowerBoundValue();
	const float LocalViewRangeMax = LocalViewRange.GetUpperBoundValue();
	const float LocalSequenceLength = LocalViewRangeMax - LocalViewRangeMin;
	if (LocalSequenceLength <= 0)
	{
		return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	}

	ActActionSequence::FActActionScrubRangeToScreen RangeToScreen(LocalViewRange, AllottedGeometry.Size);

	// ** draw tick marks
	const float MajorTickHeight = 9.0f;
	const ESlateDrawEffect DrawEffects = bParentEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;
	const bool bMirrorLabels = TimeSliderController.Pin()->bMirrorLabels;
	ActActionSequence::FActActionDrawTickArgs DrawTickArgs
	{
		AllottedGeometry,
		MyCullingRect,
		FLinearColor::White,
		bMirrorLabels ? 0.0f : FMath::Abs(AllottedGeometry.Size.Y - MajorTickHeight),
		MajorTickHeight,
		LayerId,
		DrawEffects,
		false,
		bMirrorLabels
	};
	DrawTicks(OutDrawElements, LocalViewRange, RangeToScreen, DrawTickArgs);

	// draw playback & selection range
	ActActionSequence::FActActionPaintPlaybackRangeArgs PaintPlaybackRangeArgs
	{
		bMirrorLabels ? FEditorStyle::GetBrush("Sequencer.Timeline.PlayRange_Bottom_L") : FEditorStyle::GetBrush("Sequencer.Timeline.PlayRange_Top_L"),
		bMirrorLabels ? FEditorStyle::GetBrush("Sequencer.Timeline.PlayRange_Bottom_R") : FEditorStyle::GetBrush("Sequencer.Timeline.PlayRange_Top_R"),
		6.0f,
		0.0f
	};

	LayerId = DrawPlaybackRange(AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, RangeToScreen, PaintPlaybackRangeArgs);
	LayerId = DrawSubSequenceRange(AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, RangeToScreen, PaintPlaybackRangeArgs);

	PaintPlaybackRangeArgs.SolidFillOpacity = 0.05f;

	// Draw the scrub handle
	FQualifiedFrameTime ScrubPosition = FQualifiedFrameTime(TimeSliderArgs.ScrubPosition.Get(), TimeSliderArgs.TickResolution.Get());
	const FFrameRate DisplayRate = TimeSliderArgs.DisplayRate.Get();
	ActActionSequence::FActActionScrubberMetrics ScrubberMetrics = ActActionSequence::ActActionStaticUtil::GetScrubPixelMetrics(DisplayRate, ScrubPosition, RangeToScreen);
	const float HandleStart = ScrubberMetrics.HandleRangePx.GetLowerBoundValue();
	const float HandleEnd = ScrubberMetrics.HandleRangePx.GetUpperBoundValue();

	FPaintGeometry PaintGeometry = AllottedGeometry.ToPaintGeometry(FVector2D(HandleStart, 0), FVector2D(HandleEnd - HandleStart, AllottedGeometry.Size.Y));
	FLinearColor ScrubColor = InWidgetStyle.GetColorAndOpacityTint();
	// TODO:Sequencer this color should be specified in the style
	ScrubColor.A *= 0.75f;
	ScrubColor.B *= 0.1f;
	ScrubColor.G *= 0.2f;

	const FSlateBrush* FrameBlockScrubHandleUpBrush = FEditorStyle::GetBrush(TEXT("Sequencer.Timeline.FrameBlockScrubHandleUp"));
	const FSlateBrush* FrameBlockScrubHandleDownBrush = FEditorStyle::GetBrush(TEXT("Sequencer.Timeline.FrameBlockScrubHandleDown"));
	const FSlateBrush* VanillaScrubHandleUpBrush = FEditorStyle::GetBrush(TEXT("Sequencer.Timeline.VanillaScrubHandleUp"));
	const FSlateBrush* VanillaScrubHandleDownBrush = FEditorStyle::GetBrush(TEXT("Sequencer.Timeline.VanillaScrubHandleDown"));
	const FSlateBrush* Brush = ScrubberMetrics.Style == ActActionSequence::ESequencerScrubberStyle::Vanilla
		                           ? (bMirrorLabels ? VanillaScrubHandleUpBrush : VanillaScrubHandleDownBrush)
		                           : (bMirrorLabels ? FrameBlockScrubHandleUpBrush : FrameBlockScrubHandleDownBrush);
	const int32 ArrowLayer = LayerId + 2;
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		ArrowLayer,
		PaintGeometry,
		Brush,
		DrawEffects,
		ScrubColor
	);

	// ** DrawMarkedFrames Removed

	// Draw the current time next to the scrub handle
	FLinearColor TextColor = FLinearColor::Yellow;
	FString FrameString = TimeSliderArgs.ScrubPositionText.Get();
	if (!TimeSliderArgs.ScrubPositionText.IsSet())
	{
		FrameString = TimeSliderArgs.NumericTypeInterface->ToString(TimeSliderArgs.ScrubPosition.Get().GetFrame().Value);
	}
	FSlateFontInfo SmallLayoutFont = FCoreStyle::GetDefaultFontStyle("Regular", 10);
	const TSharedRef<FSlateFontMeasure> SlateFontMeasureService = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	FVector2D TextSize = SlateFontMeasureService->Measure(FrameString, SmallLayoutFont);

	// Flip the text position if getting near the end of the view range
	static const float TextOffsetPx = 2.f;
	bool bDrawLeft = (AllottedGeometry.Size.X - HandleEnd) < ((TextSize.X + 14.f) - TextOffsetPx);
	float TextPosition = bDrawLeft ? (HandleStart - TextSize.X - TextOffsetPx) : (HandleEnd + TextOffsetPx);
	FVector2D TextOffset(TextPosition, bMirrorLabels ? (DrawTickArgs.AllottedGeometry.Size.Y - TextSize.Y) : 0.0f);
	FSlateDrawElement::MakeText(
		OutDrawElements,
		DrawTickArgs.StartLayer + 1,
		DrawTickArgs.AllottedGeometry.ToPaintGeometry(TextOffset, TextSize),
		FrameString,
		SmallLayoutFont,
		DrawTickArgs.DrawEffects,
		TextColor
	);

	if (TimeSliderController.IsValid() && TimeSliderController.Pin()->MouseDragType == ActActionSequence::EDragType::DRAG_SETTING_RANGE)
	{
		TSharedRef<FActActionTimeSliderController> TimeSliderControllerRef = TimeSliderController.Pin().ToSharedRef();
		FFrameRate TickResolution = TimeSliderArgs.TickResolution.Get();
		ActActionSequence::FActActionAnimatedRange AnimatedRange = TimeSliderArgs.ViewRange.Get();
		FFrameTime MouseDownTime[2];
		ActActionSequence::FActActionScrubRangeToScreen MouseDownRange(AnimatedRange, TimeSliderControllerRef->MouseDownGeometry.Size);
		MouseDownTime[0] = TimeSliderControllerRef->ComputeFrameTimeFromMouse(TimeSliderControllerRef->MouseDownGeometry, TimeSliderControllerRef->MouseDownPosition[0], MouseDownRange);
		MouseDownTime[1] = TimeSliderControllerRef->ComputeFrameTimeFromMouse(TimeSliderControllerRef->MouseDownGeometry, TimeSliderControllerRef->MouseDownPosition[1], MouseDownRange);
		float MouseStartPosX = RangeToScreen.InputToLocalX(MouseDownTime[0] / TickResolution);
		float MouseEndPosX = RangeToScreen.InputToLocalX(MouseDownTime[1] / TickResolution);
		float RangePosX = MouseStartPosX < MouseEndPosX ? MouseStartPosX : MouseEndPosX;
		float RangeSizeX = FMath::Abs(MouseStartPosX - MouseEndPosX);
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId + 1,
			AllottedGeometry.ToPaintGeometry(FVector2D(RangePosX, 0.0f), FVector2D(RangeSizeX, AllottedGeometry.Size.Y)),
			bMirrorLabels ? VanillaScrubHandleDownBrush : VanillaScrubHandleUpBrush,
			DrawEffects,
			MouseStartPosX < MouseEndPosX ? FLinearColor(0.5f, 0.5f, 0.5f) : FLinearColor(0.25f, 0.3f, 0.3f)
		);
	}

	return ArrowLayer;
}


FReply SActActionTimeSliderWidget::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	TimeSliderController.Pin()->OnMouseButtonDown(MyGeometry, MouseEvent);
	return FReply::Handled().CaptureMouse(AsShared()).PreventThrottling();
}

FReply SActActionTimeSliderWidget::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	return TimeSliderController.Pin()->OnMouseButtonUp(MyGeometry, MouseEvent);
}

FReply SActActionTimeSliderWidget::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	return TimeSliderController.Pin()->OnMouseMove(MyGeometry, MouseEvent);
}

FReply SActActionTimeSliderWidget::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	return TimeSliderController.Pin()->OnMouseWheel(MyGeometry, MouseEvent);
}

void SActActionTimeSliderWidget::DrawTicks(FSlateWindowElementList& OutDrawElements, const TRange<double>& ViewRange, const ActActionSequence::FActActionScrubRangeToScreen& RangeToScreen, ActActionSequence::FActActionDrawTickArgs& InArgs) const
{
	const ActActionSequence::FActActionTimeSliderArgs TimeSliderArgs = TimeSliderController.Pin()->GetTimeSliderArgs();
	const FFrameRate TickResolution = TimeSliderArgs.TickResolution.Get();
	const FFrameRate DisplayRate = TimeSliderArgs.DisplayRate.Get();
	const FPaintGeometry PaintGeometry = InArgs.AllottedGeometry.ToPaintGeometry();
	const FSlateFontInfo SmallLayoutFont = FCoreStyle::GetDefaultFontStyle("Regular", 8);

	double MajorGridStep = 0.0;
	int32 MinorDivisions = 0;
	if (!ActActionSequence::ActActionStaticUtil::GetGridMetrics(TimeSliderArgs.NumericTypeInterface, InArgs.AllottedGeometry.Size.X, ViewRange.GetLowerBoundValue(), ViewRange.GetUpperBoundValue(), MajorGridStep, MinorDivisions))
	{
		return;
	}

	if (InArgs.bOnlyDrawMajorTicks)
	{
		MinorDivisions = 0;
	}

	TArray<FVector2D> LinePoints;
	LinePoints.SetNumUninitialized(2);

	const double FirstMajorLine = FMath::FloorToDouble(ViewRange.GetLowerBoundValue() / MajorGridStep) * MajorGridStep;
	const double LastMajorLine = FMath::CeilToDouble(ViewRange.GetUpperBoundValue() / MajorGridStep) * MajorGridStep;
	const FFrameNumber FrameNumber = ConvertFrameTime(TimeSliderArgs.ScrubPosition.Get(), TickResolution, DisplayRate).FloorToFrame();
	const float FlooredScrubPx = RangeToScreen.InputToLocalX(FrameNumber / DisplayRate);
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
			FString FrameString = TimeSliderArgs.NumericTypeInterface->ToString((CurrentMajorLine * TickResolution).RoundToFrame().Value);

			// Space the text between the tick mark but slightly above
			FVector2D TextOffset(MajorLinePx + 5.f, InArgs.bMirrorLabels ? 1.f : FMath::Abs(InArgs.AllottedGeometry.Size.Y - (InArgs.MajorTickHeight + 3.f)));
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

			LinePoints[0] = FVector2D(MinorLinePx, InArgs.bMirrorLabels ? 0.0f : FMath::Abs(InArgs.AllottedGeometry.Size.Y - MinorTickHeight));
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

int32 SActActionTimeSliderWidget::DrawPlaybackRange(const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const ActActionSequence::FActActionScrubRangeToScreen& RangeToScreen,
                                                    const ActActionSequence::FActActionPaintPlaybackRangeArgs& Args) const
{
	const ActActionSequence::FActActionTimeSliderArgs TimeSliderArgs = TimeSliderController.Pin()->GetTimeSliderArgs();
	if (!TimeSliderArgs.PlaybackRange.IsSet())
	{
		return LayerId;
	}

	const uint8 OpacityBlend = TimeSliderArgs.SubSequenceRange.Get().IsSet() ? 128 : 255;

	const TRange<FFrameNumber> PlaybackRange = TimeSliderArgs.PlaybackRange.Get();
	const FFrameRate TickResolution = TimeSliderArgs.TickResolution.Get();
	const float PlaybackRangeL = RangeToScreen.InputToLocalX(PlaybackRange.GetLowerBoundValue() / TickResolution);
	const float PlaybackRangeR = RangeToScreen.InputToLocalX(PlaybackRange.GetUpperBoundValue() / TickResolution) - 1;

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId + 1,
		AllottedGeometry.ToPaintGeometry(FVector2D(PlaybackRangeL, 0.0f), FVector2D(Args.BrushWidth, AllottedGeometry.Size.Y)),
		Args.StartBrush,
		ESlateDrawEffect::None,
		FColor(32, 128, 32, OpacityBlend) // 120, 75, 50 (HSV)
	);

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId + 1,
		AllottedGeometry.ToPaintGeometry(FVector2D(PlaybackRangeR - Args.BrushWidth, 0.0f), FVector2D(Args.BrushWidth, AllottedGeometry.Size.Y)),
		Args.EndBrush,
		ESlateDrawEffect::None,
		FColor(128, 32, 32, OpacityBlend) // 0, 75, 50 (HSV)
	);

	// Black tint for excluded regions
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId + 1,
		AllottedGeometry.ToPaintGeometry(FVector2D(0.0f, 0.0f), FVector2D(PlaybackRangeL, AllottedGeometry.Size.Y)),
		FEditorStyle::GetBrush("WhiteBrush"),
		ESlateDrawEffect::None,
		FLinearColor::Black.CopyWithNewOpacity(0.3f * OpacityBlend / 255.f)
	);

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId + 1,
		AllottedGeometry.ToPaintGeometry(FVector2D(PlaybackRangeR, 0.0f), FVector2D(AllottedGeometry.Size.X - PlaybackRangeR, AllottedGeometry.Size.Y)),
		FEditorStyle::GetBrush("WhiteBrush"),
		ESlateDrawEffect::None,
		FLinearColor::Black.CopyWithNewOpacity(0.3f * OpacityBlend / 255.f)
	);

	return LayerId + 1;
}


int32 SActActionTimeSliderWidget::DrawSubSequenceRange(const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const ActActionSequence::FActActionScrubRangeToScreen& RangeToScreen,
                                                       const ActActionSequence::FActActionPaintPlaybackRangeArgs& Args) const
{
	const ActActionSequence::FActActionTimeSliderArgs TimeSliderArgs = TimeSliderController.Pin()->GetTimeSliderArgs();
	TOptional<TRange<FFrameNumber>> RangeValue;
	RangeValue = TimeSliderArgs.SubSequenceRange.Get(RangeValue);

	if (!RangeValue.IsSet() || RangeValue->IsEmpty())
	{
		return LayerId;
	}

	const FFrameRate Resolution = TimeSliderArgs.TickResolution.Get();
	const FFrameNumber LowerFrame = RangeValue.GetValue().GetLowerBoundValue();
	const FFrameNumber UpperFrame = RangeValue.GetValue().GetUpperBoundValue();

	const float SubSequenceRangeL = RangeToScreen.InputToLocalX(LowerFrame / Resolution) - 1;
	const float SubSequenceRangeR = RangeToScreen.InputToLocalX(UpperFrame / Resolution) + 1;

	static const FSlateBrush* LineBrushL(FEditorStyle::GetBrush("Sequencer.Timeline.PlayRange_L"));
	static const FSlateBrush* LineBrushR(FEditorStyle::GetBrush("Sequencer.Timeline.PlayRange_R"));

	const FColor GreenTint(32, 128, 32); // 120, 75, 50 (HSV)
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId + 1,
		AllottedGeometry.ToPaintGeometry(FVector2D(SubSequenceRangeL, 0.0f), FVector2D(Args.BrushWidth, AllottedGeometry.Size.Y)),
		LineBrushL,
		ESlateDrawEffect::None,
		GreenTint
	);

	const FColor RedTint(128, 32, 32); // 0, 75, 50 (HSV)
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId + 1,
		AllottedGeometry.ToPaintGeometry(FVector2D(SubSequenceRangeR - Args.BrushWidth, 0.0f), FVector2D(Args.BrushWidth, AllottedGeometry.Size.Y)),
		LineBrushR,
		ESlateDrawEffect::None,
		RedTint
	);

	// Black tint for excluded regions
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId + 1,
		AllottedGeometry.ToPaintGeometry(FVector2D(0.0f, 0.0f), FVector2D(SubSequenceRangeL, AllottedGeometry.Size.Y)),
		FEditorStyle::GetBrush("WhiteBrush"),
		ESlateDrawEffect::None,
		FLinearColor::Black.CopyWithNewOpacity(0.3f)
	);

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId + 1,
		AllottedGeometry.ToPaintGeometry(FVector2D(SubSequenceRangeR, 0.0f), FVector2D(AllottedGeometry.Size.X - SubSequenceRangeR, AllottedGeometry.Size.Y)),
		FEditorStyle::GetBrush("WhiteBrush"),
		ESlateDrawEffect::None,
		FLinearColor::Black.CopyWithNewOpacity(0.3f)
	);

	// Hash applied to the left and right of the sequence bounds
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId + 1,
		AllottedGeometry.ToPaintGeometry(FVector2D(SubSequenceRangeL - 16.f, 0.0f), FVector2D(16.f, AllottedGeometry.Size.Y)),
		FEditorStyle::GetBrush("Sequencer.Timeline.SubSequenceRangeHashL"),
		ESlateDrawEffect::None,
		GreenTint
	);

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId + 1,
		AllottedGeometry.ToPaintGeometry(FVector2D(SubSequenceRangeR, 0.0f), FVector2D(16.f, AllottedGeometry.Size.Y)),
		FEditorStyle::GetBrush("Sequencer.Timeline.SubSequenceRangeHashR"),
		ESlateDrawEffect::None,
		RedTint
	);

	return LayerId + 1;
}
