#pragma once

#include "RAPIER/Core/UUID.h"
#include "RAPIER/Core/Timestep.h"
#include "RAPIER/Renderer/EditorCamera.h"

#include <entt.hpp>
//#include <glm/glm.hpp>

class b2World;

namespace RAPIER
{
	class Entity;
	using EntityMap = std::unordered_map<UUID, Entity>;

	class Scene : public RefCounted
	{
	public:
		Scene(const std::string& debugName = "Scene");
		~Scene();

		void Init();

		void OnUpdateRuntime(Timestep ts);
		void OnUpdateEditor(Timestep ts, EditorCamera& camera);
		void OnViewportResize(uint32_t width, uint32_t height);

		void OnRuntimeStart();
		void OnRuntimeStop();

		Entity GetPrimaryCameraEntity();
		bool HasPrimaryCameraEntity();

		Entity CreateEntity(const std::string& name = std::string());
		Entity CreateEntityWithUUID(UUID uuid, const std::string& name = "");
		void DestroyEntity(Entity entity);

		void DuplicateEntity(Entity entity);

		template<typename... Components>
		auto GetAllEntitiesWith() { return m_Registry.view<Components...>(); }
		Entity FindEntityByTag(const std::string& tag);

		Entity FindEntityByUUID(UUID id);
		const EntityMap& GetEntityMap() const { return m_EntityIDMap; }
		void CopyTo(Ref<Scene>& target);

		UUID GetUUID() const { return m_SceneID; }
		const std::string& GetName() const { return m_DebugName; }

		static Ref<Scene> GetScene(UUID uuid);
	private:
		template<typename T>
		void OnComponentAdded(Entity entity, T& component);
	protected:
		UUID m_SceneID;
		//entt::entity m_SceneEntity;
		entt::registry m_Registry;
		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;

		b2World* m_PhysicsWorld = nullptr;

		std::string m_DebugName;

		EntityMap m_EntityIDMap;
		bool m_IsPlaying = false;

		friend class Entity;
		friend class SceneSerializer;
		friend class SceneHierarchyPanel;
	};	//	END class Scene

}	//	END namespace RAPIER
