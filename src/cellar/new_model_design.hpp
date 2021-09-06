#pragma once

namespace osc {
    class UiModel final {
        std::unique_ptr<OpenSim::Model> m_Model;
        std::unique_ptr<SimTK::State> m_State;
        Scene_decorations m_Decorations;
        OpenSim::Component* m_Selected;
        OpenSim::Component* m_Hovered;
        OpenSim::Component* m_Isolated;
        std::chrono::system_clock::time_point m_Timestamp;
        bool m_ModelOrSubComponentIsDirty;
        bool m_StateIsDirty;
        bool m_DecorationsAreDirty;

    public:
        UiModel();
        UiModel(std::unique_ptr<OpenSim::Model>);

        OpenSim::Model const& getModel() const;
        OpenSim::Model& updModel();
        void setModelDirty();
        void unsetModelDirty();

        SimTK::State const& getState() const;
        SimTK::State& updState();
        void setStateDirty();
        void unsetStateDirty();

        Scene_decorations const& getDecorations() const;
        Scene_decorations& updDecorations();
        void setDecorationsDirty();
        void unsetDecorationsDirty();

        OpenSim::Component const* getSelection() const;
        OpenSim::Componnet* updSelection();  // note: sets *model* as dirty
        void setSelection(OpenSim::Component const*);

        OpenSim::Component const* getHover() const;
        OpenSim::Component* updHover();  // note: sets *model* as dirty
        void setHover(OpenSim::Component const*);

        OpenSim::Component const* getIsolated() const;
        OpenSim::Component* updIsolated();  // note: sets *model* as dirty
        void setIsolated(OpenSim::Component const*);

        std::chrono::system_clock::time_point getTimestamp() const;

        void clearAnyPointersTo(OpenSim::Component const*);
        void updateStateAndDecorationsForDirtyModel(Scene_generator&);
    };

    class UndoableUiModel final {
        std::unique_ptr<UiModel> m_ActiveModel;
        Circular_buffer<std::unique_ptr<UiModel>> m_UndoBuffer;
        Circular_buffer<std::unique_ptr<UiModel>> m_RedoBuffer;

    public:
        bool canUndo() const;
        void doUndo();

        bool canRedo();
        void doRedo();

        OpenSim::Model const& getModel() const;
        OpenSim::Model& updModel();
        void setModelDirty();
        void unsetModelDirty();

        SimTK::State const& getState() const;
        SimTK::State& updState();
        void setStateDirty();
        void unsetStateDirty();

        Scene_decorations const& getDecorations() const;
        Scene_decorations& updDecorations();
        void setDecorationsDirty();
        void unsetDecorationsDirty();

        OpenSim::Component const* getSelection() const;
        OpenSim::Componnet* updSelection();  // note: sets *model* as dirty
        void setSelection(OpenSim::Component const*);

        OpenSim::Component const* getHover() const;
        OpenSim::Component* updHover();  // note: sets *model* as dirty
        void setHover(OpenSim::Component const*);

        OpenSim::Component const* getIsolated() const;
        OpenSim::Component* updIsolated();  // note: sets *model* as dirty
        void setIsolated(OpenSim::Component const*);

        void clearAnyPointersTo(OpenSim::Component const*);
        void updateStateAndDecorationsForDirtyModels(Scene_generator&);
    };

    class MainEditorState final {
        UndoableUiModel m_EditedModel;
        Scene_generator m_SceneGenerator;
    };
}
