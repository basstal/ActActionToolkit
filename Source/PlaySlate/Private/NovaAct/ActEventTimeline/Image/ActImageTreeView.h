#pragma once


#include "Common/NovaStruct.h"
#include "Misc/TextFilterExpressionEvaluator.h"

class IActImageTrackBase;
class UActAnimation;
class SActImageTrackCarWidget;
class SActImagePoolAreaPanel;
class SActImageTreeViewTableRow;
class SActImageHorizontalBox;
class SActImagePoolWidget;

class SActImageTreeView : public STreeView<TSharedRef<SActImageTreeViewTableRow>>
{
public:
	typedef STreeView::FArguments FArguments;

	virtual ~SActImageTreeView() override;
	// ReSharper disable once CppHidingFunction
	void Construct(const FArguments& InArgs, const TSharedRef<SActImagePoolAreaPanel>& InActImageTrackAreaPanel, const TArray<TSharedRef<SActImageTreeViewTableRow>>& TreeViewTableRows);


	/**
	 * 从InParent节点中获取所有的孩子节点，过滤掉隐藏的节点
	 * @param InParent
	 * @param OutChildren
	 */
	void OnGetChildren(TSharedRef<SActImageTreeViewTableRow> InParent, TArray<TSharedRef<SActImageTreeViewTableRow>>& OutChildren) const;

	/**
	 * 当展开状态改变的回调
	 * @param InDisplayNode
	 * @param bIsExpanded
	 */
	void OnExpansionChanged(TSharedRef<SActImageTreeViewTableRow> InDisplayNode, bool bIsExpanded);
	/**
	 * 依据特定的数据生成一行 TableRow，供TreeView回调使用，同时在TrackAreaPanel也生成一个对应的TrackLaneWidget 
	 * @param InActImageTreeViewTableRow
	 * @param OwnerTable
	 * @return 
	 */
	TSharedRef<ITableRow> OnTreeViewGenerateRow(TSharedRef<SActImageTreeViewTableRow> InActImageTreeViewTableRow, const TSharedRef<STableViewBase>& OwnerTable);
	/**
	 * TreeView 过滤文本改变的绑定函数
	 *
	 * @param InFilterText 传入的过滤文本
	 */
	void OnFilterChanged(FText InFilterText);
	/**
	 * 展开所有节点
	 */
	void ExpandAllItems();

	FVector2D GetScrollDistance() override;

	FVector2D GetScrollDistanceRemaining() override;

protected:
	TSharedPtr<SActImagePoolAreaPanel> ActImageAreaPanel;
	// DataBinding Handle
	FDelegateHandle OnFilterChangedHandle;
	/** Compiled filter search terms. */
	TSharedPtr<FTextFilterExpressionEvaluator> TextFilter;
};
