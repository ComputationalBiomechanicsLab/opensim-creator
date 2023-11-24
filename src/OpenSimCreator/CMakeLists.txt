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

    Documents/MeshWarp/TPSDocument.hpp
    Documents/MeshWarp/TPSDocumentElementID.hpp
    Documents/MeshWarp/TPSDocumentInputElementType.hpp
    Documents/MeshWarp/TPSDocumentInputIdentifier.hpp
    Documents/MeshWarp/TPSDocumentLandmarkPair.hpp

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

    Documents/ModelGraph/BodyEl.cpp
    Documents/ModelGraph/BodyEl.hpp
    Documents/ModelGraph/CommittableModelGraph.hpp
    Documents/ModelGraph/CommittableModelGraphActions.cpp
    Documents/ModelGraph/CommittableModelGraphActions.hpp
    Documents/ModelGraph/CrossrefDescriptor.hpp
    Documents/ModelGraph/CrossrefDirection.hpp
    Documents/ModelGraph/GroundEl.cpp
    Documents/ModelGraph/GroundEl.hpp
    Documents/ModelGraph/ISceneElLookup.hpp
    Documents/ModelGraph/JointEl.cpp
    Documents/ModelGraph/JointEl.hpp
    Documents/ModelGraph/MeshEl.cpp
    Documents/ModelGraph/MeshEl.hpp
    Documents/ModelGraph/ModelCreationFlags.hpp
    Documents/ModelGraph/ModelGraph.cpp
    Documents/ModelGraph/ModelGraph.hpp
    Documents/ModelGraph/ModelGraphHelpers.cpp
    Documents/ModelGraph/ModelGraphHelpers.hpp
    Documents/ModelGraph/ModelGraphIDs.cpp
    Documents/ModelGraph/ModelGraphIDs.hpp
    Documents/ModelGraph/ModelGraphOpenSimBridge.cpp
    Documents/ModelGraph/ModelGraphOpenSimBridge.hpp
    Documents/ModelGraph/ModelGraphStrings.hpp
    Documents/ModelGraph/SceneEl.cpp
    Documents/ModelGraph/SceneEl.hpp
    Documents/ModelGraph/SceneElClass.hpp
    Documents/ModelGraph/SceneElCRTP.hpp
    Documents/ModelGraph/SceneElFlags.hpp
    Documents/ModelGraph/SceneElHelpers.cpp
    Documents/ModelGraph/SceneElHelpers.hpp
    Documents/ModelGraph/SceneElVariant.hpp
    Documents/ModelGraph/StationEl.cpp
    Documents/ModelGraph/StationEl.hpp

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

    UI/Middleware/EditorAPI.hpp
    UI/Middleware/MainUIStateAPI.hpp
    UI/Middleware/PopupAPI.hpp
    UI/Middleware/SimulatorUIAPI.hpp

    UI/Panels/CoordinateEditorPanel.cpp
    UI/Panels/CoordinateEditorPanel.hpp
    UI/Panels/ModelEditorViewerPanel.cpp
    UI/Panels/ModelEditorViewerPanel.hpp
    UI/Panels/ModelEditorViewerPanelLayer.hpp
    UI/Panels/ModelEditorViewerPanelLayerFlags.hpp
    UI/Panels/ModelEditorViewerPanelParameters.hpp
    UI/Panels/ModelEditorViewerPanelRightClickEvent.hpp
    UI/Panels/ModelEditorViewerPanelState.cpp
    UI/Panels/ModelEditorViewerPanelState.hpp
    UI/Panels/ModelMusclePlotPanel.cpp
    UI/Panels/ModelMusclePlotPanel.hpp
    UI/Panels/NavigatorPanel.cpp
    UI/Panels/NavigatorPanel.hpp
    UI/Panels/OutputPlotsPanel.cpp
    UI/Panels/OutputPlotsPanel.hpp
    UI/Panels/OutputWatchesPanel.cpp
    UI/Panels/OutputWatchesPanel.hpp
    UI/Panels/PropertiesPanel.cpp
    UI/Panels/PropertiesPanel.hpp
    UI/Panels/SelectionDetailsPanel.cpp
    UI/Panels/SelectionDetailsPanel.hpp
    UI/Panels/SimulationDetailsPanel.cpp
    UI/Panels/SimulationDetailsPanel.hpp
    UI/Panels/SimulationViewerPanel.cpp
    UI/Panels/SimulationViewerPanel.hpp
    UI/Panels/SimulationViewerPanelParameters.hpp
    UI/Panels/SimulationViewerRightClickEvent.hpp

    UI/Tabs/Experimental/MeshHittestTab.cpp
    UI/Tabs/Experimental/MeshHittestTab.hpp
    UI/Tabs/Experimental/RendererGeometryShaderTab.cpp
    UI/Tabs/Experimental/RendererGeometryShaderTab.hpp
    UI/Tabs/Experimental/TPS2DTab.cpp
    UI/Tabs/Experimental/TPS2DTab.hpp
    UI/Tabs/MeshImporter/ChooseElLayer.hpp
    UI/Tabs/MeshImporter/DrawableThing.hpp
    UI/Tabs/MeshImporter/ImportStationsFromCSVPopup.cpp
    UI/Tabs/MeshImporter/ImportStationsFromCSVPopup.hpp
    UI/Tabs/MeshImporter/MeshImporterHover.hpp
    UI/Tabs/MeshImporter/MeshImporterSharedState.hpp
    UI/Tabs/MeshImporter/MeshImporterTab.cpp
    UI/Tabs/MeshImporter/MeshImporterTab.hpp
    UI/Tabs/MeshImporter/MeshImporterUILayer.hpp
    UI/Tabs/MeshImporter/MeshImporterUILayerHost.hpp
    UI/Tabs/MeshImporter/MeshLoader.cpp
    UI/Tabs/MeshImporter/MeshLoader.hpp
    UI/Tabs/MeshImporter/Select2MeshPointsLayer.hpp
    UI/Tabs/MeshWarper/MeshWarpingTab.cpp
    UI/Tabs/MeshWarper/MeshWarpingTab.hpp
    UI/Tabs/FrameDefinitionTab.cpp
    UI/Tabs/FrameDefinitionTab.hpp
    UI/Tabs/LoadingTab.cpp
    UI/Tabs/LoadingTab.hpp
    UI/Tabs/ModelEditorTab.cpp
    UI/Tabs/ModelEditorTab.hpp
    UI/Tabs/OpenSimCreatorTabRegistry.cpp
    UI/Tabs/OpenSimCreatorTabRegistry.hpp
    UI/Tabs/PerformanceAnalyzerTab.cpp
    UI/Tabs/PerformanceAnalyzerTab.hpp
    UI/Tabs/SimulatorTab.cpp
    UI/Tabs/SimulatorTab.hpp
    UI/Tabs/SplashTab.cpp
    UI/Tabs/SplashTab.hpp

    UI/Widgets/AddBodyPopup.cpp
    UI/Widgets/AddBodyPopup.hpp
    UI/Widgets/AddComponentPopup.cpp
    UI/Widgets/AddComponentPopup.hpp
    UI/Widgets/BasicWidgets.cpp
    UI/Widgets/BasicWidgets.hpp
    UI/Widgets/ComponentContextMenu.cpp
    UI/Widgets/ComponentContextMenu.hpp
    UI/Widgets/ComponentDetails.cpp
    UI/Widgets/ComponentDetails.hpp
    UI/Widgets/EditorTabStatusBar.cpp
    UI/Widgets/EditorTabStatusBar.hpp
    UI/Widgets/ExportPointsPopup.cpp
    UI/Widgets/ExportPointsPopup.hpp
    UI/Widgets/GeometryPathPropertyEditorPopup.cpp
    UI/Widgets/GeometryPathPropertyEditorPopup.hpp
    UI/Widgets/MainMenu.cpp
    UI/Widgets/MainMenu.hpp
    UI/Widgets/ModelActionsMenuItems.cpp
    UI/Widgets/ModelActionsMenuItems.hpp
    UI/Widgets/ModelEditorMainMenu.cpp
    UI/Widgets/ModelEditorMainMenu.hpp
    UI/Widgets/ModelEditorToolbar.cpp
    UI/Widgets/ModelEditorToolbar.hpp
    UI/Widgets/ModelSelectionGizmo.cpp
    UI/Widgets/ModelSelectionGizmo.hpp
    UI/Widgets/ObjectPropertiesEditor.cpp
    UI/Widgets/ObjectPropertiesEditor.hpp
    UI/Widgets/ParamBlockEditorPopup.cpp
    UI/Widgets/ParamBlockEditorPopup.hpp
    UI/Widgets/ReassignSocketPopup.cpp
    UI/Widgets/ReassignSocketPopup.hpp
    UI/Widgets/Select1PFPopup.cpp
    UI/Widgets/Select1PFPopup.hpp
    UI/Widgets/SelectComponentPopup.cpp
    UI/Widgets/SelectComponentPopup.hpp
    UI/Widgets/SelectGeometryPopup.cpp
    UI/Widgets/SelectGeometryPopup.hpp
    UI/Widgets/SimulationOutputPlot.cpp
    UI/Widgets/SimulationOutputPlot.hpp
    UI/Widgets/SimulationScrubber.cpp
    UI/Widgets/SimulationScrubber.hpp
    UI/Widgets/SimulationToolbar.cpp
    UI/Widgets/SimulationToolbar.hpp
    UI/Widgets/UiModelViewer.cpp
    UI/Widgets/UiModelViewer.hpp
    UI/Widgets/VirtualModelStatePairContextMenu.cpp
    UI/Widgets/VirtualModelStatePairContextMenu.hpp

    UI/MainUIScreen.cpp
    UI/MainUIScreen.hpp

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
