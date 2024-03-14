#include "ActImagePoolWidget.h"

#include "ActImageTreeViewTableRow.h"
#include "ImageTrackTypes/ActImageTrackBase.h"
#include "Widgets/SWeakWidget.h"

SActImagePoolWidget::Slot::Slot(const TSharedRef<SActImagePoolWidget>& InSlotContent)
{
	SlotContent = InSlotContent;

	HAlignment = HAlign_Fill;
	VAlignment = VAlign_Top;

	AttachWidget(
		SNew(SWeakWidget)
		.Clipping(EWidgetClipping::ClipToBounds)
		.PossiblyNullContent(InSlotContent)
	);
}

float SActImagePoolWidget::Slot::GetVerticalOffset() const
{
	return SlotContent.IsValid() ? SlotContent->GetPhysicalPosition() : 0.f;
}
EHorizontalAlignment SActImagePoolWidget::Slot::GetHorizontalAlignment() const
{
	return HAlignment;
}
EVerticalAlignment SActImagePoolWidget::Slot::GetVerticalAlignment() const
{
	return VAlignment;
}

void SActImagePoolWidget::Construct(const FArguments& InArgs, const TSharedRef<IActImageTrackBase>& InActImageTack)
{
	ActImageTrack = InActImageTack;
	ChildSlot
	[
		InActImageTack->GenerateContentWidgetForLaneWidget(SharedThis(this))
	];
}

float SActImagePoolWidget::GetPhysicalPosition() const
{
	if (ActImageTrack.IsValid())
	{
		typedef TMap<TSharedPtr<IActImageTrackBase>, TWeakPtr<SActImageTreeViewTableRow>> TableRowMap;
		auto DB = GetDataBindingSP(TableRowMap, "ImageTrack2TableRow");
		if (DB)
		{
			TSharedPtr<TableRowMap> ImageTrack2TableRow = DB->GetData();
			if (auto* WeakTableRowPtr = ImageTrack2TableRow->Find(ActImageTrack))
			{
				return WeakTableRowPtr->Pin()->ComputeTrackPosition();
			}
		}
	}
	return 0.0f;
}
