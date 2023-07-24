#include "ExportPointsPopup.hpp"

#include "OpenSimCreator/Model/VirtualConstModelStatePair.hpp"
#include "OpenSimCreator/Utils/OpenSimHelpers.hpp"

#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Formats/CSV.hpp>
#include <oscar/Platform/Log.hpp>
#include <oscar/Platform/os.hpp>
#include <oscar/Utils/Cpp20Shims.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/SetHelpers.hpp>
#include <oscar/Utils/StringHelpers.hpp>
#include <oscar/Widgets/StandardPopup.hpp>

#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Simulation/Model/AbstractPathPoint.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PathPoint.h>
#include <OpenSim/Simulation/Model/Point.h>
#include <OpenSim/Simulation/Model/Station.h>
#include <OpenSim/Simulation/Wrap/PathWrapPoint.h>
#include <Simbody.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>

namespace
{
	osc::CStringView constexpr c_ExplanationText = "Exports the chosen points within the model, potentially with respect to a chosen frame, as a standard data file (CSV)";
	osc::CStringView constexpr c_OriginalFrameLabel = "(original frame)";

	struct PointSelectorState final {
		std::string searchString;
		std::unordered_set<std::string> selectedPointAbsPaths;
	};

	struct FrameSelectorState final {
		std::optional<std::string> maybeSelectedFrameAbsPath;
	};

	struct OutputFormatState final {
		bool exportPointNamesAsAbsPaths = true;
	};

	struct PointInfo final {
		PointInfo(
			SimTK::Vec3 location_,
			OpenSim::ComponentPath frameAbsPath_) :

			location{location_},
			frameAbsPath{frameAbsPath_}
		{
		}

		SimTK::Vec3 location;
		OpenSim::ComponentPath frameAbsPath;
	};

	std::optional<PointInfo> TryGetPointInfo(
		OpenSim::Component const& c,
		SimTK::State const& st)
	{
		if (auto const* pwp = dynamic_cast<OpenSim::PathWrapPoint const*>(&c))
		{
			// BODGE/HACK: path wrap points don't update the cache correctly?
			return std::nullopt;
		}
		else if (auto const* station = dynamic_cast<OpenSim::Station const*>(&c))
		{
			// BODGE/HACK: OpenSim redundantly stores path point information
			// in a child called 'station'. These must be filtered because, otherwise,
			// the user will just see a bunch of 'station' entries below each path
			// point
			{
				OpenSim::Component const* owner = osc::GetOwner(*station);
				if (station->getName() == "station" && dynamic_cast<OpenSim::PathPoint const*>(owner))
				{
					return std::nullopt;
				}
			}

			return PointInfo
			{
				station->get_location(),
				osc::GetAbsolutePath(station->getParentFrame()),
			};
		}
		else if (auto const* pp = dynamic_cast<OpenSim::PathPoint const*>(&c))
		{
			return PointInfo
			{
				pp->getLocation(st),
				osc::GetAbsolutePath(pp->getParentFrame()),
			};
		}
		else if (auto const* point = dynamic_cast<OpenSim::Point const*>(&c))
		{
			return PointInfo
			{
				point->getLocationInGround(st),
				OpenSim::ComponentPath{"/ground"},
			};
		}
		else if (auto const* frame = dynamic_cast<OpenSim::Frame const*>(&c))
		{
			return PointInfo
			{
				frame->getPositionInGround(st),
				OpenSim::ComponentPath{"/ground"},
			};
		}
		else
		{
			return std::nullopt;
		}
	}

	bool IsPoint(OpenSim::Component const& c, SimTK::State const& st)
	{
		return TryGetPointInfo(c, st) != std::nullopt;
	}

	void DrawExplanationHeader()
	{
		ImGui::Text("Description:");
		ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
		ImGui::TextWrapped(c_ExplanationText.c_str());
		ImGui::PopStyleColor();
	}

	bool ShouldShowInPointSelector(
		PointSelectorState const& state,
		OpenSim::Component const& c,
		SimTK::State const& st)
	{
		return
			IsPoint(c, st) &&
			osc::ContainsSubstringCaseInsensitive(c.getName(), state.searchString);
	}

	void TryDrawPointSelector(
		PointSelectorState& state,
		OpenSim::Component const& c,
		SimTK::State const& st)
	{
		if (!ShouldShowInPointSelector(state, c, st))
		{
			return;
		}

		std::string const p = osc::GetAbsolutePathString(c);

		bool selected = osc::Contains(state.selectedPointAbsPaths, p);
		if (ImGui::Checkbox(c.getName().c_str(), &selected))
		{
			if (selected)
			{
				state.selectedPointAbsPaths.insert(p);
			}
			else
			{
				state.selectedPointAbsPaths.erase(p);
			}
		}

		if (ImGui::IsItemHovered())
		{
			osc::BeginTooltip();
			ImGui::TextUnformatted(c.getName().c_str());
			ImGui::SameLine();
			ImGui::TextDisabled("%s", c.getConcreteClassName().c_str());

			if (std::optional<PointInfo> const pi = TryGetPointInfo(c, st))
			{
				ImGui::TextDisabled("Expressed In: %s", pi->frameAbsPath.toString().c_str());
			}

			osc::EndTooltip();
		}
	}

	void DrawFilteredPointList(
		PointSelectorState& state,
		OpenSim::Model const& model,
		SimTK::State const& st)
	{
		auto color = ImGui::GetStyle().Colors[ImGuiCol_FrameBg];
		color.w *= 0.5f;

		ImGui::PushStyleColor(ImGuiCol_FrameBg, color);
		bool const showingListBox = ImGui::BeginListBox("list");
		ImGui::PopStyleColor();

		if (showingListBox)
		{
			int imguiID = 0;
			for (OpenSim::Component const& c : model.getComponentList())
			{
				ImGui::PushID(imguiID++);
				TryDrawPointSelector(state, c, st);
				ImGui::PopID();
			}
			ImGui::EndListBox();
		}
	}

	void ActionSelectAllListedComponents(
		PointSelectorState& state,
		OpenSim::Model const& model,
		SimTK::State const& st)
	{
		for (OpenSim::Component const& c : model.getComponentList())
		{
			if (ShouldShowInPointSelector(state, c, st))
			{
				state.selectedPointAbsPaths.insert(osc::GetAbsolutePathString(c));
			}
		}
	}

	void ActionDeselectAllListedComponents(
		PointSelectorState& state,
		OpenSim::Model const& model,
		SimTK::State const& st)
	{
		for (OpenSim::Component const& c : model.getComponentList())
		{
			if (ShouldShowInPointSelector(state, c, st))
			{
				state.selectedPointAbsPaths.erase(osc::GetAbsolutePathString(c));
			}
		}
	}

	void ActionClearSelectedComponents(PointSelectorState& state)
	{
		state.selectedPointAbsPaths.clear();
	}

	void DrawSelectionManipulatorButtons(
		PointSelectorState& state,
		OpenSim::Model const& model,
		SimTK::State const& st)
	{
		if (ImGui::Button("Select Listed"))
		{
			ActionSelectAllListedComponents(state, model, st);
		}

		ImGui::SameLine();

		if (ImGui::Button("De-Select Listed"))
		{
			ActionDeselectAllListedComponents(state, model, st);
		}

		ImGui::SameLine();

		if (ImGui::Button("Clear Selection"))
		{
			ActionClearSelectedComponents(state);
		}
	}

	void DrawPointSelector(
		PointSelectorState& state,
		OpenSim::Model const& model,
		SimTK::State const& st)
	{
		ImGui::Text("Which Points:");
		osc::InputString("search", state.searchString);
		DrawFilteredPointList(state, model, st);
		DrawSelectionManipulatorButtons(state, model, st);
	}

	OpenSim::Component const* TryGetMaybeSelectedFrameOrNullptr(
		FrameSelectorState const& uiState,
		OpenSim::Model const& model)
	{
		return uiState.maybeSelectedFrameAbsPath ?
			osc::FindComponent(model, *uiState.maybeSelectedFrameAbsPath) :
			nullptr;
	}

	std::string CalcComboLabel(
		FrameSelectorState const& uiState,
		OpenSim::Model const& model)
	{
		OpenSim::Component const* c = TryGetMaybeSelectedFrameOrNullptr(uiState, model);
		return c ? c->getName() : std::string{c_OriginalFrameLabel};
	}

	void DrawFrameSelector(
		FrameSelectorState& uiState,
		OpenSim::Model const& model,
		SimTK::State const& st)
	{
		ImGui::Text("Express Points In:");

		std::string const label = CalcComboLabel(uiState, model);

		if (ImGui::BeginCombo("Frame", label.c_str()))
		{
			// draw "original frame" selector...
			if (ImGui::Selectable(c_OriginalFrameLabel.c_str(), uiState.maybeSelectedFrameAbsPath != std::nullopt))
			{
				uiState.maybeSelectedFrameAbsPath.reset();
			}

			// ... then draw one selector per frame in the model
			int imguiID = 0;
			for (OpenSim::Frame const& frame : model.getComponentList<OpenSim::Frame>())
			{
				std::string const absPath = osc::GetAbsolutePathString(frame);
				bool const selected = uiState.maybeSelectedFrameAbsPath == absPath;

				ImGui::PushID(imguiID++);
				if (ImGui::Selectable(frame.getName().c_str(), selected))
				{
					uiState.maybeSelectedFrameAbsPath = absPath;
				}
				ImGui::PopID();
			}

			ImGui::EndCombo();
		}
	}

	void DrawOutputFormatEditor(OutputFormatState& uiState)
	{
		ImGui::Checkbox("Export Point Names as Absolute Paths", &uiState.exportPointNamesAsAbsPaths);
		osc::DrawTooltipBodyOnlyIfItemHovered("If selected, the exported point name will be the full path to the point (e.g. `/forceset/somemuscle/geometrypath/pointname`), rather than just the name of the point (e.g. `pointname`)");
	}

	enum class ExportStepReturn {
		UserCancelled = 0,
		IoError,
		Done,
		TOTAL,
	};

	std::optional<SimTK::Transform> TryGetTransformToReexpressPointsIn(
		OpenSim::Model const& model,
		SimTK::State const& state,
		std::optional<std::string> const& maybeAbsPathOfFrameToReexpressPointsIn)
	{
		if (!maybeAbsPathOfFrameToReexpressPointsIn)
		{
			return std::nullopt;  // caller doesn't want re-expression
		}

		OpenSim::Frame const* const frame = osc::FindComponent<OpenSim::Frame>(model, *maybeAbsPathOfFrameToReexpressPointsIn);
		if (!frame)
		{
			return std::nullopt;  // the selected frame doesn't exist in the model (bug?)
		}

		return frame->getTransformInGround(state).invert();
	}

	std::vector<std::string> GetSortedListOfOutputPointAbsPaths(
		std::unordered_set<std::string> const& unorderedPointAbsPaths,
		bool shouldExportPointsWithAbsPathNames)
	{
		std::vector<std::string> rv(unorderedPointAbsPaths.begin(), unorderedPointAbsPaths.end());
		if (shouldExportPointsWithAbsPathNames)
		{
			std::sort(rv.begin(), rv.end());
		}
		else
		{
			auto const isEndOfPathLexographicallyLess = [](std::string const& a, std::string const& b)
			{
				return osc::SubstringAfterLast(a, '/') < osc::SubstringAfterLast(b, '/');
			};
			std::sort(rv.begin(), rv.end(), isEndOfPathLexographicallyLess);
		}
		return rv;
	}

	SimTK::Vec3 CalcReexpressedFrame(
		OpenSim::Model const& model,
		SimTK::State const& state,
		PointInfo const& pointInfo,
		SimTK::Transform const& ground2otherFrame)
	{
		OpenSim::Frame const* const frame = osc::FindComponent<OpenSim::Frame>(model, pointInfo.frameAbsPath);
		if (!frame)
		{
			return pointInfo.location;  // cannot find frame (bug?)
		}

		return ground2otherFrame * frame->getTransformInGround(state) * pointInfo.location;
	}

	ExportStepReturn PromptUserForSaveLocationAndExportPoints(
		OpenSim::Model const& model,
		SimTK::State const& state,
		std::unordered_set<std::string> const& pointAbsPaths,
		std::optional<std::string> const& maybeAbsPathOfFrameToReexpressPointsIn,
		bool shouldExportPointsWithAbsPathNames)
	{
		std::optional<std::filesystem::path> const saveLoc = osc::PromptUserForFileSaveLocationAndAddExtensionIfNecessary("csv");
		if (!saveLoc)
		{
			return ExportStepReturn::UserCancelled;
		}

		std::ofstream fOut;
		fOut.exceptions(std::ios_base::badbit | std::ios_base::failbit);
		fOut.open(*saveLoc);
		if (!fOut)
		{
			return ExportStepReturn::IoError;
		}


		// write header
		osc::WriteCSVRow(
			fOut,
			osc::to_array<std::string>({ "Name", "X", "Y", "Z" })
		);

		std::vector<std::string> const sortedRowAbsPaths = GetSortedListOfOutputPointAbsPaths(
			pointAbsPaths,
			shouldExportPointsWithAbsPathNames
		);

		std::optional<SimTK::Transform> const maybeGround2ReexpressedFrame = TryGetTransformToReexpressPointsIn(
			model,
			state,
			maybeAbsPathOfFrameToReexpressPointsIn
		);

		for (std::string const& path : sortedRowAbsPaths)
		{
			OpenSim::Component const* c = osc::FindComponent(model, path);
			if (!c)
			{
				continue;  // no longer exists in model
			}

			std::optional<PointInfo> const pi = TryGetPointInfo(*c, state);
			if (!pi)
			{
				continue;  // cannot extract point info for the component
			}

			SimTK::Vec3 const position = maybeGround2ReexpressedFrame ?
				CalcReexpressedFrame(model, state, *pi, *maybeGround2ReexpressedFrame) :
				pi->location;

			std::string const name = shouldExportPointsWithAbsPathNames ?
				osc::GetAbsolutePathString(*c) :
				c->getName();

			auto const columns = osc::to_array<std::string>(
			{
				name,
				std::to_string(position[0]),
				std::to_string(position[1]),
				std::to_string(position[2]),
			});

			osc::WriteCSVRow(fOut, columns);
		}

		return ExportStepReturn::Done;
	}
}

class osc::ExportPointsPopup::Impl final : public osc::StandardPopup {
public:
	Impl(
		std::string_view popupName_,
		std::shared_ptr<VirtualConstModelStatePair const> model_) :

		StandardPopup{popupName_},
		m_Model{std::move(model_)}
	{
	}

private:
	void implDrawContent() final
	{
		OpenSim::Model const& model = m_Model->getModel();
		SimTK::State const& state = m_Model->getState();

		DrawExplanationHeader();
		ImGui::Separator();
		DrawPointSelector(m_PointSelectorState, model, state);
		ImGui::Separator();
		DrawFrameSelector(m_FrameSelectorState, model, state);
		ImGui::Separator();
		DrawOutputFormatEditor(m_OutputFormatState);
		ImGui::Separator();
		drawBottomButtons();
	}

	void drawBottomButtons()
	{
		if (ImGui::Button("Cancel"))
		{
			requestClose();
		}

		ImGui::SameLine();

		if (ImGui::Button(ICON_FA_UPLOAD " Export to CSV"))
		{
			static_assert(static_cast<size_t>(ExportStepReturn::TOTAL) == 3, "review error handling");
			ExportStepReturn const rv = PromptUserForSaveLocationAndExportPoints(
				m_Model->getModel(),
				m_Model->getState(),
				m_PointSelectorState.selectedPointAbsPaths,
				m_FrameSelectorState.maybeSelectedFrameAbsPath,
				m_OutputFormatState.exportPointNamesAsAbsPaths
			);
			if (rv == ExportStepReturn::Done)
			{
				requestClose();
			}
		}
	}

	std::shared_ptr<VirtualConstModelStatePair const> m_Model;
	PointSelectorState m_PointSelectorState;
	FrameSelectorState m_FrameSelectorState;
	OutputFormatState m_OutputFormatState;
};


// public API

osc::ExportPointsPopup::ExportPointsPopup(
	std::string_view popupName,
	std::shared_ptr<VirtualConstModelStatePair const> model_) :

	m_Impl{std::make_unique<Impl>(popupName, std::move(model_))}
{
}
osc::ExportPointsPopup::ExportPointsPopup(ExportPointsPopup&&) noexcept = default;
osc::ExportPointsPopup& osc::ExportPointsPopup::operator=(ExportPointsPopup&&) noexcept = default;
osc::ExportPointsPopup::~ExportPointsPopup() noexcept = default;

bool osc::ExportPointsPopup::implIsOpen() const
{
	return m_Impl->isOpen();
}

void osc::ExportPointsPopup::implOpen()
{
	m_Impl->open();
}

void osc::ExportPointsPopup::implClose()
{
	m_Impl->close();
}

bool osc::ExportPointsPopup::implBeginPopup()
{
	return m_Impl->beginPopup();
}

void osc::ExportPointsPopup::implDrawPopupContent()
{
	m_Impl->drawPopupContent();
}

void osc::ExportPointsPopup::implEndPopup()
{
	m_Impl->endPopup();
}
