#ifndef TEST_ECS_PIPELINE_BUILDER_H
#define TEST_ECS_PIPELINE_BUILDER_H

#include "tests/test_macros.h"

#include "../components/component.h"
#include "../databags/databag.h"
#include "../modules/godot/nodes/ecs_utilities.h"
#include "../pipeline/pipeline.h"
#include "../pipeline/pipeline_builder.h"
#include "../storage/dense_vector_storage.h"
#include "test_utilities.h"

// The idea to verify that the pipeline is correctly constructed we need to
// verify the following:
// 1. The Systems with implicit dependencies are correctly taken into account.
//      - The Systems implicit priority is used in this case, where the
//        priority is given by the registration order.
//      - The Systems implicit dependency, where two system fetch the same
//        Compoenents and DataBags mutable, are resolved.
//      - The systems that fetch the sane Components and DataBag immutable
//        can run in parallel.
// 2. The Systems with explicit dependencies are correctly taken into account.
//      - The Systems explicit priority, build with (`after` and `before`) is
//        correctly resolved.
// 3. The Systems phase is correctly used.
// 4. The SystemBundles are correctly expanded.
// 5. The SystemBundles explicit dependencies are take into account.
// 6. The Query filer `Not` can be used to specialize the fetched component
//    so two systems that fetch the same component mutable may run in parallel
//    thanks to the specialization given by `Not`.
// 7. All the above must be valid for C++ and Scripted systems.
// 9. Make sure that the systems that fetch World, SceneTreeDatabag are always
//     are always executed in single thread.
// 10. Detect when an event isn't catched by any system, tell how to fix it.
// 11. Test the World and SceneTreeDatabag systems are always run in single thread.

struct PbComponentA {
	COMPONENT(PbComponentA, DenseVectorStorage)
};

struct PbComponentB {
	COMPONENT(PbComponentB, DenseVectorStorage)
};

struct PbDatabagA : public godex::Databag {
	DATABAG(PbDatabagA)
};

namespace godex_tests {

TEST_CASE("[Modules][ECS] Initialize PipelineBuilder tests.") {
	// Just initializes the common Components and Databags used by the following tests.
	ECS::register_component<PbComponentA>();
	ECS::register_component<PbComponentB>();
	ECS::register_databag<PbDatabagA>();
}
} // namespace godex_tests

void test_A_system_1(Query<PbComponentA, const PbComponentB> &p_query) {}
void test_A_system_3(Query<const PbComponentA, const PbComponentB> &p_query, PbDatabagA *p_db_a) {}
void test_A_system_5(Query<const PbComponentA, const PbComponentB> &p_query, const PbDatabagA *p_db_a) {}
void test_A_system_14(Query<const PbComponentA, const PbComponentB> &p_query) {}
void test_A_system_15(Query<PbComponentA, PbComponentB> &p_query) {}

namespace godex_tests {
TEST_CASE("[Modules][ECS] Verify the PipelineBuilder takes into account implicit and explicit dependencies.") {
	initialize_script_ecs();

	ECS::register_system(test_A_system_15, "test_A_system_15")
			.set_phase(PHASE_CONFIG);

	{
		// Create the script.
		String code;
		code += "extends System\n";
		code += "\n";
		code += "func _prepare():\n";
		code += "	execute_in_phase(ECS.PHASE_POST_PROCESS)\n";
		code += "	with_component(ECS.PbComponentA, IMMUTABLE)\n";
		code += "	with_component(ECS.PbComponentB, IMMUTABLE)\n";
		code += "	with_databag(ECS.PbDatabagA, IMMUTABLE)\n";
		code += "\n";
		code += "func _for_each(a, b, c):\n";
		code += "	pass\n";
		code += "\n";

		CHECK(build_and_register_ecs_script("test_A_system_8.gd", code));
	}

	ECS::register_system(test_A_system_1, "test_A_system_1");

	{
		// Create the script.
		String code;
		code += "extends System\n";
		code += "\n";
		code += "func _prepare():\n";
		code += "	execute_in_phase(ECS.PHASE_CONFIG)\n";
		code += "	with_component(ECS.PbComponentA, IMMUTABLE)\n";
		code += "	with_component(ECS.PbComponentB, IMMUTABLE)\n";
		code += "	with_databag(ECS.PbDatabagA, IMMUTABLE)\n";
		code += "\n";
		code += "func _for_each(a, b, c):\n";
		code += "	pass\n";
		code += "\n";

		CHECK(build_and_register_ecs_script("test_A_system_0.gd", code));
	}

	{
		// Create the script.
		String code;
		code += "extends System\n";
		code += "\n";
		code += "func _prepare():\n";
		code += "	with_component(ECS.PbComponentA, IMMUTABLE)\n";
		code += "	with_component(ECS.PbComponentB, MUTABLE)\n";
		code += "\n";
		code += "func _for_each(a, b):\n";
		code += "	pass\n";
		code += "\n";

		CHECK(build_and_register_ecs_script("test_A_system_2.gd", code));
	}

	ECS::register_system(test_A_system_3, "test_A_system_3");

	{
		// Create the script.
		String code;
		code += "extends System\n";
		code += "\n";
		code += "func _prepare():\n";
		code += "	with_component(ECS.PbComponentA, IMMUTABLE)\n";
		code += "	with_component(ECS.PbComponentB, IMMUTABLE)\n";
		code += "\n";
		code += "func _for_each(a, b):\n";
		code += "	pass\n";
		code += "\n";

		CHECK(build_and_register_ecs_script("test_A_system_4.gd", code));
	}

	ECS::register_system(test_A_system_5, "test_A_system_5");

	{
		// Create the script.
		String code;
		code += "extends System\n";
		code += "\n";
		code += "func _prepare():\n";
		code += "	with_component(ECS.PbComponentA, IMMUTABLE)\n";
		code += "	with_component(ECS.PbComponentB, IMMUTABLE)\n";
		code += "	with_databag(ECS.PbDatabagA, IMMUTABLE)\n";
		code += "\n";
		code += "func _for_each(a, b, c):\n";
		code += "	pass\n";
		code += "\n";

		CHECK(build_and_register_ecs_script("test_A_system_6.gd", code));
	}

	{
		// Create the script.
		String code;
		code += "extends System\n";
		code += "\n";
		code += "func _prepare():\n";
		code += "	execute_in_phase(ECS.PHASE_CONFIG)\n";
		code += "	with_component(ECS.PbComponentA, IMMUTABLE)\n";
		code += "	with_component(ECS.PbComponentB, IMMUTABLE)\n";
		code += "	with_databag(ECS.PbDatabagA, IMMUTABLE)\n";
		code += "\n";
		code += "func _for_each(a, b, c):\n";
		code += "	pass\n";
		code += "\n";

		CHECK(build_and_register_ecs_script("test_A_system_7.gd", code));
	}

	{
		// Create the script.
		String code;
		code += "extends System\n";
		code += "\n";
		code += "func _prepare():\n";
		code += "	execute_before(ECS.test_A_system_1)\n";
		code += "	with_component(ECS.PbComponentA, IMMUTABLE)\n";
		code += "	with_component(ECS.PbComponentB, IMMUTABLE)\n";
		code += "	with_databag(ECS.PbDatabagA, IMMUTABLE)\n";
		code += "\n";
		code += "func _for_each(a, b, c):\n";
		code += "	pass\n";
		code += "\n";

		CHECK(build_and_register_ecs_script("test_A_system_9.gd", code));
	}

	{
		// Create the script.
		String code;
		code += "extends System\n";
		code += "\n";
		code += "func _prepare():\n";
		code += "	execute_after(ECS.test_A_system_13_gd)\n";
		code += "	with_component(ECS.PbComponentA, IMMUTABLE)\n";
		code += "	with_component(ECS.PbComponentB, IMMUTABLE)\n";
		code += "\n";
		code += "func _for_each(a, b):\n";
		code += "	pass\n";
		code += "\n";

		CHECK(build_and_register_ecs_script("test_A_system_10.gd", code));
	}
	{
		// Create the script.
		String code;
		code += "extends System\n";
		code += "\n";
		code += "func _prepare():\n";
		code += "	execute_after(ECS.test_A_system_12_gd)\n";
		code += "	with_component(ECS.PbComponentA, IMMUTABLE)\n";
		code += "	with_component(ECS.PbComponentB, IMMUTABLE)\n";
		code += "\n";
		code += "func _for_each(a, b):\n";
		code += "	pass\n";
		code += "\n";

		CHECK(build_and_register_ecs_script("test_A_system_11.gd", code));
	}
	{
		// Create the script.
		String code;
		code += "extends System\n";
		code += "\n";
		code += "func _prepare():\n";
		code += "	with_component(ECS.PbComponentA, IMMUTABLE)\n";
		code += "	with_component(ECS.PbComponentB, IMMUTABLE)\n";
		code += "\n";
		code += "func _for_each(a, b):\n";
		code += "	pass\n";
		code += "\n";

		CHECK(build_and_register_ecs_script("test_A_system_12.gd", code));
	}
	{
		// Create the script.
		String code;
		code += "extends System\n";
		code += "\n";
		code += "func _prepare():\n";
		code += "	execute_before(ECS.test_A_system_12_gd)\n";
		code += "	with_component(ECS.PbComponentA, IMMUTABLE)\n";
		code += "	with_component(ECS.PbComponentB, IMMUTABLE)\n";
		code += "\n";
		code += "func _for_each(a, b):\n";
		code += "	pass\n";
		code += "\n";

		CHECK(build_and_register_ecs_script("test_A_system_13.gd", code));
	}

	ECS::register_system(test_A_system_14, "test_A_system_14")
			.before("test_A_system_9.gd");

	flush_ecs_script_preparation();

	Vector<StringName> system_bundles;

	Vector<StringName> systems;
	// Insert the systems with random order.
	systems.push_back(StringName("test_A_system_14"));
	systems.push_back(StringName("test_A_system_13.gd"));
	systems.push_back(StringName("test_A_system_12.gd"));
	systems.push_back(StringName("test_A_system_11.gd"));
	systems.push_back(StringName("test_A_system_10.gd"));
	systems.push_back(StringName("test_A_system_8.gd"));
	systems.push_back(StringName("test_A_system_6.gd"));
	systems.push_back(StringName("test_A_system_5"));
	systems.push_back(StringName("test_A_system_4.gd"));
	systems.push_back(StringName("test_A_system_3"));
	systems.push_back(StringName("test_A_system_0.gd"));
	systems.push_back(StringName("test_A_system_2.gd"));
	systems.push_back(StringName("test_A_system_1"));
	systems.push_back(StringName("test_A_system_7.gd"));
	systems.push_back(StringName("test_A_system_9.gd"));
	systems.push_back(StringName("test_A_system_15"));

	Pipeline pipeline;
	PipelineBuilder::build_pipeline(system_bundles, systems, &pipeline);

	const int stage_test_A_system_14 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_A_system_14")));
	const int stage_test_A_system_13 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_A_system_13.gd")));
	const int stage_test_A_system_12 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_A_system_12.gd")));
	const int stage_test_A_system_11 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_A_system_11.gd")));
	const int stage_test_A_system_10 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_A_system_10.gd")));
	const int stage_test_A_system_8 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_A_system_8.gd")));
	const int stage_test_A_system_6 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_A_system_6.gd")));
	const int stage_test_A_system_5 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_A_system_5")));
	const int stage_test_A_system_4 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_A_system_4.gd")));
	const int stage_test_A_system_3 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_A_system_3")));
	const int stage_test_A_system_0 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_A_system_0.gd")));
	const int stage_test_A_system_2 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_A_system_2.gd")));
	const int stage_test_A_system_1 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_A_system_1")));
	const int stage_test_A_system_7 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_A_system_7.gd")));
	const int stage_test_A_system_9 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_A_system_9.gd")));
	const int stage_test_A_system_15 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_A_system_15")));

	// Verify the test_A_system_15 is the first one being executed, and it's not
	// executed in parallel with any other system.
	// It's executed first because it run in the config phase and also thanks to
	// its implicit priority, and implicit dependency given by the mutable Query.
	CHECK(stage_test_A_system_15 < stage_test_A_system_7);

	// The phase config merges with the process phase:

	// test_A_system_14 has an explicit dependency with test_A_system_9, it runs
	// before.
	CHECK(stage_test_A_system_14 < stage_test_A_system_9);

	// However, the test_A_system_7.gd and test_A_system_0.gd is compatible
	// to run with with both in parallel. The optimization algorithm will decide
	// where to put this sysetm.
	CHECK(stage_test_A_system_7 < stage_test_A_system_1);
	CHECK(stage_test_A_system_0 < stage_test_A_system_1);

	// Verify the test_A_system_9.gd is executed before test_A_system_1 and in
	// parallel with test_A_system_0.gd
	CHECK(stage_test_A_system_9 < stage_test_A_system_1);

	// Verify the test_A_system_2.gd runs after test_A_system_1 because they have
	// an implicit dependency and test_A_system_1 is registered before.
	CHECK(stage_test_A_system_1 < stage_test_A_system_2);

	// Verify the test_A_system_3 and test_A_system_4.gd may run in parallel
	// but always after test_A_system_2.gd
	CHECK(stage_test_A_system_4 > stage_test_A_system_2);
	CHECK(stage_test_A_system_3 > stage_test_A_system_2);

	// Verify the test_A_system_5 and test_A_system_6.gd may run in parallel,
	// but always after test_A_system_3 and test_A_system_4.gd.
	CHECK(stage_test_A_system_6 > stage_test_A_system_3);
	CHECK(stage_test_A_system_5 > stage_test_A_system_3);
	CHECK(stage_test_A_system_6 > stage_test_A_system_4);
	CHECK(stage_test_A_system_5 > stage_test_A_system_4);

	// Verify the test_A_system_8 run the last one, since it's marked as post
	// process.
	CHECK(stage_test_A_system_8 >= stage_test_A_system_5);

	// Verify explicit dependencies:

	// Verify the test_A_system_13 is executed before test_A_system_12.
	CHECK(stage_test_A_system_13 < stage_test_A_system_12);

	// Verify the test_A_system_11 is executed after test_A_system_12.
	CHECK(stage_test_A_system_11 > stage_test_A_system_12);

	// Verify the test_A_system_10 is executed after test_A_system_13.
	CHECK(stage_test_A_system_10 > stage_test_A_system_13);

	// Verify the test_A_system_14 is executed before test_A_system_9.
	CHECK(stage_test_A_system_14 < stage_test_A_system_9);

	finalize_script_ecs();
}
} // namespace godex_tests

void test_B_system_1(Query<PbComponentA> &p_query) {}
void test_B_system_2(Query<PbComponentA> &p_query) {}
void test_B_system_3(Query<PbComponentA> &p_query) {}
void test_B_system_4(Query<PbComponentA> &p_query) {}
void test_B_system_9(Query<PbComponentA> &p_query) {}
void test_B_system_10(Query<PbComponentA> &p_query) {}
void test_B_system_12(Query<const PbComponentA> &p_query) {}

namespace godex_tests {
TEST_CASE("[Modules][ECS] Verify the PipelineBuilder bundles.") {
	// We have three bundles:
	// 1. C++ only
	// 2. GDScript only
	// 3. GDScript that bundles C++ systems
	// The bundle 1, depends explicitely on the system of bundle 2.
	// Add one syngle system with explicit dependency.
	initialize_script_ecs();

	ECS::register_system(test_B_system_12, "test_B_system_12")
			.after("test_B_system_11.gd");

	ECS::register_system_bundle("CppBundle")
			.add(ECS::register_system(test_B_system_4, "test_B_system_4") // Registered as first on purpose, to test the phase
							.set_phase(PHASE_POST_PROCESS))
			.add(ECS::register_system(test_B_system_2, "test_B_system_2")
							.set_phase(PHASE_PROCESS))
			.add(ECS::register_system(test_B_system_3, "test_B_system_3")
							.set_phase(PHASE_PROCESS))
			.add(ECS::register_system(test_B_system_1, "test_B_system_1") // Registered as last on purpose, to test the phase
							.set_phase(PHASE_CONFIG));

	{
		// Create the script.
		String code;
		code += "extends System\n";
		code += "\n";
		code += "func _prepare():\n";
		code += "	execute_in_phase(ECS.PHASE_CONFIG)\n";
		code += "	with_component(ECS.PbComponentB, IMMUTABLE)\n";
		code += "\n";
		code += "func _for_each(a):\n";
		code += "	pass\n";
		code += "\n";

		CHECK(build_and_register_ecs_script("test_B_system_5.gd", code));
	}
	{
		// Create the script.
		String code;
		code += "extends System\n";
		code += "\n";
		code += "func _prepare():\n";
		code += "	with_component(ECS.PbComponentB, IMMUTABLE)\n";
		code += "\n";
		code += "func _for_each(a):\n";
		code += "	pass\n";
		code += "\n";

		CHECK(build_and_register_ecs_script("test_B_system_6.gd", code));
	}
	{
		// Create the script.
		String code;
		code += "extends SystemBundle\n";
		code += "\n";
		code += "func _prepare():\n";
		code += "	add(ECS.test_B_system_5_gd)\n";
		code += "	add(ECS.test_B_system_6_gd)\n";
		code += "\n";

		CHECK(build_and_register_ecs_script("GDSBundle.gd", code));
	}

	// -- GDS that uses C++ --

	{
		// Create the script.
		String code;
		code += "extends System\n";
		code += "\n";
		code += "func _prepare():\n";
		code += "	execute_in_phase(ECS.PHASE_CONFIG)\n";
		code += "	with_component(ECS.PbComponentA, IMMUTABLE)\n";
		code += "\n";
		code += "func _for_each(a, b):\n";
		code += "	pass\n";
		code += "\n";

		CHECK(build_and_register_ecs_script("test_B_system_7.gd", code));
	}
	{
		// Create the script.
		String code;
		code += "extends System\n";
		code += "\n";
		code += "func _prepare():\n";
		code += "	with_component(ECS.PbComponentA, MUTABLE)\n";
		code += "	with_databag(ECS.PbDatabagA, MUTABLE)\n";
		code += "\n";
		code += "func _for_each(a, b):\n";
		code += "	pass\n";
		code += "\n";

		CHECK(build_and_register_ecs_script("test_B_system_8.gd", code));
	}

	ECS::register_system(test_B_system_9, "test_B_system_9");
	ECS::register_system(test_B_system_10, "test_B_system_10");

	{
		// Create the script.
		String code;
		code += "extends SystemBundle\n";
		code += "\n";
		code += "func _prepare():\n";
		code += "	execute_before(ECS.test_B_system_3)\n";
		code += "	execute_after(ECS.test_B_system_11_gd)\n";
		code += "	add(ECS.test_B_system_9)\n";
		code += "	add(ECS.test_B_system_7_gd)\n";
		code += "	add(ECS.test_B_system_8_gd)\n";
		code += "	add(ECS.test_B_system_10)\n";
		code += "\n";

		CHECK(build_and_register_ecs_script("GDSUsesCppBundle.gd", code));
	}

	{
		// Create the script.
		String code;
		code += "extends System\n";
		code += "\n";
		code += "func _prepare():\n";
		code += "	with_component(ECS.PbComponentA, IMMUTABLE)\n";
		code += "\n";
		code += "func _for_each(a):\n";
		code += "	pass\n";
		code += "\n";

		CHECK(build_and_register_ecs_script("test_B_system_11.gd", code));
	}

	flush_ecs_script_preparation();

	Vector<StringName> system_bundles;
	system_bundles.push_back("CppBundle");
	system_bundles.push_back("GDSBundle.gd");
	system_bundles.push_back("GDSUsesCppBundle.gd");

	Vector<StringName> systems;
	systems.push_back("test_B_system_11.gd");
	systems.push_back("test_B_system_12");

	Pipeline pipeline;
	PipelineBuilder::build_pipeline(system_bundles, systems, &pipeline);

	// C++ bundle
	const int stage_test_B_system_1 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_B_system_1")));
	const int stage_test_B_system_2 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_B_system_2")));
	const int stage_test_B_system_3 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_B_system_3")));
	const int stage_test_B_system_4 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_B_system_4")));

	// test_B_system_1 is marked to run in config phase, and has an implicit
	// dependecy with test_B_system_2.
	CHECK(stage_test_B_system_1 < stage_test_B_system_2);

	// test_B_system_2 has an implicit dependecy with test_B_system_3, and its
	// explicit priority (given by the system bundle insert order) forces it to
	// run before.
	CHECK(stage_test_B_system_2 < stage_test_B_system_3);

	// test_B_system_3 has an implicit dependecy with test_B_system_4 that is
	// marked to run in phase POST_PROCESS, so it runs as last.
	CHECK(stage_test_B_system_3 < stage_test_B_system_4);

	// GDScript bundle
	const int stage_test_B_system_5 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_B_system_5.gd")));
	const int stage_test_B_system_6 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_B_system_6.gd")));

	// The GDSBundle add two systems that even if run in differn phases they have
	// no implicit or explicit dependencies with anything else.
	// They can run in the same thread or one after the other depending how the
	// optimizer moves it:
	CHECK(stage_test_B_system_5 <= stage_test_B_system_6);

	// GDScript + cpp bundle

	const int stage_test_B_system_7 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_B_system_7.gd")));
	const int stage_test_B_system_8 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_B_system_8.gd")));
	const int stage_test_B_system_9 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_B_system_9")));
	const int stage_test_B_system_10 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_B_system_10")));

	// This bundle has an explicit dependency with system 6, all systems must run
	// before.
	CHECK(stage_test_B_system_7 < stage_test_B_system_3);
	CHECK(stage_test_B_system_8 < stage_test_B_system_3);
	CHECK(stage_test_B_system_9 < stage_test_B_system_3);
	CHECK(stage_test_B_system_10 < stage_test_B_system_3);

	// Make sure the explicit priority set by the bundle is respected.
	CHECK(stage_test_B_system_9 < stage_test_B_system_7);
	CHECK(stage_test_B_system_7 < stage_test_B_system_8);
	CHECK(stage_test_B_system_8 < stage_test_B_system_10);

	// Verify spare systems order.

	const int stage_test_B_system_11 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_B_system_11.gd")));
	const int stage_test_B_system_12 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_B_system_12")));

	CHECK(stage_test_B_system_11 < stage_test_B_system_9);
	CHECK(stage_test_B_system_11 < stage_test_B_system_12);

	finalize_script_ecs();
}
} // namespace godex_tests

void test_C_system_1(Query<const PbComponentA> &p_query) {}
void test_C_system_2(Query<const PbComponentA> &p_query) {}
void test_C_system_3(Query<const PbComponentA> &p_query) {}
void test_C_system_4(const World *) {}
void test_C_system_5(const SceneTreeDatabag *) {}
void test_C_system_6(Query<const PbComponentA> &p_query) {}
void test_C_system_7(Query<const PbComponentA> &p_query) {}

namespace godex_tests {
TEST_CASE("[Modules][ECS] Verify the PipelineBuilder bundles.") {
	initialize_script_ecs();

	ECS::register_system_bundle("TestC_CppBundle")
			.add(ECS::register_system(test_C_system_1, "test_C_system_1"))
			.add(ECS::register_system(test_C_system_2, "test_C_system_2"))
			.add(ECS::register_system(test_C_system_3, "test_C_system_3"))
			.add(ECS::register_system(test_C_system_4, "test_C_system_4"))
			.add(ECS::register_system(test_C_system_5, "test_C_system_5"))
			.add(ECS::register_system(test_C_system_6, "test_C_system_6"))
			.add(ECS::register_system(test_C_system_7, "test_C_system_7"));

	{
		// Create the script.
		String code;
		code += "extends System\n";
		code += "\n";
		code += "func _prepare():\n";
		code += "	with_component(ECS.PbComponentA, IMMUTABLE)\n";
		code += "	with_databag(ECS.World, IMMUTABLE)\n";
		code += "\n";
		code += "func _for_each(a, b):\n";
		code += "	pass\n";
		code += "\n";

		CHECK(build_and_register_ecs_script("test_C_system_8.gd", code));
	}
	{
		// Create the script.
		String code;
		code += "extends System\n";
		code += "\n";
		code += "func _prepare():\n";
		code += "	with_component(ECS.PbComponentA, IMMUTABLE)\n";
		code += "	with_databag(ECS.SceneTreeDatabag, IMMUTABLE)\n";
		code += "\n";
		code += "func _for_each(a, b):\n";
		code += "	pass\n";
		code += "\n";

		CHECK(build_and_register_ecs_script("test_C_system_9.gd", code));
	}
	{
		// Create the script.
		String code;
		code += "extends System\n";
		code += "\n";
		code += "func _prepare():\n";
		code += "	with_component(ECS.PbComponentA, IMMUTABLE)\n";
		code += "\n";
		code += "func _for_each(a):\n";
		code += "	pass\n";
		code += "\n";

		CHECK(build_and_register_ecs_script("test_C_system_10.gd", code));
	}
	{
		// Create the script.
		String code;
		code += "extends System\n";
		code += "\n";
		code += "func _prepare():\n";
		code += "	with_component(ECS.PbComponentA, IMMUTABLE)\n";
		code += "\n";
		code += "func _for_each(a):\n";
		code += "	pass\n";
		code += "\n";

		CHECK(build_and_register_ecs_script("test_C_system_11.gd", code));
	}
	{
		// Create the script.
		String code;
		code += "extends SystemBundle\n";
		code += "\n";
		code += "func _prepare():\n";
		code += "	add(ECS.test_C_system_8_gd)\n";
		code += "	add(ECS.test_C_system_9_gd)\n";
		code += "	add(ECS.test_C_system_10_gd)\n";
		code += "	add(ECS.test_C_system_11_gd)\n";
		code += "\n";

		CHECK(build_and_register_ecs_script("TestC_GDSBundle.gd", code));
	}

	flush_ecs_script_preparation();

	Vector<StringName> system_bundles;
	system_bundles.push_back("TestC_CppBundle");
	system_bundles.push_back("TestC_GDSBundle.gd");

	Vector<StringName> systems;

	Pipeline pipeline;
	PipelineBuilder::build_pipeline(system_bundles, systems, &pipeline);

	const int stage_test_C_system_1 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_C_system_1")));
	const int stage_test_C_system_2 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_C_system_2")));
	const int stage_test_C_system_3 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_C_system_3")));
	const int stage_test_C_system_4 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_C_system_4")));
	const int stage_test_C_system_5 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_C_system_5")));
	const int stage_test_C_system_6 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_C_system_6")));
	const int stage_test_C_system_7 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_C_system_7")));
	const int stage_test_C_system_8 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_C_system_8.gd")));
	const int stage_test_C_system_9 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_C_system_9.gd")));
	const int stage_test_C_system_10 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_C_system_10.gd")));
	const int stage_test_C_system_11 = pipeline.get_system_stage(ECS::get_system_id(StringName("test_C_system_11.gd")));

	// Make sure the systems that fetch World an SceneTreeDatabag runs always in
	// single thread.
	CHECK(stage_test_C_system_4 != stage_test_C_system_1);
	CHECK(stage_test_C_system_4 != stage_test_C_system_2);
	CHECK(stage_test_C_system_4 != stage_test_C_system_3);
	CHECK(stage_test_C_system_4 != stage_test_C_system_5);
	CHECK(stage_test_C_system_4 != stage_test_C_system_6);
	CHECK(stage_test_C_system_4 != stage_test_C_system_7);
	CHECK(stage_test_C_system_4 != stage_test_C_system_8);
	CHECK(stage_test_C_system_4 != stage_test_C_system_9);
	CHECK(stage_test_C_system_4 != stage_test_C_system_10);
	CHECK(stage_test_C_system_4 != stage_test_C_system_11);

	CHECK(stage_test_C_system_5 != stage_test_C_system_1);
	CHECK(stage_test_C_system_5 != stage_test_C_system_2);
	CHECK(stage_test_C_system_5 != stage_test_C_system_3);
	CHECK(stage_test_C_system_5 != stage_test_C_system_4);
	CHECK(stage_test_C_system_5 != stage_test_C_system_6);
	CHECK(stage_test_C_system_5 != stage_test_C_system_7);
	CHECK(stage_test_C_system_5 != stage_test_C_system_8);
	CHECK(stage_test_C_system_5 != stage_test_C_system_9);
	CHECK(stage_test_C_system_5 != stage_test_C_system_10);
	CHECK(stage_test_C_system_5 != stage_test_C_system_11);

	CHECK(stage_test_C_system_8 != stage_test_C_system_1);
	CHECK(stage_test_C_system_8 != stage_test_C_system_2);
	CHECK(stage_test_C_system_8 != stage_test_C_system_3);
	CHECK(stage_test_C_system_8 != stage_test_C_system_4);
	CHECK(stage_test_C_system_8 != stage_test_C_system_5);
	CHECK(stage_test_C_system_8 != stage_test_C_system_6);
	CHECK(stage_test_C_system_8 != stage_test_C_system_7);
	CHECK(stage_test_C_system_8 != stage_test_C_system_9);
	CHECK(stage_test_C_system_8 != stage_test_C_system_10);
	CHECK(stage_test_C_system_8 != stage_test_C_system_11);

	CHECK(stage_test_C_system_9 != stage_test_C_system_1);
	CHECK(stage_test_C_system_9 != stage_test_C_system_2);
	CHECK(stage_test_C_system_9 != stage_test_C_system_3);
	CHECK(stage_test_C_system_9 != stage_test_C_system_4);
	CHECK(stage_test_C_system_9 != stage_test_C_system_5);
	CHECK(stage_test_C_system_9 != stage_test_C_system_6);
	CHECK(stage_test_C_system_9 != stage_test_C_system_7);
	CHECK(stage_test_C_system_9 != stage_test_C_system_8);
	CHECK(stage_test_C_system_9 != stage_test_C_system_10);
	CHECK(stage_test_C_system_9 != stage_test_C_system_11);

	finalize_script_ecs();
}
} // namespace godex_tests

#endif // TEST_ECS_PIPELINE_BUILDER_H
