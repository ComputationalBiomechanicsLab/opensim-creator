find_package(OpenSim REQUIRED CONFIG)

add_library(OpenSimCreator STATIC

    ComponentRegistry/ComponentRegistry.h
    ComponentRegistry/ComponentRegistryBase.cpp
    ComponentRegistry/ComponentRegistryBase.h
    ComponentRegistry/ComponentRegistryEntry.h
    ComponentRegistry/ComponentRegistryEntryBase.cpp
    ComponentRegistry/ComponentRegistryEntryBase.h
    ComponentRegistry/StaticComponentRegistries.cpp
    ComponentRegistry/StaticComponentRegistries.h

    Documents/CustomComponents/CrossProductDefinedFrame.cpp
    Documents/CustomComponents/CrossProductDefinedFrame.h
    Documents/CustomComponents/CrossProductEdge.cpp
    Documents/CustomComponents/CrossProductEdge.h
    Documents/CustomComponents/Edge.cpp
    Documents/CustomComponents/Edge.h
    Documents/CustomComponents/EdgePoints.h
    Documents/CustomComponents/ICustomDecorationGenerator.h
    Documents/CustomComponents/InMemoryMesh.cpp
    Documents/CustomComponents/InMemoryMesh.h
    Documents/CustomComponents/MidpointLandmark.cpp
    Documents/CustomComponents/MidpointLandmark.h
    Documents/CustomComponents/PointToPointEdge.cpp
    Documents/CustomComponents/PointToPointEdge.h
    Documents/CustomComponents/SphereLandmark.cpp
    Documents/CustomComponents/SphereLandmark.h

    Documents/FrameDefinition/FrameDefinitionActions.cpp
    Documents/FrameDefinition/FrameDefinitionActions.h
    Documents/FrameDefinition/FrameDefinitionHelpers.cpp
    Documents/FrameDefinition/FrameDefinitionHelpers.h

    Documents/Landmarks/Landmark.h
    Documents/Landmarks/LandmarkCSVFlags.h
    Documents/Landmarks/LandmarkHelpers.cpp
    Documents/Landmarks/LandmarkHelpers.h
    Documents/Landmarks/NamedLandmark.h

    Documents/MeshImporter/Body.cpp
    Documents/MeshImporter/Body.h
    Documents/MeshImporter/CrossrefDescriptor.h
    Documents/MeshImporter/CrossrefDirection.h
    Documents/MeshImporter/Document.cpp
    Documents/MeshImporter/DocumentHelpers.cpp
    Documents/MeshImporter/DocumentHelpers.h
    Documents/MeshImporter/Ground.cpp
    Documents/MeshImporter/Ground.h
    Documents/MeshImporter/IObjectFinder.h
    Documents/MeshImporter/Joint.cpp
    Documents/MeshImporter/Joint.h
    Documents/MeshImporter/Mesh.cpp
    Documents/MeshImporter/Mesh.h
    Documents/MeshImporter/MIClass.h
    Documents/MeshImporter/MIIDs.cpp
    Documents/MeshImporter/MIObject.cpp
    Documents/MeshImporter/MIObject.h
    Documents/MeshImporter/MIObjectCRTP.h
    Documents/MeshImporter/MIObjectFlags.h
    Documents/MeshImporter/MIObjectHelpers.cpp
    Documents/MeshImporter/MIObjectHelpers.h
    Documents/MeshImporter/MIIDs.h
    Documents/MeshImporter/MIStrings.h
    Documents/MeshImporter/MIVariant.h
    Documents/MeshImporter/OpenSimBridge.cpp
    Documents/MeshImporter/OpenSimBridge.h
    Documents/MeshImporter/OpenSimExportFlags.h
    Documents/MeshImporter/Station.cpp
    Documents/MeshImporter/Station.h
    Documents/MeshImporter/UndoableActions.cpp
    Documents/MeshImporter/UndoableActions.h
    Documents/MeshImporter/UndoableDocument.h

    Documents/MeshWarper/NamedLandmarkPair3D.h
    Documents/MeshWarper/TPSDocument.cpp
    Documents/MeshWarper/TPSDocument.h
    Documents/MeshWarper/TPSDocumentElement.h
    Documents/MeshWarper/TPSDocumentElementID.h
    Documents/MeshWarper/TPSDocumentElementType.h
    Documents/MeshWarper/TPSDocumentHelpers.cpp
    Documents/MeshWarper/TPSDocumentHelpers.h
    Documents/MeshWarper/TPSDocumentInputIdentifier.h
    Documents/MeshWarper/TPSDocumentLandmarkPair.h
    Documents/MeshWarper/TPSDocumentNonParticipatingLandmark.h
    Documents/MeshWarper/TPSWarpResultCache.h
    Documents/MeshWarper/UndoableTPSDocument.h
    Documents/MeshWarper/UndoableTPSDocumentActions.cpp
    Documents/MeshWarper/UndoableTPSDocumentActions.h

    Documents/Model/BasicModelStatePair.cpp
    Documents/Model/BasicModelStatePair.h
    Documents/Model/IConstModelStatePair.h
    Documents/Model/IModelStatePair.h
    Documents/Model/ModelStateCommit.cpp
    Documents/Model/ModelStateCommit.h
    Documents/Model/ModelStatePairInfo.cpp
    Documents/Model/ModelStatePairInfo.h
    Documents/Model/ObjectPropertyEdit.cpp
    Documents/Model/ObjectPropertyEdit.h
    Documents/Model/UndoableModelActions.cpp
    Documents/Model/UndoableModelActions.h
    Documents/Model/UndoableModelStatePair.cpp
    Documents/Model/UndoableModelStatePair.h

    Documents/ModelWarper/CachedModelWarper.cpp
    Documents/ModelWarper/CachedModelWarper.h
    Documents/ModelWarper/FrameWarperFactories.cpp
    Documents/ModelWarper/FrameWarperFactories.h
    Documents/ModelWarper/ICloneable.h
    Documents/ModelWarper/IdentityFrameWarperFactory.cpp
    Documents/ModelWarper/IdentityFrameWarperFactory.h
    Documents/ModelWarper/IFrameWarper.h
    Documents/ModelWarper/IFrameWarperFactory.h
    Documents/ModelWarper/IPointWarper.h
    Documents/ModelWarper/IPointWarperFactory.h
    Documents/ModelWarper/IValidateable.cpp
    Documents/ModelWarper/IValidateable.h
    Documents/ModelWarper/IWarpDetailProvider.h
    Documents/ModelWarper/MaybePairedLandmark.h
    Documents/ModelWarper/ModelWarpConfiguration.cpp
    Documents/ModelWarper/ModelWarpConfiguration.h
    Documents/ModelWarper/ModelWarpDocument.cpp
    Documents/ModelWarper/ModelWarpDocument.h
    Documents/ModelWarper/PointWarperFactories.cpp
    Documents/ModelWarper/PointWarperFactories.h
    Documents/ModelWarper/StationDefinedFrameWarperFactory.cpp
    Documents/ModelWarper/StationDefinedFrameWarperFactory.h
    Documents/ModelWarper/TPSLandmarkPairWarperFactory.cpp
    Documents/ModelWarper/TPSLandmarkPairWarperFactory.h
    Documents/ModelWarper/ValidationCheckResult.h
    Documents/ModelWarper/ValidationCheckState.h
    Documents/ModelWarper/WarpableOpenSimComponent.h
    Documents/ModelWarper/WarpDetail.h

    Documents/OutputExtractors/ComponentOutputExtractor.cpp
    Documents/OutputExtractors/ComponentOutputExtractor.h
    Documents/OutputExtractors/ComponentOutputSubfield.cpp
    Documents/OutputExtractors/ComponentOutputSubfield.h
    Documents/OutputExtractors/ConcatenatingOutputExtractor.cpp
    Documents/OutputExtractors/ConcatenatingOutputExtractor.h
    Documents/OutputExtractors/IntegratorOutputExtractor.cpp
    Documents/OutputExtractors/IntegratorOutputExtractor.h
    Documents/OutputExtractors/IOutputExtractor.cpp
    Documents/OutputExtractors/IOutputExtractor.h
    Documents/OutputExtractors/MultiBodySystemOutputExtractor.cpp
    Documents/OutputExtractors/MultiBodySystemOutputExtractor.h
    Documents/OutputExtractors/OutputExtractor.cpp
    Documents/OutputExtractors/OutputExtractor.h
    Documents/OutputExtractors/OutputExtractorDataType.h
    Documents/OutputExtractors/OutputValueExtractor.h

    Documents/Simulation/ForwardDynamicSimulation.cpp
    Documents/Simulation/ForwardDynamicSimulation.h
    Documents/Simulation/ForwardDynamicSimulator.cpp
    Documents/Simulation/ForwardDynamicSimulator.h
    Documents/Simulation/ForwardDynamicSimulatorParams.cpp
    Documents/Simulation/ForwardDynamicSimulatorParams.h
    Documents/Simulation/IntegratorMethod.cpp
    Documents/Simulation/IntegratorMethod.h
    Documents/Simulation/ISimulation.h
    Documents/Simulation/Simulation.h
    Documents/Simulation/SimulationClock.h
    Documents/Simulation/SimulationClocks.h
    Documents/Simulation/SimulationHelpers.cpp
    Documents/Simulation/SimulationHelpers.h
    Documents/Simulation/SimulationModelStatePair.cpp
    Documents/Simulation/SimulationModelStatePair.h
    Documents/Simulation/SimulationReport.cpp
    Documents/Simulation/SimulationReport.h
    Documents/Simulation/SimulationStatus.cpp
    Documents/Simulation/SimulationStatus.h
    Documents/Simulation/SingleStateSimulation.cpp
    Documents/Simulation/SingleStateSimulation.h
    Documents/Simulation/StoFileSimulation.h
    Documents/Simulation/StoFileSimulation.cpp

    Graphics/CachedModelRenderer.cpp
    Graphics/CachedModelRenderer.h
    Graphics/ComponentAbsPathDecorationTagger.cpp
    Graphics/ComponentAbsPathDecorationTagger.h
    Graphics/ComponentSceneDecorationFlagsTagger.cpp
    Graphics/ComponentSceneDecorationFlagsTagger.h
    Graphics/CustomRenderingOptionFlags.cpp
    Graphics/CustomRenderingOptionFlags.h
    Graphics/CustomRenderingOptions.cpp
    Graphics/CustomRenderingOptions.h
    Graphics/OpenSimDecorationOptions.cpp
    Graphics/OpenSimDecorationOptions.h
    Graphics/ModelRendererParams.cpp
    Graphics/ModelRendererParams.h
    Graphics/MuscleColoringStyle.cpp
    Graphics/MuscleColoringStyle.h
    Graphics/MuscleDecorationStyle.cpp
    Graphics/MuscleDecorationStyle.h
    Graphics/MuscleSizingStyle.cpp
    Graphics/MuscleSizingStyle.h
    Graphics/OpenSimDecorationGenerator.cpp
    Graphics/OpenSimDecorationGenerator.h
    Graphics/OpenSimDecorationOptionFlags.cpp
    Graphics/OpenSimDecorationOptionFlags.h
    Graphics/OpenSimGraphicsHelpers.cpp
    Graphics/OpenSimGraphicsHelpers.h
    Graphics/OverlayDecorationGenerator.cpp
    Graphics/OverlayDecorationGenerator.h
    Graphics/OverlayDecorationOptionFlags.cpp
    Graphics/OverlayDecorationOptionFlags.h
    Graphics/OverlayDecorationOptions.cpp
    Graphics/OverlayDecorationOptions.h
    Graphics/SimTKDecorationGenerator.cpp
    Graphics/SimTKDecorationGenerator.h
    Graphics/SimTKMeshLoader.cpp
    Graphics/SimTKMeshLoader.h

    Platform/OpenSimCreatorApp.cpp
    Platform/OpenSimCreatorApp.h
    Platform/RecentFile.h
    Platform/RecentFiles.cpp
    Platform/RecentFiles.h

    UI/Experimental/MeshHittestTab.cpp
    UI/Experimental/MeshHittestTab.h
    UI/Experimental/PreviewExperimentalDataTab.cpp
    UI/Experimental/PreviewExperimentalDataTab.h
    UI/Experimental/RendererGeometryShaderTab.cpp
    UI/Experimental/RendererGeometryShaderTab.h
    UI/Experimental/TPS2DTab.cpp
    UI/Experimental/TPS2DTab.h

    UI/FrameDefinition/FrameDefinitionTab.cpp
    UI/FrameDefinition/FrameDefinitionTab.h
    UI/FrameDefinition/FrameDefinitionTabToolbar.cpp
    UI/FrameDefinition/FrameDefinitionTabToolbar.h
    UI/FrameDefinition/FrameDefinitionUIHelpers.cpp
    UI/FrameDefinition/FrameDefinitionUIHelpers.h

    UI/MeshImporter/ChooseElLayer.h
    UI/MeshImporter/DrawableThing.h
    UI/MeshImporter/IMeshImporterUILayerHost.h
    UI/MeshImporter/MeshImporterHover.h
    UI/MeshImporter/MeshImporterSharedState.h
    UI/MeshImporter/MeshImporterTab.cpp
    UI/MeshImporter/MeshImporterTab.h
    UI/MeshImporter/MeshImporterUILayer.h
    UI/MeshImporter/MeshLoader.cpp
    UI/MeshImporter/MeshLoader.h
    UI/MeshImporter/Select2MeshPointsLayer.h

    UI/MeshWarper/MeshWarpingTab.cpp
    UI/MeshWarper/MeshWarpingTab.h
    UI/MeshWarper/MeshWarpingTabActionsMenu.h
    UI/MeshWarper/MeshWarpingTabContextMenu.h
    UI/MeshWarper/MeshWarpingTabDecorationGenerators.h
    UI/MeshWarper/MeshWarpingTabEditMenu.h
    UI/MeshWarper/MeshWarpingTabFileMenu.h
    UI/MeshWarper/MeshWarpingTabHover.h
    UI/MeshWarper/MeshWarpingTabInputMeshPanel.h
    UI/MeshWarper/MeshWarpingTabMainMenu.h
    UI/MeshWarper/MeshWarpingTabNavigatorPanel.h
    UI/MeshWarper/MeshWarpingTabPanel.h
    UI/MeshWarper/MeshWarpingTabResultMeshPanel.h
    UI/MeshWarper/MeshWarpingTabSharedState.h
    UI/MeshWarper/MeshWarpingTabStatusBar.h
    UI/MeshWarper/MeshWarpingTabToolbar.h
    UI/MeshWarper/MeshWarpingTabUserSelection.h

    UI/ModelEditor/AddBodyPopup.cpp
    UI/ModelEditor/AddBodyPopup.h
    UI/ModelEditor/AddComponentPopup.cpp
    UI/ModelEditor/AddComponentPopup.h
    UI/ModelEditor/ComponentContextMenu.cpp
    UI/ModelEditor/ComponentContextMenu.h
    UI/ModelEditor/CoordinateEditorPanel.cpp
    UI/ModelEditor/CoordinateEditorPanel.h
    UI/ModelEditor/EditorTabStatusBar.cpp
    UI/ModelEditor/EditorTabStatusBar.h
    UI/ModelEditor/ExportPointsPopup.cpp
    UI/ModelEditor/ExportPointsPopup.h
    UI/ModelEditor/IEditorAPI.h
    UI/ModelEditor/ModelActionsMenuItems.cpp
    UI/ModelEditor/ModelActionsMenuItems.h
    UI/ModelEditor/ModelEditorTab.cpp
    UI/ModelEditor/ModelEditorTab.h
    UI/ModelEditor/ModelEditorMainMenu.cpp
    UI/ModelEditor/ModelEditorMainMenu.h
    UI/ModelEditor/ModelEditorToolbar.cpp
    UI/ModelEditor/ModelEditorToolbar.h
    UI/ModelEditor/ModelMusclePlotPanel.cpp
    UI/ModelEditor/ModelMusclePlotPanel.h
    UI/ModelEditor/OutputWatchesPanel.cpp
    UI/ModelEditor/OutputWatchesPanel.h
    UI/ModelEditor/ReassignSocketPopup.cpp
    UI/ModelEditor/ReassignSocketPopup.h
    UI/ModelEditor/SelectComponentPopup.cpp
    UI/ModelEditor/SelectComponentPopup.h
    UI/ModelEditor/SelectGeometryPopup.cpp
    UI/ModelEditor/SelectGeometryPopup.h
    UI/ModelEditor/Select1PFPopup.cpp
    UI/ModelEditor/Select1PFPopup.h

    UI/ModelWarper/ChecklistPanel.cpp
    UI/ModelWarper/ChecklistPanel.h
    UI/ModelWarper/FileMenu.cpp
    UI/ModelWarper/FileMenu.h
    UI/ModelWarper/MainMenu.h
    UI/ModelWarper/ModelWarperTab.cpp
    UI/ModelWarper/ModelWarperTab.h
    UI/ModelWarper/ModelWarperUIHelpers.cpp
    UI/ModelWarper/ModelWarperUIHelpers.h
    UI/ModelWarper/ResultModelViewerPanel.cpp
    UI/ModelWarper/ResultModelViewerPanel.h
    UI/ModelWarper/SourceModelViewerPanel.cpp
    UI/ModelWarper/SourceModelViewerPanel.h
    UI/ModelWarper/Toolbar.cpp
    UI/ModelWarper/Toolbar.h
    UI/ModelWarper/UIState.cpp
    UI/ModelWarper/UIState.h

    UI/Shared/BasicWidgets.cpp
    UI/Shared/BasicWidgets.h
    UI/Shared/ChooseComponentsEditorLayer.cpp
    UI/Shared/ChooseComponentsEditorLayer.h
    UI/Shared/ChooseComponentsEditorLayerParameters.h
    UI/Shared/GeometryPathEditorPopup.cpp
    UI/Shared/GeometryPathEditorPopup.h
    UI/Shared/ImportStationsFromCSVPopup.cpp
    UI/Shared/ImportStationsFromCSVPopup.h
    UI/Shared/MainMenu.cpp
    UI/Shared/MainMenu.h
    UI/Shared/ModelEditorViewerPanel.cpp
    UI/Shared/ModelEditorViewerPanel.h
    UI/Shared/ModelEditorViewerPanelLayer.h
    UI/Shared/ModelEditorViewerPanelLayerFlags.h
    UI/Shared/ModelEditorViewerPanelParameters.h
    UI/Shared/ModelEditorViewerPanelRightClickEvent.h
    UI/Shared/ModelEditorViewerPanelState.cpp
    UI/Shared/ModelEditorViewerPanelState.h
    UI/Shared/ModelSelectionGizmo.cpp
    UI/Shared/ModelSelectionGizmo.h
    UI/Shared/NavigatorPanel.cpp
    UI/Shared/NavigatorPanel.h
    UI/Shared/ObjectPropertiesEditor.cpp
    UI/Shared/ObjectPropertiesEditor.h
    UI/Shared/ParamBlockEditorPopup.cpp
    UI/Shared/ParamBlockEditorPopup.h
    UI/Shared/PropertiesPanel.cpp
    UI/Shared/PropertiesPanel.h
    UI/Shared/Readonly3DModelViewer.cpp
    UI/Shared/Readonly3DModelViewer.h

    UI/Simulation/ISimulatorUIAPI.cpp
    UI/Simulation/ISimulatorUIAPI.h
    UI/Simulation/ModelStatePairContextMenu.cpp
    UI/Simulation/ModelStatePairContextMenu.h
    UI/Simulation/OutputPlotsPanel.cpp
    UI/Simulation/OutputPlotsPanel.h
    UI/Simulation/SelectionDetailsPanel.cpp
    UI/Simulation/SimulationDetailsPanel.cpp
    UI/Simulation/SimulationDetailsPanel.h
    UI/Simulation/SimulationOutputPlot.cpp
    UI/Simulation/SimulationOutputPlot.h
    UI/Simulation/SimulationScrubber.cpp
    UI/Simulation/SimulationScrubber.h
    UI/Simulation/SimulationTab.cpp
    UI/Simulation/SimulationTab.h
    UI/Simulation/SimulationTabMainMenu.cpp
    UI/Simulation/SimulationTabMainMenu.h
    UI/Simulation/SimulationToolbar.cpp
    UI/Simulation/SimulationToolbar.h
    UI/Simulation/SimulationUILoopingState.h
    UI/Simulation/SimulationUIPlaybackState.h
    UI/Simulation/SelectionDetailsPanel.h
    UI/Simulation/SimulationViewerPanel.cpp
    UI/Simulation/SimulationViewerPanel.h
    UI/Simulation/SimulationViewerPanelParameters.h
    UI/Simulation/SimulationViewerRightClickEvent.h

    UI/IMainUIStateAPI.h
    UI/IPopupAPI.h
    UI/LoadingTab.cpp
    UI/LoadingTab.h
    UI/MainUIScreen.cpp
    UI/MainUIScreen.h
    UI/OpenSimCreatorTabRegistry.cpp
    UI/OpenSimCreatorTabRegistry.h
    UI/OpenSimCreatorTabs.h
    UI/PerformanceAnalyzerTab.cpp
    UI/PerformanceAnalyzerTab.h
    UI/SplashTab.cpp
    UI/SplashTab.h

    Utils/LandmarkPair3D.cpp
    Utils/LandmarkPair3D.h
    Utils/OpenSimHelpers.cpp
    Utils/OpenSimHelpers.h
    Utils/ParamBlock.h
    Utils/ParamBlock.cpp
    Utils/ParamValue.h
    Utils/ShapeFitters.cpp
    Utils/ShapeFitters.h
    Utils/SimTKHelpers.cpp
    Utils/SimTKHelpers.h
    Utils/TPS3D.cpp
    Utils/TPS3D.h
 )

# OpenSimCreatorConfig.h
#
# a configured header file that contains values that are accessible in the source
# code at compile-time
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/OpenSimCreatorConfig.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/OpenSimCreator/OpenSimCreatorConfig.h"
)

target_include_directories(OpenSimCreator PUBLIC

    # so that the source code can `#include <OpenSimCreator/SomeModule.h>`
    ${CMAKE_CURRENT_SOURCE_DIR}/..

    # so that source code can `#include <OpenSimCreator/OpenSimCreatorConfig.h>`
    "${CMAKE_CURRENT_BINARY_DIR}/generated/"
)

target_link_libraries(OpenSimCreator PUBLIC
    oscar_compiler_configuration
    oscar
    oscar_demos
    oscar_learnopengl

    osimCommon
    osimSimulation
    osimActuators
    osimAnalyses
    osimTools
    osimLepton
    osimExampleComponents
)

# additional compile options (on top of `oscar_compiler_configuration`)
target_compile_options(OpenSimCreator PUBLIC

    # msvc (Windows) flags
    $<$<CXX_COMPILER_ID:MSVC>:

        # disable warning 4996: Simbody uses a deprecated iterator class
        /wd4996

        # disable warning about signed/unsigned comparison mismatch
        /wd4389
    >

    # gcc/clang
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:
        # disable extra-semicolon detection: broken by OpenSim_DECLARE_ macros (see: opensim-core/3496)
        -Wno-extra-semi
    >
)
