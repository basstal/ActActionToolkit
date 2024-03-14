#include "NovaActUICommandInfo.h"

#include "EditorStyleSet.h"


#define LOCTEXT_NAMESPACE "NovaAct"

FNovaActUICommandInfo::FNovaActUICommandInfo()
	: TCommands<FNovaActUICommandInfo>(
		TEXT("AnimSequenceCurveEditor"),
		NSLOCTEXT("Contexts", "AnimSequenceTimelineEditor", "Anim Sequence Timeline Editor"),
		NAME_None,
		FEditorStyle::GetStyleSetName()
	) {}

void FNovaActUICommandInfo::RegisterCommands()
{
	UI_COMMAND(EditSelectedCurves, "Edit Selected Curves", "Edit the selected curves in the curve editor tab", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(RemoveSelectedCurves, "Remove Selected Curves", "Remove the selected curves", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(AddNotifyTrack, "Add Notify Track", "Add a new notify track", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(InsertNotifyTrack, "Insert Notify Track", "Insert a new notify track above here", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(RemoveNotifyTrack, "Remove Notify Track", "Remove this notify track", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(AddCurve, "Add Curve...", "Add a new variable float curve", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(EditCurve, "Edit Curve", "Edit this curve in the curve editor tab", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(ShowCurveKeys, "Show Curve Keys", "Show keys for all curves in the timeline", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(AddMetadata, "Add Metadata...", "Add a new constant (metadata) float curve", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(ConvertCurveToMetaData,
	           "Convert To Metadata",
	           "Convert this curve to a constant (metadata) curve.\nThis is a destructive operation and will remove all keys from this curve.",
	           EUserInterfaceActionType::Button,
	           FInputChord());
	UI_COMMAND(ConvertMetaDataToCurve, "Convert To Curve", "Convert this metadata curve to a full curve", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(RemoveCurve, "Remove Curve", "Remove this curve", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(RemoveAllCurves, "Remove All Curves", "Remove all the curves in this animation", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(DisplaySeconds, "Seconds", "Display the time in seconds", EUserInterfaceActionType::RadioButton, FInputChord());
	UI_COMMAND(DisplayFrames, "Frames", "Display the time in frames", EUserInterfaceActionType::RadioButton, FInputChord());
	UI_COMMAND(DisplayPercentage,
	           "Percentage",
	           "Display the percentage along with the time with the scrubber",
	           EUserInterfaceActionType::ToggleButton,
	           FInputChord());
	UI_COMMAND(DisplaySecondaryFormat,
	           "Secondary",
	           "Display the time or frame count as a secondary format along with the scrubber",
	           EUserInterfaceActionType::ToggleButton,
	           FInputChord());
	UI_COMMAND(SnapToFrames, "Frames", "Snap to frame boundaries", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(SnapToNotifies, "Notifies", "Snap to notifies and notify states", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(SnapToMontageSections, "Montage Sections", "Snap to montage sections", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(SnapToCompositeSegments, "Composite Segments", "Snap to composite segments", EUserInterfaceActionType::ToggleButton, FInputChord());
}

FNovaActNotifiesPanelCommands::FNovaActNotifiesPanelCommands()
	: TCommands<FNovaActNotifiesPanelCommands>("AnimNotifyPanel",
	                                           NSLOCTEXT("Contexts", "AnimNotifyPanel", "Anim Notify Panel"),
	                                           NAME_None,
	                                           FEditorStyle::GetStyleSetName()) {}

void FNovaActNotifiesPanelCommands::RegisterCommands()
{
	UI_COMMAND(DeleteNotify, "Delete", "Deletes the selected notifies.", EUserInterfaceActionType::Button, FInputChord(EKeys::Platform_Delete));
	UI_COMMAND(CopyNotifies, "Copy", "Copy animation notify events.", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::C));
	UI_COMMAND(PasteNotifies, "Paste", "Paste animation notify event here.", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::V));
}


#undef LOCTEXT_NAMESPACE
