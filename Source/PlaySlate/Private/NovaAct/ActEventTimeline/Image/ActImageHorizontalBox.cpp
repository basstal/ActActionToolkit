#include "ActImageHorizontalBox.h"

#include "Common/NovaConst.h"
#include "ImageTrackTypes/ActImageTrackFolder.h"
#include "ImageTrackTypes/ActImageTrackNotify.h"

#include "NovaAct/ActEventTimeline/Image/ActImageTreeView.h"
#include "NovaAct/ActEventTimeline/Image/ActImageTreeViewTableRow.h"
#include "NovaAct/ActEventTimeline/Image/ActImagePoolAreaPanel.h"

#include "Widgets/Layout/SScrollBorder.h"

#define LOCTEXT_NAMESPACE "NovaAct"

SActImageHorizontalBox::~SActImageHorizontalBox()
{
	// UE_LOG(LogNovaAct, Log, TEXT("SActImageHorizontalBox::~SActImageHorizontalBox"));
	// NovaDB::Delete("ImageTrack2LaneWidget");
	// NovaDB::Delete("ImageTrack2TableRow");
}

void SActImageHorizontalBox::Construct(const FArguments& InArgs)
{
	// ImageTack 到 LaneWidget 的映射关系
	typedef TMap<TSharedPtr<IActImageTrackBase>, TWeakPtr<SActImagePoolWidget>> PoolWidgetMap;
	TSharedPtr<PoolWidgetMap> ImageTrack2LaneWidget = MakeShareable(new PoolWidgetMap());
	NovaDB::CreateSP("ImageTrack2LaneWidget", ImageTrack2LaneWidget);
	// ImageTrack 到 TableRow 的映射关系
	typedef TMap<TSharedPtr<IActImageTrackBase>, TWeakPtr<SActImageTreeViewTableRow>> TableRowMap;
	TSharedPtr<TableRowMap> ImageTrack2TableRow = MakeShareable(new TableRowMap());
	NovaDB::CreateSP("ImageTrack2TableRow", ImageTrack2TableRow);

	const TSharedRef<SScrollBar> ScrollBar = SNew(SScrollBar)
		.Thickness(FVector2D(9.0f, 9.0f));
	const TSharedRef<SScrollBar> PinnedAreaScrollBar = SNew(SScrollBar)
		.Thickness(FVector2D(9.0f, 9.0f));

	ActImageTrackAreaPanel = SNew(SActImagePoolAreaPanel);
	ActImageTreeView = SNew(SActImageTreeView, ActImageTrackAreaPanel.ToSharedRef(), TreeViewItemSource)
		.ExternalScrollbar(ScrollBar);
	// .TreeItemsSource(&TreeViewItemSource);
	// ** 初始化 TreeView 的 ItemSource 添加 NotifyFolder 和 Notifies
	TSharedRef<FActImageTrackFolder> ActImageTrackFolder = MakeShareable(new FActImageTrackFolder());
	TSharedRef<SActImageTreeViewTableRow> NotifyFolder = SNew(SActImageTreeViewTableRow,
	                                                          ActImageTreeView.ToSharedRef(),
	                                                          ActImageTrackFolder);
	TreeViewItemSource.Add(NotifyFolder);
	ImageTrack2TableRow->Add(ActImageTrackFolder, NotifyFolder);
	TSharedRef<FActImageTrackNotify> ActImageTrackNotify = MakeShareable(new FActImageTrackNotify());
	TSharedRef<SActImageTreeViewTableRow> Notifies = SNew(SActImageTreeViewTableRow,
	                                                      ActImageTreeView.ToSharedRef(),
	                                                      ActImageTrackNotify);
	Notifies->SetParent(NotifyFolder);
	ImageTrack2TableRow->Add(ActImageTrackNotify, Notifies);
	ActImageTreeView->ExpandAllItems();

	PinnedActImageTrackAreaPanel = SNew(SActImagePoolAreaPanel);
	PinnedActImageTreeView = SNew(SActImageTreeView, PinnedActImageTrackAreaPanel.ToSharedRef(), TreeViewItemSource)
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
