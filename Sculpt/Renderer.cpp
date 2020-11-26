
// Header
#include "Renderer.hpp"

// GLM
#include <glm/gtc/matrix_transform.hpp>

#include "Application.hpp"


using namespace scme;


void MeshRenderer::generateVertices()
{
	uint32_t vertex_count = 0;

	// Count the number of vertices to rendered
	for (scme::SculptMesh& mesh : application.meshes) {

		for (Poly& poly : mesh.polys) {

			if (poly.vs[3] == nullptr) {
				vertex_count += 3;
			}
			else {
				vertex_count += 6;
			}
		}
	}
	vertices.resize(vertex_count);

	// Load vertices into common buffer
	uint32_t vi = 0;

	for (scme::SculptMesh& mesh : application.meshes) {

		mesh._vertex_start = vi;

		for (Poly& poly : mesh.polys) {

			if (poly.vs[3] == nullptr) {

				vertices[vi].pos = dxConvert(poly.vs[0]->pos);
				vertices[vi + 1].pos = dxConvert(poly.vs[1]->pos);
				vertices[vi + 2].pos = dxConvert(poly.vs[2]->pos);

				vertices[vi].normal = dxConvert(poly.vs[0]->normal);
				vertices[vi + 1].normal = dxConvert(poly.vs[1]->normal);
				vertices[vi + 2].normal = dxConvert(poly.vs[2]->normal);

				vi += 3;
			}
			else {
				std::array<Vertex*, 6> tess;

				if (poly.tesselation_type == 0) {

					tess[0] = poly.vs[0];
					tess[1] = poly.vs[2];
					tess[2] = poly.vs[3];

					tess[3] = poly.vs[0];
					tess[4] = poly.vs[1];
					tess[5] = poly.vs[2];
				}
				else {
					tess[0] = poly.vs[0];
					tess[1] = poly.vs[1];
					tess[2] = poly.vs[3];

					tess[3] = poly.vs[1];
					tess[4] = poly.vs[2];
					tess[5] = poly.vs[3];
				}

				for (uint32_t i = 0; i < 6; i++) {
					vertices[vi + i].pos = dxConvert(tess[i]->pos);
					vertices[vi + i].normal = dxConvert(tess[i]->normal);
				}

				vi += 6;
			}
		}

		mesh._vertex_count = vi - mesh._vertex_start;
	}

	// Load instances
	uint32_t inst_idx = 0;
	instances.resize(application.instances.size());
	mesh_draws.resize(application.instances.size());

	// Create Drawcalls
	for (MeshInstance& mesh_instance : application.instances) {

		GPU_MeshInstance& gpu_inst = instances[inst_idx];
		gpu_inst.pos = dxConvert(mesh_instance.pos);
		gpu_inst.rot = dxConvert(mesh_instance.rot);

		DrawMesh& draw_mesh = mesh_draws[inst_idx];
		draw_mesh.vertex_start = mesh_instance.mesh->_vertex_start;
		draw_mesh.vertex_count = mesh_instance.mesh->_vertex_count;
		draw_mesh.instance_start = inst_idx;
	}
}

void MeshRenderer::generateUniform()
{
	uniform.camera_pos = dxConvert(application.camera_pos);
	uniform.camera_quat = dxConvert(application.camera_quat_inv);
	uniform.camera_forward = dxConvert(application.camera_forward);

	uniform.perspective_matrix = dxConvert(glm::perspectiveFovRH_ZO(toRad(application.camera_field_of_view),
		(float)viewport_width, (float)viewport_height,
		application.camera_z_near, application.camera_z_far));
}

ErrStack MeshRenderer::loadVertices()
{
	ErrStack err_stack;

	checkErrStack(vbuff.load(vertices.data(), vertices.size() * sizeof(GPU_MeshVertex)),
		"failed to load mesh vertices");

	checkErrStack(instabuff.load(instances.data(), instances.size() * sizeof(GPU_MeshInstance)),
		"failed to load mesh instances");

	return err_stack;
}

ErrStack MeshRenderer::loadUniform()
{
	ErrStack err_stack;

	checkErrStack(ubuff.load(&uniform, sizeof(GPU_MeshUniform)),
		"failed to load uniform buffer");

	return err_stack;
}

ErrStack MeshRenderer::draw(nui::SurfaceEvent& event)
{
	ErrStack err_stack;
	HRESULT hr = S_OK;

	viewport_width = event.surface_width;
	viewport_height = event.surface_height;

	if (dev5 == nullptr) {

		// Init
		dev5 = event.dev5;
		de_ctx3 = event.de_ctx3;
		
		load_vertices = true;
		load_uniform = true;

		// Depth Resource
		{
			D3D11_TEXTURE2D_DESC desc = {};
			desc.Width = this->viewport_width;
			desc.Height = this->viewport_height;
			desc.MipLevels = 1;
			desc.ArraySize = 1;
			desc.Format = DXGI_FORMAT_D32_FLOAT;
			desc.SampleDesc.Count = 1;
			desc.SampleDesc.Quality = 0;
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
			desc.CPUAccessFlags = 0;
			desc.MiscFlags = 0;

			checkHResult(dev5->CreateTexture2D(&desc, nullptr, depth_tex.GetAddressOf()),
				"failed to create mesh depth texture");

			D3D11_DEPTH_STENCIL_VIEW_DESC view_desc = {};
			view_desc.Format = DXGI_FORMAT_D32_FLOAT;
			view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
			view_desc.Texture2D.MipSlice = 0;

			checkHResult(dev5->CreateDepthStencilView(depth_tex.Get(), &view_desc, depth_view.GetAddressOf()),
				"failed to create mesh depth view");
		}

		// Vertex Buffer
		{
			D3D11_BUFFER_DESC desc = {};
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;

			vbuff.create(dev5, de_ctx3, desc);
		}

		// Instance Buffer
		{
			D3D11_BUFFER_DESC desc = {};
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;

			instabuff.create(dev5, de_ctx3, desc);
		}

		// Uniform Buffer
		{
			D3D11_BUFFER_DESC desc = {};
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;

			ubuff.create(dev5, de_ctx3, desc);
		}

		// Vertex Shader
		{
			checkErrStack1(io::readLocalFile("Sculpt/CompiledShaders/MeshVS.cso", mesh_vs_cso));

			checkHResult(dev5->CreateVertexShader(mesh_vs_cso.data(), mesh_vs_cso.size(), nullptr,
				mesh_vs.GetAddressOf()),
				"failed to create mesh vertex shader");
		}

		// Pixel Shader
		{
			checkErrStack1(io::readLocalFile("Sculpt/CompiledShaders/MeshPS.cso", mesh_ps_cso));

			checkHResult(dev5->CreatePixelShader(mesh_ps_cso.data(), mesh_ps_cso.size(), nullptr,
				mesh_ps.GetAddressOf()),
				"failed to create mesh pixel shader");
		}

		// Vertex Input Layout
		{
			auto vertex_elems = GPU_MeshVertex::getInputLayout();
			auto instance_elems = GPU_MeshInstance::getInputLayout();

			std::vector<D3D11_INPUT_ELEMENT_DESC> elems;
			elems.resize(vertex_elems.size() + instance_elems.size());
			
			uint32_t idx = 0;
			for (D3D11_INPUT_ELEMENT_DESC& elem: vertex_elems) {
				elems[idx++] = elem;
			}

			for (D3D11_INPUT_ELEMENT_DESC& elem : instance_elems) {
				elems[idx++] = elem;
			}

			checkHResult(dev5->CreateInputLayout(elems.data(), elems.size(),
				mesh_vs_cso.data(), mesh_vs_cso.size(), mesh_il.GetAddressOf()),
				"failed to create mesh vertex input layout");
		}

		// Mesh Rasterization State
		{
			D3D11_RASTERIZER_DESC desc = {};
			desc.FillMode = D3D11_FILL_SOLID;
			desc.CullMode = D3D11_CULL_BACK;
			desc.FrontCounterClockwise = false;
			desc.DepthBias = 0;
			desc.DepthBiasClamp = 0;
			desc.SlopeScaledDepthBias = 0;
			desc.DepthClipEnable = false;
			desc.ScissorEnable = true;
			desc.MultisampleEnable = false;
			desc.AntialiasedLineEnable = false;

			checkHResult(dev5->CreateRasterizerState(&desc, mesh_rs.GetAddressOf()),
				"failed to create mesh rasterized state");
		}

		// Mesh Blend States
		{
			D3D11_BLEND_DESC desc = {};
			desc.AlphaToCoverageEnable = false;
			desc.IndependentBlendEnable = true;
			std::memset(desc.RenderTarget, 0, 8);

			desc.RenderTarget[0].BlendEnable = false;
			desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
			desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
			desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

			desc.RenderTarget[1].BlendEnable = false;
			desc.RenderTarget[1].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

			checkHResult(dev5->CreateBlendState(&desc, mesh_bs.GetAddressOf()),
				"failed to create mesh blend state");
		}

		// Mesh Depth Stencil
		{
			D3D11_DEPTH_STENCIL_DESC desc = {};
			desc.DepthEnable = true;
			desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
			desc.DepthFunc = D3D11_COMPARISON_LESS;
			desc.StencilEnable = false;
			desc.StencilReadMask = 0xFF;
			desc.StencilWriteMask = 0xFF;

			desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
			desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
			desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
			desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

			desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
			desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
			desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
			desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

			checkHResult(dev5->CreateDepthStencilState(&desc, mesh_dss.GetAddressOf()),
				"failed to create mesh depth stencil state");
		}
	}

	if (load_vertices) {
		generateVertices();
		checkErrStack1(loadVertices());

		load_vertices = false;
	}

	if (load_uniform) {
		generateUniform();
		checkErrStack1(loadUniform());

		load_uniform = false;
	}

	// Command List
	de_ctx3->ClearState();
	de_ctx3->ClearDepthStencilView(depth_view.Get(), D3D11_CLEAR_DEPTH, 1, 0);

	// for each instance draw

	for (DrawMesh& draw : mesh_draws) {

		// Input Assembly
		de_ctx3->IASetInputLayout(mesh_il.Get());
		de_ctx3->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		{
			std::array<ID3D11Buffer*, 2> buffers = {
				vbuff.buff.Get(), instabuff.buff.Get()
			};
			std::array<uint32_t, 2> strides = {
				sizeof(GPU_MeshVertex), sizeof(GPU_MeshInstance)
			};
			std::array<uint32_t, 2> offsets = {
				0, 0
			};
			de_ctx3->IASetVertexBuffers(0, buffers.size(), buffers.data(), strides.data(), offsets.data());
		}

		// Vertex Shader
		de_ctx3->VSSetConstantBuffers(0, 1, ubuff.buff.GetAddressOf());
		de_ctx3->VSSetShader(mesh_vs.Get(), nullptr, 0);

		// Rasterization
		{
			D3D11_VIEWPORT viewport;
			viewport.TopLeftX = (float)event.pos.x;
			viewport.TopLeftY = (float)event.pos.y;
			viewport.Width = (float)event.surface_width;
			viewport.Height = (float)event.surface_height;
			viewport.MinDepth = 0;
			viewport.MaxDepth = 1;

			de_ctx3->RSSetViewports(1, &viewport);

			D3D11_RECT sccissor;
			sccissor.left = event.pos.x;
			sccissor.top = event.pos.y;
			sccissor.right = event.pos.x + event.surface_width;
			sccissor.bottom = event.pos.y + event.surface_height;

			de_ctx3->RSSetScissorRects(1, &sccissor);

			de_ctx3->RSSetState(mesh_rs.Get());
		}

		// Pixel Shader
		de_ctx3->PSSetConstantBuffers(0, 1, ubuff.buff.GetAddressOf());
		de_ctx3->PSSetShader(mesh_ps.Get(), nullptr, 0);

		// Output Merger
		{
			float blend_factor[4] = { 1, 1, 1, 1 };
			de_ctx3->OMSetBlendState(mesh_bs.Get(), blend_factor, 0xFFFF'FFFF);

			std::array<ID3D11RenderTargetView*, 2> rtvs = {
				event.compose_rtv, event.next_parents_clip_mask_rtv
			};

			de_ctx3->OMSetDepthStencilState(mesh_dss.Get(), 1);

			de_ctx3->OMSetRenderTargets(rtvs.size(), rtvs.data(), depth_view.Get());
		}

		// Draw
		de_ctx3->DrawInstanced(draw.vertex_count, 1, draw.vertex_start, draw.instance_start);
	}

	return err_stack;
}

ErrStack geometryDraw(nui::SurfaceEvent& event)
{
	MeshRenderer* r = (MeshRenderer*)event.user_data;
	return r->draw(event);;
}