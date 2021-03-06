#pragma once

// Standard
#include <unordered_set>
#include <set>
#include <list>
#include <functional>

#include "SculptMesh.hpp"
#include "Renderer.hpp"


/* TODO:
- select stuff
  implement UI clear elements function to remove elements in the next frame
  need some kind of context for input
  need selection overlay FX
- delete vertex
- delete edge

- createUnorderedPoly, winding based on normal
- frame camera to mesh
- Surface Detail shading mode
- compute shader mesh deform
- save mesh to file and load from file
- vert groups, edge groups, poly groups
- mesh LOD using the AABBs
- MeshInstanceAABB
- dynamic shader reloading
*/


// Forward
namespace nui {
	class Window;
}

struct MeshLayer;
struct MeshDrawcall;
struct MeshInstanceSet;
struct MeshInstance;
struct Mesh;


enum class DisplayMode {
	SOLID,
	WIREFRAME_OVERLAY,
	WIREFRANE
};

/* which subprimitive holds the surface data to respond to the light */
namespace GPU_ShadingNormal {
	enum {
		VERTEX,
		POLY,
		TESSELATION  // quads are split into 2 triangles and each has a normal
	};
}

//
//enum class SelectionMode {
//
//};


/* light that is relative to the camera orientation */
struct CameraLight {
	glm::vec3 normal;

	glm::vec3 color;
	float intensity;
};

enum class AABB_RenderMode {
	NO_RENDER,  // disable AABB rendering
	LEAF_ONLY,  // only render AABB that have vertices assigned to it
	NORMAL  // render leafs and AABB that have child AABBs
};

// how to display a set of instances of a mesh
struct MeshDrawcall {
	std::string name;
	std::string description;

	DisplayMode display_mode;
	bool is_back_culled;
	AABB_RenderMode aabb_render_mode;
};


enum class ModifiedInstanceType {
	UPDATE,
	DELETED
};

// What instance was modified and what was modified
struct ModifiedMeshInstance {
	uint32_t idx;

	ModifiedInstanceType type;
};


// Size and position of a instance
struct MeshTransform {
	glm::vec3 pos = { .0f, .0f, .0f };
	glm::quat rot = { 1.0f, .0f, .0f, .0f };
	glm::vec3 scale = { 1.0f, 1.0f, 1.0f };
};

// The apperance of the material beloging to a mesh
struct PhysicalBasedMaterial {
	glm::vec3 albedo_color = { 1.0f, 0.0f, 0.0f };
	float roughness = 0.3f;
	float metallic = 0.1f;
	float specular = 0.04f;
};

// Coloring of a instance's wireframe
struct MeshWireframeColors {
	glm::vec3 front_color = { 0.0f, 1.0f, 0.0f };
	glm::vec4 back_color = { 0.0f, 0.20f, 0.0f, 1.0f };
	glm::vec3 tesselation_front_color = { 0.0f, 1.0f, 0.0f };
	glm::vec4 tesselation_back_color = { 0.0f, 0.20f, 0.0f, 1.0f };
	float tesselation_split_count = 4.0f;
	float tesselation_gap = 0.5f;
};


// A copy of a mesh with slightly different look
struct MeshInstance {
	MeshInstanceSet* instance_set;
	uint32_t index_in_buffer;

	MeshLayer* parent_layer;

	std::string name;
	std::string description;
	bool visible;  // not really used
	// bool can_collide;

	// Instance Data
	MeshTransform transform;

	PhysicalBasedMaterial pbr_material;
	MeshWireframeColors wireframe_colors;

	bool instance_select_outline;

public:
	scme::SculptMesh& getSculptMesh();

	// Scheduling methods
	// Each of the below methods will schedule the renderer to load data on the GPU
	// Avoid calling more than one method

	// mark to have it's data updated on the GPU
	void markFullUpdate();
};


struct MeshInstanceRef {
	MeshInstanceSet* instance_set;
	uint32_t index_in_buffer;

public:
	bool operator==(MeshInstanceRef& other);

public:
	MeshInstance* get();
};


// Instances of mesh to rendered with a certain drawcall
struct MeshInstanceSet {
	Mesh* parent_mesh;

	MeshDrawcall* drawcall;  // with what settings to render this set of instances

	// the instances of the mesh to be rendered in one drawcall
	// adding or removing cause MeshInstance* to be invalid
	SparseVector<MeshInstance> instances;
	std::vector<ModifiedMeshInstance> modified_instances;
	dx11::ArrayBuffer<GPU_MeshInstance> gpu_instances;
	ComPtr<ID3D11ShaderResourceView> gpu_instances_srv;
};

// Vertices and indexes of geometry
struct Mesh {
	scme::SculptMesh mesh;

	std::list<MeshInstanceSet> sets;

	// should vertices for AABBs be generated for rendering and should they be rendered
	AABB_RenderMode aabb_render_mode;
	AABB_RenderMode prev_aabb_mode;  // to make AABB vertex buffer be recreated on AABB render mode switch
};


struct MeshLayer {
	MeshLayer* _parent;
	std::unordered_set<MeshLayer*> _children;

	std::string name;
	std::string description;
	std::list<MeshInstanceRef> instances;
};


namespace SelectionMode {
	enum {
		INSTANCE = 1 << 0,
		POLY     = 1 << 1,
		EDGE     = 1 << 2,
		VERTEX   = 1 << 3
	};
}

namespace InteractionModes {
	enum {
		NOTHING,
		DEFAULT,

		INSTANCE_SELECTION,
		INSTANCE_MOVE,
		INSTANCE_ROTATION,
		INSTANCE_SCALE,

		MESH,
		VERTEX_SELECT,
		EDGE_SELECT,
		POLY_SELECT,

		SCULPT,
		STANDARD_BRUSH,

		ENUM_SIZE
	};
}

struct InteractionMode {
	uint32_t parent;
	std::vector<uint32_t> children;

	std::function<void()> enter;
	std::function<void()> exit;
};


// Mesh Creation Parameters

struct CreateTriangleInfo {
	MeshTransform transform;
	float size = 1.f;
};

struct CreateQuadInfo {
	MeshTransform transform;
	float size = 1.f;
};

struct CreateWavyGridInfo {
	MeshTransform transform;
	float size = 1.f;
};

struct CreateCubeInfo {
	MeshTransform transform;
	float size = 1.f;
};

struct CreateCylinderInfo {
	MeshTransform transform;
	float diameter = 1.f;
	float height = 1.f;

	uint32_t rows = 2;
	uint32_t columns = 3;
	bool with_caps = true;
};

struct CreateUV_SphereInfo {
	MeshTransform transform;
	float diameter = 1.0f;

	uint32_t rows = 2;
	uint32_t columns = 3;
};

struct GLTF_ImporterSettings {
	MeshLayer* dest_layer = nullptr;
	MeshDrawcall* dest_drawcall = nullptr;
};

struct CreateLineInfo {
	glm::vec3 origin = { 0.f, 0.f, 0.f};
	glm::vec3 direction = { 0.f, 0.f, -1.0f };
	float length = 1.0f;
};


struct RaytraceInstancesResult {
	MeshInstanceRef inst_ref;
	uint32_t poly;
	glm::vec3 local_isect;  // local position of intersection same for instances of the same mesh
	glm::vec3 global_isect;  // local position of intersection
};

struct UserInterface {
	nui::Flex* viewport;
};


class Application {
public:
	nui::Instance ui_instance;
	nui::Window* main_window;
	UserInterface ui;

	// Interaction
	uint32_t now_interaction;
	std::array<InteractionMode, InteractionModes::ENUM_SIZE> interactions;

	// Meshes
	std::list<Mesh> meshes;

	// Drawcalls
	std::list<MeshDrawcall> drawcalls;

	// Layers
	std::list<MeshLayer> layers;


	// Selection

	// current selection of instances
	std::list<MeshInstanceRef> instance_selection;


	// current mesh in sculpt mode
	Mesh* sculpt_target;

	// Shading
	uint32_t shading_normal;  // what normal to use when shading the mesh in the pixel shader

	// Lighting
	std::array<CameraLight, 8> lights;  // lights don't have position the only have a direction
	float ambient_intensity;  // base ambient light added to the color


	// Camera

	// 3D position in space where the camera is starring at,
	// used to tweak camera sensitivity to make movement consistent across al mesh sizes
	glm::vec3 camera_focus;

	// horizontal field of view used in the perspective matrix
	float camera_field_of_view;

	// defines the clipping planes used in rendering (does not affect depth)
	float camera_z_near;
	float camera_z_far;

	// camera position in global space
	glm::vec3 camera_pos;

	// reverse camera rotation applied in vertex shader
	glm::quat camera_quat_inv;

	// camera's direction of pointing
	glm::vec3 camera_forward;

	// camera movement sensitivity
	float camera_orbit_sensitivity;
	float camera_pan_sensitivity;
	float camera_dolly_sensitivity;

public:
	Mesh& _createMesh();
	MeshInstanceRef _addInstance(Mesh& mesh, MeshLayer* dest_layer, MeshDrawcall* dest_drawcall);
	void _unparentInstanceFromParentLayer(MeshInstanceRef& instance);

	struct _RaytraceInstancesResult {
		MeshInstance* inst;
		uint32_t poly;
		glm::vec3 local_isect;  // local position of intersection same for instances of the same mesh
		glm::vec3 global_isect;  // local position of intersection
	};
public:

	// Interaction
	
	void navigateUp();

	void navigateToChild(uint32_t interaction_mode);


	// Scene

	// clear scene state and inits to default values
	void resetScene();


	// Instances

	// Invalidates MeshInstance*
	MeshInstanceRef copyInstance(MeshInstanceRef& source);

	// Invalidates MeshInstance*
	void deleteInstance(MeshInstanceRef& inst);

	// unparent from parent layer and make child to destination layer
	void transferInstanceToLayer(MeshInstanceRef& instance, MeshLayer* dest_layer);

	// Copies Instance from one set to another set of the same Mesh that has a set with the
	// destination drawcall. If the set with the destination drawcall does not exist it is created
	// 
	// Returns a reference to instance allocated on a different instance set of the same mesh
	// Invalidates MeshInstance* and MeshInstanceRef
	//MeshInstanceRef moveInstanceToDrawcall(MeshInstanceRef& instance, MeshDrawcall* dest_drawcall);

	void rotateInstance(MeshInstance* mesh_instance, float x, float y, float z);


	// Drawcalls

	// Creates a drawcall to be associated with instances
	MeshDrawcall* createDrawcall();

	// Deletes a drawcall and moves instances to the last used drawcall
	//void deleteDrawcall();

	// get's the default drawcall referenced in the menus
	MeshDrawcall& getRootDrawcall();

	// Iterate over all instances that are rendered with that drawcall and turn on AABB
	// vertex generation
	// needs to be retoggled if mesh changes to recreate AABB vertex buffers
	void setAABB_RenderModeForDrawcall(MeshDrawcall* drawcall, AABB_RenderMode aabb_render_mode);


	// Layers

	// Creates a new layer by default parented to root
	MeshLayer* createLayer(MeshLayer* parent = nullptr);

	// unparent and make child to parent layer
	void transferLayer(MeshLayer* child_layer, MeshLayer* parent_layer);

	// Recursivelly sets the visibility state for all instances of a layer
	void setLayerVisibility(MeshLayer* layer, bool visible_state);


	// Create Meshes
	// All of them invalidate MeshInstance*

	MeshInstanceRef createEmptyMesh(MeshLayer* dest_layer = nullptr, MeshDrawcall* dest_drawcall = nullptr);
	MeshInstanceRef createTriangle(CreateTriangleInfo& info, MeshLayer* dest_layer = nullptr, MeshDrawcall* dest_drawcall = nullptr);
	MeshInstanceRef createQuad(CreateQuadInfo& info, MeshLayer* dest_layer = nullptr, MeshDrawcall* dest_drawcall = nullptr);
	MeshInstanceRef createWavyGrid(CreateWavyGridInfo& info, MeshLayer* dest_layer = nullptr, MeshDrawcall* dest_drawcall = nullptr);
	MeshInstanceRef createCube(CreateCubeInfo& infos, MeshLayer* dest_layer = nullptr, MeshDrawcall* dest_drawcall = nullptr);
	MeshInstanceRef createCylinder(CreateCylinderInfo& info, MeshLayer* dest_layer = nullptr, MeshDrawcall* dest_drawcall = nullptr);
	MeshInstanceRef createUV_Sphere(CreateUV_SphereInfo& info, MeshLayer* dest_layer = nullptr, MeshDrawcall* dest_drawcall = nullptr);
	
	// import a GLTF from path
	ErrStack importMeshesFromGLTF_File(io::FilePath& path, GLTF_ImporterSettings& settings,
		std::vector<MeshInstanceRef>* r_instances = nullptr);

	//MeshInstance* createLine(CreateLineInfo& info, MeshLayer* dest_layer = nullptr, MeshDrawcall* dest_drawcall = nullptr);
	
	// all sources get converted into geometry and added to the destination mesh
	// Invalidates MeshInstance*
	void joinMeshes(std::vector<MeshInstanceRef>& sources, uint32_t destination_idx);


	// Sculpt Mode

	bool enterSculptMode();


	// Raycasts

	// performs a raycast from camera position to pixel world position of the mouse
	bool mouseRaycastInstances(RaytraceInstancesResult& r_isect);


	// Camera

	// sets the camera point of focus to adjust sensitivity
	void setCameraFocus(glm::vec3& new_focus);

	// orbit camera around focus
	void arcballOrbitCamera(float deg_x, float deg_y);

	// not really used
	void pivotCameraAroundY(float deg_x, float deg_y);

	// moves the camera along the camera's up and right axes
	void panCamera(float amount_x, float amount_y);

	// moves the camera along the camera's forward axis
	void dollyCamera(float amount);

	// sets the camera's global position
	void setCameraPosition(float x, float y, float z);

	// sets the camera's rotation
	// arguments are Euler and get converted to quartenion
	void setCameraRotation(float x, float y, float z);

	//void frameCameraToSelection();


	// Scene

	// reset everything, also used for initialization
	void resetToHardcodedStartup();


	// Other

	// which primitive type will have it's normal interact with the lighting
	void setShadingNormal(uint32_t shading_normal);


	// Debug

	// trigger Visual Studio breakpoint when key is down
	void triggerBreakpointOnKey(uint32_t key_down = nui::VirtualKeys::F8);
};

extern Application application;

void endFrameEvents(nui::Window*, void*);
