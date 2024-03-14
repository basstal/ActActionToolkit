#pragma once
#include "Common/NovaEnum.h"

using namespace NovaEnum;

class SActImageTreeViewTableRow;
class IActImageTrackBase;

class SActImagePoolWidget : public SCompoundWidget
{
public:
	// ** 使用 TPanelChildren 必须有的结构
	class Slot : public TSlotBase<Slot>
	{
	public:
		Slot(const TSharedRef<SActImagePoolWidget>& InSlotContent);

		/** @return Get the vertical position of this slot inside its parent. */
		float GetVerticalOffset() const;

		/** NOTE:必须是public的因为LayoutUtils.h在用这个字段，Horizontal alignment for the slot. */
		EHorizontalAlignment HAlignment;
		/** NOTE:必须是public的因为LayoutUtils.h在用这个字段，Vertical alignment for the slot. */
		EVerticalAlignment VAlignment;

		EHorizontalAlignment GetHorizontalAlignment() const;

		EVerticalAlignment GetVerticalAlignment() const;

	protected:
		/** Slot 的具体 Widget 内容，这里是WeakPtr是因为基类有对 Widget 的管理 */
		TSharedPtr<SActImagePoolWidget> SlotContent;
	};
	SLATE_BEGIN_ARGS(SActImagePoolWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<IActImageTrackBase>& InActImageTack);

	/** Get the desired physical vertical position of this track */
	float GetPhysicalPosition() const;
protected:
	/** Track 相关数据结构 */
	TSharedPtr<IActImageTrackBase> ActImageTrack;
};
