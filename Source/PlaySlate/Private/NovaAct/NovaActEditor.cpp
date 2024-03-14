#include "NovaActEditor.h"

#include "PlaySlate.h"
#include "Common/NovaConst.h"
#include "Common/NovaDataBinding.h"
#include "NovaAct/Assets/ActAnimation.h"

#include "NovaAct/ActViewport/ActViewportPreviewScene.h"
#include "NovaAct/ActEventTimeline/ActEventTimelineWidget.h"
#include "NovaAct/ActAssetDetails/ActAssetDetailsWidget.h"

#include "FrameNumberNumericInterface.h"
#include "NovaActEditorMode.h"
#include "ActViewport/ActViewport.h"
#include "Common/NovaStaticFunction.h"
#include "NovaAct/ActEventTimeline/Image/Subs/NovaActUICommandInfo.h"

#define LOCTEXT_NAMESPACE "NovaAct"

using namespace NovaStruct;
using namespace NovaConst;

FNovaActEditor::FNovaActEditor(UActAnimation* InActAnimation)
{
	check(InActAnimation);
	NovaDB::CreateUObject("ActAnimation", InActAnimation);
	NovaDB::Create("ActAnimation/AnimSequence", &InActAnimation->AnimSequence);
	NovaDB::Create("ActAnimation/AnimBlueprint", &InActAnimation->AnimBlueprint);

	FDelegateHandle _;
	DataBindingBindRaw(UAnimSequence**, "ActAnimation/AnimSequence", this, &FNovaActEditor::OnAnimSequenceChanged, _);
	DataBindingBindRaw(UAnimationAsset**, "ActAnimation/AnimSequence", this, &FNovaActEditor::OpenNewAnimationAssetEditTab, _)

	TSharedPtr<FActEventTimelineArgs> ActEventTimelineArgs = NovaStaticFunction::MakeActEventTimelineArgs();
	NovaDB::CreateSP("ActEventTimelineArgs", ActEventTimelineArgs);
	// ** TODO:存储在配置中
	NovaDB::Create("ColumnFillCoefficientsLeft", 0.17f);
}

FNovaActEditor::~FNovaActEditor()
{
	UE_LOG(LogNovaAct, Log, TEXT("FNovaActEditor::~FNovaActEditor"));
	ActViewportPreviewScene.Reset();

	// NovaDB::Delete("ActAnimation");
	// NovaDB::Delete("ActAnimation/AnimSequence");
	// NovaDB::Delete("ActAnimation/AnimBlueprint");
	// NovaDB::Delete("ActEventTimelineArgs");
	// NovaDB::Delete("ColumnFillCoefficientsLeft");
	// NovaDB::Delete("ActViewportPreviewScene");
}

void FNovaActEditor::AddReferencedObjects(FReferenceCollector& Collector)
{
	auto DB = GetDataBindingUObject(UActAnimation, "ActAnimation");
	if (DB)
	{
		UActAnimation* Data = DB->GetData();
		Collector.AddReferencedObject(Data);
	}
}

FString FNovaActEditor::GetReferencerName() const
{
	return "NovaActEditor";
}

bool FNovaActEditor::OnRequestClose()
{
	throw std::runtime_error("Not used");
	// UE_LOG(LogNovaAct, Log, TEXT("FNovaActEditor::OnRequestClose "));
	// NovaDB::Delete("NovaActEditor");
	// return FWorkflowCentricApplication::OnRequestClose(EAssetEditorCloseReason::AssetEditorHostClosed);
}
bool FNovaActEditor::OnRequestClose(EAssetEditorCloseReason InCloseReason)
{
	return FWorkflowCentricApplication::OnRequestClose(InCloseReason);
}

FName FNovaActEditor::GetToolkitFName() const
{
	return "ToolkitName";
}

FText FNovaActEditor::GetBaseToolkitName() const
{
	return LOCTEXT("NovaActToolkitName", "Nova act toolkit base toolkit name");
}

void FNovaActEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu_AnimationEditor", "Animation Editor"));

	auto OnSpawnTabRaw = FOnSpawnTab::CreateRaw(this, &FNovaActEditor::OnActEventTimelineTabSpawn);
	// ** NOTE:记得反注册中也要添加对应方法
	FTabSpawnerEntry& EventTimelineEntry = InTabManager->RegisterTabSpawner(ActEventTimelineTabId, OnSpawnTabRaw);
	EventTimelineEntry.SetMenuType(ETabSpawnerMenuType::Hidden);
	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);
}

void FNovaActEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	InTabManager->UnregisterTabSpawner(ActEventTimelineTabId);
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);
}

FLinearColor FNovaActEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor(0.7f, 0.0f, 0.0f, 0.5f);
}

FString FNovaActEditor::GetWorldCentricTabPrefix() const
{
	return LOCTEXT("WorldCentricTabPrefix", "world centric tab prefix.").ToString();
}

void FNovaActEditor::Tick(float DeltaTime)
{
	// ** 使得 Client 能够保持 Tick，否则在失去焦点时（例如按下鼠标后） Client 将不会调用 Tick
	if (ActViewportPreviewScene->ActViewport)
	{
		ActViewportPreviewScene->ActViewport->GetViewportClient()->Invalidate();
	}
}

TStatId FNovaActEditor::GetStatId() const
{
	// ** TODO: 这里没处理
	RETURN_QUICK_DECLARE_CYCLE_STAT(ExampleAsyncTask, STATGROUP_ThreadPoolAsyncTasks);
}

void FNovaActEditor::CreateEditorWindow(const TSharedPtr<IToolkitHost> InIToolkitHost)
{
	FNovaActUICommandInfo::Register();

	const FPreviewScene::ConstructionValues CSV = FPreviewScene::ConstructionValues().AllowAudioPlayback(true).ShouldSimulatePhysics(true);
	ActViewportPreviewScene = MakeShareable(new FActViewportPreviewScene(CSV));
	NovaDB::CreateSP("ActViewportPreviewScene", ActViewportPreviewScene);
	auto ActAnimationDB = GetDataBindingUObject(UActAnimation, "ActAnimation");
	// Initialize the asset editor
	InitAssetEditor(EToolkitMode::Standalone,
	                InIToolkitHost,
	                NovaActAppName,
	                FTabManager::FLayout::NullLayout,
	                true,
	                true,
	                ActAnimationDB->GetData());

	AddApplicationMode(NovaActEditorMode, MakeShareable(new FNovaActEditorMode(SharedThis(this))));
	SetCurrentMode(NovaActEditorMode);

	// ** Init by resource
	NovaDB::Trigger("ActAnimation");
	NovaDB::Trigger("ActAnimation/AnimBlueprint");
	NovaDB::Trigger("ActAnimation/AnimSequence");

	// When undo occurs, get a notification so we can make sure our view is up to date
	GEditor->RegisterForUndo(this);
}

TSharedRef<SDockTab> FNovaActEditor::OnActEventTimelineTabSpawn(const FSpawnTabArgs& SpawnTabArgs)
{
	ActEventTimelineParentDockTab = SNew(SDockTab)
		.Label(NovaActEditor_ActEventTimeline)
		.TabColorScale(GetTabColorScale())
		.TabRole(ETabRole::PanelTab);
	return ActEventTimelineParentDockTab.ToSharedRef();
}

void FNovaActEditor::OnAnimSequenceChanged(UAnimSequence** InAnimSequence)
{
	if (!InAnimSequence)
	{
		return;
	}
	UAnimSequence* AnimSequence = *InAnimSequence;
	auto ActEventTimelineArgsDB = GetDataBindingSP(FActEventTimelineArgs, "ActEventTimelineArgs");
	if (!AnimSequence || !ActEventTimelineArgsDB)
	{
		return;
	}
	UE_LOG(LogNovaAct, Log, TEXT("AnimMontage : %s"), *AnimSequence->GetName());
	const float CalculateSequenceLength = AnimSequence->GetPlayLength();
	UE_LOG(LogNovaAct, Log, TEXT("InTotalLength : %f"), CalculateSequenceLength);
	// ** 限制显示的最大长度为当前的Sequence总时长
	auto ActEventTimelineArgs = ActEventTimelineArgsDB->GetData();
	ActEventTimelineArgs->ClampRange = TRange<double>(0, CalculateSequenceLength);
	ActEventTimelineArgs->SetViewRangeClamped(0, CalculateSequenceLength);
	ActEventTimelineArgs->TickResolution = AnimSequence->GetSamplingFrameRate();
	// NovaDB::Trigger("ActEventTimelineArgs/ViewRange");
	// NovaDB::Trigger("ActEventTimelineArgs");
}

TSharedRef<SWidget> FNovaActEditor::MakeEditTabContent(UAnimationAsset* InAnimationAsset)
{
	if (!InAnimationAsset)
	{
		return SNullWidget::NullWidget;
	}

	if (Cast<UAnimSequence>(InAnimationAsset))
	{
		InAnimationAsset->SetFlags(RF_Transactional);
		return SNew(SActEventTimelineWidget);
	}
	return SNullWidget::NullWidget;
}

void FNovaActEditor::OpenNewAnimationAssetEditTab(UAnimationAsset** InAnimationAsset)
{
	if (!InAnimationAsset)
	{
		return;
	}
	UE_LOG(LogNovaAct, Log, TEXT("FNovaActEditor::OpenNewAnimationAssetEditTab"));
	if (!ActEventTimelineParentDockTab && TabManager)
	{
		TabManager->TryInvokeTab(ActEventTimelineTabId);
		check(ActEventTimelineParentDockTab)
	}
	if (ActEventTimelineParentDockTab)
	{
		TSharedRef<SWidget> TabContent = MakeEditTabContent(*InAnimationAsset);
		ActEventTimelineParentDockTab->SetContent(TabContent);
		ActEventTimelineParentDockTab->ActivateInParent(ETabActivationCause::SetDirectly);
	}
}
void FNovaActEditor::OnClose()
{
	FWorkflowCentricApplication::OnClose();
}
bool FNovaActEditor::IsPrimaryEditor() const
{
	return false;
}

#undef LOCTEXT_NAMESPACE
