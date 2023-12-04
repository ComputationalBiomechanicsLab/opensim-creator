find_package(OpenSim REQUIRED CONFIG)

add_library(OpenSimCreator STATIC

    ComponentRegistry/ComponentRegistry.hpp
    ComponentRegistry/ComponentRegistryBase.cpp
    ComponentRegistry/ComponentRegistryBase.hpp
    ComponentRegistry/ComponentRegistryEntry.hpp
    ComponentRegistry/ComponentRegistryEntryBase.cpp
    ComponentRegistry/ComponentRegistryEntryBase.hpp
    ComponentRegistry/StaticComponentRegistries.cpp
    ComponentRegistry/StaticComponentRegistries.hpp

    Documents/MeshImporter/Body.cpp
    Documents/MeshImporter/Body.hpp
    Documents/MeshImporter/CrossrefDescriptor.hpp
    Documents/MeshImporter/CrossrefDirection.hpp
    Documents/MeshImporter/Document.cpp
    Documents/MeshImporter/Document.hpp
    Documents/MeshImporter/DocumentHelpers.cpp
    Documents/MeshImporter/DocumentHelpers.hpp
    Documents/MeshImporter/Ground.cpp
    Documents/MeshImporter/Ground.hpp
    Documents/MeshImporter/IObjectFinder.hpp
    Documents/MeshImporter/Joint.cpp
    Documents/MeshImporter/Joint.hpp
    Documents/MeshImporter/Mesh.cpp
    Documents/MeshImporter/Mesh.hpp
    Documents/MeshImporter/MIClass.hpp
    Documents/MeshImporter/MIIDs.cpp
    Documents/MeshImporter/MIObject.cpp
    Documents/MeshImporter/MIObject.hpp
    Documents/MeshImporter/MIObjectCRTP.hpp
    Documents/MeshImporter/MIObjectFlags.hpp
    Documents/MeshImporter/MIObjectHelpers.cpp
    Documents/MeshImporter/MIObjectHelpers.hpp
    Documents/MeshImporter/MIIDs.hpp
    Documents/MeshImporter/MIStrings.hpp
    Documents/MeshImporter/MIVariant.hpp
    Documents/MeshImporter/OpenSimBridge.cpp
    Documents/MeshImporter/OpenSimBridge.hpp
    Documents/MeshImporter/OpenSimExportFlags.hpp
    Documents/MeshImporter/Station.cpp
    Documents/MeshImporter/Station.hpp
    Documents/MeshImporter/UndoableActions.cpp
    Documents/MeshImporter/UndoableActions.hpp
    Documents/MeshImporter/UndoableDocument.hpp

    Documents/MeshWarper/NamedLandmarkPair3D.hpp
    Documents/MeshWarper/TPSDocument.cpp
    Documents/MeshWarper/TPSDocument.hpp
    Documents/MeshWarper/TPSDocumentCSVFlags.hpp
    Documents/MeshWarper/TPSDocumentElement.hpp
    Documents/MeshWarper/TPSDocumentElementID.hpp
    Documents/MeshWarper/TPSDocumentElementType.hpp
    Documents/MeshWarper/TPSDocumentHelpers.cpp
    Documents/MeshWarper/TPSDocumentHelpers.hpp
    Documents/MeshWarper/TPSDocumentInputIdentifier.hpp
    Documents/MeshWarper/TPSDocumentLandmarkPair.hpp
    Documents/MeshWarper/TPSDocumentNonParticipatingLandmark.hpp
    Documents/MeshWarper/TPSWarpResultCache.hpp
    Documents/MeshWarper/UndoableTPSDocument.hpp
    Documents/MeshWarper/UndoableTPSDocumentActions.cpp
    Documents/MeshWarper/UndoableTPSDocumentActions.hpp

    Documents/Model/BasicModelStatePair.cpp
    Documents/Model/BasicModelStatePair.hpp
    Documents/Model/ModelStateCommit.cpp
    Documents/Model/ModelStateCommit.hpp
    Documents/Model/ModelStatePairInfo.cpp
    Documents/Model/ModelStatePairInfo.hpp
    Documents/Model/ObjectPropertyEdit.cpp
    Documents/Model/ObjectPropertyEdit.hpp
    Documents/Model/UndoableModelActions.cpp
    Documents/Model/UndoableModelActions.hpp
    Documents/Model/UndoableModelStatePair.cpp
    Documents/Model/UndoableModelStatePair.hpp
    Documents/Model/VirtualConstModelStatePair.hpp
    Documents/Model/VirtualModelStatePair.hpp

    Documents/ModelWarper/Document.cpp
    Documents/ModelWarper/Document.hpp

    Documents/Simulation/ForwardDynamicSimulation.cpp
    Documents/Simulation/ForwardDynamicSimulation.hpp
    Documents/Simulation/ForwardDynamicSimulator.cpp
    Documents/Simulation/ForwardDynamicSimulator.hpp
    Documents/Simulation/ForwardDynamicSimulatorParams.cpp
    Documents/Simulation/ForwardDynamicSimulatorParams.hpp
    Documents/Simulation/IntegratorMethod.cpp
    Documents/Simulation/IntegratorMethod.hpp
    Documents/Simulation/Simulation.hpp
    Documents/Simulation/SimulationClock.hpp
    Documents/Simulation/SimulationModelStatePair.cpp
    Documents/Simulation/SimulationModelStatePair.hpp
    Documents/Simulation/SimulationReport.cpp
    Documents/Simulation/SimulationReport.hpp
    Documents/Simulation/SimulationStatus.cpp
    Documents/Simulation/SimulationStatus.hpp
    Documents/Simulation/SingleStateSimulation.cpp
    Documents/Simulation/SingleStateSimulation.hpp
    Documents/Simulation/StoFileSimulation.hpp
    Documents/Simulation/StoFileSimulation.cpp
    Documents/Simulation/VirtualSimulation.hpp

    Graphics/CachedModelRenderer.cpp
    Graphics/CachedModelRenderer.hpp
    Graphics/ComponentAbsPathDeocrationTagger.cpp
    Graphics/ComponentAbsPathDecorationTagger.hpp
    Graphics/ComponentSceneDecorationFlagsTagger.cpp
    Graphics/ComponentSceneDecorationFlagsTagger.hpp
    Graphics/CustomRenderingOptionFlags.cpp
    Graphics/CustomRenderingOptionFlags.hpp
    Graphics/CustomRenderingOptions.cpp
    Graphics/CustomRenderingOptions.hpp
    Graphics/OpenSimDecorationOptions.cpp
    Graphics/OpenSimDecorationOptions.hpp
    Graphics/ModelRendererParams.cpp
    Graphics/ModelRendererParams.hpp
    Graphics/MuscleColoringStyle.cpp
    Graphics/MuscleColoringStyle.hpp
    Graphics/MuscleDecorationStyle.cpp
    Graphics/MuscleDecorationStyle.hpp
    Graphics/MuscleSizingStyle.cpp
    Graphics/MuscleSizingStyle.hpp
    Graphics/OpenSimDecorationGenerator.cpp
    Graphics/OpenSimDecorationGenerator.hpp
    Graphics/OpenSimDecorationOptionFlags.cpp
    Graphics/OpenSimDecorationOptionFlags.hpp
    Graphics/OpenSimGraphicsHelpers.cpp
    Graphics/OpenSimGraphicsHelpers.hpp
    Graphics/OverlayDecorationGenerator.cpp
    Graphics/OverlayDecorationGenerator.hpp
    Graphics/OverlayDecorationOptionFlags.cpp
    Graphics/OverlayDecorationOptionFlags.hpp
    Graphics/OverlayDecorationOptions.cpp
    Graphics/OverlayDecorationOptions.hpp
    Graphics/SimTKDecorationGenerator.cpp
    Graphics/SimTKDecorationGenerator.hpp
    Graphics/SimTKMeshLoader.cpp
    Graphics/SimTKMeshLoader.hpp

    OutputExtractors/ComponentOutputExtractor.cpp
    OutputExtractors/ComponentOutputExtractor.hpp
    OutputExtractors/IntegratorOutputExtractor.cpp
    OutputExtractors/IntegratorOutputExtractor.hpp
    OutputExtractors/MultiBodySystemOutputExtractor.cpp
    OutputExtractors/MultiBodySystemOutputExtractor.hpp
    OutputExtractors/OutputExtractor.cpp
    OutputExtractors/OutputExtractor.hpp
    OutputExtractors/VirtualOutputExtractor.hpp

    Platform/OpenSimCreatorApp.cpp
    Platform/OpenSimCreatorApp.hpp
    Platform/RecentFile.hpp
    Platform/RecentFiles.cpp
    Platform/RecentFiles.hpp

    UI/Experimental/MeshHittestTab.cpp
    UI/Experimental/MeshHittestTab.hpp
    UI/Experimental/RendererGeometryShaderTab.cpp
    UI/Experimental/RendererGeometryShaderTab.hpp
    UI/Experimental/TPS2DTab.cpp
    UI/Experimental/TPS2DTab.hpp

    UI/MeshImporter/ChooseElLayer.hpp
    UI/MeshImporter/DrawableThing.hpp
    UI/MeshImporter/ImportStationsFromCSVPopup.cpp
    UI/MeshImporter/ImportStationsFromCSVPopup.hpp
    UI/MeshImporter/MeshImporterHover.hpp
    UI/MeshImporter/MeshImporterSharedState.hpp
    UI/MeshImporter/MeshImporterTab.cpp
    UI/MeshImporter/MeshImporterTab.hpp
    UI/MeshImporter/MeshImporterUILayer.hpp
    UI/MeshImporter/MeshImporterUILayerHost.hpp
    UI/MeshImporter/MeshLoader.cpp
    UI/MeshImporter/MeshLoader.hpp
    UI/MeshImporter/Select2MeshPointsLayer.hpp

    UI/MeshWarper/MeshWarpingTab.cpp
    UI/MeshWarper/MeshWarpingTab.hpp
    UI/MeshWarper/MeshWarpingTabActionsMenu.hpp
    UI/MeshWarper/MeshWarpingTabContextMenu.hpp
    UI/MeshWarper/MeshWarpingTabDecorationGenerators.hpp
    UI/MeshWarper/MeshWarpingTabEditMenu.hpp
    UI/MeshWarper/MeshWarpingTabFileMenu.hpp
    UI/MeshWarper/MeshWarpingTabHover.hpp
    UI/MeshWarper/MeshWarpingTabInputMeshPanel.hpp
    UI/MeshWarper/MeshWarpingTabMainMenu.hpp
    UI/MeshWarper/MeshWarpingTabNavigatorPanel.hpp
    UI/MeshWarper/MeshWarpingTabPanel.hpp
    UI/MeshWarper/MeshWarpingTabResultMeshPanel.hpp
    UI/MeshWarper/MeshWarpingTabSharedState.hpp
    UI/MeshWarper/MeshWarpingTabStatusBar.hpp
    UI/MeshWarper/MeshWarpingTabToolbar.hpp
    UI/MeshWarper/MeshWarpingTabUserSelection.hpp

    UI/ModelEditor/AddBodyPopup.cpp
    UI/ModelEditor/AddBodyPopup.hpp
    UI/ModelEditor/AddComponentPopup.cpp
    UI/ModelEditor/AddComponentPopup.hpp
    UI/ModelEditor/ComponentContextMenu.cpp
    UI/ModelEditor/ComponentContextMenu.hpp
    UI/ModelEditor/CoordinateEditorPanel.cpp
    UI/ModelEditor/CoordinateEditorPanel.hpp
    UI/ModelEditor/EditorAPI.hpp
    UI/ModelEditor/EditorTabStatusBar.cpp
    UI/ModelEditor/EditorTabStatusBar.hpp
    UI/ModelEditor/ExportPointsPopup.cpp
    UI/ModelEditor/ExportPointsPopup.hpp
    UI/ModelEditor/ModelActionsMenuItems.cpp
    UI/ModelEditor/ModelActionsMenuItems.hpp
    UI/ModelEditor/ModelEditorTab.cpp
    UI/ModelEditor/ModelEditorTab.hpp
    UI/ModelEditor/ModelEditorMainMenu.cpp
    UI/ModelEditor/ModelEditorMainMenu.hpp
    UI/ModelEditor/ModelEditorToolbar.cpp
    UI/ModelEditor/ModelEditorToolbar.hpp
    UI/ModelEditor/ModelMusclePlotPanel.cpp
    UI/ModelEditor/ModelMusclePlotPanel.hpp
    UI/ModelEditor/OutputWatchesPanel.cpp
    UI/ModelEditor/OutputWatchesPanel.hpp
    UI/ModelEditor/ReassignSocketPopup.cpp
    UI/ModelEditor/ReassignSocketPopup.hpp
    UI/ModelEditor/SelectComponentPopup.cpp
    UI/ModelEditor/SelectComponentPopup.hpp
    UI/ModelEditor/SelectGeometryPopup.cpp
    UI/ModelEditor/SelectGeometryPopup.hpp
    UI/ModelEditor/Select1PFPopup.cpp
    UI/ModelEditor/Select1PFPopup.hpp

    UI/ModelWarper/ChecklistPanel.cpp
    UI/ModelWarper/ChecklistPanel.hpp
    UI/ModelWarper/FileMenu.cpp
    UI/ModelWarper/FileMenu.hpp
    UI/ModelWarper/MainMenu.hpp
    UI/ModelWarper/ModelViewerPanel.cpp
    UI/ModelWarper/ModelViewerPanel.hpp
    UI/ModelWarper/ModelWarperTab.cpp
    UI/ModelWarper/ModelWarperTab.hpp
    UI/ModelWarper/Toolbar.cpp
    UI/ModelWarper/Toolbar.hpp
    UI/ModelWarper/UIState.hpp

    UI/Shared/BasicWidgets.cpp
    UI/Shared/BasicWidgets.hpp
    UI/Shared/GeometryPathPropertyEditorPopup.cpp
    UI/Shared/GeometryPathPropertyEditorPopup.hpp

    UI/Shared/MainMenu.cpp
    UI/Shared/MainMenu.hpp
    UI/Shared/ModelEditorViewerPanel.cpp
    UI/Shared/ModelEditorViewerPanel.hpp
    UI/Shared/ModelEditorViewerPanelLayer.hpp
    UI/Shared/ModelEditorViewerPanelLayerFlags.hpp
    UI/Shared/ModelEditorViewerPanelParameters.hpp
    UI/Shared/ModelEditorViewerPanelRightClickEvent.hpp
    UI/Shared/ModelEditorViewerPanelState.cpp
    UI/Shared/ModelEditorViewerPanelState.hpp
    UI/Shared/ModelSelectionGizmo.cpp
    UI/Shared/ModelSelectionGizmo.hpp
    UI/Shared/NavigatorPanel.cpp
    UI/Shared/NavigatorPanel.hpp
    UI/Shared/ObjectPropertiesEditor.cpp
    UI/Shared/ObjectPropertiesEditor.hpp
    UI/Shared/ParamBlockEditorPopup.cpp
    UI/Shared/ParamBlockEditorPopup.hpp
    UI/Shared/PropertiesPanel.cpp
    UI/Shared/PropertiesPanel.hpp

    UI/Simulation/ComponentDetails.cpp
    UI/Simulation/ComponentDetails.hpp
    UI/Simulation/OutputPlotsPanel.cpp
    UI/Simulation/OutputPlotsPanel.hpp
    UI/Simulation/SelectionDetailsPanel.cpp
    UI/Simulation/SimulationDetailsPanel.cpp
    UI/Simulation/SimulationDetailsPanel.hpp
    UI/Simulation/SimulationOutputPlot.cpp
    UI/Simulation/SimulationOutputPlot.hpp
    UI/Simulation/SimulationScrubber.cpp
    UI/Simulation/SimulationScrubber.hpp
    UI/Simulation/SimulationToolbar.cpp
    UI/Simulation/SimulationToolbar.hpp
    UI/Simulation/SelectionDetailsPanel.hpp
    UI/Simulation/SimulationViewerPanel.cpp
    UI/Simulation/SimulationViewerPanel.hpp
    UI/Simulation/SimulationViewerPanelParameters.hpp
    UI/Simulation/SimulationViewerRightClickEvent.hpp
    UI/Simulation/SimulatorTab.cpp
    UI/Simulation/SimulatorTab.hpp
    UI/Simulation/SimulatorUIAPI.hpp
    UI/Simulation/VirtualModelStatePairContextMenu.cpp
    UI/Simulation/VirtualModelStatePairContextMenu.hpp
    UI/Simulation/UiModelViewer.cpp
    UI/Simulation/UiModelViewer.hpp

    UI/FrameDefinitionTab.cpp
    UI/FrameDefinitionTab.hpp
    UI/LoadingTab.cpp
    UI/LoadingTab.hpp
    UI/MainUIStateAPI.hpp
    UI/MainUIScreen.cpp
    UI/MainUIScreen.hpp
    UI/OpenSimCreatorTabRegistry.cpp
    UI/OpenSimCreatorTabRegistry.hpp
    UI/PerformanceAnalyzerTab.cpp
    UI/PerformanceAnalyzerTab.hpp
    UI/PopupAPI.hpp
    UI/SplashTab.cpp
    UI/SplashTab.hpp

    Utils/LandmarkPair3D.cpp
    Utils/LandmarkPair3D.hpp
    Utils/OpenSimHelpers.cpp
    Utils/OpenSimHelpers.hpp
    Utils/ParamBlock.hpp
    Utils/ParamBlock.cpp
    Utils/ParamValue.hpp
    Utils/ShapeFitters.cpp
    Utils/ShapeFitters.hpp
    Utils/SimTKHelpers.cpp
    Utils/SimTKHelpers.hpp
    Utils/TPS3D.cpp
    Utils/TPS3D.hpp
)

# OpenSimCreatorConfig.hpp
#
# a configured header file that contains values that are accessible in the source
# code at compile-time
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/OpenSimCreatorConfig.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/OpenSimCreator/OpenSimCreatorConfig.hpp"
)

target_include_directories(OpenSimCreator PUBLIC

    # so that the source code can `#include <OpenSimCreator/SomeModule.hpp>`
    ${CMAKE_CURRENT_SOURCE_DIR}/..

    # so that source code can `#include <OpenSimCreator/OpenSimCreatorConfig.hpp>`
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

# HACK: fix simbody not linking all necessary libraries
if(WIN32)
    # https://stackoverflow.com/questions/71250816/cmake-how-to-handle-dependencies-of-imported-library-targets-with-target-runti

    add_library(hack_libgcc_s_sjlj-1 SHARED IMPORTED)
    set_target_properties(hack_libgcc_s_sjlj-1 PROPERTIES
        IMPORTED_IMPLIB "${Simbody_LIB_DIR}/libblas.lib"
        IMPORTED_LOCATION "${Simbody_BIN_DIR}/libgcc_s_sjlj-1.dll"
    )

    add_library(hack_libgfortran-3 SHARED IMPORTED)
    set_target_properties(hack_libgfortran-3 PROPERTIES
        IMPORTED_IMPLIB "${Simbody_LIB_DIR}/libblas.lib"
        IMPORTED_LOCATION "${Simbody_BIN_DIR}/libgfortran-3.dll"
    )

    add_library(hack_libquadmath-0 SHARED IMPORTED)
    set_target_properties(hack_libquadmath-0 PROPERTIES
        IMPORTED_IMPLIB "${Simbody_LIB_DIR}/libblas.lib"
        IMPORTED_LOCATION "${Simbody_BIN_DIR}/libquadmath-0.dll"
    )

    target_link_libraries(OpenSimCreator PUBLIC
        hack_libgcc_s_sjlj-1
        hack_libgfortran-3
        hack_libquadmath-0
    )
endif()

# additional compile options (on top of `oscar_compiler_configuration`)
target_compile_options(OpenSimCreator PUBLIC

    # msvc (Windows) flags
    $<$<CXX_COMPILER_ID:MSVC>:

        # disable warning 4996: Simbody uses a deprecated iterator class
        /wd4996

        # disable "assignment within conditional expression" warning
        #
        # it's detected in `OpenSim/Common/Component.h:2521
        /wd4706

        # disable warning about signed/unsigned comparison mismatch
        /wd4389
    >

    # gcc/clang
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:
        # disable extra-semicolon detection: broken by OpenSim_DECLARE_ macros (see: opensim-core/3496)
        -Wno-extra-semi
    >
)
