#pragma once

#include "ClassViewerModule.h"
#include "EditorStyleSet.h"
#include "FrameNumberNumericInterface.h"
#include "NovaDataBinding.h"
#include "SCurveEditor.h"
#include "Common/NovaStruct.h"
#include "Fonts/FontMeasure.h"

#define LOCTEXT_NAMESPACE "NovaAct"

using namespace NovaStruct;

class NovaStaticFunction
{
public:
	static float GetBoundsZOffset(const FBoxSphereBounds& Bounds)
	{
		// Return half the height of the bounds plus one to avoid ZFighting with the floor plane
		return Bounds.BoxExtent.Z + 1;
	}

	static FFrameNumber DiscreteExclusiveUpper(const TRangeBound<FFrameNumber>& InUpperBound)
	{
		check(!InUpperBound.IsOpen());

		// Add one for inclusive upper bounds since they finish on the next subsequent frame
		static const int32 Offsets[] = {0, 1};
		const int32 OffsetIndex = (int32)InUpperBound.IsInclusive();

		return InUpperBound.GetValue() + Offsets[OffsetIndex];
	}

	static FFrameNumber DiscreteInclusiveLower(const TRangeBound<FFrameNumber>& InLowerBound)
	{
		check(!InLowerBound.IsOpen());

		// Add one for exclusive lower bounds since they start on the next subsequent frame
		static const int32 Offsets[] = {0, 1};
		const int32 OffsetIndex = (int32)InLowerBound.IsExclusive();

		return InLowerBound.GetValue() + Offsets[OffsetIndex];
	}

	/**
	 * 根据鼠标位置计算当前 时间轴刷 对应的动画时间
	 *
	 * @param Geometry 时间轴刷的 几何体
	 * @param ScreenSpacePosition 鼠标对应的 屏幕坐标 位置
	 * @param RangeToScreen 屏幕距离到时间的转换结构
	 * @param TickResolution 时间到帧时间的转换结构
	 */
	static FFrameTime ComputeFrameTimeFromMouse(const FGeometry& Geometry,
	                                            FVector2D ScreenSpacePosition,
	                                            FActSliderScrubRangeToScreen RangeToScreen,
	                                            const FFrameRate& TickResolution)
	{
		const FVector2D CursorPos = Geometry.AbsoluteToLocal(ScreenSpacePosition);
		const double MouseValue = RangeToScreen.LocalXToInput(CursorPos.X);
		return MouseValue * TickResolution;
	}

	/**
	 * @return Get the pixel metrics of the Scrubber
	 * @param DisplayRate		当前帧率
	 * @param ScrubTime			The qualified time of the scrubber
	 * @param RangeToScreen		Range to screen helper
	 */
	static FActSliderScrubberMetrics MakePixelScrubberMetrics(const FFrameRate& DisplayRate,
	                                                          const FQualifiedFrameTime& ScrubTime,
	                                                          const FActSliderScrubRangeToScreen& RangeToScreen)
	{
		FActSliderScrubberMetrics Metrics;

		const FFrameNumber Frame = ScrubTime.ConvertTo(DisplayRate).FloorToFrame();

		float FrameStartPixel = RangeToScreen.InputToLocalX(Frame / DisplayRate);
		float FrameEndPixel = RangeToScreen.InputToLocalX((Frame + 1) / DisplayRate) - 1;

		float RoundedStartPixel = FMath::RoundToInt(FrameStartPixel);
		FrameEndPixel -= (FrameStartPixel - RoundedStartPixel);

		FrameStartPixel = RoundedStartPixel;
		FrameEndPixel = FMath::Max(FrameEndPixel, FrameStartPixel + 1);

		// Store off the pixel width of the frame
		Metrics.FrameExtentsPx = TRange<float>(FrameStartPixel, FrameEndPixel);

		// Set the style of the scrub handle
		Metrics.Style = EActSliderScrubberStyle::Vanilla;

		// Always draw the extents on the section area for frame block styles
		Metrics.bDrawExtents = Metrics.Style == EActSliderScrubberStyle::FrameBlock;

		static float MinScrubSize = 14.f;
		// If it's vanilla style or too small to show the frame width, set that up
		if (Metrics.Style == EActSliderScrubberStyle::Vanilla || FrameEndPixel - FrameStartPixel < MinScrubSize)
		{
			Metrics.Style = EActSliderScrubberStyle::Vanilla;

			// ** 定位到帧
			float ScrubPixel = RangeToScreen.InputToLocalX(ScrubTime.Time.FloorToFrame().Value * ScrubTime.Rate.AsInterval());
			Metrics.HandleRangePx = TRange<float>(ScrubPixel - MinScrubSize * .5f, ScrubPixel + MinScrubSize * .5f);
		}
		else
		{
			Metrics.HandleRangePx = Metrics.FrameExtentsPx;
		}

		return Metrics;
	}

	static bool GetGridMetrics(const TSharedRef<FActEventTimelineArgs>& InActEventTimelineArgs,
	                           const float PhysicalWidth,
	                           double& OutMajorInterval,
	                           int32& OutMinorDivisions)
	{
		FSlateFontInfo SmallLayoutFont = FCoreStyle::GetDefaultFontStyle("Regular", 8);
		TSharedRef<FSlateFontMeasure> FontMeasureService = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();

		// ** Use the end of the view as the longest number
		FFrameRate FocusedDisplayRate = InActEventTimelineArgs->TickResolution;
		TSharedPtr<FFrameNumberInterface> NumericTypeInterface = InActEventTimelineArgs->NumericTypeInterface;
		double InViewEnd = InActEventTimelineArgs->ViewRange->GetUpperBoundValue();
		FString TickString = NumericTypeInterface ? NumericTypeInterface->ToString((InViewEnd * FocusedDisplayRate).FrameNumber.Value) : FString();
		FVector2D MaxTextSize = FontMeasureService->Measure(TickString, SmallLayoutFont);

		float MinTickPx = MaxTextSize.X + 5.f;
		float DesiredMajorTickPx = MaxTextSize.X * 2.0f;

		if (PhysicalWidth > 0)
		{
			return ComputeGridSpacing(
				FocusedDisplayRate,
				PhysicalWidth / (InActEventTimelineArgs->ViewRange.Get()->Size<double>()),
				OutMajorInterval,
				OutMinorDivisions,
				MinTickPx,
				DesiredMajorTickPx);
		}

		return false;
	}

	// FFrameRate::ComputeGridSpacing doesnt deal well with prime numbers, so we have a custom impl here
	static bool ComputeGridSpacing(const FFrameRate& InFrameRate,
	                               float PixelsPerSecond,
	                               double& OutMajorInterval,
	                               int32& OutMinorDivisions,
	                               float MinTickPx,
	                               float DesiredMajorTickPx)
	{
		// First try built-in spacing
		bool bResult = InFrameRate.ComputeGridSpacing(PixelsPerSecond, OutMajorInterval, OutMinorDivisions, MinTickPx, DesiredMajorTickPx);
		if (!bResult || OutMajorInterval == 1.0)
		{
			if (PixelsPerSecond <= 0.f)
			{
				return false;
			}

			const int32 RoundedFPS = FMath::RoundToInt(InFrameRate.AsDecimal());

			if (RoundedFPS > 0)
			{
				// Showing frames
				TArray<int32, TInlineAllocator<10>> CommonBases;

				// Divide the rounded frame rate by 2s, 3s or 5s recursively
				{
					const int32 Denominators[] = {2, 3, 5};

					int32 LowestBase = RoundedFPS;
					for (;;)
					{
						CommonBases.Add(LowestBase);

						if (LowestBase % 2 == 0) { LowestBase = LowestBase / 2; }
						else if (LowestBase % 3 == 0) { LowestBase = LowestBase / 3; }
						else if (LowestBase % 5 == 0) { LowestBase = LowestBase / 5; }
						else
						{
							int32 LowestResult = LowestBase;
							for (int32 Denominator : Denominators)
							{
								int32 Result = LowestBase / Denominator;
								if (Result > 0 && Result < LowestResult)
								{
									LowestResult = Result;
								}
							}

							if (LowestResult < LowestBase)
							{
								LowestBase = LowestResult;
							}
							else
							{
								break;
							}
						}
					}
				}

				Algo::Reverse(CommonBases);

				const int32 Scale = FMath::CeilToInt(DesiredMajorTickPx / PixelsPerSecond * InFrameRate.AsDecimal());
				const int32 BaseIndex = FMath::Min(Algo::LowerBound(CommonBases, Scale), CommonBases.Num() - 1);
				const int32 Base = CommonBases[BaseIndex];

				int32 MajorIntervalFrames = FMath::CeilToInt(Scale / float(Base)) * Base;
				OutMajorInterval = MajorIntervalFrames * InFrameRate.AsInterval();

				// Find the lowest number of divisions we can show that's larger than the minimum tick size
				OutMinorDivisions = 0;
				for (int32 DivIndex = 0; DivIndex < BaseIndex; ++DivIndex)
				{
					if (Base % CommonBases[DivIndex] == 0)
					{
						int32 MinorDivisions = MajorIntervalFrames / CommonBases[DivIndex];
						if (OutMajorInterval / MinorDivisions * PixelsPerSecond >= MinTickPx)
						{
							OutMinorDivisions = MinorDivisions;
							break;
						}
					}
				}
			}
		}

		return OutMajorInterval != 0;
	}

	static TSharedRef<SWidget> MakeTrackButton(FText HoverText, FOnGetContent MenuContent, const TAttribute<bool>& HoverState)
	{
		FSlateFontInfo SmallLayoutFont = FCoreStyle::GetDefaultFontStyle("Regular", 8);

		TSharedRef<STextBlock> ComboButtonText = SNew(STextBlock)
			.Text(HoverText)
			.Font(SmallLayoutFont)
			.ColorAndOpacity(FSlateColor::UseForeground());

		TSharedRef<SComboButton> ComboButton =

			SNew(SComboButton)
			.HasDownArrow(false)
			.ButtonStyle(FEditorStyle::Get(), "HoverHintOnly")
			.ForegroundColor(FSlateColor::UseForeground())
			.OnGetMenuContent(MenuContent)
			.ContentPadding(FMargin(5, 2))
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.ButtonContent()
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				  .AutoWidth()
				  .VAlign(VAlign_Center)
				  .Padding(FMargin(0, 0, 2, 0))
				[
					SNew(SImage)
					.ColorAndOpacity(FSlateColor::UseForeground())
					.Image(FEditorStyle::GetBrush("ComboButton.Arrow"))
				]

				+ SHorizontalBox::Slot()
				  .VAlign(VAlign_Center)
				  .AutoWidth()
				[
					ComboButtonText
				]
			];

		auto GetRolloverVisibility = [WeakComboButton = TWeakPtr<SComboButton>(ComboButton), HoverState]()
		{
			TSharedPtr<SComboButton> ComboButton = WeakComboButton.Pin();
			if (HoverState.Get() || ComboButton->IsOpen())
			{
				return EVisibility::SelfHitTestInvisible;
			}
			else
			{
				return EVisibility::Collapsed;
			}
		};

		TAttribute<EVisibility> Visibility = TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateLambda(GetRolloverVisibility));
		ComboButtonText->SetVisibility(Visibility);

		return ComboButton;
	}

	static TSharedPtr<FActEventTimelineArgs> MakeActEventTimelineArgs()
	{
		// ** EventTimeline 共享参数的初始化
		TSharedPtr<FActEventTimelineArgs> ActEventTimelineArgs = MakeShareable(new FActEventTimelineArgs());
		auto TickResolutionAttrLambda = MakeAttributeLambda([]()
		{
			auto DB = GetDataBindingSP(FActEventTimelineArgs, "ActEventTimelineArgs");
			TSharedPtr<FActEventTimelineArgs> ActEventTimelineArgs = DB->GetData();
			return ActEventTimelineArgs->TickResolution;
		});
		ActEventTimelineArgs->NumericTypeInterface = MakeShareable(new FFrameNumberInterface(MakeAttributeLambda([]()
		                                                                                     {
			                                                                                     return EFrameNumberDisplayFormats::Frames;
		                                                                                     }),
		                                                                                     0,
		                                                                                     TickResolutionAttrLambda,
		                                                                                     TickResolutionAttrLambda));
		return ActEventTimelineArgs;
	}

	static FName GetNewTrackName(UAnimSequenceBase* InAnimSequenceBase)
	{
		TArray<FName> TrackNames;
		TrackNames.Reserve(50);

		for (const FAnimNotifyTrack& Track : InAnimSequenceBase->AnimNotifyTracks)
		{
			TrackNames.Add(Track.TrackName);
		}

		FName NameToTest;
		int32 TrackIndex = 1;

		do
		{
			NameToTest = *FString::FromInt(TrackIndex++);
		}
		while (TrackNames.Contains(NameToTest));

		return NameToTest;
	}


	static FText MakeTooltipFromTime(const UAnimSequenceBase* InSequence, float InSeconds, float InDuration)
	{
		const FText Frame = FText::AsNumber(InSequence->GetFrameAtTime(InSeconds));
		const FText Seconds = FText::AsNumber(InSeconds);

		if (InDuration > 0.0f)
		{
			const FText Duration = FText::AsNumber(InDuration);
			return FText::Format(LOCTEXT("NodeToolTipLong", "@ {0} sec (frame {1}) for {2} sec"), Seconds, Frame, Duration);
		}
		else
		{
			return FText::Format(LOCTEXT("NodeToolTipShort", "@ {0} sec (frame {1})"), Seconds, Frame);
		}
	}

	static void OnViewRangePan(float InDeltaPanDistance, const FVector2D& InSize)
	{
		auto DB = GetDataBindingSP(FActEventTimelineArgs, "ActEventTimelineArgs");
		if (!DB)
		{
			return;
		}
		TSharedPtr<FActEventTimelineArgs> ActEventTimelineArgs = DB->GetData();
		double ViewInputMin = ActEventTimelineArgs->ViewRange->GetLowerBoundValue();
		double ViewInputMax = ActEventTimelineArgs->ViewRange->GetLowerBoundValue();
		FTrackScaleInfo ScaleInfo(ViewInputMin,
		                          ViewInputMax,
		                          0.f,
		                          0.f,
		                          InSize);

		float InputDeltaX = ScaleInfo.PixelsPerInput > 0.0f ? InDeltaPanDistance / ScaleInfo.PixelsPerInput : 0.0f;
		float NewViewInputMin = ViewInputMin + InputDeltaX;
		float NewViewInputMax = ViewInputMax + InputDeltaX;
		ActEventTimelineArgs->SetViewRangeClamped(NewViewInputMin, NewViewInputMax);
	}

	static bool OnSnapTime(float& InOutTime, float InSnapMargin, TArrayView<const FName> InSkippedSnapTypes)
	{
		// TODO: 在工程中 没找到开启 Snap 的 Option
		// double DoubleTime = (double)InOutTime;
		// bool bResult = Snap(DoubleTime, (double)InSnapMargin, InSkippedSnapTypes);
		// InOutTime = DoubleTime;
		return false;
	}
};

#undef LOCTEXT_NAMESPACE
