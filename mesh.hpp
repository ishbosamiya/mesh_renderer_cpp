#ifndef MESH_HPP
#define MESH_HPP

#include <vector>
#include <cassert>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <limits>

#include "gpu_immediate.hpp"
#include "math.hpp"
#include "primitives.hpp"
#include "misc.hpp"

using namespace std;

class Vert;
class Node;
class Edge;
class Face;
class Mesh;

/* Important to note that Node is the world space vertex which can
 * have UV space coordinates thus is split into Node and
 * Vert.
 * Generally Node would be called a vertex */

/* Stores the UV information and corresponding World Space Node */
class Vert {
 public:
  vector<Face *> adj_f; /* reference to adjacent faces wrt to the
                                UV space */
  Node *node;           /*reference to node of vert */
  int index;            /* position in Mesh.verts */
  Vec2 uv;              /* UV coordinates of vert */

  Vert() : node(0), index(-1)
  {
  }
  Vert(const Vec2 &uv) : uv(uv)
  {
  }

  bool isOnSeamOrBoundary();
};

/* Stores the World Space coordinates */
class Node {
 public:
  vector<Vert *> verts; /* This helps in storing all the
                         * references to the UV's of
                         * the Node */
  vector<Edge *> adj_e; /* reference to adjacent edges of the
                         * node */
  int index;            /* position in Mesh.nodes */
  Vec3 x;               /* world space position of node */
  Vec3 n;               /* world space normal */

  Node() : index(-1)
  {
  }
  Node(const Vec3 &x) : x(x)
  {
  }
  Node(const Vec3 &x, const Vec3 &n) : x(x), n(n)
  {
  }

  /* Get Vert on the opposite side of this Node for the Edge created by this Node
   * and other->node */
  Vert *adjacent(Vert *other);

  bool isOnSeamOrBoundary();

  virtual ~Node()
  {
  }
};

/* Stores the Edge data */
class Edge {
 public:
  Node *n[2];     /* reference to nodes of edge */
  Face *adj_f[2]; /* reference to adjacent faces of edge */
  int index;      /*position in Mesh.edges */

  Edge() : index(-1)
  {
  }

  Edge(Node *n0, Node *n1)
  {
    n[0] = n0;
    n[1] = n1;
  }

  /* Get Vert of edge whose node matches n[edge_node] */
  Vert *getVert(int face_side, int edge_node);
  /* Get Vert of adj_f[face_side] that is not part of this edge */
  Vert *getOtherVertOfFace(int face_side);

  bool isOnSeamOrBoundary();
};

/* Stores the Face data */
/* Only triangles */
class Face : public Primitive {
 public:
  Vert *v[3];              /* reference to verts of the face */
  Edge *adj_e[3];          /* reference to adjacent edges of the face */
  /* unsigned int index */ /* position in Mesh.faces, is in Primitive */
  Vec3 n;                  /* normal */

  Face()
  {
    for (int i = 0; i < 3; i++) {
      v[i] = NULL;
      adj_e[i] = NULL;
    }
  }

  Face(Vert *v0, Vert *v1, Vert *v2)
  {
    v[0] = v0;
    v[1] = v1;
    v[2] = v2;
  }

  bool isOnSeamOrBoundary();
};

/* Stores the overall Mesh data */
class Mesh : public Primitive {
 private:
  void setIndices();
  void deleteMesh();

 public:
  Mesh()
  {
  }

  Mesh(const string &filename)
  {
    Mesh::loadObj(filename);
  }

  Mesh(const string &filename, Shader *shader) : Primitive(shader)
  {
    Mesh::loadObj(filename);
  }

  Mesh(const string &filename, Vec3 pos) : Primitive(pos)
  {
    Mesh::loadObj(filename);
  }

  Mesh(const string &filename, Vec3 pos, Shader *shader) : Primitive(pos, shader)
  {
    Mesh::loadObj(filename);
  }

  Mesh(const string &filename, Vec3 pos, Vec3 scale) : Primitive(pos, scale)
  {
    Mesh::loadObj(filename);
  }

  Mesh(const string &filename, Vec3 pos, Vec3 scale, Shader *shader)
      : Primitive(pos, scale, shader)
  {
    Mesh::loadObj(filename);
  }

  Mesh(Shader *shader) : Primitive(shader)
  {
  }

  Mesh(Vec3 pos) : Primitive(pos)
  {
  }

  Mesh(Vec3 pos, Shader *shader) : Primitive(pos, shader)
  {
  }

  Mesh(Vec3 pos, Vec3 scale) : Primitive(pos, scale)
  {
  }

  Mesh(Vec3 pos, Vec3 scale, Shader *shader) : Primitive(pos, scale, shader)
  {
  }

  vector<Vert *> verts;
  vector<Node *> nodes;
  vector<Edge *> edges;
  vector<Face *> faces;

  virtual void add(Vert *vert);
  virtual void add(Node *node);
  virtual void add(Edge *edge);
  virtual void add(Face *face);

  virtual void remove(Vert *vert);
  virtual void remove(Node *node);
  virtual void remove(Edge *edge);
  virtual void remove(Face *face);

  virtual void loadObj(const string &file);
  void saveObj(const string &filename);

  void shadeSmooth();

  virtual void draw();
  void drawWireframe(glm::mat4 projection, glm::mat4 view, Vec4 color);
  void drawFaceNormals(glm::mat4 projection, glm::mat4 view, Vec4 color, double length);
  void drawUVs(glm::mat4 projection, glm::mat4 view, Vec3 pos, Vec3 scale, Vec4 color);

  virtual bool intersectionTest(const Vec3 &p, Vec3 &r_normal, double &r_distance)
  {
    return false;
  }

  virtual void applyTransformation();
  virtual void unapplyTransformation();

  void updateFaceNormals();

  ~Mesh()
  {
    deleteMesh();
  }
};

inline Edge *getEdge(const Node *n0, const Node *n1)
{
  for (int i = 0; i < (int)n0->adj_e.size(); i++) {
    Edge *edge = n0->adj_e[i];
    if (edge->n[0] == n1 || edge->n[1] == n1) {
      return edge;
    }
  }
  return NULL;
}

#endif
