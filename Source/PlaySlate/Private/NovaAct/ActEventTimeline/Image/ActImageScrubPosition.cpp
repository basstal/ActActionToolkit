#include "ActImageScrubPosition.h"

#include "Common/NovaDataBinding.h"
#include "Common/NovaStaticFunction.h"

void SActImageScrubPosition::Construct(const FArguments& InArgs)
{
	Clipping = EWidgetClipping::ClipToBounds;
	// Visibility = EVisibility::HitTestInvisible;
}

int32 SActImageScrubPosition::OnPaint(const FPaintArgs& Args,
                                      const FGeometry& AllottedGeometry,
                                      const FSlateRect& MyCullingRect,
                                      FSlateWindowElementList& OutDrawElements,
                                      int32 LayerId,
                                      const FWidgetStyle& InWidgetStyle,
                                      bool bParentEnabled) const
{
	auto ActEventTimelineArgsDB = GetDataBindingSP(FActEventTimelineArgs, "ActEventTimelineArgs");
	TSharedPtr<FActEventTimelineArgs> ActEventTimelineArgs = ActEventTimelineArgsDB->GetData();
	auto LocalViewRange = *ActEventTimelineArgs->ViewRange;
	const FActSliderScrubRangeToScreen RangeToScreen(LocalViewRange, AllottedGeometry.Size);

	FQualifiedFrameTime ScrubPosition = FQualifiedFrameTime(FFrameTime(ActEventTimelineArgs->CurrentTime->GetFrame()), ActEventTimelineArgs->TickResolution);
	const FFrameRate TickResolution = ActEventTimelineArgs->TickResolution;
	FActSliderScrubberMetrics ScrubMetrics = NovaStaticFunction::MakePixelScrubberMetrics(TickResolution, ScrubPosition, RangeToScreen);

	const ESlateDrawEffect DrawEffects = ShouldBeEnabled(bParentEnabled) ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;
	if (ScrubMetrics.bDrawExtents)
	{
		// Draw a box for the scrub position
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId + 1,
			AllottedGeometry.ToPaintGeometry(FVector2D(ScrubMetrics.FrameExtentsPx.GetLowerBoundValue(), 0.0f),
			                                 FVector2D(ScrubMetrics.FrameExtentsPx.Size<float>(), AllottedGeometry.Size.Y)),
			FEditorStyle::GetBrush(TEXT("Sequencer.Timeline.ScrubFill")),
			DrawEffects,
			FLinearColor::White.CopyWithNewOpacity(0.5f)
		);
	}

	// Draw a line for the scrub position
	TArray<FVector2D> LinePoints;
	float LinePos = RangeToScreen.InputToLocalX(ScrubPosition.AsSeconds());
	LinePoints.AddUninitialized(2);
	LinePoints[0] = FVector2D(LinePos, 0.0f);
	LinePoints[1] = FVector2D(LinePos, FMath::FloorToFloat(AllottedGeometry.Size.Y));

	FSlateDrawElement::MakeLines(
		OutDrawElements,
		LayerId + 1,
		AllottedGeometry.ToPaintGeometry(),
		LinePoints,
		DrawEffects,
		FLinearColor(1.f, 1.f, 1.f, .5f),
		false
	);

	return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
}
