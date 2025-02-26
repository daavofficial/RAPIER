#include "rppch.h"
#include "RAPIER/Renderer/Renderer.h"
#include "RAPIER/Renderer/Renderer2D.h"
#include "RAPIER/Renderer/SceneRenderer.h"

namespace RAPIER
{
	static RendererAPI* s_RendererAPI = nullptr;

	void Renderer::Init()
	{
		RP_PROFILE_FUNC();

		RenderCommand::Init();
		SceneRenderer::Init();
		Renderer2D::Init();
	}

	void Renderer::Shutdown()
	{
		Renderer2D::Shutdown();
	}

	void Renderer::OnWindowResize(uint32_t width, uint32_t height)
	{
		RenderCommand::SetViewport(0, 0, width, height);
	}

	void Renderer::Submit(const Ref<Shader>& shader, const Ref<VertexArray>& vertexArray, const glm::mat4& transform)
	{
		shader->Bind();
		shader->SetMat4("u_ViewProjection", SceneRenderer::s_SceneData->ViewProjectionMatrix);
		shader->SetMat4("u_Transform", transform);

		vertexArray->Bind();
		RenderCommand::DrawIndexed(vertexArray);
	}

}	//	END namespace RAPIER
