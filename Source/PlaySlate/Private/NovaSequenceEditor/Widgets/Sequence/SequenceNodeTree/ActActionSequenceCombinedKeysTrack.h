﻿#pragma once

class FActActionSequenceTreeViewNode;

class SActActionSequenceCombinedKeysTrack : public SLeafWidget
{
public:
SLATE_BEGIN_ARGS(SActActionSequenceCombinedKeysTrack)
		{
		}

		/** The view range of the section area */
		SLATE_ATTRIBUTE(TRange<double>, ViewRange)
		/** The tick resolution of the current sequence*/
		SLATE_ATTRIBUTE(FFrameRate, TickResolution)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedRef<FActActionSequenceTreeViewNode> InRootNode);
protected:
	//~Begin SWidget interface
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float) const override;
	//~End SWidget interface
protected:
	TSharedPtr<FActActionSequenceTreeViewNode> RootNode;
	TAttribute<TRange<double>> ViewRange;
	TAttribute<FFrameRate> TickResolution;
	/** Cached pixel positions for all keys in the current view range */
	TArray<float> KeyDrawPositions;
};
