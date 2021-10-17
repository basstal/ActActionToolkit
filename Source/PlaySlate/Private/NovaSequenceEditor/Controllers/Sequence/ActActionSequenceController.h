﻿#pragma once

#include "Utils/ActActionSequenceUtil.h"
#include "IContentBrowserSingleton.h"

class FActActionSequenceEditor;
class FActActionTimeSliderController;
class SActActionSequenceWidget;
class ASkeletalMeshActor;
class UActActionSequence;
class FActActionSequenceTreeViewNode;
class SActActionViewportWidget;
class FActActionSequenceSectionOverlayController;
/**
 * Sequence Tab的主要控制器
 * 对应的View模块为SActActionSequenceWidget
 */
class FActActionSequenceController : public TSharedFromThis<FActActionSequenceController>, FTickableEditorObject
{
public:
	FActActionSequenceController(const TSharedRef<FActActionSequenceEditor>& InActActionSequenceEditor);
	virtual ~FActActionSequenceController() override;

	/**
	* 构造Sequence的Widget为SActActionSequenceWidget，同时初始化TreeView相关内容
	*
	* @param ViewParams 构造Widget使用的相关参数
	*/
	void MakeSequenceWidget(ActActionSequence::FActActionSequenceViewParams ViewParams);

	//~Begin FTickableEditorObject interface
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	//~End FTickableEditorObject interface

	/**
	 * 调用已注册的TrackEditor的Create代理，并收集创建的TrackEditor实例
	 */
	void ExecuteTrackEditorCreateDelegate();
	/**
	 * 构建AddTrack菜单的内容
	 *
	 * @param MenuBuilder 被修改的菜单构建者
	 */
	void BuildAddTrackMenu(FMenuBuilder& MenuBuilder);
	/** @param InAnimSequence 被添加的AnimSequence实例 */
	void AddAnimSequenceTrack(UAnimSequence* InAnimSequence);
	/**
	 * 设置当前的播放状态
	 *
	 * @param InPlaybackStatus 设置为该播放状态
	 */
	void SetPlaybackStatus(ActActionSequence::EPlaybackType InPlaybackStatus);
	/**
	 * 将当前播放状态设置为Stopped
	 */
	void Pause();
	/**
	 * @return 返回当前的帧时间
	 */
	FFrameTime GetLocalFrameTime() const;
	/**
	 * @return 获得当前时间帧的显示文本
	 */
	FString GetFrameTimeText() const;
	/**
	 * 时间轴拖拽器开始拖拽的回调
	 */
	void OnBeginScrubbing();
	/**
	* 时间轴拖拽器结束拖拽的回调
	*/
	void OnEndScrubbing();
	/**
	 * 设置全局时间点到指定时间
	 *
	 * @param InFrameTime 设置的时间点
	 */
	void SetGlobalTime(FFrameTime InFrameTime) const;
	/**
	 * 设置局部时间点到指定时间
	 * 
	 * @param InFrameTime 设置的时间点
	 */
	void SetLocalTimeDirectly(FFrameTime InFrameTime) const;
	/**
	 * 时间轴拖拽器位置改变的回调
	 */
	void OnScrubPositionChanged(FFrameTime NewScrubPosition, bool bScrubbing);
	/**
	 * @return 当前时间轴的显示范围
	 */
	ActActionSequence::FActActionAnimatedRange GetViewRange() const;
	/**
	 * 往AddTrack菜单中填充内容
	 * 
	 * @param MenuBuilder 被填充的菜单对象
	 */
	void PopulateAddMenuContext(FMenuBuilder& MenuBuilder);
protected:
	/**
	 * 对Editor的弱引用，调用编辑器资源和相关工具方法
	 */
	TWeakPtr<FActActionSequenceEditor> ActActionSequenceEditor;
	/**
	 * 所有已注册的CreateTrackEditor代理方法，在FActActionTrackEditorBase的子类中实现
	 */
	TArray<ActActionSequence::OnCreateTrackEditorDelegate> TrackEditorDelegates;
	/** List of tools we own */
	TArray<TSharedPtr<FActActionTrackEditorBase>> TrackEditors;
	/**
	 * 当前的播放状态
	 */
	ActActionSequence::EPlaybackType PlaybackState;
	/**
	 * TimeSlider的Controller
	 */
	TSharedPtr<FActActionTimeSliderController> ActActionTimeSliderController;
	/**
	 * 所有可见节点DisplayedRootNodes的父节点，
	 * Sequence中所有可见根节点都储存在NodeTree中作为子节点
	 */
	TSharedPtr<FActActionSequenceTreeViewNode> ActActionSequenceTreeViewNode;
	/**
	 * TODO:取一个更好的字段名
	 * SectionOverlay的Controller，这个用来绘制TickLines
	 */
	TSharedPtr<FActActionSequenceSectionOverlayController> ActActionSequenceSectionOverlayController0;
	/**
	 * TODO:取一个更好的字段名
	 * SectionOverlay的Controller，这个用来绘制Scrub位置
	 */
	TSharedPtr<FActActionSequenceSectionOverlayController> ActActionSequenceSectionOverlayController1;
	/**
	 * UMG Sequence main
	 */
	TSharedPtr<SActActionSequenceWidget> ActActionSequenceWidget;
	/** Numeric type interface used for converting parsing and generating strings from numbers */
	TSharedPtr<INumericTypeInterface<double>> NumericTypeInterface;
	/**
	 * TimeSlider的控制相关参数
	 */
	ActActionSequence::FActActionTimeSliderArgs TimeSliderArgs;
	/**
	 * TODO:这里要改成帧
	 * 当前Sequence时间轴的显示的范围，这里的单位是秒
	 */
	TRange<double> TargetViewRange;
public:
	const TRange<double>& GetTargetViewRange() const
	{
		return TargetViewRange;
	}

	ActActionSequence::EPlaybackType GetPlaybackStatus() const
	{
		return PlaybackState;
	}

	TSharedRef<SActActionSequenceWidget> GetActActionSequenceWidget() const
	{
		return ActActionSequenceWidget.ToSharedRef();
	}

	TSharedRef<FActActionSequenceTreeViewNode> GetActActionSequenceTreeViewRoot() const
	{
		return ActActionSequenceTreeViewNode.ToSharedRef();
	}

	TSharedRef<FActActionTimeSliderController> GetActActionTimeSliderController() const
	{
		return ActActionTimeSliderController.ToSharedRef();
	}

	TSharedRef<FActActionSequenceEditor> GetActActionSequenceEditor() const
	{
		return ActActionSequenceEditor.Pin().ToSharedRef();
	}

	// TODO:取一个更好的函数名 
	TSharedRef<FActActionSequenceSectionOverlayController> GetActActionSequenceSectionOverlayController0() const
	{
		return ActActionSequenceSectionOverlayController0.ToSharedRef();
	}

	// TODO:取一个更好的函数名 
	TSharedRef<FActActionSequenceSectionOverlayController> GetActActionSequenceSectionOverlayController1() const
	{
		return ActActionSequenceSectionOverlayController1.ToSharedRef();
	}

	ActActionSequence::FActActionTimeSliderArgs& GetTimeSliderArgs()
	{
		return TimeSliderArgs;
	}
};
