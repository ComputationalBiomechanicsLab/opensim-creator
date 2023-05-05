#pragma once

#include "src/Graphics/RenderTextureDescriptor.hpp"

#include <memory>

namespace osc
{
	class RenderBuffer final {
	public:
		RenderBuffer() = delete;
		RenderBuffer(RenderBuffer const&) = delete;
		RenderBuffer(RenderBuffer&&) noexcept = delete;
		RenderBuffer& operator=(RenderBuffer const&) = delete;
		RenderBuffer& operator=(RenderBuffer&&) noexcept = delete;
		~RenderBuffer() noexcept;

	private:
		friend class GraphicsBackend;
		friend class RenderTexture;

		explicit RenderBuffer(
			std::shared_ptr<RenderTextureDescriptor>,
			bool isColor
		);

		class Impl;
		std::unique_ptr<Impl> m_Impl;
	};
}