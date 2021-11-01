﻿#include "ActImageHorizontalBox.h"

#include "Common/NovaConst.h"

#include "NovaAct/ActEventTimeline/Image/ActImageTreeView.h"
#include "NovaAct/ActEventTimeline/Image/ActImageTreeViewTableRow.h"
#include "NovaAct/ActEventTimeline/Image/ActImageTrackAreaPanel.h"

#include "Widgets/Layout/SScrollBorder.h"

#define LOCTEXT_NAMESPACE "NovaAct"

SActImageHorizontalBox::~SActImageHorizontalBox()
{
	UE_LOG(LogNovaAct, Log, TEXT("SActImageHorizontalBox::~SActImageHorizontalBox"));
}

void SActImageHorizontalBox::Construct(const FArguments& InArgs)
{
	const TSharedRef<SScrollBar> ScrollBar = SNew(SScrollBar)
		.Thickness(FVector2D(9.0f, 9.0f));
	const TSharedRef<SScrollBar> PinnedAreaScrollBar = SNew(SScrollBar)
		.Thickness(FVector2D(9.0f, 9.0f));
	ActImageTrackAreaPanel = SNew(SActImageTrackAreaPanel);
	ActImageTreeView = SNew(SActImageTreeView, ActImageTrackAreaPanel.ToSharedRef())
		.ExternalScrollbar(ScrollBar);
	PinnedActImageTrackAreaPanel = SNew(SActImageTrackAreaPanel);
	PinnedActImageTreeView = SNew(SActImageTreeView, PinnedActImageTrackAreaPanel.ToSharedRef())
		.ExternalScrollbar(PinnedAreaScrollBar);


	auto FillLeftAttr = TAttribute<float>::Create(TAttribute<float>::FGetter::CreateLambda([]()
	{
		auto DB = GetDataBinding(float, "ColumnFillCoefficientsLeft");
		return DB ? DB->GetData() : 0;
	}));
	auto FillRightAttr = TAttribute<float>::Create(TAttribute<float>::FGetter::CreateLambda([]()
	{
		auto DB = GetDataBinding(float, "ColumnFillCoefficientsLeft");
		return DB ? (1 - DB->GetData()) : 1;
	}));

	AddSlot()
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SScrollBorder, ActImageTreeView.ToSharedRef())
			[
				SNew(SHorizontalBox)

				// outliner tree
				+ SHorizontalBox::Slot()
				.FillWidth(FillLeftAttr)
				[
					SNew(SBox)
					[
						ActImageTreeView.ToSharedRef()
					]
				]

				// track area
				+ SHorizontalBox::Slot()
				.FillWidth(FillRightAttr)
				[
					SNew(SBox)
				.Padding(NovaConst::ResizeBarPadding)
				.Clipping(EWidgetClipping::ClipToBounds)
					[
						ActImageTrackAreaPanel.ToSharedRef()
					]
				]
			]
		]

		+ SOverlay::Slot()
		.HAlign(HAlign_Right)
		[
			ScrollBar
		]
	];
}

#undef LOCTEXT_NAMESPACE
