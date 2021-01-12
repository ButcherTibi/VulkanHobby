
#include "NuiLibrary.hpp"
#include "Application.hpp"


void onCameraOrbitKeyDown(nui::KeyDownEvent& event)
{
	nui::Surface* wrap = std::get_if<nui::Surface>(&event.source->elem);

	wrap->beginMouseDelta();
}

void onCameraOrbitKeyHeld(nui::KeyHeldDownEvent& event)
{
	int32_t delta_x;
	int32_t delta_y;
	application.window->getMouseDelta(delta_x, delta_y);

	float scaling = application.camera_orbit_sensitivity * application.window->delta_time;
	application.arcballOrbitCamera((float)delta_x * scaling, (float)delta_y * scaling);
}

void onCameraOrbitKeyUp(nui::KeyUpEvent& event)
{
	nui::Surface* wrap = std::get_if<nui::Surface>(&event.source->elem);

	wrap->endMouseDelta();
}

void onCameraPanKeyDown(nui::KeyDownEvent& event)
{
	nui::Surface* wrap = std::get_if<nui::Surface>(&event.source->elem);

	wrap->beginMouseDelta();
}

void onCameraPanKeyHeld(nui::KeyHeldDownEvent& event)
{
	int32_t delta_x;
	int32_t delta_y;
	application.window->getMouseDelta(delta_x, delta_y);

	float scaling = application.camera_pan_sensitivity * application.window->delta_time;
	application.panCamera((float)-delta_x * scaling, (float)-delta_y * scaling);
}

void onCameraPanKeyUp(nui::KeyUpEvent& event)
{
	nui::Surface* wrap = std::get_if<nui::Surface>(&event.source->elem);

	wrap->endMouseDelta();
}

void onCameraDollyScroll(nui::MouseScrollEvent& event)
{
	application.dollyCamera(event.scroll_delta * application.camera_dolly_sensitivity);
}

void onCameraResetKeyDown(nui::KeyDownEvent& event)
{
	application.setCameraPosition(0, 0, 10);
	application.setCameraRotation(0, 0, 0);
}

int WINAPI WinMain(_In_ HINSTANCE hinstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR pCmdLine, _In_ int nCmdShow)
{
	ErrStack err_stack;

	// Application
	{
		// Layers
		application.last_used_layer = &application.layers.emplace_back();
		application.last_used_layer->parent = nullptr;
		application.last_used_layer->name = "Root Layer";

		// Instances
		application.instance_id = 0;

		// Drawcalls
		application.last_used_drawcall = &application.drawcalls.emplace_back();
		application.last_used_drawcall->name = "Root Drawcall";
		application.last_used_drawcall->rasterizer_mode = RasterizerMode::SOLID;
		application.last_used_drawcall->show_aabbs = false;

		// Shading
		application.shading_normal = ShadingNormal::VERTEX;

		// Lighting
		application.lights[0].normal = toNormal(45, 45);
		application.lights[0].color = { 1, 1, 1 };
		application.lights[0].intensity = 1.f;

		application.lights[1].normal = toNormal(-45, 45);
		application.lights[1].color = { 1, 1, 1 };
		application.lights[1].intensity = 1.f;

		application.lights[2].normal = toNormal(45, -45);
		application.lights[2].color = { 1, 1, 1 };
		application.lights[2].intensity = 1.f;

		application.lights[3].normal = toNormal(-45, -45);
		application.lights[3].color = { 1, 1, 1 };
		application.lights[3].intensity = 1.f;

		application.lights[4].intensity = 0.f;
		application.lights[5].intensity = 0.f;
		application.lights[6].intensity = 0.f;
		application.lights[7].intensity = 0.f;

		application.ambient_intensity = 0.03f;

		// Camera
		application.camera_focus = { 0, 0, 0 };
		application.camera_field_of_view = 15;
		application.camera_z_near = 100'000'000.f;
		application.camera_z_far = 0.1f;
		application.camera_pos = { 0, 0, 0 };
		application.camera_quat_inv = { 1, 0, 0, 0 };
		application.camera_forward = { 0, 0, -1 };
		application.camera_orbit_sensitivity = 50000;
		application.camera_pan_sensitivity = 250;
		application.camera_dolly_sensitivity = 0.001f;

		// Tests	
		application.setCameraPosition(0, 0, 10);
		application.setCameraFocus(glm::vec3(0, 0, 0));

		MeshDrawcall* drawcall_1 = application.createDrawcall();
		drawcall_1->rasterizer_mode = RasterizerMode::SOLID;

		MeshDrawcall* drawcall_2 = application.createDrawcall();
		drawcall_2->rasterizer_mode = RasterizerMode::SOLID_WITH_WIREFRAME_NONE;

		MeshDrawcall* drawcall_3 = application.createDrawcall();
		drawcall_3->rasterizer_mode = RasterizerMode::SOLID;

		/*{
			CreateTriangleInfo info;
			info.transform.pos = { 0, -2, 0 };

			application.createTriangleMesh(info, nullptr, drawcall_2);
		}*/
		
		/*{
			CreateQuadInfo info;
			info.transform.pos = { 0, -4, 0 };

			application.createQuadMesh(info, nullptr, drawcall_2);
		}
		
		{
			CreateCubeInfo info;
			info.transform.pos = { 0, -6, 0};

			application.createCubeMesh(info, nullptr, drawcall_2);
		}

		{
			CreateCylinderInfo info;
			info.transform.pos = { 0, -8, 0 };

			info.vertical_sides = 2;
			info.horizontal_sides = 15;

			application.createCylinder(info, nullptr, drawcall_2);
		}*/
		
		/*{
			CreateUV_SphereInfo info;
			info.transform.pos = { 0, 0, 0 };
			info.diameter = 1.0f;

			info.vertical_sides = 15;
			info.horizontal_sides = 15;

			MeshInstance* inst = application.createUV_Sphere(info, nullptr, drawcall_2);
		}*/

		/*uint32_t rows = 7;
		uint32_t cols = 7;
		for (uint32_t row = 0; row < rows; row += 1) {
			for (uint32_t col = 0; col < cols; col += 1) {

				CreateUV_SphereInfo info;
				info.transform.pos = { col * 2, row * 2, 0 };
				info.diameter = 1;

				info.vertical_sides = 40;
				info.horizontal_sides = 80;

				MeshInstance& inst = application.createUV_Sphere(info);
				inst.pbr_material.albedo_color = { 1, 0, 0 };
				inst.pbr_material.roughness = (float)col / (cols - 1);
				inst.pbr_material.metallic = (float)row / (rows - 1);
			}
		}*/

		{
			io::FilePath path;
			path.recreateRelative("Sculpt/Meshes/Journey/scene.gltf");

			GLTF_ImporterSettings settings;
			settings.dest_drawcall = drawcall_1;

			std::vector<MeshInstance*> new_instances;

			err_stack = application.importMeshesFromGLTF_File(path, settings, &new_instances);
			if (err_stack.isBad()) {
				err_stack.debugPrint();
				return 1;
			}

			for (MeshInstance*& inst : new_instances) {
				inst = application.copyInstance(inst);
				inst->pos.x = 100;
				application.transferToDrawcall(inst, drawcall_2);
			}

			for (MeshInstance*& inst : new_instances) {
				inst = application.copyInstance(inst);
				inst->pos.x = 200;
				application.transferToDrawcall(inst, drawcall_1);
			}
		}
	}

	nui::Instance instance;
	err_stack = instance.create();
	if (err_stack.isBad()) {
		err_stack.debugPrint();
		return 1;
	}

	nui::WindowCreateInfo info;
	info.width = 1027;
	info.height = 720;

	err_stack = instance.createWindow(info, application.window);
	if (err_stack.isBad()) {
		err_stack.debugPrint();
		return 1;
	}

	// UI code
	nui::Surface* surface = application.window->addSurface();
	surface->gpu_callback = geometryDraw;
	surface->user_data = &application.renderer;

	// Camera Orbit
	surface->addKeyDownEvent(onCameraOrbitKeyDown, nui::VirtualKeys::RIGHT_MOUSE_BUTTON);
	surface->addKeyHeldDownEvent(onCameraOrbitKeyHeld, nui::VirtualKeys::RIGHT_MOUSE_BUTTON);
	surface->addKeyUpEvent(onCameraOrbitKeyUp, nui::VirtualKeys::RIGHT_MOUSE_BUTTON);

	// Camera Pan
	surface->addKeyDownEvent(onCameraPanKeyDown, nui::VirtualKeys::MIDDLE_MOUSE_BUTTON);
	surface->addKeyHeldDownEvent(onCameraPanKeyHeld, nui::VirtualKeys::MIDDLE_MOUSE_BUTTON);
	surface->addKeyUpEvent(onCameraPanKeyUp, nui::VirtualKeys::MIDDLE_MOUSE_BUTTON);

	// Camera Dolly
	surface->setMouseScrollEvent(onCameraDollyScroll);

	// Camera Reset
	surface->addKeyDownEvent(onCameraResetKeyDown, nui::VirtualKeys::R);

	while (!application.window->close) {

		err_stack = instance.update();
		if (err_stack.isBad()) {
			err_stack.debugPrint();
			return 1;
		}
	}

	return 0;
}

int main(int argc, char** argv)
{
	return WinMain(GetModuleHandleA(NULL), NULL, "", 1);
}
