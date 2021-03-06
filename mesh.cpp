#include "mesh.hpp"

bool Vert::isOnSeamOrBoundary()
{
  return node->isOnSeamOrBoundary();
}

Vert *Node::adjacent(Vert *other)
{
  Edge *edge = getEdge(this, other->node);
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2; j++) {
      if (edge->getVert(j, i) == other) {
        return edge->getVert(j, 1 - i);
      }
    }
  }

  return NULL;
}

bool Node::isOnSeamOrBoundary()
{
  int adj_e_size = adj_e.size();
  for (int i = 0; i < adj_e_size; i++) {
    if (adj_e[i]->isOnSeamOrBoundary()) {
      return true;
    }
  }
  return false;
}

/* Get Vert of edge whose node matches n[edge_node] */
Vert *Edge::getVert(int face_side, int edge_node)
{
  if (!adj_f[face_side]) {
    return NULL;
  }
  for (int i = 0; i < 3; i++) {
    if (adj_f[face_side]->v[i]->node == n[edge_node]) {
      return adj_f[face_side]->v[i];
    }
  }
  return NULL;
}

/* Get Vert of adj_f[face_side] that is not part of this edge */
Vert *Edge::getOtherVertOfFace(int face_side)
{
  if (!adj_f[face_side]) {
    return NULL;
  }
  for (int i = 0; i < 3; i++) {
    if (adj_f[face_side]->v[i]->node != n[0] && adj_f[face_side]->v[i]->node != n[1]) {
      return adj_f[face_side]->v[i];
    }
  }
  return NULL;
}

static void connectVertWithNode(Vert *vert, Node *node);

bool Edge::isOnSeamOrBoundary()
{
  return !adj_f[0] || !adj_f[1] || getVert(0, 0) != getVert(1, 0) ||
         getVert(0, 1) != getVert(1, 1);
}

bool Face::isOnSeamOrBoundary()
{
  return adj_e[0]->isOnSeamOrBoundary() || adj_e[1]->isOnSeamOrBoundary() ||
         adj_e[2]->isOnSeamOrBoundary();
}

void Mesh::add(Vert *vert)
{
  verts.push_back(vert);
  vert->node = NULL;
  vert->adj_f.clear();
  vert->index = verts.size() - 1;
}

void Mesh::add(Node *node)
{
  nodes.push_back(node);
  node->adj_e.clear();
  for (int i = 0; i < node->verts.size(); i++) {
    node->verts[i]->node = node;
  }
  node->index = nodes.size() - 1;
}

void Mesh::add(Edge *edge)
{
  edges.push_back(edge);
  edge->adj_f[0] = NULL;
  edge->adj_f[1] = NULL;
  include(edge, edge->n[0]->adj_e);
  include(edge, edge->n[1]->adj_e);
  edge->index = edges.size() - 1;
}

static void add_edges_if_needed(Mesh &mesh, const Face *face)
{
  for (int i = 0; i < 3; i++) {
    Node *n0 = face->v[i]->node, *n1 = face->v[NEXT(i)]->node;
    if (getEdge(n0, n1) == NULL) {
      mesh.add(new Edge(n0, n1));
    }
  }
}

void Mesh::add(Face *face)
{
  faces.push_back(face);
  add_edges_if_needed(*this, face);
  for (int i = 0; i < 3; i++) {
    Vert *v0 = face->v[NEXT(i)];
    Vert *v1 = face->v[PREV(i)];
    include(face, v0->adj_f);
    Edge *e = getEdge(v0->node, v1->node);
    face->adj_e[i] = e;
    int side = e->n[0] == v0->node ? 0 : 1;
    e->adj_f[side] = face;
  }
  face->index = faces.size() - 1;
}

void Mesh::remove(Vert *vert)
{
  assert(vert->adj_f.empty()); /* ensure that adjacent faces don't
                                  exist */
  exclude(vert, verts);
}

void Mesh::remove(Node *node)
{
  assert(node->adj_e.empty()); /* ensure that adjacent edges don't
                                  exist */
  exclude(node, nodes);
}

void Mesh::remove(Edge *edge)
{
  assert(!edge->adj_f[0] && !edge->adj_f[1]); /* ensure that adjacent
                                                 faces don't exist */
  exclude(edge, edges);
  exclude(edge, edge->n[0]->adj_e);
  exclude(edge, edge->n[1]->adj_e);
}

void Mesh::remove(Face *face)
{
  exclude(face, faces);
  for (int i = 0; i < 3; i++) {
    Vert *v0 = face->v[NEXT(i)];
    exclude(face, v0->adj_f);
    Edge *e = face->adj_e[i];
    int side = e->n[0] == v0->node ? 0 : 1;
    e->adj_f[side] = NULL;
  }
}

void Mesh::setIndices()
{
  for (int i = 0; i < verts.size(); i++) {
    verts[i]->index = i;
  }
  for (int i = 0; i < faces.size(); i++) {
    faces[i]->index = i;
  }
  for (int i = 0; i < nodes.size(); i++) {
    nodes[i]->index = i;
  }
  for (int i = 0; i < edges.size(); i++) {
    edges[i]->index = i;
  }
}

static void getValidLine(istream &in, string &line)
{
  do {
    getline(in, line);
  } while (in && (line.length() == 0 || line[0] == '#'));
}

static void connectVertWithNode(Vert *vert, Node *node)
{
  vert->node = node;
  include(vert, node->verts);
}

static double angle(const Vec3 &x0, const Vec3 &x1, const Vec3 &x2)
{
  Vec3 e1 = (x1 - x0).normalized();
  Vec3 e2 = (x2 - x0).normalized();
  return acos(clamp(e1.dot(e2), -1., 1.));
}

static vector<Face *> triangulate(const vector<Vert *> &verts)
{
  int n = verts.size();
  double best_min_angle = 0;
  int best_root = -1;
  for (int i = 0; i < n; i++) {
    double min_angle = infinity;
    const Vert *vert0 = verts[i];
    for (int j = 2; j < n; j++) {
      const Vert *vert1 = verts[(i + j - 1) % n], *vert2 = verts[(i + j) % n];
      min_angle = min(min_angle,
                      angle(vert0->node->x, vert1->node->x, vert2->node->x),
                      angle(vert1->node->x, vert2->node->x, vert0->node->x),
                      angle(vert2->node->x, vert0->node->x, vert1->node->x));
    }
    if (min_angle > best_min_angle) {
      best_min_angle = min_angle;
      best_root = i;
    }
  }
  int i = best_root;
  Vert *vert0 = verts[i];
  vector<Face *> tris;
  for (int j = 2; j < n; j++) {
    Vert *vert1 = verts[(i + j - 1) % n], *vert2 = verts[(i + j) % n];
    tris.push_back(new Face(vert0, vert1, vert2));
  }
  return tris;
}

void Mesh::loadObj(const string &file)
{
  /* TODO(Ish): need to delete the existing mesh structure before
   * loading obj */
  fstream fin(file.c_str(), ios::in);
  if (!fin) {
    cout << "error: No file found at " << file << endl;
    return;
  }

  vector<Vec3> normals;
  while (fin) {
    string line;
    getValidLine(fin, line);
    stringstream linestream(line);
    string keyword;
    linestream >> keyword;
    if (keyword == "vt") {
      Vec2 uv;
      linestream >> uv[0] >> uv[1];
      this->add(new Vert(uv));
    }
    else if (keyword == "v") {
      Vec3 x;
      linestream >> x[0] >> x[1] >> x[2];
      this->add(new Node(x, Vec3(0.0, 0.0, 0.0)));
    }
    else if (keyword == "vn") {
      Vec3 n;
      linestream >> n[0] >> n[1] >> n[2];
      normals.push_back(n);
    }
    else if (keyword == "e") {
      int n0, n1;
      linestream >> n0 >> n1;
      this->add(new Edge(this->nodes[n0 - 1], this->nodes[n1 - 1]));
    }
    else if (keyword == "f") {
      vector<Vert *> verts;
      vector<Node *> nodes;
      string w;
      while (linestream >> w) {
        stringstream wstream(w);
        int v, n, vn; /* vt, v, vn */
        char c, c2;
        wstream >> n >> c >> v >> c2 >> vn;
        nodes.push_back(this->nodes[n - 1]);
        this->nodes[n - 1]->n = normals[vn - 1];
        if (wstream) {
          verts.push_back(this->verts[v - 1]);
        }
        else if (!nodes.back()->verts.empty()) {
          verts.push_back(nodes.back()->verts[0]);
        }
        else {
          verts.push_back(new Vert(Vec2(nodes.back()->x[0], nodes.back()->x[1])));
          this->add(verts.back());
        }
      }
      for (int v = 0; v < verts.size(); v++) {
        connectVertWithNode(verts[v], nodes[v]);
      }
      vector<Face *> faces = triangulate(verts);
      for (int f = 0; f < faces.size(); f++) {
        this->add(faces[f]);
      }
    }
  }
  normals.clear();
}

void Mesh::saveObj(const string &filename)
{
  /* TODO(ish): setIndices() here might be useless, some basic
   * testing, the indices remain the same between loading a mesh and
   * immediately doing this, might change when the mesh is modified */
  setIndices();
  fstream fout(filename.c_str(), ios::out);
  for (int i = 0; i < nodes.size(); i++) {
    const Node *node = nodes[i];
    fout << "v " << node->x[0] << " " << node->x[1] << " " << node->x[2] << endl;
  }
  for (int i = 0; i < verts.size(); i++) {
    const Vert *vert = verts[i];
    fout << "vt " << vert->uv[0] << " " << vert->uv[1] << endl;
  }
  for (int i = 0; i < nodes.size(); i++) {
    const Node *node = nodes[i];
    fout << "vn " << node->n[0] << " " << node->n[1] << " " << node->n[2] << endl;
  }
  for (int i = 0; i < faces.size(); i++) {
    const Face *face = faces[i];
    fout << "f " << face->v[0]->node->index + 1 << "/" << face->v[0]->index + 1 << "/"
         << face->v[0]->node->index + 1 << " " << face->v[1]->node->index + 1 << "/"
         << face->v[1]->index + 1 << "/" << face->v[0]->node->index + 1 << " "
         << face->v[2]->node->index + 1 << "/" << face->v[2]->index + 1 << "/"
         << face->v[0]->node->index + 1 << endl;
  }
}

void Mesh::shadeSmooth()
{
  for (int i = 0; i < nodes.size(); i++) {
    Node *node = nodes[i];

    /* TODO(ish): There is a faster way to do this, right now there are
       duplicate faces for which the normals are calculated */
    Vec3 n(0.0, 0.0, 0.0);
    for (int v = 0; v < node->verts.size(); v++) {
      const Vert *vert = node->verts[v];
      for (int f = 0; f < vert->adj_f.size(); f++) {
        const Face *face = vert->adj_f[f];

        int j = find(vert, face->v);
        int j1 = (j + 1) % 3;
        int j2 = (j + 2) % 3;

        Vec3 e1 = face->v[j1]->node->x - vert->node->x;
        Vec3 e2 = face->v[j2]->node->x - vert->node->x;

        n += e1.cross(e2) / (2 * norm2(e1) * norm2(e2));
      }
    }

    node->n = n.normalized();
  }
}

void Mesh::draw()
{
  this->setShaderModelMatrix();
  GPUVertFormat *format = immVertexFormat();
  uint pos_attr = format->addAttribute("in_pos", GPU_COMP_F32, 3, GPU_FETCH_FLOAT);
  /* uint uv_attr = format->addAttribute("in_uv", GPU_COMP_F32, 2, GPU_FETCH_FLOAT); */
  uint normal_attr = format->addAttribute("in_normal", GPU_COMP_F32, 3, GPU_FETCH_FLOAT);

  auto face_len = this->faces.size();
  immBegin(GPU_PRIM_TRIS, face_len * 3, this->shader);

  for (int i = 0; i < face_len; i++) {
    auto &x1 = this->faces[i]->v[0]->node->x;
    auto &uv1 = this->faces[i]->v[0]->uv;
    auto &n1 = this->faces[i]->v[0]->node->n;

    auto &x2 = this->faces[i]->v[1]->node->x;
    auto &uv2 = this->faces[i]->v[1]->uv;
    auto &n2 = this->faces[i]->v[1]->node->n;

    auto &x3 = this->faces[i]->v[2]->node->x;
    auto &uv3 = this->faces[i]->v[2]->uv;
    auto &n3 = this->faces[i]->v[2]->node->n;

    /* immAttr2f(uv_attr, uv1[0], uv1[1]); */
    immAttr3f(normal_attr, n1[0], n1[1], n1[2]);
    immVertex3f(pos_attr, x1[0], x1[1], x1[2]);

    /* immAttr2f(uv_attr, uv2[0], uv2[1]); */
    immAttr3f(normal_attr, n2[0], n2[1], n2[2]);
    immVertex3f(pos_attr, x2[0], x2[1], x2[2]);

    /* immAttr2f(uv_attr, uv3[0], uv3[1]); */
    immAttr3f(normal_attr, n3[0], n3[1], n3[2]);
    immVertex3f(pos_attr, x3[0], x3[1], x3[2]);
  }

  immEnd();
}

void Mesh::drawWireframe(glm::mat4 projection, glm::mat4 view, Vec4 color)
{
  int edge_len = edges.size();
  glEnable(GL_LINE_SMOOTH);
  glLineWidth(1.2);

  GPUVertFormat *format = immVertexFormat();
  uint pos = format->addAttribute("pos", GPU_COMP_F32, 3, GPU_FETCH_FLOAT);
  uint col = format->addAttribute("color", GPU_COMP_F32, 4, GPU_FETCH_FLOAT);

  static Shader smooth_shader("shaders/shader_3D_smooth_color.vert",
                              "shaders/shader_3D_smooth_color.frag");
  glm::mat4 model = glm::mat4(1.0);
  model = glm::translate(model, vec3ToGlmVec3(this->pos));
  model = glm::scale(model, vec3ToGlmVec3(this->scale));
  smooth_shader.use();
  smooth_shader.setMat4("projection", projection);
  smooth_shader.setMat4("view", view);
  smooth_shader.setMat4("model", model);

  immBegin(GPU_PRIM_LINES, edge_len * 2, &smooth_shader);

  for (int i = 0; i < edge_len; i++) {
    immAttr4f(col, color[0], color[1], color[2], color[3]);
    Vec3 &x1 = edges[i]->n[0]->x;
    immVertex3f(pos, x1[0], x1[1], x1[2]);

    immAttr4f(col, color[0], color[1], color[2], color[3]);
    Vec3 &x2 = edges[i]->n[1]->x;
    immVertex3f(pos, x2[0], x2[1], x2[2]);
  }

  immEnd();
}

void Mesh::drawUVs(glm::mat4 projection, glm::mat4 view, Vec3 pos, Vec3 scale, Vec4 color)
{
  int faces_size = faces.size();
  glEnable(GL_LINE_SMOOTH);
  glLineWidth(1.2);

  GPUVertFormat *format = immVertexFormat();
  uint pos_attr = format->addAttribute("pos", GPU_COMP_F32, 3, GPU_FETCH_FLOAT);
  uint col_attr = format->addAttribute("color", GPU_COMP_F32, 4, GPU_FETCH_FLOAT);

  static Shader smooth_shader("shaders/shader_3D_smooth_color.vert",
                              "shaders/shader_3D_smooth_color.frag");
  glm::mat4 model = glm::mat4(1.0);
  model = glm::translate(model, vec3ToGlmVec3(pos));
  model = glm::scale(model, vec3ToGlmVec3(scale));
  smooth_shader.use();
  smooth_shader.setMat4("projection", projection);
  smooth_shader.setMat4("view", view);
  smooth_shader.setMat4("model", model);

  immBeginAtMost(GPU_PRIM_LINES, faces_size * 6, &smooth_shader);

  for (int i = 0; i < faces_size; i++) {
    Vec2 &x0 = faces[i]->v[0]->uv;
    Vec2 &x1 = faces[i]->v[1]->uv;
    Vec2 &x2 = faces[i]->v[2]->uv;

    immAttr4f(col_attr, color[0], color[1], color[2], color[3]);
    immVertex3f(pos_attr, x0[0], x0[1], 0.0);
    immAttr4f(col_attr, color[0], color[1], color[2], color[3]);
    immVertex3f(pos_attr, x1[0], x1[1], 0.0);

    immAttr4f(col_attr, color[0], color[1], color[2], color[3]);
    immVertex3f(pos_attr, x1[0], x1[1], 0.0);
    immAttr4f(col_attr, color[0], color[1], color[2], color[3]);
    immVertex3f(pos_attr, x2[0], x2[1], 0.0);

    immAttr4f(col_attr, color[0], color[1], color[2], color[3]);
    immVertex3f(pos_attr, x2[0], x2[1], 0.0);
    immAttr4f(col_attr, color[0], color[1], color[2], color[3]);
    immVertex3f(pos_attr, x0[0], x0[1], 0.0);
  }

  immEnd();
}

void Mesh::drawFaceNormals(glm::mat4 projection, glm::mat4 view, Vec4 color, double length)
{
  int faces_len = faces.size();

  glEnable(GL_LINE_SMOOTH);
  glLineWidth(1.2);

  GPUVertFormat *format = immVertexFormat();
  uint pos = format->addAttribute("pos", GPU_COMP_F32, 3, GPU_FETCH_FLOAT);
  uint col = format->addAttribute("color", GPU_COMP_F32, 4, GPU_FETCH_FLOAT);

  static Shader smooth_shader("shaders/shader_3D_smooth_color.vert",
                              "shaders/shader_3D_smooth_color.frag");
  glm::mat4 model = glm::mat4(1.0);
  model = glm::translate(model, vec3ToGlmVec3(this->pos));
  model = glm::scale(model, vec3ToGlmVec3(this->scale));
  smooth_shader.use();
  smooth_shader.setMat4("projection", projection);
  smooth_shader.setMat4("view", view);
  smooth_shader.setMat4("model", model);

  immBegin(GPU_PRIM_LINES, faces_len * 2, &smooth_shader);

  for (int i = 0; i < faces_len; i++) {
    immAttr4f(col, color[0], color[1], color[2], color[3]);
    Vec3 x1 = (faces[i]->v[0]->node->x + faces[i]->v[1]->node->x + faces[i]->v[2]->node->x) / 3.0;
    immVertex3f(pos, x1[0], x1[1], x1[2]);

    immAttr4f(col, color[0], color[1], color[2], color[3]);
    Vec3 x2 = x1 + (length * faces[i]->n);
    immVertex3f(pos, x2[0], x2[1], x2[2]);
  }

  immEnd();
}

void Mesh::applyTransformation()
{
  if (pos == Vec3(0, 0, 0) && scale == Vec3(1, 1, 1)) {
  }
  else {
    glm::mat4 model = glm::mat4(1.0);
    model = glm::translate(model, vec3ToGlmVec3(pos));
    model = glm::scale(model, vec3ToGlmVec3(scale));
    const int num_nodes = nodes.size();
    for (int i = 0; i < num_nodes; i++) {
      Node *node = nodes[i];
      node->x = glmVec4ToVec3(model * glm::vec4(vec3ToGlmVec3(node->x), 1.0));
    }
  }
}

void Mesh::unapplyTransformation()
{
  if (pos == Vec3(0, 0, 0) && scale == Vec3(1, 1, 1)) {
  }
  else {
    glm::mat4 model_inv = glm::mat4(1.0);
    model_inv = glm::translate(model_inv, vec3ToGlmVec3(pos));
    model_inv = glm::scale(model_inv, vec3ToGlmVec3(scale));
    model_inv = glm::inverse(model_inv);
    const int num_nodes = nodes.size();
    for (int i = 0; i < num_nodes; i++) {
      Node *node = nodes[i];
      node->x = glmVec4ToVec3(model_inv * glm::vec4(vec3ToGlmVec3(node->x), 1.0));
    }
  }
}

void Mesh::updateFaceNormals()
{
  int faces_size = faces.size();

  for (int i = 0; i < faces_size; i++) {
    Vec3 &x0 = faces[i]->v[0]->node->x;
    Vec3 &x1 = faces[i]->v[1]->node->x;
    Vec3 &x2 = faces[i]->v[2]->node->x;

    faces[i]->n = normal(x0, x1, x2).normalized();
  }
}

void Mesh::deleteMesh()
{
  for (int i = 0; i < verts.size(); i++) {
    delete verts[i];
  }
  for (int i = 0; i < nodes.size(); i++) {
    delete nodes[i];
  }
  for (int i = 0; i < edges.size(); i++) {
    delete edges[i];
  }
  for (int i = 0; i < faces.size(); i++) {
    delete faces[i];
  }

  verts.clear();
  verts.shrink_to_fit();
  nodes.clear();
  nodes.shrink_to_fit();
  edges.clear();
  edges.shrink_to_fit();
  faces.clear();
  faces.shrink_to_fit();
}
