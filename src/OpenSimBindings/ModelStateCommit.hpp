#pragma once

#include "src/Utils/SynchronizedValue.hpp"
#include "src/Utils/UID.hpp"

#include <chrono>
#include <memory>
#include <string>
#include <string_view>

namespace OpenSim
{
	class Model;
}

namespace osc
{
	class VirtualConstModelStatePair;
}

namespace osc
{
	// immutable, reference-counted handle to a "Model+State commit", which is effectively
	// what is saved upon each user action
	class ModelStateCommit final {
	public:
		ModelStateCommit(VirtualConstModelStatePair const&, std::string_view message);
		ModelStateCommit(VirtualConstModelStatePair const&, std::string_view message, UID parent);
		ModelStateCommit(ModelStateCommit const&);
		ModelStateCommit(ModelStateCommit&&) noexcept;
		ModelStateCommit& operator=(ModelStateCommit const&);
		ModelStateCommit& operator=(ModelStateCommit&&) noexcept;
		~ModelStateCommit() noexcept;

		UID getID() const;
		bool hasParent() const;
		UID getParentID() const;
		std::chrono::system_clock::time_point getCommitTime() const;
		SynchronizedValueGuard<OpenSim::Model const> getModel() const;
		UID getModelVersion() const;
		float getFixupScaleFactor() const;

	private:
		friend bool operator==(ModelStateCommit const& a, ModelStateCommit const& b);

		class Impl;
		std::shared_ptr<Impl const> m_Impl;
	};

	inline bool operator==(ModelStateCommit const& a, ModelStateCommit const& b)
	{
		return a.m_Impl == b.m_Impl;
	}
}