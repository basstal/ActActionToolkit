#include "ActEventTimelineWidget.h"

#include "PlaySlate.h"
#include "Common/NovaConst.h"

#include "NovaAct/ActEventTimeline/Slider/ActSliderWidget.h"
#include "NovaAct/ActEventTimeline/Image/ActImageHorizontalBox.h"
#include "NovaAct/ActViewport/ActViewport.h"
#include "NovaAct/ActEventTimeline/Components/ActTransportControlsWidget.h"
#include "NovaAct/ActEventTimeline/Components/ActSplitterOverlay.h"

#include "EditorStyleSet.h"
#include "FrameNumberNumericInterface.h"
#include "Common/NovaDataBinding.h"
#include "Image/ActImageScrubPosition.h"
#include "NovaAct/ActEventTimeline/Image/ActImageThickLine.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SGridPanel.h"

#define LOCTEXT_NAMESPACE "NovaAct"

using namespace NovaConst;

SActEventTimelineWidget::~SActEventTimelineWidget()
{
	UE_LOG(LogNovaAct, Log, TEXT("SActEventTimelineWidget::~SActEventTimelineWidget"));
	// NovaDB::Delete("TreeViewFilterText");
}

void SActEventTimelineWidget::Construct(const FArguments& InArgs)
{
	TAttribute<float> FillLeftAttr = TAttribute<float>::Create(TAttribute<float>::FGetter::CreateLambda([]()
	{
		auto DB = GetDataBinding(float, "ColumnFillCoefficientsLeft");
		return DB ? DB->GetData() : 0;
	}));
	TAttribute<float> FillRightAttr = TAttribute<float>::Create(TAttribute<float>::FGetter::CreateLambda([]()
	{
		auto DB = GetDataBinding(float, "ColumnFillCoefficientsLeft");
		return DB ? (1 - DB->GetData()) : 1;
	}));

	TSharedPtr<SGridPanel> GridPanel = nullptr;

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		[
			SNew(SSplitter)
			.Orientation(Orient_Horizontal)
			+ SSplitter::Slot()
			.Value(0.9f)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					// ** 第0列，第0行
					SAssignNew(GridPanel, SGridPanel)
					.FillRow(1, 1.0f)
					.FillColumn(0, FillLeftAttr)
					.FillColumn(1, FillRightAttr)
					+ SGridPanel::Slot(0, 0)
					[
						SNew(SBorder)
						.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
						[
							SNew(SSpacer)
						]
					]

					// ** 第0列，第0行
					+ SGridPanel::Slot(0, 0, SGridPanel::Layer(10))
					[
						SNew(SBorder)
						.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
						.Padding(FMargin(0.6f, 0.3f))
						.Clipping(EWidgetClipping::ClipToBounds)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.VAlign(VAlign_Center)
							[
								SNew(SSearchBox)
								.HintText(LOCTEXT("SearchNodesHint", "Search Tracks"))
								.OnTextChanged(this, &SActEventTimelineWidget::OnTreeViewFilterChanged)
							]
						]
					]


					// ** 第0列，第1行，整个 Widget 主要的区域，中间行，显示所有的 Event 以及它们的 Track - Notify
					+ SGridPanel::Slot(0, 1, SGridPanel::Layer(10))
					.ColumnSpan(2)
					[
						SNew(SActImageHorizontalBox)
					]

					+ SGridPanel::Slot(0, 2, SGridPanel::Layer(10))
					  .VAlign(VAlign_Center)
					  .HAlign(HAlign_Center)
					[
						SNew(SActTransportControlsWidget)
					]

					// ** 第1列，第0行
					+ SGridPanel::Slot(1, 0)
					  .Padding(ResizeBarPadding)
					  .RowSpan(2)
					[
						SNew(SBorder)
						.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
						[
							SNew(SSpacer)
						]
					]

					// ** 第1列，第1行，Overlay that draws the tick lines
					+ SGridPanel::Slot(1, 1, SGridPanel::Layer(10))
					.Padding(ResizeBarPadding)
					[
						SNew(SActImageThickLine)
						.Visibility(EVisibility::HitTestInvisible)
					]

					// ** 第1列，第1行，Overlay that draws the scrub position
					+ SGridPanel::Slot(1, 1, SGridPanel::Layer(20))
					.Padding(ResizeBarPadding)
					[
						SNew(SActImageScrubPosition)
						.Visibility(EVisibility::HitTestInvisible)
					]
				]

				+ SOverlay::Slot()
				[
					// 绘制Overlay用于拖拽决定Sequence左右两块区域的占比
					SNew(SActSplitterOverlay)
					.Style(FEditorStyle::Get(), "AnimTimeline.Outliner.Splitter")
					.Visibility(EVisibility::SelfHitTestInvisible)
					+ SSplitter::Slot()
					  .Value(FillLeftAttr)
					  .OnSlotResized(SSplitter::FOnSlotResized::CreateLambda([](float NewSizeCoefficient)
					  {
						  auto DB = GetDataBinding(float, "ColumnFillCoefficientsLeft");
						  DB->SetData(NewSizeCoefficient);
					  }))
					[
						SNew(SSpacer)
					]

					+ SSplitter::Slot()
					  .Value(FillRightAttr)
					  .OnSlotResized(SSplitter::FOnSlotResized::CreateLambda([](float NewSizeCoefficient)
					  {
						  auto DB = GetDataBinding(float, "ColumnFillCoefficientsLeft");
						  DB->SetData(1 - NewSizeCoefficient);
					  }))
					[
						SNew(SSpacer)
					]
				]
			]
		]
	];


	if (GridPanel)
	{
		ActEventTimelineSliderWidget = SNew(SActSliderWidget, GridPanel.ToSharedRef());
	}
}

void SActEventTimelineWidget::OnTreeViewFilterChanged(const FText& InFilter)
{
	auto DB = GetDataBinding(FText, "TreeViewFilterText");
	if (!DB)
	{
		DB = NovaDB::Create("TreeViewFilterText", InFilter);
	}
	DB->SetData(InFilter);
}

#undef LOCTEXT_NAMESPACE
