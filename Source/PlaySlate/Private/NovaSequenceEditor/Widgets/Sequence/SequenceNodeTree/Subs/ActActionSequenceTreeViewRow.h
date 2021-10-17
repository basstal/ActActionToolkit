﻿#pragma once

class FActActionSequenceTreeViewNode;
class SActActionSequenceTrackLane;

class SActActionSequenceTreeViewRow : public SMultiColumnTableRow<TSharedRef<FActActionSequenceTreeViewNode>>
{
	DECLARE_DELEGATE_RetVal_ThreeParams(TSharedRef<SWidget>, OnGenerateWidgetForColumnDelegate, const TSharedRef<FActActionSequenceTreeViewNode>&, const FName&, const TSharedRef<SActActionSequenceTreeViewRow>&);
public:
SLATE_BEGIN_ARGS(SActActionSequenceTreeViewRow)
		{
		}

		/** Delegate to invoke to create a new column for this row */
		SLATE_EVENT(OnGenerateWidgetForColumnDelegate, OnGenerateWidgetForColumn)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTableView, const TSharedRef<FActActionSequenceTreeViewNode>& InNode);
	//~Begin SMultiColumnTableRow interface
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override;
	//~End SMultiColumnTableRow interface

	void AddTrackAreaReference(const TSharedRef<SActActionSequenceTrackLane>& Lane);

	TOptional<EItemDropZone> OnCanAcceptDrop(const FDragDropEvent& DragDropEvent, EItemDropZone InItemDropZone, TSharedRef<FActActionSequenceTreeViewNode> DisplayNode);
	FReply OnAcceptDrop(const FDragDropEvent& DragDropEvent, EItemDropZone InItemDropZone, TSharedRef<FActActionSequenceTreeViewNode> DisplayNode);
	/** Gets the padding for this row based on whether it is a root node or not */
	FMargin GetRowPadding() const;


protected:
	/** The item associated with this row of data */
	TWeakPtr<FActActionSequenceTreeViewNode> Node;

	/** Delegate to call to create a new widget for a particular column. */
	OnGenerateWidgetForColumnDelegate OnGenerateWidgetForColumn;
	TWeakPtr<SActActionSequenceTrackLane> TrackLaneReference;
};
