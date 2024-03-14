#include "ActImageTreeView.h"

#include "ActImagePoolWidget.h"
#include "PlaySlate.h"
#include "NovaAct/ActEventTimeline/Image/ActImageTreeViewTableRow.h"
#include "NovaAct/ActEventTimeline/Image/ActImagePoolAreaPanel.h"

#include "Common/NovaDataBinding.h"
#include "ImageTrackTypes/ActImageTrackBase.h"
#include "Misc/TextFilterExpressionEvaluator.h"

SActImageTreeView::~SActImageTreeView()
{
	// NovaDB::Delete("ActImageTrack/Refresh");
}

void SActImageTreeView::Construct(const FArguments& InArgs, const TSharedRef<SActImagePoolAreaPanel>& InActImageTrackAreaPanel, const TArray<TSharedRef<SActImageTreeViewTableRow>>& TreeViewTableRows)
{
	NovaDB::CreateSP<IActImageTrackBase>("ActImageTrack/Refresh", nullptr);
	// ** TODO:应该把这个放到外面去
	ActImageAreaPanel = InActImageTrackAreaPanel;

	TextFilter = MakeShareable(new FTextFilterExpressionEvaluator(ETextFilterExpressionEvaluatorMode::BasicString));

	HeaderRow = SNew(SHeaderRow)
		.Visibility(EVisibility::Collapsed)
		+ SHeaderRow::Column(FName("Outliner"))
		.FillWidth(1.0f);
	FArguments TreeViewArgs;
	{
		// ** TODO:在新版本 ue 5.3 中这里可能导致问题
		// TreeViewArgs._TreeItemsSource = InArgs._TreeItemsSource;
		TreeViewArgs._SelectionMode = ESelectionMode::None;
		TreeViewArgs._HeaderRow = HeaderRow;
		TreeViewArgs._HighlightParentNodesForSelection = true;
		TreeViewArgs._AllowInvisibleItemSelection = true;
		TreeViewArgs._Clipping = EWidgetClipping::ClipToBounds;
		TreeViewArgs._OnGetChildren.BindRaw(this, &SActImageTreeView::OnGetChildren);
		TreeViewArgs._OnExpansionChanged.BindRaw(this, &SActImageTreeView::OnExpansionChanged);
		TreeViewArgs._ExternalScrollbar = InArgs._ExternalScrollbar;
		TreeViewArgs._OnGenerateRow.BindRaw(this, &SActImageTreeView::OnTreeViewGenerateRow);
		TreeViewArgs.TreeItemsSource(&TreeViewTableRows);
	}
	STreeView::Construct(TreeViewArgs);

	if (!NovaDB::Get("TreeViewFilterText"))
	{
		NovaDB::Create("TreeViewFilterText", FText::GetEmpty());
	}
	DataBindingBindRaw(FText, "TreeViewFilterText", this, &SActImageTreeView::OnFilterChanged, OnFilterChangedHandle);
}

void SActImageTreeView::OnGetChildren(TSharedRef<SActImageTreeViewTableRow> InParent,
                                      TArray<TSharedRef<SActImageTreeViewTableRow>>& OutChildren) const
{
	if (!TextFilter->GetFilterText().IsEmpty())
	{
		for (const TSharedRef<SActImageTreeViewTableRow>& Child : InParent->GetChildNodes())
		{
			TSharedPtr<FActImageTrackArgs> ActImageTrackArgs = Child->ActImageTrack->ActImageTrackArgs;
			if (!ActImageTrackArgs->bSupportFiltering ||
				TextFilter->TestTextFilter(FBasicStringFilterExpressionContext(ActImageTrackArgs->DisplayName.ToString())))
			{
				OutChildren.Add(Child);
			}
		}
	}
	else
	{
		OutChildren.Append(InParent->GetChildNodes());
	}
}

void SActImageTreeView::OnExpansionChanged(TSharedRef<SActImageTreeViewTableRow> InDisplayNode, bool bIsExpanded)
{
	UE_LOG(LogNovaAct, Log, TEXT("InDisplayNode->GetPathName : %s, bIsExpanded : %d"), *InDisplayNode->GetPathName(), bIsExpanded);
	typedef TMap<TSharedPtr<IActImageTrackBase>, TWeakPtr<SActImagePoolWidget>> PoolWidgetMap;
	auto DB = GetDataBindingSP(PoolWidgetMap, "ImageTrack2LaneWidget");
	if (!DB)
	{
		return;
	}

	if (TSharedPtr<PoolWidgetMap> ImageTrack2LaneWidget = DB->GetData())
	{
		for (const TSharedRef<SActImageTreeViewTableRow>& ChildNode : InDisplayNode->GetChildNodes())
		{
			TSharedPtr<IActImageTrackBase> ActImageTrack = ChildNode->ActImageTrack;
			TWeakPtr<SActImagePoolWidget>* WeakTrackPanelPtr = ImageTrack2LaneWidget->Find(ActImageTrack);
			if (WeakTrackPanelPtr && WeakTrackPanelPtr->IsValid())
			{
				ImageTrack2LaneWidget->FindChecked(ActImageTrack).Pin()->SetVisibility(bIsExpanded ? EVisibility::Visible : EVisibility::Collapsed);
			}
		}
	}
}

TSharedRef<ITableRow> SActImageTreeView::OnTreeViewGenerateRow(TSharedRef<SActImageTreeViewTableRow> InActImageTreeViewTableRow,
                                                               const TSharedRef<STableViewBase>& OwnerTable)
{
	typedef TMap<TSharedPtr<IActImageTrackBase>, TWeakPtr<SActImagePoolWidget>> PoolWidgetMap;
	auto DB = GetDataBindingSP(PoolWidgetMap, "ImageTrack2LaneWidget");
	if (!DB)
	{
		UE_LOG(LogNovaAct, Error, TEXT("DataBindingSP for type TMap<TSharedPtr<IActImageTrackBase>, TWeakPtr<SActImagePoolWidget>> don't exist"));
		return InActImageTreeViewTableRow;
	}
	// Ensure the track area is kept up to date with the virtualized scroll of the tree view
	TSharedPtr<IActImageTrackBase> ActImageTrack = InActImageTreeViewTableRow->ActImageTrack;
	TSharedPtr<PoolWidgetMap> ImageTrack2LaneWidget = DB->GetData();
	TSharedPtr<SActImagePoolWidget> LaneWidget = ImageTrack2LaneWidget->FindRef(ActImageTrack).Pin();

	if (!LaneWidget && ActImageTrack)
	{
		// Add a track slot for the row
		LaneWidget = ActImageAreaPanel->MakeLaneWidgetForTrack(ActImageTrack.ToSharedRef());
		ImageTrack2LaneWidget->Add(ActImageTrack, LaneWidget);
	}

	return InActImageTreeViewTableRow;
}


void SActImageTreeView::OnFilterChanged(FText InFilterText)
{
	if (InFilterText.IsEmpty())
	{
		return;
	}
	TextFilter->SetFilterText(InFilterText);
	RequestTreeRefresh();
}

void SActImageTreeView::ExpandAllItems()
{
	if (!HasValidRootItemsSource())
	{
		return;
	}
	// expand all
	for (const TSharedRef<SActImageTreeViewTableRow>& TableRow : GetRootItems())
	{
		SetItemExpansion(TableRow, true);
	}
}

FVector2D SActImageTreeView::GetScrollDistance()
{
	if (ScrollBar)
	{
		return FVector2D(0, ScrollBar->DistanceFromTop());
	}
	return FVector2D(0, 0);
}
FVector2D SActImageTreeView::GetScrollDistanceRemaining()
{
	if (ScrollBar)
	{
		return FVector2D(0, ScrollBar->DistanceFromBottom());
	}
	return FVector2D(0, 0);
}