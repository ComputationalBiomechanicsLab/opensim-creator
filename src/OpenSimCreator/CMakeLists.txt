find_package(OpenSim REQUIRED CONFIG)

# [lib]OpenSimCreator:
#
#     library code behind the OpenSim Creator application
add_library(OpenSimCreator STATIC

    Bindings/SimTKDecorationGenerator.cpp
    Bindings/SimTKDecorationGenerator.hpp
    Bindings/SimTKMeshLoader.cpp
    Bindings/SimTKMeshLoader.hpp
    Bindings/SimTKHelpers.cpp
    Bindings/SimTKHelpers.hpp

    Graphics/CachedModelRenderer.cpp
    Graphics/CachedModelRenderer.hpp
    Graphics/ComponentAbsPathDeocrationTagger.cpp
    Graphics/ComponentAbsPathDecorationTagger.hpp
    Graphics/ComponentSceneDecorationFlagsTagger.cpp
    Graphics/ComponentSceneDecorationFlagsTagger.hpp
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
    Graphics/OpenSimGraphicsHelpers.cpp
    Graphics/OpenSimGraphicsHelpers.hpp
    Graphics/OverlayDecorationGenerator.cpp
    Graphics/OverlayDecorationGenerator.hpp
    Graphics/OverlayDecorationOptions.cpp
    Graphics/OverlayDecorationOptions.hpp

    Model/BasicModelStatePair.cpp
    Model/BasicModelStatePair.hpp
    Model/ModelStateCommit.cpp
    Model/ModelStateCommit.hpp
    Model/ModelStatePairInfo.cpp
    Model/ModelStatePairInfo.hpp
    Model/ObjectPropertyEdit.cpp
    Model/ObjectPropertyEdit.hpp
    Model/UndoableModelStatePair.cpp
    Model/UndoableModelStatePair.hpp
    Model/VirtualConstModelStatePair.hpp
    Model/VirtualModelStatePair.hpp

    Outputs/ComponentOutputExtractor.cpp
    Outputs/ComponentOutputExtractor.hpp
    Outputs/IntegratorOutputExtractor.cpp
    Outputs/IntegratorOutputExtractor.hpp
    Outputs/MultiBodySystemOutputExtractor.cpp
    Outputs/MultiBodySystemOutputExtractor.hpp
    Outputs/OutputExtractor.cpp
    Outputs/OutputExtractor.hpp
    Outputs/VirtualOutputExtractor.hpp

    Registry/ComponentRegistry.hpp
    Registry/ComponentRegistryBase.cpp
    Registry/ComponentRegistryBase.hpp
    Registry/ComponentRegistryEntry.hpp
    Registry/ComponentRegistryEntryBase.cpp
    Registry/ComponentRegistryEntryBase.hpp
    Registry/StaticComponentRegistries.cpp
    Registry/StaticComponentRegistries.hpp

    Simulation/ForwardDynamicSimulation.cpp
    Simulation/ForwardDynamicSimulation.hpp
    Simulation/ForwardDynamicSimulator.cpp
    Simulation/ForwardDynamicSimulator.hpp
    Simulation/ForwardDynamicSimulatorParams.cpp
    Simulation/ForwardDynamicSimulatorParams.hpp
    Simulation/IntegratorMethod.cpp
    Simulation/IntegratorMethod.hpp
    Simulation/Simulation.hpp
    Simulation/SimulationClock.hpp
    Simulation/SimulationModelStatePair.cpp
    Simulation/SimulationModelStatePair.hpp
    Simulation/SimulationReport.cpp
    Simulation/SimulationReport.hpp
    Simulation/SimulationStatus.cpp
    Simulation/SimulationStatus.hpp
    Simulation/SingleStateSimulation.cpp
    Simulation/SingleStateSimulation.hpp
    Simulation/StoFileSimulation.hpp
    Simulation/StoFileSimulation.cpp
    Simulation/VirtualSimulation.hpp

    UI/Middleware/EditorAPI.hpp
    UI/Middleware/MainUIStateAPI.hpp
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

    UI/Screens/MainUIScreen.cpp
    UI/Screens/MainUIScreen.hpp

    UI/Tabs/Experimental/MeshHittestTab.cpp
    UI/Tabs/Experimental/MeshHittestTab.hpp
    UI/Tabs/Experimental/RendererGeometryShaderTab.cpp
    UI/Tabs/Experimental/RendererGeometryShaderTab.hpp
    UI/Tabs/Experimental/TPS2DTab.cpp
    UI/Tabs/Experimental/TPS2DTab.hpp
    UI/Tabs/FrameDefinitionTab.cpp
    UI/Tabs/FrameDefinitionTab.hpp
    UI/Tabs/LoadingTab.cpp
    UI/Tabs/LoadingTab.hpp
    UI/Tabs/MeshImporterTab.cpp
    UI/Tabs/MeshImporterTab.hpp
    UI/Tabs/MeshWarpingTab.cpp
    UI/Tabs/MeshWarpingTab.hpp
    UI/Tabs/ModelEditorTab.cpp
    UI/Tabs/ModelEditorTab.hpp
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

    Utils/OpenSimHelpers.cpp
    Utils/OpenSimHelpers.hpp
    Utils/ParamBlock.hpp
    Utils/ParamBlock.cpp
    Utils/ParamValue.hpp
    Utils/TPS3D.cpp
    Utils/TPS3D.hpp
    Utils/UndoableModelActions.cpp
    Utils/UndoableModelActions.hpp

    OpenSimCreatorApp.cpp
    OpenSimCreatorApp.hpp
 )

# OpenSimCreator: compile-time includes
target_include_directories(OpenSimCreator PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# OpenSimCreator: libraries
target_link_libraries(OpenSimCreator PUBLIC
    oscar_configuration
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

# OpenSimCreator: link options
target_link_options(OpenSimCreator PUBLIC

    # Linux /w GCC or Clang
    $<$<AND:$<BOOL:LINUX>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>>:

        # make `ld` emit symbols for all functions - even if they aren't
        # externally used (with an exe, only `main` is used, really)
        #
        # this makes the binary a little bigger (~400 KB) but means
        # that the application can emit its own stack trace /w function
        # names - even in Release mode.
        -rdynamic
    >

    # GCC or Clang: handle OSC_USE_ASAN
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:
        $<$<BOOL:${OSC_USE_ASAN}>:-fsanitize=address>
    >
)
