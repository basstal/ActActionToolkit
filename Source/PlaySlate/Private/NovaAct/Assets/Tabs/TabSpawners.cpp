#include "TabSpawners.h"

#include "EditorStyleSet.h"
#include "Common/NovaConst.h"
#include "NovaAct/ActViewport/ActViewportTabWidget.h"
#include "WorkflowOrientedApp/ApplicationMode.h"

#define LOCTEXT_NAMESPACE "NovaAct"

using namespace NovaConst;

FActAssetDetailsTabSummoner::FActAssetDetailsTabSummoner(const TSharedRef<FAssetEditorToolkit>& InAssetEditorToolkit)
	: FWorkflowTabFactory(ActAssetDetailsTabId, InAssetEditorToolkit)
{
	TabLabel = LOCTEXT("ActAssetDetails", "NovaActEditor ActAssetDetails tab name.");
	TabIcon = FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Details");
	bIsSingleton = true;
	ViewMenuDescription = LOCTEXT("DetailsDescription", "Details");
	ViewMenuTooltip = LOCTEXT("DetailsToolTip", "Shows the details tab for selected objects.");
	ActAssetDetailsWidget = SNew(SActAssetDetailsWidget);
}

TSharedRef<SWidget> FActAssetDetailsTabSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	return ActAssetDetailsWidget.ToSharedRef();
}

FText FActAssetDetailsTabSummoner::GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const
{
	return LOCTEXT("PersonaDetailsToolTip", "Edit the details of selected objects.");
}

FActViewportSummoner::FActViewportSummoner(const TSharedRef<FAssetEditorToolkit>& InAssetEditorToolkit, int32 InViewportIndex)
	: FWorkflowTabFactory(ActViewportTabIds[InViewportIndex], InAssetEditorToolkit)
{
	FText NumberText = FText::AsNumber(InViewportIndex + 1);
	TabLabel = FText::Format(LOCTEXT("ViewportTabTitle", "Viewport {0}"), NumberText);
	TabIcon = FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Viewports");
	bIsSingleton = true;
	ViewMenuDescription = FText::Format(LOCTEXT("ViewportViewFormat", "Viewport {0}"), NumberText);
	ViewMenuTooltip = LOCTEXT("ActViewport", "NovaActEditor ActViewport tab name.");
}

FTabSpawnerEntry& FActViewportSummoner::RegisterTabSpawner(TSharedRef<FTabManager> TabManager, const FApplicationMode* CurrentApplicationMode) const
{
	FTabSpawnerEntry& SpawnerEntry = FWorkflowTabFactory::RegisterTabSpawner(TabManager, nullptr);

	if (CurrentApplicationMode)
	{
		// find an existing workspace item or create new
		TSharedPtr<FWorkspaceItem> GroupItem = nullptr;
		TSharedRef<FWorkspaceItem> MenuCategory = CurrentApplicationMode->GetWorkspaceMenuCategory();
		for (const TSharedRef<FWorkspaceItem>& Item : MenuCategory->GetChildItems())
		{
			if (Item->GetDisplayName().ToString() == LOCTEXT("ViewportsSubMenu", "Viewports").ToString())
			{
				GroupItem = Item;
				break;
			}
		}

		if (!GroupItem)
		{
			GroupItem = MenuCategory->AddGroup(LOCTEXT("ViewportsSubMenu", "Viewports"),
			                                   LOCTEXT("ViewportsSubMenu_Tooltip", "Open a new viewport on the scene"),
			                                   FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Viewports"));
		}

		SpawnerEntry.SetGroup(GroupItem.ToSharedRef());
	}

	return SpawnerEntry;
}

TSharedRef<SWidget> FActViewportSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	return SNew(SActViewportTabWidget);
}


#undef LOCTEXT_NAMESPACE
