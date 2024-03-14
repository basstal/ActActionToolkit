#pragma once

#include "Common/NovaStruct.h"

using namespace NovaDelegate;
using namespace NovaStruct;

class SActSliderWidget;

class SActImageScrubPosition : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SActImageScrubPosition) { }
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	//~Begin SWidget interface
	virtual int32 OnPaint(const FPaintArgs& Args,
	                      const FGeometry& AllottedGeometry,
	                      const FSlateRect& MyCullingRect,
	                      FSlateWindowElementList& OutDrawElements,
	                      int32 LayerId,
	                      const FWidgetStyle& InWidgetStyle,
	                      bool bParentEnabled) const override;
	
	//~End SWidget interface
};
