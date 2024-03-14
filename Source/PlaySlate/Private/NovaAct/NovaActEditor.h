#pragma once

#include "IAnimationEditor.h"
#include "Common/NovaStruct.h"

class FActAssetDetailsNotifyHook;
class FActViewportPreviewScene;
class UActAnimation;
class FActViewportClient;
class FActAssetDetailsBrain;

using namespace NovaDelegate;
using namespace NovaStruct;

/**
 * 资源编辑器的入口和管理者，提供一些工具方法和资源对象指针，该对象会与编辑器的主页签一同释放
 */
class FNovaActEditor : public FWorkflowCentricApplication, public FGCObject, public FEditorUndoClient, public FTickableEditorObject
{
public:
	FNovaActEditor(UActAnimation* InActAnimation);
	virtual ~FNovaActEditor() override;

	//~Begin FGCObject interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override;
	//~End FGCObject interface

	//~Begin FWorkflowCentricApplication interface
	virtual bool OnRequestClose() override;
	bool OnRequestClose(EAssetEditorCloseReason InCloseReason) override;
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	//~End FWorkflowCentricApplication interfaced

	//~Begin FTickableEditorObject interface
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	//~End FTickableEditorObject interface

	/**
	 * 构造编辑器窗口，设置当前编辑的资源实例及相关参数
	 *
	 * @param InIToolkitHost 仅透传到对应接口，无实际作用
	 */
	void CreateEditorWindow(const TSharedPtr<IToolkitHost> InIToolkitHost);
	/**
	 * 生成 EventTimeline Tab 的回调注册函数,
	 *
	 * @param SpawnTabArgs 生成 Tab 使用的参数
	 * @return 生成的 Tab Widget
	 */
	TSharedRef<SDockTab> OnActEventTimelineTabSpawn(const FSpawnTabArgs& SpawnTabArgs);
	/**
	 * 数据绑定 AnimSequence 数据改变的回调
	 *
	 * @param InAnimSequence 数据绑定传入参数
	 */
	void OnAnimSequenceChanged(UAnimSequence** InAnimSequence);
	/**
	 * 根据 AnimationAsset 资源类型创建 Tab 界面的内容
	 * @param InAnimationAsset
	 * @return 具体创建的对应资源类型编辑 Widget 填充在 ActEventTimelineTab 中
	 */
	TSharedRef<SWidget> MakeEditTabContent(UAnimationAsset* InAnimationAsset);
	/**
	 * 打开资源类型对应的编辑 Tab，目前仅有 AnimSequence 的编辑界面，这也是数据绑定到 AnimSequenceBase 上的
	 *
	 * @param InAnimationAsset
	 */
	void OpenNewAnimationAssetEditTab(UAnimationAsset** InAnimationAsset);

	void OnClose() override;
	virtual bool IsPrimaryEditor() const override;
protected:
	/** PreviewScene , Editor 没有销毁的情况下不会为空 */
	TSharedPtr<FActViewportPreviewScene> ActViewportPreviewScene;
	/** EventTimeline Dock Tab */
	TSharedPtr<SDockTab> ActEventTimelineParentDockTab;
};
