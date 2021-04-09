#ifndef MATH_HPP
#define MATH_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <Eigen/Core>
#include <Eigen/Dense>
#include <Eigen/Geometry>

typedef double Scalar;
typedef Eigen::Matrix<Scalar, 2, 1> Vec2;
typedef Eigen::Matrix<Scalar, 3, 1> Vec3;
typedef Eigen::Matrix<Scalar, 4, 1> Vec4;

template<int m> inline Scalar norm2(const Eigen::Matrix<Scalar, m, 1> &v)
{
  return v.dot(v);
}

inline Vec3 normal(Vec3 &a0, Vec3 &b0, Vec3 &c0)
{
  return b0 - a0.cross(c0 - a0);
}

inline glm::vec3 vec3ToGlmVec3(const Vec3 &v)
{
  return glm::vec3(v[0], v[1], v[2]);
}

inline Vec3 glmVec4ToVec3(const glm::vec4 &v)
{
  return Vec3(v[0], v[1], v[2]);
}

#endif
