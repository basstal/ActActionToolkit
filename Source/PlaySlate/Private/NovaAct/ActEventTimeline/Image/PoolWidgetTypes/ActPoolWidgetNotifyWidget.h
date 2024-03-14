#pragma once
#include "StatusBarSubsystem.h"
#include "NovaAct/ActEventTimeline/Image/Subs/ActNotifyPoolNotifyNodeWidget.h"

class SActImageTrackCarWidget;
class SActNotifyPoolLaneWidget;
class SActNotifyPoolEditorLaneWidget;
class IActImageTrackBase;
class FActImageTrackNotify;


class SActPoolWidgetNotifyWidget : public SCompoundWidget, public FEditorUndoClient
{
public:
	SLATE_BEGIN_ARGS(SActPoolWidgetNotifyWidget) {}
	SLATE_END_ARGS()

	virtual ~SActPoolWidgetNotifyWidget() override;

	void Construct(const FArguments& InArgs, const TSharedRef<FActImageTrackNotify>& InActImageTrackNotify);

	//~Begin SCompoundWidget interface
	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
	//~End SCompoundWidget interface

	/** Handler for delete command */
	void OnDeletePressed();


	void DeleteSelectedNodeObjects();

	// this just refresh notify tracks - UI purpose only
	// do not call this from here. This gets called by asset. 
	void RefreshNotifyTracks();
	/**
	 * Called when a track changes it's selection; iterates all tracks collecting selected items
	 */
	void OnTrackSelectionChanged();

	/**
	 * 刷新 Lane 显示的内容
	 *
	 * @param InActImageTrack
	 */
	void OnLaneContentRefresh(TSharedPtr<IActImageTrackBase> InActImageTrack);
	/**
	 * Handler that is called when the user starts dragging a node
	 * @param InNotifyNode
	 * @param InMouseEvent
	 * @param bDragOnMarker
	 */
	FReply OnNotifyNodeDragStarted(const TSharedRef<SActNotifyPoolNotifyNodeWidget>& InNotifyNode,
	                               const FPointerEvent& InMouseEvent,
	                               const bool bDragOnMarker);

	/**
	 * 从指定的 Lane 中设置选中指定的 InNotifyNode
	 * @param InNotifyNode
	 * @param Append 是否为添加状态，否则会先取消所有已选中的 NotifyNode
	 */
	void SelectNotifyNode(const TSharedRef<SActNotifyPoolNotifyNodeWidget>& InNotifyNode,
	                      bool Append);
	/**
	 * 取消所有 Lane 的所有 NotifyNode 的选中状态
	 */
	void DeselectAllNotifies();

	/**
	 * 显示提示
	 */
	void OnNotifyNodesBeingDraggedStatusBarMessage();
	/**
	 * 显示提示
	 */
	void OnNotifyStateBeingDraggedStatusBarMessage();

	/**
	 * 开关 指定节点的选中状态
	 * @param InNotifyNode
	 */
	void ToggleNotifyNodeSelectStatus(const TSharedRef<SActNotifyPoolNotifyNodeWidget>& InNotifyNode);

	/**
	 * 判断指定节点是否被选中
	 * @param InNotifyNode
	 * @return 
	 */
	bool IsNotifyNodeSelected(const TSharedRef<SActNotifyPoolNotifyNodeWidget const>& InNotifyNode) const;
	/**
	 * 是否仅单个节点被选中
	 * @return 
	 */
	bool IsSingleNotifyNodeSelected();
protected:
	TSharedPtr<FActImageTrackNotify> ActImageTrackNotify;


	/** Cached list of Notify editor tracks */
	TArray<TSharedPtr<SActNotifyPoolEditorLaneWidget>> NotifyEditorTracks;

	/** 所有已被添加的 TrackCar Widget */
	TArray<TSharedRef<SActImageTrackCarWidget>> ActImageTrackCarWidgets;

	TSharedPtr<SBorder> PanelArea;

	/** Recursion guard for selection */
	bool bIsSelecting;
	bool bIsUpdating;

	/** Handle to status bar message */
	FStatusBarMessageHandle StatusBarMessageHandle;
	// 所有被选中的节点
	TSet<TSharedRef<SWidget const>> SelectedNotifyNodes;

	TSharedPtr<SVerticalBox> NotifySlots;
public:
	/** Cached list of anim tracks for notify node drag drop */
	TArray<TSharedPtr<SActNotifyPoolLaneWidget>> NotifyLanes;
	/** Store the position of a currently dragged node for display across tracks */
	float CurrentDragXPosition;
};
