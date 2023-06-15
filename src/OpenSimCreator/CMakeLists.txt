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
    Graphics/ComponentAbsPathDecorationTagger.hpp
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

    MiddlewareAPIs/EditorAPI.hpp
    MiddlewareAPIs/MainUIStateAPI.hpp
    MiddlewareAPIs/SimulatorUIAPI.hpp

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

    Panels/CoordinateEditorPanel.cpp
    Panels/CoordinateEditorPanel.hpp
    Panels/ModelEditorViewerPanel.cpp
    Panels/ModelEditorViewerPanel.hpp
    Panels/ModelEditorViewerPanelLayer.hpp
    Panels/ModelEditorViewerPanelLayerFlags.hpp
    Panels/ModelEditorViewerPanelParameters.hpp
    Panels/ModelEditorViewerPanelRightClickEvent.hpp
    Panels/ModelEditorViewerPanelState.cpp
    Panels/ModelEditorViewerPanelState.hpp
    Panels/ModelMusclePlotPanel.cpp
    Panels/ModelMusclePlotPanel.hpp
    Panels/NavigatorPanel.cpp
    Panels/NavigatorPanel.hpp
    Panels/OutputPlotsPanel.cpp
    Panels/OutputPlotsPanel.hpp
    Panels/OutputWatchesPanel.cpp
    Panels/OutputWatchesPanel.hpp
    Panels/PropertiesPanel.cpp
    Panels/PropertiesPanel.hpp
    Panels/SelectionDetailsPanel.cpp
    Panels/SelectionDetailsPanel.hpp
    Panels/SimulationDetailsPanel.cpp
    Panels/SimulationDetailsPanel.hpp
    Panels/SimulationViewerPanel.cpp
    Panels/SimulationViewerPanel.hpp

    Screens/MainUIScreen.cpp
    Screens/MainUIScreen.hpp

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

    Tabs/Experimental/MeshHittestTab.cpp
    Tabs/Experimental/MeshHittestTab.hpp
    Tabs/Experimental/ModelWarpingTab.cpp
    Tabs/Experimental/ModelWarpingTab.hpp
    Tabs/Experimental/PreviewExperimentalDataTab.cpp
    Tabs/Experimental/PreviewExperimentalDataTab.hpp
    Tabs/Experimental/RendererGeometryShaderTab.cpp
    Tabs/Experimental/RendererGeometryShaderTab.hpp
    Tabs/Experimental/TPS2DTab.cpp
    Tabs/Experimental/TPS2DTab.hpp
    Tabs/Experimental/TPS3DTab.cpp
    Tabs/Experimental/TPS3DTab.hpp
    Tabs/Experimental/ExcitationEditorTab.cpp
    Tabs/Experimental/ExcitationEditorTab.hpp
    Tabs/FrameDefinitionTab.cpp
    Tabs/FrameDefinitionTab.hpp
    Tabs/LoadingTab.cpp
    Tabs/LoadingTab.hpp
    Tabs/MeshImporterTab.cpp
    Tabs/MeshImporterTab.hpp
    Tabs/ModelEditorTab.cpp
    Tabs/ModelEditorTab.hpp
    Tabs/PerformanceAnalyzerTab.cpp
    Tabs/PerformanceAnalyzerTab.hpp
    Tabs/SimulatorTab.cpp
    Tabs/SimulatorTab.hpp
    Tabs/SplashTab.cpp
    Tabs/SplashTab.hpp

    Utils/OpenSimHelpers.cpp
    Utils/OpenSimHelpers.hpp
    Utils/ParamBlock.hpp
    Utils/ParamBlock.cpp
    Utils/ParamValue.hpp
    Utils/TPS3D.cpp
    Utils/TPS3D.hpp

    Widgets/AddBodyPopup.cpp
    Widgets/AddBodyPopup.hpp
    Widgets/AddComponentPopup.cpp
    Widgets/AddComponentPopup.hpp
    Widgets/BasicWidgets.cpp
    Widgets/BasicWidgets.hpp
    Widgets/ComponentContextMenu.cpp
    Widgets/ComponentContextMenu.hpp
    Widgets/ComponentDetails.cpp
    Widgets/ComponentDetails.hpp
    Widgets/EditorTabStatusBar.cpp
    Widgets/EditorTabStatusBar.hpp
    Widgets/GeometryPathPropertyEditorPopup.cpp
    Widgets/GeometryPathPropertyEditorPopup.hpp
    Widgets/MainMenu.cpp
    Widgets/MainMenu.hpp
    Widgets/ModelActionsMenuItems.cpp
    Widgets/ModelActionsMenuItems.hpp
    Widgets/ModelEditorMainMenu.cpp
    Widgets/ModelEditorMainMenu.hpp
    Widgets/ModelEditorToolbar.cpp
    Widgets/ModelEditorToolbar.hpp
    Widgets/ModelSelectionGizmo.cpp
    Widgets/ModelSelectionGizmo.hpp
    Widgets/ObjectPropertiesEditor.cpp
    Widgets/ObjectPropertiesEditor.hpp
    Widgets/ParamBlockEditorPopup.cpp
    Widgets/ParamBlockEditorPopup.hpp
    Widgets/ReassignSocketPopup.cpp
    Widgets/ReassignSocketPopup.hpp
    Widgets/Select1PFPopup.cpp
    Widgets/Select1PFPopup.hpp
    Widgets/SelectComponentPopup.cpp
    Widgets/SelectComponentPopup.hpp
    Widgets/SelectGeometryPopup.cpp
    Widgets/SelectGeometryPopup.hpp
    Widgets/SimulationOutputPlot.cpp
    Widgets/SimulationOutputPlot.hpp
    Widgets/SimulationScrubber.cpp
    Widgets/SimulationScrubber.hpp
    Widgets/SimulationToolbar.cpp
    Widgets/SimulationToolbar.hpp
    Widgets/UiModelViewer.cpp
    Widgets/UiModelViewer.hpp

    ActionFunctions.cpp
    ActionFunctions.hpp
    OpenSimApp.cpp
    OpenSimApp.hpp
    TypeRegistry.cpp
    TypeRegistry.hpp
)

# OpenSimCreator: compile-time includes
target_include_directories(OpenSimCreator PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# OpenSimCreator: libraries
target_link_libraries(OpenSimCreator PUBLIC
    oscar_configuration
    oscar
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

        # turn warning level up to "production grade" (note: /W4 explodes because of Simbody)
        /W3
    >
)

# OpenSimCreator: link options
target_link_options(OpenSimCreator PUBLIC

    # MSVC (Windows)
    $<$<CXX_COMPILER_ID:MSVC>:

        # ignore error from ImGui, which uses locally-defined symbols
        /ignore:4217
    >

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
