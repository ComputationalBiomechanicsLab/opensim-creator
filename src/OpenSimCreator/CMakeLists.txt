find_package(OpenSim REQUIRED CONFIG)

# [lib]OpenSimCreator:
#
#     library code behind the OpenSim Creator application
add_library(OpenSimCreator STATIC

    FrameDefinition/FrameDefinitionTab.cpp
    FrameDefinition/FrameDefinitionTab.hpp

    Graphics/CachedModelRenderer.cpp
    Graphics/CachedModelRenderer.hpp
    Graphics/CustomDecorationOptions.cpp
    Graphics/CustomDecorationOptions.hpp
    Graphics/CustomRenderingOptions.cpp
    Graphics/CustomRenderingOptions.hpp
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
    Graphics/SimTKDecorationGenerator.cpp
    Graphics/SimTKDecorationGenerator.hpp
    Graphics/SimTKMeshLoader.cpp
    Graphics/SimTKMeshLoader.hpp

    MiddlewareAPIs/EditorAPI.hpp
    MiddlewareAPIs/MainUIStateAPI.hpp
    MiddlewareAPIs/SimulatorUIAPI.hpp

    Panels/CoordinateEditorPanel.cpp
    Panels/CoordinateEditorPanel.hpp
    Panels/ModelEditorViewerPanel.cpp
    Panels/ModelEditorViewerPanel.hpp
    Panels/ModelEditorViewerPanelRightClickEvent.hpp
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
    BasicModelStatePair.cpp
    BasicModelStatePair.hpp
    ComponentOutputExtractor.cpp
    ComponentOutputExtractor.hpp
    ForwardDynamicSimulation.cpp
    ForwardDynamicSimulation.hpp
    ForwardDynamicSimulator.cpp
    ForwardDynamicSimulator.hpp
    ForwardDynamicSimulatorParams.cpp
    ForwardDynamicSimulatorParams.hpp
    IntegratorMethod.cpp
    IntegratorMethod.hpp
    IntegratorOutputExtractor.cpp
    IntegratorOutputExtractor.hpp
    ModelStateCommit.cpp
    ModelStateCommit.hpp
    MultiBodySystemOutputExtractor.cpp
    MultiBodySystemOutputExtractor.hpp
    ObjectPropertyEdit.cpp
    ObjectPropertyEdit.hpp
    OpenSimApp.cpp
    OpenSimApp.hpp
    OpenSimHelpers.cpp
    OpenSimHelpers.hpp
    OutputExtractor.cpp
    OutputExtractor.hpp
    ParamBlock.hpp
    ParamBlock.cpp
    ParamValue.hpp
    SimTKHelpers.cpp
    SimTKHelpers.hpp
    Simulation.hpp
    SimulationClock.hpp
    SimulationModelStatePair.cpp
    SimulationModelStatePair.hpp
    SimulationReport.cpp
    SimulationReport.hpp
    SimulationStatus.cpp
    SimulationStatus.hpp
    SingleStateSimulation.cpp
    SingleStateSimulation.hpp
    StoFileSimulation.hpp
    StoFileSimulation.cpp
    TPS3D.cpp
    TPS3D.hpp
    TypeRegistry.cpp
    TypeRegistry.hpp
    UndoableModelStatePair.cpp
    UndoableModelStatePair.hpp
    VirtualConstModelStatePair.hpp
    VirtualModelStatePair.hpp
    VirtualOutputExtractor.hpp
    VirtualSimulation.hpp
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
