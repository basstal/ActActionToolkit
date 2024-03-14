#include "ActSliderViewRangeBarWidget.h"

#include "EditorStyleSet.h"
#include "PlaySlate.h"
#include "Common/NovaDataBinding.h"
#include "NovaAct/ActEventTimeline/Slider/ActSliderWidget.h"


SActSliderViewRangeBarWidget::SActSliderViewRangeBarWidget()
	: bHandleDragged(false),
	  bLeftHandleDragged(false),
	  bRightHandleDragged(false),
	  bHandleHovered(false),
	  bLeftHandleHovered(false),
	  bRightHandleHovered(false) {}

SActSliderViewRangeBarWidget::~SActSliderViewRangeBarWidget()
{
	UE_LOG(LogNovaAct, Log, TEXT("SActSliderViewRangeBarWidget::~SActSliderViewRangeBarWidget"));
}

void SActSliderViewRangeBarWidget::Construct(const FArguments& InArgs)
{
	ResetState();
}

FVector2D SActSliderViewRangeBarWidget::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
	return FVector2D(56.0f, HandleSize);
}

int32 SActSliderViewRangeBarWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	static const FSlateBrush* RangeHandleLeft = FEditorStyle::GetBrush(TEXT("Sequencer.Timeline.RangeHandleLeft"));
	static const FSlateBrush* RangeHandleRight = FEditorStyle::GetBrush(TEXT("Sequencer.Timeline.RangeHandleRight"));
	static const FSlateBrush* RangeHandle = FEditorStyle::GetBrush(TEXT("Sequencer.Timeline.RangeHandle"));

	float LeftHandleOffset = 0.f;
	float HandleOffset = 0.f;
	float RightHandleOffset = 0.f;
	ComputeHandleOffsets(LeftHandleOffset, RightHandleOffset, HandleOffset, AllottedGeometry.GetLocalSize().X);

	static const FName SelectionColorName("SelectionColor");
	const FLinearColor SelectionColor = FEditorStyle::GetSlateColor(SelectionColorName).GetColor(FWidgetStyle());

	// Draw the handle box
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId,
		AllottedGeometry.ToPaintGeometry(FVector2D(HandleOffset, 0.0f), FVector2D(RightHandleOffset - LeftHandleOffset - HandleSize, HandleSize)),
		RangeHandle,
		ESlateDrawEffect::None,
		(bHandleDragged || bHandleHovered) ? SelectionColor : FLinearColor::Gray);

	// Draw the left handle box
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId,
		AllottedGeometry.ToPaintGeometry(FVector2D(LeftHandleOffset, 0.0f), FVector2D(HandleSize, HandleSize)),
		RangeHandleLeft,
		ESlateDrawEffect::None,
		(bLeftHandleDragged || bLeftHandleHovered) ? SelectionColor : FLinearColor::Gray);

	// Draw the right handle box
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId,
		AllottedGeometry.ToPaintGeometry(FVector2D(RightHandleOffset, 0.0f), FVector2D(HandleSize, HandleSize)),
		RangeHandleRight,
		ESlateDrawEffect::None,
		(bRightHandleDragged || bRightHandleHovered) ? SelectionColor : FLinearColor::Gray);

	SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, ShouldBeEnabled(bParentEnabled));

	return LayerId;
}

FReply SActSliderViewRangeBarWidget::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	MouseDownPosition = MouseEvent.GetScreenSpacePosition();
	bHandleDragged = bHandleHovered;
	bLeftHandleDragged = bLeftHandleHovered;
	bRightHandleDragged = bRightHandleHovered;
	if (bHandleDragged || bLeftHandleDragged || bRightHandleDragged)
	{
		auto ActEventTimelineArgsDB = GetDataBindingSP(FActEventTimelineArgs, "ActEventTimelineArgs");
		TSharedPtr<FActEventTimelineArgs> ActEventTimelineArgs = ActEventTimelineArgsDB->GetData();
		MouseDownViewRange = *ActEventTimelineArgs->ViewRange;
		return FReply::Handled().CaptureMouse(AsShared());
	}
	return FReply::Unhandled();
}

FReply SActSliderViewRangeBarWidget::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	ResetState();

	return FReply::Handled().ReleaseMouseCapture();
}

FReply SActSliderViewRangeBarWidget::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (HasMouseCapture())
	{
		const double DragDelta = ComputeDragDelta(MouseEvent, MyGeometry.GetLocalSize().X);
		auto ActEventTimelineArgsDB = GetDataBindingSP(FActEventTimelineArgs, "ActEventTimelineArgs");
		TSharedPtr<FActEventTimelineArgs> ActEventTimelineArgs = ActEventTimelineArgsDB->GetData();

		if (bHandleDragged)
		{
			ActEventTimelineArgs->SetViewRangeClamped(MouseDownViewRange.GetLowerBoundValue() + DragDelta,
			                                          MouseDownViewRange.GetUpperBoundValue() + DragDelta);
			// NovaDB::Trigger("ActEventTimelineArgs/ViewRange");
		}
		else if (bLeftHandleDragged || bRightHandleDragged)
		{
			double NewIn = bLeftHandleDragged ? MouseDownViewRange.GetLowerBoundValue() + DragDelta : MouseDownViewRange.GetLowerBoundValue();
			double NewOut = bRightHandleDragged ? MouseDownViewRange.GetUpperBoundValue() + DragDelta : MouseDownViewRange.GetUpperBoundValue();
			if (MouseEvent.IsShiftDown())
			{
				NewIn -= bRightHandleDragged ? DragDelta : 0;
				NewOut -= bLeftHandleDragged ? DragDelta : 0;
			}

			// In cases of extreme zoom the drag delta will be greater than the difference between In/Out.
			// This causes zooming to then become pan at extreme levels which is undesirable.
			if (NewIn >= NewOut)
			{
				return FReply::Handled();
			}
			ActEventTimelineArgs->SetViewRangeClamped(NewIn, NewOut);
			// NovaDB::Trigger("ActEventTimelineArgs/ViewRange");
		}

		return FReply::Handled();
	}
	else
	{
		ResetHoveredState();

		float LeftHandleOffset = 0.f;
		float HandleOffset = 0.f;
		float RightHandleOffset = 0.f;
		ComputeHandleOffsets(LeftHandleOffset, RightHandleOffset, HandleOffset, MyGeometry.GetLocalSize().X);

		const FGeometry LeftHandleRect = MyGeometry.MakeChild(FVector2D(LeftHandleOffset, 0.f), FVector2D(HandleSize, HandleSize));
		const FGeometry RightHandleRect = MyGeometry.MakeChild(FVector2D(RightHandleOffset, 0.f), FVector2D(HandleSize, HandleSize));
		const FGeometry HandleRect = MyGeometry.MakeChild(FVector2D(HandleOffset, 0.0f), FVector2D(RightHandleOffset - LeftHandleOffset - HandleSize, HandleSize));
		const FVector2D LocalMousePosition = MouseEvent.GetScreenSpacePosition();

		if (HandleRect.IsUnderLocation(LocalMousePosition))
		{
			bHandleHovered = true;
		}
		else if (LeftHandleRect.IsUnderLocation(LocalMousePosition))
		{
			bLeftHandleHovered = true;
		}
		else if (RightHandleRect.IsUnderLocation(LocalMousePosition))
		{
			bRightHandleHovered = true;
		}
	}
	return FReply::Unhandled();
}

void SActSliderViewRangeBarWidget::OnMouseLeave(const FPointerEvent& MouseEvent)
{
	SCompoundWidget::OnMouseLeave(MouseEvent);

	if (!HasMouseCapture())
	{
		ResetHoveredState();
	}
}

FReply SActSliderViewRangeBarWidget::OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent)
{
	ResetState();

	OnMouseMove(InMyGeometry, InMouseEvent);
	ResetState();
	return FReply::Unhandled();
}

void SActSliderViewRangeBarWidget::ComputeHandleOffsets(float& LeftHandleOffset, float& RightHandleOffset, float& HandleOffset, int32 GeometryWidth) const
{
	auto ActEventTimelineArgsDB = GetDataBindingSP(FActEventTimelineArgs, "ActEventTimelineArgs");
	TSharedPtr<FActEventTimelineArgs> ActEventTimelineArgs = ActEventTimelineArgsDB->GetData();
	float InTime = ActEventTimelineArgs->ViewRange->GetLowerBoundValue();
	float OutTime = ActEventTimelineArgs->ViewRange->GetUpperBoundValue();
	float EndTime = ActEventTimelineArgs->ClampRange.GetUpperBoundValue();
	float BeginTime = ActEventTimelineArgs->ClampRange.GetLowerBoundValue();
	if (EndTime - BeginTime <= 0)
	{
		return;
	}
	const float UnitsToPixel = (GeometryWidth - HandleSize * 2) / (EndTime - BeginTime);

	LeftHandleOffset = (InTime - BeginTime) * UnitsToPixel;
	HandleOffset = LeftHandleOffset + HandleSize;
	RightHandleOffset = HandleOffset + (OutTime - InTime) * UnitsToPixel;

	const float ScrubberWidth = RightHandleOffset - LeftHandleOffset - HandleSize;
	if (ScrubberWidth < HandleSize * 2)
	{
		HandleOffset = HandleOffset - (HandleSize * 2 - ScrubberWidth) / 2.f;
		LeftHandleOffset = HandleOffset - HandleSize;
		RightHandleOffset = HandleOffset + HandleSize * 2;
	}
}

double SActSliderViewRangeBarWidget::ComputeDragDelta(const FPointerEvent& MouseEvent, float GeometryWidth) const
{
	auto ActEventTimelineArgsDB = GetDataBindingSP(FActEventTimelineArgs, "ActEventTimelineArgs");
	TSharedPtr<FActEventTimelineArgs> ActEventTimelineArgs = ActEventTimelineArgsDB->GetData();
	float ClampSize = ActEventTimelineArgs->ClampRange.Size<double>();
	const float DragDistance = (MouseEvent.GetScreenSpacePosition() - MouseDownPosition).X;
	const double PixelToUnits = ClampSize / (GeometryWidth - HandleSize * 2);
	// UE_LOG(LogNovaAct, Log, TEXT("DragDelta : %f"), DragDelta);
	return DragDistance * PixelToUnits;
}

void SActSliderViewRangeBarWidget::ResetState()
{
	bHandleDragged = false;
	bLeftHandleDragged = false;
	bRightHandleDragged = false;
	ResetHoveredState();
}

void SActSliderViewRangeBarWidget::ResetHoveredState()
{
	bHandleHovered = false;
	bLeftHandleHovered = false;
	bRightHandleHovered = false;
}
