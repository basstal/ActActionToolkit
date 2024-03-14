#pragma once
#include "SCurveEditor.h"
#include "Common/NovaStruct.h"

using namespace NovaStruct;

class SActNotifyPoolNotifyNodeWidget;

class SActNotifyPoolLaneWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SActNotifyPoolLaneWidget)
			: _LaneIndex(INDEX_NONE) {}

		SLATE_ARGUMENT(int32, LaneIndex)
	SLATE_END_ARGS()
	/** Type used for list widget of tracks */
	void Construct(const FArguments& InArgs);

	//~Begin SCompoundWidget interface
	virtual int32 OnPaint(const FPaintArgs& Args,
	                      const FGeometry& AllottedGeometry,
	                      const FSlateRect& MyCullingRect,
	                      FSlateWindowElementList& OutDrawElements,
	                      int32 LayerId,
	                      const FWidgetStyle& InWidgetStyle,
	                      bool bParentEnabled) const override;
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
	//~End SCompoundWidget interface

	void Update();
	FMargin GetNotifyTrackPadding(int32 NotifyIndex) const;
	FText GetNodeTooltip();
	FName GetName();
	FLinearColor GetEditorColor();

	/**
	 * 从 NodeSlots 中删除已选中的 NotifyNode，并且将它们添加到 DragNodes中
	 * @param DragNodes 被删除的 NotifyNode
	 */
	void DisconnectSelectedNodesForDrag(TArray<TSharedPtr<SActNotifyPoolNotifyNodeWidget>>& DragNodes);

	/**
	 * 获得与当前 CursorPosition 相交的 NotifyNode
	 *
	 * @param CursorPosition 传入的指针位置
	 * @return 
	 */
	TSharedPtr<SActNotifyPoolNotifyNodeWidget> GetHitNotifyNode(const FVector2D& CursorPosition);
	/**
	 * 构造一个右键菜单
	 * @param MouseEvent
	 * @param NotifyNode
	 */
	TSharedPtr<SWidget> SummonContextMenu(const FPointerEvent& MouseEvent,
	                                      const TSharedPtr<SActNotifyPoolNotifyNodeWidget> NotifyNode);
	/**
	 * 绑定到菜单按钮上的回调函数
	 * @param MenuBuilder
	 * @param bIsReplaceWithMenu
	 */
	void FillNewNotifyStateMenu(FMenuBuilder& MenuBuilder, bool bIsReplaceWithMenu);

	/**
	 * 绑定到菜单按钮上的回调函数
	 * @param MenuBuilder
	 * @param bIsReplaceWithMenu
	 */
	void FillNewNotifyMenu(FMenuBuilder& MenuBuilder, bool bIsReplaceWithMenu);

	/**
	 * 绑定到按钮上的回调函数
	 */
	void OnNewNotifyClicked();
	/**
	 * 绑定到按钮上的回调函数
	 * @param NewNotifyName
	 * @param CommitInfo
	 */
	void AddNewNotify(const FText& NewNotifyName, ETextCommit::Type CommitInfo);
	/**
	 * 构造 AnimNotifyEvent 数据绑定回调
	 * @param InActCreateNewNotify 
	 */
	void OnCreateNewNotify(TSharedPtr<FActCreateNewNotify> InActCreateNewNotify);
	/**
	 * 添加一个 AnimNotifyEvent 到 AnimSequence
	 * @param NewNotifyName
	 * @param NotifyClass
	 * @param StartTime
	 * @param OutAnimNotifyEvent
	 * @return 
	 */
	bool CreateNewNotify(FString NewNotifyName, UClass* NotifyClass, float StartTime, FAnimNotifyEvent& OutAnimNotifyEvent);
protected:
	TSharedPtr<SBorder> TrackBorder;
	/** Cache the SOverlay used to store all this tracks nodes */
	TSharedPtr<SOverlay> NodeSlots;
	// 用来记录右键打开菜单的时间位置，以便赋值给新建的 AnimNotifyEvent
	float LastClickedTime;

	FDelegateHandle OnCreateNewNotifyBinding;
public:
	int32 LaneIndex;
	// 缓存的 TrackScale 结构
	TSharedPtr<FTrackScaleInfo> CachedScaleInfo;
	// 缓存的 带缩放的 多边形大小
	FVector2D CachedAllottedGeometrySizeScaled;
	// 在这个 Lane 上的所有 NotifyNode
	TArray<TSharedPtr<SActNotifyPoolNotifyNodeWidget>> NotifyNodes;
};

struct FActDragDropLaneClampInfo
{
	int32 LanePos;
	int32 LaneSnapTestPos;
	TSharedPtr<SActNotifyPoolLaneWidget> Lane;
};
