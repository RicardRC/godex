
#include "./register_types.h"

#include "core/config/engine.h"
#include "core/object/message_queue.h"
#include "ecs.h"
#include "iterators/dynamic_query.h"
#include "systems/dynamic_system.h"

#include "godot/components/mesh_component.h"
#include "godot/components/transform_component.h"
#include "godot/editor_plugins/editor_world_ecs.h"
#include "godot/editor_plugins/entity_editor_plugin.h"
#include "godot/nodes/ecs_utilities.h"
#include "godot/nodes/ecs_world.h"
#include "godot/nodes/entity.h"
#include "godot/resources/godot_engine_resources.h"
#include "godot/systems/physics_process_system.h"

// TODO improve this workflow once the new pipeline is integrated.
class REP : public Object {
public:
	// TODO this should go in a better place, like at the end of the
	// engine setup: https://github.com/godotengine/godot-proposals/issues/1593
	void setup_ecs() {
		if (Engine::get_singleton()->is_editor_hint()) {
			if (EditorNode::get_singleton() != nullptr) {
				// Setup editor plugins
				EditorNode::get_singleton()->add_editor_plugin(memnew(EntityEditorPlugin(EditorNode::get_singleton())));
				EditorNode::get_singleton()->add_editor_plugin(memnew(WorldECSEditorPlugin(EditorNode::get_singleton())));
			}
		} else {
			// Load the Scripted Components/Resources/Systems
			ScriptECS::register_runtime_scripts();
		}
	}
} rep;

void register_godex_types() {
	godex::DynamicSystemInfo::for_each_name = StringName("_for_each");

	ClassDB::register_class<ECS>();
	ClassDB::register_class<WorldECS>();
	ClassDB::register_class<PipelineECS>();
	ClassDB::register_class<Entity>();
	ClassDB::register_class<Component>();
	ClassDB::register_class<System>();
	ClassDB::register_class<godex::DynamicQuery>();

	// Create and register singleton
	ECS *ecs = memnew(ECS);
	ECS::__set_singleton(ecs);
	Engine::get_singleton()->add_singleton(Engine::Singleton("ECS", ecs));

	// Register in editor things.
	if (MessageQueue::get_singleton()) {
		MessageQueue::get_singleton()->push_callable(callable_mp(&rep, &REP::setup_ecs));
	}

	// ~ Register engine components ~
	ECS::register_component<MeshComponent>();
	ECS::register_component<TransformComponent>();

	// ~ Register engine resources ~
	ECS::register_resource<World>();
	ECS::register_resource<GodotIteratorInfoResource>();
	ECS::register_resource<OsResource>();
	ECS::register_resource<EngineResource>();
	ECS::register_resource<Physics3DServerResource>();
	ECS::register_resource<MessageQueueResource>();

	// ~ Register engine systems ~
	{
		// Register 3D physics systems.
		const godex::system_id id = ECS::register_dynamic_system("PhysicsSystemDispatcher", "System that dispatches the specified pipeline at fixed rate. The rate is defined by `Physics Hz` in the project settings.");
		create_physics_system_dispatcher(ECS::get_dynamic_system_info(id));

		ECS::register_system(call_physics_process, "CallPhysicsProcess", "Updates the Godot Nodes (2D/3D) transform and fetches the events from the physics engine.");
		ECS::register_system(step_physics_server_3d, "StepPhysicsServer3D", "Steps the PhysicsServer3D.");
	}

	// ~~ Register Godot singleton ~~
	// NOTE: These singleton doesn't store data but are here only to provide
	// access to functionalities. This enforces the separation between data and
	// functions.
	// TODO improve this, maybe propose to GODOT to add `static functions`
	// access?
	WorldECSCommands::get_singleton();
}

void unregister_godex_types() {
	ECS *ecs = ECS::get_singleton();
	ECS::__set_singleton(nullptr);
	memdelete(ecs);
}
