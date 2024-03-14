#include "ActSliderViewRangeWidget.h"

#include "EditorStyleSet.h"
#include "Common/NovaDataBinding.h"
#include "NovaAct/ActEventTimeline/Slider/ActSliderViewRangeBarWidget.h"
#include "NovaAct/ActEventTimeline/Slider/ActSliderWidget.h"
#include "Widgets/Input/SSpinBox.h"
#include "FrameNumberNumericInterface.h"

#define LOCTEXT_NAMESPACE "NovaAct"

void SActSliderViewRangeWidget::Construct(const FArguments& InArgs)
{
	auto ActEventTimelineArgsDB = GetDataBindingSP(FActEventTimelineArgs, "ActEventTimelineArgs");
	TSharedPtr<FActEventTimelineArgs> ActEventTimelineArgs = ActEventTimelineArgsDB->GetData();

	ChildSlot
		.HAlign(HAlign_Fill)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			  .VAlign(VAlign_Center)
			  .AutoWidth()
			  .Padding(2.f)
			[
				SNew(SBox)
				.MinDesiredWidth(64)
				.HAlign(HAlign_Center)
				[
					SNew(SSpinBox<double>)
					.Value(this, &SActSliderViewRangeWidget::ViewBeginTime)
					.ToolTipText(LOCTEXT("ViewBeginTimeTooltip", "View Range Begin Time"))
					.OnValueCommitted(this, &SActSliderViewRangeWidget::OnViewBeginTimeCommitted)
					.OnValueChanged_Lambda([this](double InNewValue)
					                      {
						                      OnViewBeginTimeCommitted(InNewValue, ETextCommit::Default);
					                      })
					.MinValue(TOptional<double>())
					.MaxValue(TOptional<double>())
					.Style(&FEditorStyle::Get().GetWidgetStyle<FSpinBoxStyle>("Sequencer.HyperlinkSpinBox"))
					.TypeInterface(ActEventTimelineArgs->NumericTypeInterface)
					.ClearKeyboardFocusOnCommit(true)
					.Delta(this, &SActSliderViewRangeWidget::GetSpinboxDelta)
					.LinearDeltaSensitivity(25)
				]
			]

			+ SHorizontalBox::Slot()
			  .FillWidth(1.0f)
			  .Padding(2.0f, 4.0f)
			  .VAlign(VAlign_Center)
			[
				SNew(SActSliderViewRangeBarWidget)
			]

			+ SHorizontalBox::Slot()
			  .VAlign(VAlign_Center)
			  .AutoWidth()
			  .Padding(2.f)
			[
				SNew(SBox)
				.MinDesiredWidth(64)
				.HAlign(HAlign_Center)
				[
					SNew(SSpinBox<double>)
					.Value(this, &SActSliderViewRangeWidget::ViewEndTime)
					.ToolTipText(LOCTEXT("ViewEndTimeTooltip", "View Range End Time"))
					.OnValueCommitted(this, &SActSliderViewRangeWidget::OnViewEndTimeCommitted)
					.OnValueChanged_Lambda([this](double InNewValue)
					                      {
						                      OnViewEndTimeCommitted(InNewValue, ETextCommit::Default);
					                      })
					.MinValue(TOptional<double>())
					.MaxValue(TOptional<double>())
					.Style(&FEditorStyle::Get().GetWidgetStyle<FSpinBoxStyle>("Sequencer.HyperlinkSpinBox"))
					.TypeInterface(ActEventTimelineArgs->NumericTypeInterface)
					.ClearKeyboardFocusOnCommit(true)
					.Delta(this, &SActSliderViewRangeWidget::GetSpinboxDelta)
					.LinearDeltaSensitivity(25)
				]
			]
		];
}


double SActSliderViewRangeWidget::ViewBeginTime() const
{
	auto ActEventTimelineArgsDB = GetDataBindingSP(FActEventTimelineArgs, "ActEventTimelineArgs");
	TSharedPtr<FActEventTimelineArgs> ActEventTimelineArgs = ActEventTimelineArgsDB->GetData();
	const FFrameRate TickResolution = ActEventTimelineArgs->TickResolution;
	// View range is in seconds so we convert it to tick resolution
	const FFrameTime Time = ActEventTimelineArgs->ViewRange->GetLowerBoundValue() * TickResolution;
	return Time.GetFrame().Value;
}

double SActSliderViewRangeWidget::ViewEndTime() const
{
	auto ActEventTimelineArgsDB = GetDataBindingSP(FActEventTimelineArgs, "ActEventTimelineArgs");
	TSharedPtr<FActEventTimelineArgs> ActEventTimelineArgs = ActEventTimelineArgsDB->GetData();
	const FFrameRate TickResolution = ActEventTimelineArgs->TickResolution;
	// View range is in seconds so we convert it to tick resolution
	const FFrameTime Time = ActEventTimelineArgs->ViewRange->GetUpperBoundValue() * TickResolution;
	return Time.GetFrame().Value;
}


void SActSliderViewRangeWidget::OnViewBeginTimeCommitted(double InFrameValue, ETextCommit::Type InTextCommit) const
{
	auto ActEventTimelineArgsDB = GetDataBindingSP(FActEventTimelineArgs, "ActEventTimelineArgs");
	TSharedPtr<FActEventTimelineArgs> ActEventTimelineArgs = ActEventTimelineArgsDB->GetData();
	double BeginTime = ActEventTimelineArgs->TickResolution.AsInterval() * InFrameValue;
	ActEventTimelineArgs->SetViewRangeClamped(BeginTime, ActEventTimelineArgs->ViewRange->GetUpperBoundValue());
}

void SActSliderViewRangeWidget::OnViewEndTimeCommitted(double InFrameValue, ETextCommit::Type InTextCommit) const
{
	auto ActEventTimelineArgsDB = GetDataBindingSP(FActEventTimelineArgs, "ActEventTimelineArgs");
	TSharedPtr<FActEventTimelineArgs> ActEventTimelineArgs = ActEventTimelineArgsDB->GetData();
	double EndTime = ActEventTimelineArgs->TickResolution.AsInterval() * InFrameValue;
	ActEventTimelineArgs->SetViewRangeClamped(ActEventTimelineArgs->ViewRange->GetLowerBoundValue(), EndTime);
}


double SActSliderViewRangeWidget::GetSpinboxDelta() const
{
	auto ActEventTimelineArgsDB = GetDataBindingSP(FActEventTimelineArgs, "ActEventTimelineArgs");
	TSharedPtr<FActEventTimelineArgs> ActEventTimelineArgs = ActEventTimelineArgsDB->GetData();
	return ActEventTimelineArgs->TickResolution.AsDecimal() * ActEventTimelineArgs->TickResolution.AsInterval();
}
#undef LOCTEXT_NAMESPACE
