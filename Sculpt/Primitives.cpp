
// Header
#include "SculptMesh.hpp"


using namespace scme;


void Vertex::init()
{
	edge = 0xFFFF'FFFF;
	aabb = 0xFFFF'FFFF;
}

bool Vertex::isPoint()
{
	return edge == 0xFFFF'FFFF;
}

bool VertexBoundingBox::isLeaf()
{
	return children[0] == 0xFFFF'FFFF;
}

bool VertexBoundingBox::hasVertices()
{
	return verts.size() > 0;
}

uint32_t VertexBoundingBox::inWhichChildDoesPositionReside(glm::vec3& pos)
{
	assert_cond(isLeaf() == false, "should not be called for leafs because leafs don't have children");

	// above
	if (pos.y >= mid.y) {

		// forward
		if (pos.z >= mid.z) {

			// left
			if (pos.x <= mid.x) {
				return children[0];
			}
			// right
			else {
				return children[1];
			}
		}
		// back
		else {
			// left
			if (pos.x <= mid.x) {
				return children[2];
			}
			// right
			else {
				return children[3];
			}
		}
	}
	// below
	else {
		// forward
		if (pos.z >= mid.z) {

			// left
			if (pos.x <= mid.x) {
				return children[4];
			}
			// right
			else {
				return children[5];
			}
		}
		// back
		else {
			// left
			if (pos.x <= mid.x) {
				return children[6];
			}
		}
	}

	// right
	return children[7];
}

uint32_t& Edge::nextEdgeOf(uint32_t vertex_idx)
{
	if (v0 == vertex_idx) {
		return v0_next_edge;
	}

	assert_cond(v1 == vertex_idx, "malformed iteration of edge around vertex");

	return v1_next_edge;
}

uint32_t& Edge::prevEdgeOf(uint32_t vertex_idx)
{
	if (v0 == vertex_idx) {
		return v0_prev_edge;
	}

	assert_cond(v1 == vertex_idx, "malformed iteration of edge around vertex");

	return v1_prev_edge;
}

void Edge::setPrevNextEdges(uint32_t vertex_idx, uint32_t prev_edge_idx, uint32_t next_edge_idx)
{
	if (v0 == vertex_idx) {
		v0_prev_edge = prev_edge_idx;
		v0_next_edge = next_edge_idx;
	}
	else {
		v1_prev_edge = prev_edge_idx;
		v1_next_edge = next_edge_idx;
	}
}

void SculptMesh::_deleteVertexMemory(uint32_t vertex_idx)
{
	verts.erase(vertex_idx);

	ModifiedVertex& modified_vertex = modified_verts.emplace_back();
	modified_vertex.idx = vertex_idx;
	modified_vertex.state = ModifiedVertexState::DELETED;
}

void SculptMesh::_deleteEdgeMemory(uint32_t edge_idx)
{
	edges.erase(edge_idx);
}

void SculptMesh::_deletePolyMemory(uint32_t poly_idx)
{
	polys.erase(poly_idx);

	ModifiedPoly& modified_poly = modified_polys.emplace_back();
	modified_poly.idx = poly_idx;
	modified_poly.state = ModifiedPolyState::DELETED;

	dirty_index_buff = true;
}

void SculptMesh::printEdgeListOfVertex(uint32_t vertex_idx)
{
	Vertex& vertex = verts[vertex_idx];

	uint32_t edge_idx = vertex.edge;
	Edge* edge = &edges[edge_idx];

	printf("[");
	do {
		printf("%d, ", edge_idx);

		// Iter
		edge_idx = edge->nextEdgeOf(vertex_idx);
		edge = &edges[edge_idx];
	} 	while (edge_idx != vertex.edge);

	printf("]\n");
}

void SculptMesh::calcVertexNormal(uint32_t vertex_idx)
{
	Vertex* vertex = &verts[vertex_idx];

	if (vertex->edge == 0xFFFF'FFFF) {
		return;
	}

	uint32_t count = 0;
	vertex->normal = { 0, 0, 0 };

	uint32_t edge_idx = vertex->edge;
	Edge* edge = &edges[edge_idx];

	do {
		Poly* poly;
		if (edge->p0 != 0xFFFF'FFFF) {
			poly = &polys[edge->p0];
			vertex->normal += poly->normal;
			count++;
		}

		if (edge->p1 != 0xFFFF'FFFF) {
			poly = &polys[edge->p1];
			vertex->normal += poly->normal;
			count++;
		}

		// Iter
		edge_idx = edge->nextEdgeOf(vertex_idx);
		edge = &edges[edge_idx];
	}
	while (edge_idx != vertex->edge);

	vertex->normal /= count;
	vertex->normal = glm::normalize(vertex->normal);
}

void SculptMesh::markVertexFullUpdate(uint32_t vertex)
{
	ModifiedVertex& modified_vertex = modified_verts.emplace_back();
	modified_vertex.idx = vertex;
	modified_vertex.state = ModifiedVertexState::UPDATE;

	this->dirty_vertex_list = true;
	this->dirty_vertex_pos = true;
	this->dirty_vertex_normals = true;
}

void SculptMesh::deleteVertex(uint32_t)
{
	//Vertex* vertex = &verts[vertex_idx];

	//// vertex is point
	//if (vertex->away_loop == 0xFFFF'FFFF) {
	//	throw std::exception("TODO");
	//	return;
	//}

	//Loop* start_loop = &loops[vertex->away_loop];

	// iter around vertex
	// for each delete_loop
	//   remove delete_loop from mirror loop list

	// find all polygons atached
	// if triangle then delete
	// if quad then trim to triangle
}

void SculptMesh::_transferVertexToAABB(uint32_t v, uint32_t dest_aabb)
{
	Vertex* vertex = &verts[v];
	VertexBoundingBox& destination_aabb = aabbs[dest_aabb];

	// remove from old AABB
	if (vertex->aabb != 0xFFFF'FFFF) {

		VertexBoundingBox& source_aabb = aabbs[vertex->aabb];

		source_aabb.verts_deleted_count -= 1;
		source_aabb.verts[vertex->idx_in_aabb] = 0xFFFF'FFFF;
	}

	vertex->aabb = dest_aabb;
	vertex->idx_in_aabb = destination_aabb.verts.size();
	destination_aabb.verts.push_back(v);
}

void SculptMesh::moveVertexInAABBs(uint32_t vertex_idx)
{
	Vertex& vertex = verts[vertex_idx];

	// did it even leave the original AABB heuristic
	if (vertex.aabb != 0xFFFF'FFFF) {

		VertexBoundingBox& original_aabb = aabbs[vertex.aabb];

		// no significant change in position
		if (original_aabb.aabb.isPositionInside(vertex.pos)) {
			return;
		}
	}

	uint32_t now_count = 1;
	std::array<uint32_t, 8> now_aabbs = {
		root_aabb_idx
	};

	uint32_t next_count = 0;
	std::array<uint32_t, 8> next_aabbs;

	while (now_count) {

		for (uint32_t i = 0; i < now_count; i++) {

			uint32_t aabb_idx = now_aabbs[i];
			VertexBoundingBox* aabb = &aabbs[aabb_idx];

			if (aabb->aabb.isPositionInside(vertex.pos)) {

				// Leaf
				if (aabb->isLeaf()) {

					uint32_t child_vertex_count = aabb->verts.size() - aabb->verts_deleted_count;

					// leaf not full
					if (child_vertex_count < max_vertices_in_AABB) {

						_transferVertexToAABB(vertex_idx, aabb_idx);
						return;
					}
					// Subdivide
					else {
						uint32_t base_idx = aabbs.size();
						aabbs.resize(aabbs.size() + 8);

						aabb = &aabbs[aabb_idx];  // old AABB pointer invalidated because octrees resize

						aabb->aabb.subdivide(
							aabbs[base_idx + 0].aabb, aabbs[base_idx + 1].aabb,
							aabbs[base_idx + 2].aabb, aabbs[base_idx + 3].aabb,

							aabbs[base_idx + 4].aabb, aabbs[base_idx + 5].aabb,
							aabbs[base_idx + 6].aabb, aabbs[base_idx + 7].aabb,
							aabb->mid
						);

						bool found = false;

						for (i = 0; i < 8; i++) {

							uint32_t child_aabb_idx = base_idx + i;
							aabb->children[i] = child_aabb_idx;

							VertexBoundingBox& child_aabb = aabbs[child_aabb_idx];
							child_aabb.parent = aabb_idx;
							child_aabb.children[0] = 0xFFFF'FFFF;
							child_aabb.verts_deleted_count = 0;
							child_aabb.verts.reserve(max_vertices_in_AABB / 4);  // just a guess

							// transfer the excess vertex to one of child AABBs
							if (!found && child_aabb.aabb.isPositionInside(vertex.pos)) {
								_transferVertexToAABB(vertex_idx, child_aabb_idx);
								found = true;
							}
						}

						assert_cond(found == true, "vertex was not found in subdivided leaf");

						// transfer the rest of vertices of the parent to children
						for (uint32_t aabb_vertex_idx : aabb->verts) {

							// skip deleted vertex index from AABB
							if (aabb_vertex_idx == 0xFFFF'FFFF) {
								continue;
							}

							Vertex& aabb_vertex = verts[aabb_vertex_idx];

							for (i = 0; i < 8; i++) {

								uint32_t child_aabb_idx = base_idx + i;
								VertexBoundingBox& child_aabb = aabbs[child_aabb_idx];

								if (child_aabb.aabb.isPositionInside(aabb_vertex.pos)) {

									aabb_vertex.aabb = child_aabb_idx;
									aabb_vertex.idx_in_aabb = child_aabb.verts.size();

									child_aabb.verts.push_back(aabb_vertex_idx);
									break;
								}
							}
						}

						// remove vertices from parent
						aabb->verts_deleted_count = 0;
						aabb->verts.clear();
						return;
					}
				}
				// Schedule going down the graph call
				else {
					uint32_t child_idx = aabb->inWhichChildDoesPositionReside(vertex.pos);

					// unlikelly only float point errors
					if (child_idx == 0xFFFF'FFFF) {

						for (i = 0; i < 8; i++) {
							next_aabbs[next_count] = aabb->children[i];
							next_count++;
						}
					}
					else {
						next_aabbs[next_count] = child_idx;
						next_count++;
					}
					break;
				}
			}
		}

		now_aabbs.swap(next_aabbs);
		now_count = next_count;
		next_count = 0;
	}

	// position is not found in the AABB graph
	uint32_t new_root_idx = aabbs.size();
	VertexBoundingBox& new_root = aabbs.emplace_back();
	new_root.verts_deleted_count = 0;

	VertexBoundingBox& old_root = aabbs[root_aabb_idx];
	{
		float size = old_root.aabb.sizeX();

		if (vertex.pos.y > old_root.aabb.max.y) {
			new_root.aabb.min.y = old_root.aabb.min.y;
			new_root.aabb.max.y = new_root.aabb.min.y + size;
		}
		else {
			new_root.aabb.max.y = old_root.aabb.max.y;
			new_root.aabb.min.y = new_root.aabb.max.y - size;
		}

		if (vertex.pos.z > old_root.aabb.max.z) {
			new_root.aabb.min.z = old_root.aabb.min.z;
			new_root.aabb.max.z = new_root.aabb.min.z + size;
		}
		else {
			new_root.aabb.max.z = old_root.aabb.max.z;
			new_root.aabb.min.z = new_root.aabb.max.z - size;;
		}

		if (vertex.pos.x > old_root.aabb.max.x) {
			new_root.aabb.min.x = old_root.aabb.min.x;
			new_root.aabb.max.x = new_root.aabb.min.x + size;
		}
		else {
			new_root.aabb.max.x = old_root.aabb.max.x;
			new_root.aabb.min.x = new_root.aabb.max.x - size;
		}
	}

	AxisBoundingBox3D<> boxes[8];

	new_root.aabb.subdivide(boxes[0], boxes[1], boxes[2], boxes[3],
		boxes[4], boxes[5], boxes[6], boxes[7],
		new_root.mid);

	//VertexBoundingBox* aabb[7];

	for (uint32_t i = 0; i < 8; i++) {

		uint32_t child_idx;

		if (boxes[i].isPositionInside(old_root.mid)) {
			// 
		}
		else {
			child_idx = aabbs.size();

			VertexBoundingBox& child_aabb = aabbs.emplace_back();

		}

		new_root.children[i] = child_idx;
	}

}

void SculptMesh::recreateAABBs(uint32_t new_max_vertices_in_AABB)
{
	if (new_max_vertices_in_AABB) {
		this->max_vertices_in_AABB = new_max_vertices_in_AABB;
	}

	assert_cond(this->max_vertices_in_AABB > 0, "maximum number of vertices for AABB is not specified");

	aabbs.resize(1);
	root_aabb_idx = 0;

	VertexBoundingBox& root = aabbs[root_aabb_idx];
	root.parent = 0xFFFF'FFFF;
	root.children[0] = 0xFFFF'FFFF;
	root.verts_deleted_count = 0;
	root.verts.clear();

	// Calculate Root Bounding Box size
	if (verts.size()) {

		float new_root_aabb_size = 0;

		// skip first vertex
		for (auto iter = verts.begin(); iter != verts.end(); iter.next()) {

			Vertex& vertex = iter.get();
			vertex.aabb = 0xFFFF'FFFF;

			if (std::abs(vertex.pos.x) > new_root_aabb_size) {
				new_root_aabb_size = std::abs(vertex.pos.x);
			}

			if (std::abs(vertex.pos.y) > new_root_aabb_size) {
				new_root_aabb_size = std::abs(vertex.pos.y);
			}

			if (std::abs(vertex.pos.z) > new_root_aabb_size) {
				new_root_aabb_size = std::abs(vertex.pos.z);
			}
		}

		root.aabb.min = { -new_root_aabb_size, -new_root_aabb_size, -new_root_aabb_size };
		root.aabb.max = { +new_root_aabb_size, +new_root_aabb_size, +new_root_aabb_size };
		root.mid.x = (root.aabb.max.x + root.aabb.min.x) / 2.0f;
		root.mid.y = (root.aabb.max.y + root.aabb.min.y) / 2.0f;
		root.mid.x = (root.aabb.max.z + root.aabb.min.z) / 2.0f;
	}
	
	for (auto iter = verts.begin(); iter != verts.end(); iter.next()) {
		moveVertexInAABBs(iter.index());
	}
}

uint32_t SculptMesh::findEdgeBetween(uint32_t v0_idx, uint32_t v1_idx)
{
	Vertex& vertex_0 = verts[v0_idx];
	Vertex& vertex_1 = verts[v1_idx];

	if (vertex_0.edge == 0xFFFF'FFFF || vertex_1.edge == 0xFFFF'FFFF) {
		return 0xFFFF'FFFF;
	}

	uint32_t edge_idx = vertex_0.edge;
	Edge* edge = &edges[edge_idx];

	iterEdgesAroundVertexStart;
	{
		if ((edge->v0 == v0_idx && edge->v1 == v1_idx) ||
			(edge->v0 == v1_idx && edge->v1 == v0_idx))
		{
			return edge_idx;
		}
	}
	iterEdgesAroundVertexEnd(v0_idx, vertex_0.edge);

	edge_idx = vertex_1.edge;
	edge = &edges[edge_idx];

	iterEdgesAroundVertexStart;
	{
		if ((edge->v0 == v0_idx && edge->v1 == v1_idx) ||
			(edge->v0 == v1_idx && edge->v1 == v0_idx))
		{
			return edge_idx;
		}
	}
	iterEdgesAroundVertexEnd(v1_idx, vertex_1.edge);

	return 0xFFFF'FFFF;
}

glm::vec3 calcNormalForTrisPositions(Vertex* v0, Vertex* v1, Vertex* v2)
{
	glm::vec3 dir_0 = glm::normalize(v1->pos - v0->pos);
	glm::vec3 dir_1 = glm::normalize(v2->pos - v0->pos);

	return glm::normalize(-glm::cross(dir_0, dir_1));
}

void SculptMesh::registerEdgeToVertexList(uint32_t new_edge_idx, uint32_t vertex_idx)
{
	Edge& new_edge = edges[new_edge_idx];
	Vertex& vertex = verts[vertex_idx];

	// if vertex is point then vertex loop list is unused
	if (vertex.edge == 0xFFFF'FFFF) {

		new_edge.setPrevNextEdges(vertex_idx, new_edge_idx, new_edge_idx);
		vertex.edge = new_edge_idx;
	}
	else {
		uint32_t prev_edge_idx = vertex.edge;
		Edge& prev_edge = edges[prev_edge_idx];

		uint32_t next_edge_idx = prev_edge.nextEdgeOf(vertex_idx);
		Edge& next_edge = edges[next_edge_idx];

		// prev <--- new ---> next
		new_edge.setPrevNextEdges(vertex_idx, prev_edge_idx, next_edge_idx);

		// prev ---> new <--- next
		prev_edge.nextEdgeOf(vertex_idx) = new_edge_idx;
		next_edge.prevEdgeOf(vertex_idx) = new_edge_idx;
	}
}

void SculptMesh::unregisterEdgeFromVertex(Edge* delete_edge, uint32_t vertex_idx, Vertex* vertex)
{
	if (vertex->edge != 0xFFFF'FFFF) {

		uint32_t prev_edge_idx = delete_edge->prevEdgeOf(vertex_idx);
		uint32_t next_edge_idx = delete_edge->nextEdgeOf(vertex_idx);

		Edge& prev_edge = edges[prev_edge_idx];
		Edge& next_edge = edges[next_edge_idx];

		// prev <---> next
		prev_edge.nextEdgeOf(vertex_idx) = next_edge_idx;
		next_edge.prevEdgeOf(vertex_idx) = prev_edge_idx;

		// make sure that the edge list has proper start or else infinite looping can occur
		// because start_edge is not in edge list
		vertex->edge = next_edge_idx;
	}
}

void SculptMesh::registerPolyToEdge(uint32_t new_poly_idx, uint32_t edge_idx)
{
	Edge& edge = edges[edge_idx];
	if (edge.p0 == 0xFFFF'FFFF) {
		edge.p0 = new_poly_idx;
	}
	else {
		edge.p1 = new_poly_idx;
	}
}

void SculptMesh::unregisterPolyFromEdge(uint32_t delete_poly_idx, uint32_t edge_idx)
{
	Edge& edge = edges[edge_idx];
	if (edge.p0 == delete_poly_idx) {
		edge.p0 = 0xFFFF'FFFF;
	}
	else {
		edge.p1 = 0xFFFF'FFFF;
	}
}

uint32_t SculptMesh::createEdge(uint32_t v0, uint32_t v1)
{
	uint32_t new_loop_idx;;
	edges.emplace(new_loop_idx);

	setEdge(new_loop_idx, v0, v1);
	return new_loop_idx;
}

void SculptMesh::setEdge(uint32_t existing_edge_idx, uint32_t v0_idx, uint32_t v1_idx)
{
	Edge* existing_edge = &edges[existing_edge_idx];
	existing_edge->v0 = v0_idx;
	existing_edge->v1 = v1_idx;
	existing_edge->p0 = 0xFFFF'FFFF;
	existing_edge->p1 = 0xFFFF'FFFF;
	existing_edge->was_raycast_tested = false;

	registerEdgeToVertexList(existing_edge_idx, v0_idx);
	registerEdgeToVertexList(existing_edge_idx, v1_idx);
}

uint32_t SculptMesh::addEdge(uint32_t v0, uint32_t v1)
{
	uint32_t existing_loop = findEdgeBetween(v0, v1);
	if (existing_loop == 0xFFFF'FFFF) {
		return createEdge(v0, v1);
	}
	
	return existing_loop;
}

glm::vec3 SculptMesh::calcWindingNormal(Vertex* v0, Vertex* v1, Vertex* v2)
{
	return -glm::normalize(glm::cross(v1->pos - v0->pos, v2->pos - v0->pos));
}

void SculptMesh::calcPolyNormal(Poly* poly)
{
	if (poly->is_tris) {

		std::array<scme::Vertex*, 3> vs;
		getTrisPrimitives(poly, vs);

		// Triangle Normal
		poly->normal = calcWindingNormal(vs[0], vs[1], vs[2]);
		poly->tess_normals[0] = poly->normal;
		poly->tess_normals[1] = poly->normal;
	}
	else {
		std::array<scme::Vertex*, 4> vs;
		getQuadPrimitives(poly, vs);

		// Tesselation and Normals
		if (glm::distance(vs[0]->pos, vs[2]->pos) < glm::distance(vs[1]->pos, vs[3]->pos)) {

			poly->tesselation_type = 0;
			poly->tess_normals[0] = calcWindingNormal(vs[0], vs[1], vs[2]);
			poly->tess_normals[1] = calcWindingNormal(vs[0], vs[2], vs[3]);
		}
		else {
			poly->tesselation_type = 1;
			poly->tess_normals[0] = calcWindingNormal(vs[0], vs[1], vs[3]);
			poly->tess_normals[1] = calcWindingNormal(vs[1], vs[2], vs[3]);
		}
		poly->normal = glm::normalize((poly->tess_normals[0] + poly->tess_normals[1]) / 2.f);
	}
}

void SculptMesh::markPolyFullUpdate(uint32_t poly)
{
	ModifiedPoly& modified_poly = modified_polys.emplace_back();
	modified_poly.idx = poly;
	modified_poly.state = ModifiedPolyState::UPDATE;

	dirty_index_buff = true;
	dirty_tess_tris = true;
}

void SculptMesh::setTris(uint32_t new_poly_idx, uint32_t v0, uint32_t v1, uint32_t v2)
{
	Poly& new_poly = polys[new_poly_idx];
	new_poly.is_tris = 1;
	new_poly.edges[0] = addEdge(v0, v1);
	new_poly.edges[1] = addEdge(v1, v2);
	new_poly.edges[2] = addEdge(v2, v0);

	Edge* edge = &edges[new_poly.edges[0]];
	new_poly.flip_edge_0 = edge->v0 != v0;

	edge = &edges[new_poly.edges[1]];
	new_poly.flip_edge_1 = edge->v0 != v1;

	edge = &edges[new_poly.edges[2]];
	new_poly.flip_edge_2 = edge->v0 != v2;

	registerPolyToEdge(new_poly_idx, new_poly.edges[0]);
	registerPolyToEdge(new_poly_idx, new_poly.edges[1]);
	registerPolyToEdge(new_poly_idx, new_poly.edges[2]);

	markPolyFullUpdate(new_poly_idx);
}

uint32_t SculptMesh::addTris(uint32_t v0, uint32_t v1, uint32_t v2)
{
	uint32_t new_tris_idx;
	polys.emplace(new_tris_idx);
	
	setTris(new_tris_idx, v0, v1, v2);

	return new_tris_idx;
}

void SculptMesh::setQuad(uint32_t new_quad_idx, uint32_t v0, uint32_t v1, uint32_t v2, uint32_t v3)
{
	Poly& new_quad = polys[new_quad_idx];
	new_quad.is_tris = false;
	new_quad.edges[0] = addEdge(v0, v1);
	new_quad.edges[1] = addEdge(v1, v2);
	new_quad.edges[2] = addEdge(v2, v3);
	new_quad.edges[3] = addEdge(v3, v0);

	Edge* edge = &edges[new_quad.edges[0]];
	new_quad.flip_edge_0 = edge->v0 != v0;

	edge = &edges[new_quad.edges[1]];
	new_quad.flip_edge_1 = edge->v0 != v1;

	edge = &edges[new_quad.edges[2]];
	new_quad.flip_edge_2 = edge->v0 != v2;

	edge = &edges[new_quad.edges[3]];
	new_quad.flip_edge_3 = edge->v0 != v3;

	registerPolyToEdge(new_quad_idx, new_quad.edges[0]);
	registerPolyToEdge(new_quad_idx, new_quad.edges[1]);
	registerPolyToEdge(new_quad_idx, new_quad.edges[2]);
	registerPolyToEdge(new_quad_idx, new_quad.edges[3]);

	markPolyFullUpdate(new_quad_idx);
}

uint32_t SculptMesh::addQuad(uint32_t v0, uint32_t v1, uint32_t v2, uint32_t v3)
{
	uint32_t new_quad_idx;
	polys.emplace(new_quad_idx);
	
	setQuad(new_quad_idx, v0, v1, v2, v3);

	return new_quad_idx;
}

void SculptMesh::stichVerticesToVertexLooped(std::vector<uint32_t>& vertices, uint32_t target)
{
	uint32_t last = vertices.size() - 1;
	for (uint32_t i = 0; i < last; i++) {

		uint32_t v = vertices[i];
		uint32_t v_next = vertices[i + 1];
	
		addTris(v, target, v_next);
	}

	addTris(vertices[last], target, vertices[0]);
}

void SculptMesh::deletePoly(uint32_t delete_poly_idx)
{
	Poly* delete_poly = &polys[delete_poly_idx];
	
	uint32_t count;
	if (delete_poly->is_tris) {
		count = 3;
	}
	else {
		count = 4;
	}

	for (uint8_t i = 0; i < count; i++) {

		uint32_t edge_idx = delete_poly->edges[i];
		Edge* edge = &edges[edge_idx];

		Vertex* vertex = &verts[edge->v0];
		unregisterEdgeFromVertex(edge, edge->v0, vertex);

		if (edge->nextEdgeOf(edge->v0) == edge_idx) {
			_deleteVertexMemory(edge->v0);
		}

		vertex = &verts[edge->v1];
		unregisterEdgeFromVertex(edge, edge->v1, vertex);

		if (edge->nextEdgeOf(edge->v1) == edge_idx) {
			_deleteVertexMemory(edge->v1);
		}

		// unregister poly from edge
		// because edge is part of poly one poly reference will reference poly
		if (edge->p0 == delete_poly_idx) {
			edge->p0 = 0xFFFF'FFFF;
		}
		else {
			edge->p1 = 0xFFFF'FFFF;
		}

		// edge is wire
		if (edge->p0 == edge->p1) {
			_deleteEdgeMemory(edge_idx);
		}
	}

	// Now no edges reference the poly so we can safely delete the polygon
	_deletePolyMemory(delete_poly_idx);
}

void SculptMesh::getTrisPrimitives(Poly* poly,
	std::array<uint32_t, 3>& r_vs_idxs, std::array<Vertex*, 3>& r_vs)
{
	std::array<Edge*, 3> r_es;
	r_es[0] = &edges[poly->edges[0]];
	r_es[1] = &edges[poly->edges[1]];
	r_es[2] = &edges[poly->edges[2]];

	r_vs_idxs[0] = poly->flip_edge_0 ? r_es[0]->v1 : r_es[0]->v0;
	r_vs_idxs[1] = poly->flip_edge_1 ? r_es[1]->v1 : r_es[1]->v0;
	r_vs_idxs[2] = poly->flip_edge_2 ? r_es[2]->v1 : r_es[2]->v0;

	r_vs[0] = &verts[r_vs_idxs[0]];
	r_vs[1] = &verts[r_vs_idxs[1]];
	r_vs[2] = &verts[r_vs_idxs[2]];
}

void SculptMesh::getTrisPrimitives(Poly* poly, std::array<uint32_t, 3>& r_vs_idxs)
{
	std::array<Edge*, 3> r_es;
	r_es[0] = &edges[poly->edges[0]];
	r_es[1] = &edges[poly->edges[1]];
	r_es[2] = &edges[poly->edges[2]];

	r_vs_idxs[0] = poly->flip_edge_0 ? r_es[0]->v1 : r_es[0]->v0;
	r_vs_idxs[1] = poly->flip_edge_1 ? r_es[1]->v1 : r_es[1]->v0;
	r_vs_idxs[2] = poly->flip_edge_2 ? r_es[2]->v1 : r_es[2]->v0;
}

void SculptMesh::getTrisPrimitives(Poly* poly, std::array<Vertex*, 3>& r_vs)
{
	std::array<Edge*, 3> r_es;
	r_es[0] = &edges[poly->edges[0]];
	r_es[1] = &edges[poly->edges[1]];
	r_es[2] = &edges[poly->edges[2]];

	std::array<uint32_t, 3> r_vs_idxs;
	r_vs_idxs[0] = poly->flip_edge_0 ? r_es[0]->v1 : r_es[0]->v0;
	r_vs_idxs[1] = poly->flip_edge_1 ? r_es[1]->v1 : r_es[1]->v0;
	r_vs_idxs[2] = poly->flip_edge_2 ? r_es[2]->v1 : r_es[2]->v0;

	r_vs[0] = &verts[r_vs_idxs[0]];
	r_vs[1] = &verts[r_vs_idxs[1]];
	r_vs[2] = &verts[r_vs_idxs[2]];
}

void SculptMesh::getQuadPrimitives(Poly* poly,
	std::array<uint32_t, 4>& r_vs_idxs, std::array<Vertex*, 4>& r_vs)
{
	std::array<Edge*, 4> r_es;
	r_es[0] = &edges[poly->edges[0]];
	r_es[1] = &edges[poly->edges[1]];
	r_es[2] = &edges[poly->edges[2]];
	r_es[3] = &edges[poly->edges[3]];

	r_vs_idxs[0] = poly->flip_edge_0 ? r_es[0]->v1 : r_es[0]->v0;
	r_vs_idxs[1] = poly->flip_edge_1 ? r_es[1]->v1 : r_es[1]->v0;
	r_vs_idxs[2] = poly->flip_edge_2 ? r_es[2]->v1 : r_es[2]->v0;
	r_vs_idxs[3] = poly->flip_edge_3 ? r_es[3]->v1 : r_es[3]->v0;

	r_vs[0] = &verts[r_vs_idxs[0]];
	r_vs[1] = &verts[r_vs_idxs[1]];
	r_vs[2] = &verts[r_vs_idxs[2]];
	r_vs[3] = &verts[r_vs_idxs[3]];
}

void SculptMesh::getQuadPrimitives(Poly* poly, std::array<uint32_t, 4>& r_vs_idxs)
{
	std::array<Edge*, 4> r_es;
	r_es[0] = &edges[poly->edges[0]];
	r_es[1] = &edges[poly->edges[1]];
	r_es[2] = &edges[poly->edges[2]];
	r_es[3] = &edges[poly->edges[3]];

	r_vs_idxs[0] = poly->flip_edge_0 ? r_es[0]->v1 : r_es[0]->v0;
	r_vs_idxs[1] = poly->flip_edge_1 ? r_es[1]->v1 : r_es[1]->v0;
	r_vs_idxs[2] = poly->flip_edge_2 ? r_es[2]->v1 : r_es[2]->v0;
	r_vs_idxs[3] = poly->flip_edge_3 ? r_es[3]->v1 : r_es[3]->v0;
}

void SculptMesh::getQuadPrimitives(Poly* poly, std::array<Vertex*, 4>& r_vs)
{
	std::array<Edge*, 4> r_es;
	r_es[0] = &edges[poly->edges[0]];
	r_es[1] = &edges[poly->edges[1]];
	r_es[2] = &edges[poly->edges[2]];
	r_es[3] = &edges[poly->edges[3]];

	std::array<uint32_t, 4> r_vs_idxs;
	r_vs_idxs[0] = poly->flip_edge_0 ? r_es[0]->v1 : r_es[0]->v0;
	r_vs_idxs[1] = poly->flip_edge_1 ? r_es[1]->v1 : r_es[1]->v0;
	r_vs_idxs[2] = poly->flip_edge_2 ? r_es[2]->v1 : r_es[2]->v0;
	r_vs_idxs[3] = poly->flip_edge_3 ? r_es[3]->v1 : r_es[3]->v0;

	r_vs[0] = &verts[r_vs_idxs[0]];
	r_vs[1] = &verts[r_vs_idxs[1]];
	r_vs[2] = &verts[r_vs_idxs[2]];
	r_vs[3] = &verts[r_vs_idxs[3]];
}

void SculptMesh::printVerices()
{
	for (auto iter = verts.begin(); iter != verts.end(); iter.next()) {

		Vertex& vertex = iter.get();

		printf("vertex[%d].pos = { %.2f, %.2f %.2f } \n",
			iter.index(),
			vertex.pos.x,
			vertex.pos.y,
			vertex.pos.z
		);
	}
}
