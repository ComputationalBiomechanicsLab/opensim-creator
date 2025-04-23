// ORIGINAL LICENSE TEXT:
//
// https://github.com/CedricGuillemet/ImGuizmo
// v1.91.3 WIP
//
// The MIT License(MIT)
//
// Copyright(c) 2021 Cedric Guillemet
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// OPENSIM CREATOR CHANGES:
//
// This header was originally copied from a third-party source:
//
// - https://github.com/CedricGuillemet/ImGuizmo
// - commit: 6d588209f99b1324a608783d1f52faa518886c29
// - Licensed under the MIT license
// - Copyright(c) 2021 Cedric Guillemet
//
// Subsequent modifications to this file are licensed under OpenSim
// Creator's license (see LICENSE/NOTICE.txt) and have the following
// copyright:
//
// - Copyright (c) 2024 Adam Kewley
//
// Individual lines are not tagged with copyright/license notices, because
// that would be insanely impractical. The project's vcs (i.e. git) can be
// used to track each change.

// NOLINTBEGIN

#ifndef IMGUI_DEFINE_MATH_OPERATORS
    #define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui.h"
#include "imgui_internal.h"
#include "ImGuizmo.h"

#include <functional>
#include <limits>
#include <vector>

using namespace ImGuizmo;

namespace
{
    struct Context;
    constinit Context* gCurrentContext = nullptr;

    constexpr float ZPI = 3.14159265358979323846f;
    constexpr float RAD2DEG = (180.f / ZPI);
    constexpr float DEG2RAD = (ZPI / 180.f);
    constexpr float screenRotateSize = 0.06f;
    // scale a bit so translate axis do not touch when in universal
    constexpr float rotationDisplayFactor = 1.2f;
    const char* translationInfoMask[] = {
        "X : %5.3f",
        "Y : %5.3f",
        "Z : %5.3f",
        "Y : %5.3f Z : %5.3f",
        "X : %5.3f Z : %5.3f",
        "X : %5.3f Y : %5.3f",
        "X : %5.3f Y : %5.3f Z : %5.3f"
    };
    const char* const scaleInfoMask[] = {
        "X : %5.2f",
        "Y : %5.2f",
        "Z : %5.2f",
        "XYZ : %5.2f"
    };
    const char* const rotationInfoMask[] = {
        "X : %5.2f deg %5.2f rad",
        "Y : %5.2f deg %5.2f rad",
        "Z : %5.2f deg %5.2f rad",
        "Screen : %5.2f deg %5.2f rad"
    };
    constexpr int translationInfoIndex[] = {
        0,0,0,
        1,0,0,
        2,0,0,
        1,2,0,
        0,2,0,
        0,1,0,
        0,1,2
    };
    constexpr float quadMin = 0.5f;
    constexpr float quadMax = 0.8f;
    constexpr float quadUV[8] = { quadMin, quadMin, quadMin, quadMax, quadMax, quadMax, quadMax, quadMin };
    constexpr int halfCircleSegmentCount = 64;
    constexpr float snapTension = 0.5f;

    constexpr ImGuiID blank_id()
    {
        return std::numeric_limits<ImGuiID>::max();
    }

    OPERATION operator&(OPERATION lhs, OPERATION rhs)
    {
        return static_cast<OPERATION>(static_cast<int>(lhs) & static_cast<int>(rhs));
    }

    bool operator!=(OPERATION lhs, int rhs)
    {
        return static_cast<int>(lhs) != rhs;
    }

    bool Intersects(OPERATION lhs, OPERATION rhs)
    {
        return (lhs & rhs) != 0;
    }

    // True if lhs contains rhs
    bool Contains(OPERATION lhs, OPERATION rhs)
    {
        return (lhs & rhs) == rhs;
    }

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   // utility and math

   void FPU_MatrixF_x_MatrixF(const float* a, const float* b, float* r)
   {
      r[0] = a[0] * b[0] + a[1] * b[4] + a[2] * b[8] + a[3] * b[12];
      r[1] = a[0] * b[1] + a[1] * b[5] + a[2] * b[9] + a[3] * b[13];
      r[2] = a[0] * b[2] + a[1] * b[6] + a[2] * b[10] + a[3] * b[14];
      r[3] = a[0] * b[3] + a[1] * b[7] + a[2] * b[11] + a[3] * b[15];

      r[4] = a[4] * b[0] + a[5] * b[4] + a[6] * b[8] + a[7] * b[12];
      r[5] = a[4] * b[1] + a[5] * b[5] + a[6] * b[9] + a[7] * b[13];
      r[6] = a[4] * b[2] + a[5] * b[6] + a[6] * b[10] + a[7] * b[14];
      r[7] = a[4] * b[3] + a[5] * b[7] + a[6] * b[11] + a[7] * b[15];

      r[8] = a[8] * b[0] + a[9] * b[4] + a[10] * b[8] + a[11] * b[12];
      r[9] = a[8] * b[1] + a[9] * b[5] + a[10] * b[9] + a[11] * b[13];
      r[10] = a[8] * b[2] + a[9] * b[6] + a[10] * b[10] + a[11] * b[14];
      r[11] = a[8] * b[3] + a[9] * b[7] + a[10] * b[11] + a[11] * b[15];

      r[12] = a[12] * b[0] + a[13] * b[4] + a[14] * b[8] + a[15] * b[12];
      r[13] = a[12] * b[1] + a[13] * b[5] + a[14] * b[9] + a[15] * b[13];
      r[14] = a[12] * b[2] + a[13] * b[6] + a[14] * b[10] + a[15] * b[14];
      r[15] = a[12] * b[3] + a[13] * b[7] + a[14] * b[11] + a[15] * b[15];
   }

   void Frustum(float left, float right, float bottom, float top, float znear, float zfar, float* m16)
   {
      float temp, temp2, temp3, temp4;
      temp = 2.0f * znear;
      temp2 = right - left;
      temp3 = top - bottom;
      temp4 = zfar - znear;
      m16[0] = temp / temp2;
      m16[1] = 0.0;
      m16[2] = 0.0;
      m16[3] = 0.0;
      m16[4] = 0.0;
      m16[5] = temp / temp3;
      m16[6] = 0.0;
      m16[7] = 0.0;
      m16[8] = (right + left) / temp2;
      m16[9] = (top + bottom) / temp3;
      m16[10] = (-zfar - znear) / temp4;
      m16[11] = -1.0f;
      m16[12] = 0.0;
      m16[13] = 0.0;
      m16[14] = (-temp * zfar) / temp4;
      m16[15] = 0.0;
   }

   void Perspective(float fovyInDegrees, float aspectRatio, float znear, float zfar, float* m16)
   {
      float ymax, xmax;
      ymax = znear * tanf(fovyInDegrees * DEG2RAD);
      xmax = ymax * aspectRatio;
      Frustum(-xmax, xmax, -ymax, ymax, znear, zfar, m16);
   }

   void Cross(const float* a, const float* b, float* r)
   {
      r[0] = a[1] * b[2] - a[2] * b[1];
      r[1] = a[2] * b[0] - a[0] * b[2];
      r[2] = a[0] * b[1] - a[1] * b[0];
   }

   float Dot(const float* a, const float* b)
   {
      return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
   }

   void Normalize(const float* a, float* r)
   {
      float il = 1.f / (sqrtf(Dot(a, a)) + FLT_EPSILON);
      r[0] = a[0] * il;
      r[1] = a[1] * il;
      r[2] = a[2] * il;
   }

   void LookAt(const float* eye, const float* at, const float* up, float* m16)
   {
      float X[3], Y[3], Z[3], tmp[3];

      tmp[0] = eye[0] - at[0];
      tmp[1] = eye[1] - at[1];
      tmp[2] = eye[2] - at[2];
      Normalize(tmp, Z);
      Normalize(up, Y);
      Cross(Y, Z, tmp);
      Normalize(tmp, X);
      Cross(Z, X, tmp);
      Normalize(tmp, Y);

      m16[0] = X[0];
      m16[1] = Y[0];
      m16[2] = Z[0];
      m16[3] = 0.0f;
      m16[4] = X[1];
      m16[5] = Y[1];
      m16[6] = Z[1];
      m16[7] = 0.0f;
      m16[8] = X[2];
      m16[9] = Y[2];
      m16[10] = Z[2];
      m16[11] = 0.0f;
      m16[12] = -Dot(X, eye);
      m16[13] = -Dot(Y, eye);
      m16[14] = -Dot(Z, eye);
      m16[15] = 1.0f;
   }

   template <typename T> T Clamp(T x, T y, T z) { return ((x < y) ? y : ((x > z) ? z : x)); }
   template <typename T> T max(T x, T y) { return (x > y) ? x : y; }
   template <typename T> T min(T x, T y) { return (x < y) ? x : y; }
   template <typename T> bool IsWithin(T x, T y, T z) { return (x >= y) && (x <= z); }

   struct matrix_t;
   struct vec_t {
   public:
      float x = 0.0f, y = 0.0f, z = 0.0f, w = 0.0f;

      void Lerp(const vec_t& v, float t)
      {
         x += (v.x - x) * t;
         y += (v.y - y) * t;
         z += (v.z - z) * t;
         w += (v.w - w) * t;
      }

      void Set(float v) { x = y = z = w = v; }
      void Set(float _x, float _y, float _z = 0.f, float _w = 0.f) { x = _x; y = _y; z = _z; w = _w; }

      vec_t& operator -= (const vec_t& v) { x -= v.x; y -= v.y; z -= v.z; w -= v.w; return *this; }
      vec_t& operator += (const vec_t& v) { x += v.x; y += v.y; z += v.z; w += v.w; return *this; }
      vec_t& operator *= (const vec_t& v) { x *= v.x; y *= v.y; z *= v.z; w *= v.w; return *this; }
      vec_t& operator *= (float v) { x *= v;    y *= v;    z *= v;    w *= v;    return *this; }

      vec_t operator * (float f) const;
      vec_t operator - () const;
      vec_t operator - (const vec_t& v) const;
      vec_t operator + (const vec_t& v) const;
      vec_t operator * (const vec_t& v) const;

      const vec_t& operator + () const { return (*this); }
      float Length() const { return sqrtf(x * x + y * y + z * z); };
      float LengthSq() const { return (x * x + y * y + z * z); };
      vec_t Normalize() { (*this) *= (1.f / ( Length() > FLT_EPSILON ? Length() : FLT_EPSILON ) ); return (*this); }
      vec_t Normalize(const vec_t& v) { this->Set(v.x, v.y, v.z, v.w); this->Normalize(); return (*this); }
      vec_t Abs() const;

      void Cross(const vec_t& v)
      {
         vec_t res;
         res.x = y * v.z - z * v.y;
         res.y = z * v.x - x * v.z;
         res.z = x * v.y - y * v.x;

         x = res.x;
         y = res.y;
         z = res.z;
         w = 0.f;
      }

      void Cross(const vec_t& v1, const vec_t& v2)
      {
         x = v1.y * v2.z - v1.z * v2.y;
         y = v1.z * v2.x - v1.x * v2.z;
         z = v1.x * v2.y - v1.y * v2.x;
         w = 0.f;
      }

      float Dot(const vec_t& v) const
      {
         return (x * v.x) + (y * v.y) + (z * v.z) + (w * v.w);
      }

      float Dot3(const vec_t& v) const
      {
         return (x * v.x) + (y * v.y) + (z * v.z);
      }

      void Transform(const matrix_t& matrix);
      void Transform(const vec_t& s, const matrix_t& matrix);

      void TransformVector(const matrix_t& matrix);
      void TransformPoint(const matrix_t& matrix);
      void TransformVector(const vec_t& v, const matrix_t& matrix) { (*this) = v; this->TransformVector(matrix); }
      void TransformPoint(const vec_t& v, const matrix_t& matrix) { (*this) = v; this->TransformPoint(matrix); }

      float& operator [] (size_t index) { return (&x)[index]; }
      const float& operator [] (size_t index) const { return (&x)[index]; }
      bool operator!=(const vec_t& other) const { return memcmp(this, &other, sizeof(vec_t)) != 0; }
   };

   vec_t makeVect(float _x, float _y, float _z = 0.f, float _w = 0.f) { vec_t res; res.x = _x; res.y = _y; res.z = _z; res.w = _w; return res; }
   vec_t makeVect(ImVec2 v) { vec_t res; res.x = v.x; res.y = v.y; res.z = 0.f; res.w = 0.f; return res; }
   vec_t vec_t::operator * (float f) const { return makeVect(x * f, y * f, z * f, w * f); }
   vec_t vec_t::operator - () const { return makeVect(-x, -y, -z, -w); }
   vec_t vec_t::operator - (const vec_t& v) const { return makeVect(x - v.x, y - v.y, z - v.z, w - v.w); }
   vec_t vec_t::operator + (const vec_t& v) const { return makeVect(x + v.x, y + v.y, z + v.z, w + v.w); }
   vec_t vec_t::operator * (const vec_t& v) const { return makeVect(x * v.x, y * v.y, z * v.z, w * v.w); }
   vec_t vec_t::Abs() const { return makeVect(fabsf(x), fabsf(y), fabsf(z)); }

   const vec_t directionUnary[3] = { makeVect(1.f, 0.f, 0.f), makeVect(0.f, 1.f, 0.f), makeVect(0.f, 0.f, 1.f) };

   vec_t Normalized(const vec_t& v) { vec_t res; res = v; res.Normalize(); return res; }
   vec_t Cross(const vec_t& v1, const vec_t& v2)
   {
      vec_t res;
      res.x = v1.y * v2.z - v1.z * v2.y;
      res.y = v1.z * v2.x - v1.x * v2.z;
      res.z = v1.x * v2.y - v1.y * v2.x;
      res.w = 0.f;
      return res;
   }

   float Dot(const vec_t& v1, const vec_t& v2)
   {
      return (v1.x * v2.x) + (v1.y * v2.y) + (v1.z * v2.z);
   }

   vec_t BuildPlan(const vec_t& p_point1, const vec_t& p_normal)
   {
      vec_t normal, res;
      normal.Normalize(p_normal);
      res.w = normal.Dot(p_point1);
      res.x = normal.x;
      res.y = normal.y;
      res.z = normal.z;
      return res;
   }

   struct matrix_t {
   public:

      struct Directions { vec_t right, up, dir, position; };
      union {
         float m[4][4];
         float m16[16];
         Directions v;
         vec_t component[4];
      };

      matrix_t()
      {
         v = Directions{};  // TODO: this should be cleaned up
      }

      operator float* () { return m16; }
      operator const float* () const { return m16; }
      void Translation(float _x, float _y, float _z) { this->Translation(makeVect(_x, _y, _z)); }

      void Translation(const vec_t& vt)
      {
         v.right.Set(1.f, 0.f, 0.f, 0.f);
         v.up.Set(0.f, 1.f, 0.f, 0.f);
         v.dir.Set(0.f, 0.f, 1.f, 0.f);
         v.position.Set(vt.x, vt.y, vt.z, 1.f);
      }

      void Scale(float _x, float _y, float _z)
      {
         v.right.Set(_x, 0.f, 0.f, 0.f);
         v.up.Set(0.f, _y, 0.f, 0.f);
         v.dir.Set(0.f, 0.f, _z, 0.f);
         v.position.Set(0.f, 0.f, 0.f, 1.f);
      }
      void Scale(const vec_t& s) { Scale(s.x, s.y, s.z); }

      matrix_t& operator *= (const matrix_t& mat)
      {
         matrix_t tmpMat;
         tmpMat = *this;
         tmpMat.Multiply(mat);
         *this = tmpMat;
         return *this;
      }
      matrix_t operator * (const matrix_t& mat) const
      {
         matrix_t matT;
         matT.Multiply(*this, mat);
         return matT;
      }

      void Multiply(const matrix_t& matrix)
      {
         matrix_t tmp;
         tmp = *this;

         FPU_MatrixF_x_MatrixF(static_cast<const float*>(tmp), static_cast<const float*>(matrix), (float*)this);
      }

      void Multiply(const matrix_t& m1, const matrix_t& m2)
      {
         FPU_MatrixF_x_MatrixF(static_cast<const float*>(m1), static_cast<const float*>(m2), (float*)this);
      }

      float GetDeterminant() const
      {
         return m[0][0] * m[1][1] * m[2][2] + m[0][1] * m[1][2] * m[2][0] + m[0][2] * m[1][0] * m[2][1] -
            m[0][2] * m[1][1] * m[2][0] - m[0][1] * m[1][0] * m[2][2] - m[0][0] * m[1][2] * m[2][1];
      }

      float Inverse(const matrix_t& srcMatrix, bool affine = false);
      void SetToIdentity()
      {
         v.right.Set(1.f, 0.f, 0.f, 0.f);
         v.up.Set(0.f, 1.f, 0.f, 0.f);
         v.dir.Set(0.f, 0.f, 1.f, 0.f);
         v.position.Set(0.f, 0.f, 0.f, 1.f);
      }
      void Transpose()
      {
         matrix_t tmpm;
         for (int l = 0; l < 4; l++) {
            for (int c = 0; c < 4; c++) {
               tmpm.m[l][c] = m[c][l];
            }
         }
         (*this) = tmpm;
      }

      void RotationAxis(const vec_t& axis, float angle);

      void OrthoNormalize()
      {
         v.right.Normalize();
         v.up.Normalize();
         v.dir.Normalize();
      }
   };

   void vec_t::Transform(const matrix_t& matrix)
   {
      vec_t out;

      out.x = x * matrix.m[0][0] + y * matrix.m[1][0] + z * matrix.m[2][0] + w * matrix.m[3][0];
      out.y = x * matrix.m[0][1] + y * matrix.m[1][1] + z * matrix.m[2][1] + w * matrix.m[3][1];
      out.z = x * matrix.m[0][2] + y * matrix.m[1][2] + z * matrix.m[2][2] + w * matrix.m[3][2];
      out.w = x * matrix.m[0][3] + y * matrix.m[1][3] + z * matrix.m[2][3] + w * matrix.m[3][3];

      x = out.x;
      y = out.y;
      z = out.z;
      w = out.w;
   }

   void vec_t::Transform(const vec_t& s, const matrix_t& matrix)
   {
      *this = s;
      Transform(matrix);
   }

   void vec_t::TransformPoint(const matrix_t& matrix)
   {
      vec_t out;

      out.x = x * matrix.m[0][0] + y * matrix.m[1][0] + z * matrix.m[2][0] + matrix.m[3][0];
      out.y = x * matrix.m[0][1] + y * matrix.m[1][1] + z * matrix.m[2][1] + matrix.m[3][1];
      out.z = x * matrix.m[0][2] + y * matrix.m[1][2] + z * matrix.m[2][2] + matrix.m[3][2];
      out.w = x * matrix.m[0][3] + y * matrix.m[1][3] + z * matrix.m[2][3] + matrix.m[3][3];

      x = out.x;
      y = out.y;
      z = out.z;
      w = out.w;
   }

   void vec_t::TransformVector(const matrix_t& matrix)
   {
      vec_t out;

      out.x = x * matrix.m[0][0] + y * matrix.m[1][0] + z * matrix.m[2][0];
      out.y = x * matrix.m[0][1] + y * matrix.m[1][1] + z * matrix.m[2][1];
      out.z = x * matrix.m[0][2] + y * matrix.m[1][2] + z * matrix.m[2][2];
      out.w = x * matrix.m[0][3] + y * matrix.m[1][3] + z * matrix.m[2][3];

      x = out.x;
      y = out.y;
      z = out.z;
      w = out.w;
   }

   float matrix_t::Inverse(const matrix_t& srcMatrix, bool affine)
   {
      float det = 0;

      if (affine) {
         det = GetDeterminant();
         float s = 1 / det;
         m[0][0] = (srcMatrix.m[1][1] * srcMatrix.m[2][2] - srcMatrix.m[1][2] * srcMatrix.m[2][1]) * s;
         m[0][1] = (srcMatrix.m[2][1] * srcMatrix.m[0][2] - srcMatrix.m[2][2] * srcMatrix.m[0][1]) * s;
         m[0][2] = (srcMatrix.m[0][1] * srcMatrix.m[1][2] - srcMatrix.m[0][2] * srcMatrix.m[1][1]) * s;
         m[1][0] = (srcMatrix.m[1][2] * srcMatrix.m[2][0] - srcMatrix.m[1][0] * srcMatrix.m[2][2]) * s;
         m[1][1] = (srcMatrix.m[2][2] * srcMatrix.m[0][0] - srcMatrix.m[2][0] * srcMatrix.m[0][2]) * s;
         m[1][2] = (srcMatrix.m[0][2] * srcMatrix.m[1][0] - srcMatrix.m[0][0] * srcMatrix.m[1][2]) * s;
         m[2][0] = (srcMatrix.m[1][0] * srcMatrix.m[2][1] - srcMatrix.m[1][1] * srcMatrix.m[2][0]) * s;
         m[2][1] = (srcMatrix.m[2][0] * srcMatrix.m[0][1] - srcMatrix.m[2][1] * srcMatrix.m[0][0]) * s;
         m[2][2] = (srcMatrix.m[0][0] * srcMatrix.m[1][1] - srcMatrix.m[0][1] * srcMatrix.m[1][0]) * s;
         m[3][0] = -(m[0][0] * srcMatrix.m[3][0] + m[1][0] * srcMatrix.m[3][1] + m[2][0] * srcMatrix.m[3][2]);
         m[3][1] = -(m[0][1] * srcMatrix.m[3][0] + m[1][1] * srcMatrix.m[3][1] + m[2][1] * srcMatrix.m[3][2]);
         m[3][2] = -(m[0][2] * srcMatrix.m[3][0] + m[1][2] * srcMatrix.m[3][1] + m[2][2] * srcMatrix.m[3][2]);
      }
      else {
         // transpose matrix
         float src[16];
         for (int i = 0; i < 4; ++i) {
            src[i] = srcMatrix.m16[i * 4];
            src[i + 4] = srcMatrix.m16[i * 4 + 1];
            src[i + 8] = srcMatrix.m16[i * 4 + 2];
            src[i + 12] = srcMatrix.m16[i * 4 + 3];
         }

         // calculate pairs for first 8 elements (cofactors)
         float tmp[12]; // temp array for pairs
         tmp[0] = src[10] * src[15];
         tmp[1] = src[11] * src[14];
         tmp[2] = src[9] * src[15];
         tmp[3] = src[11] * src[13];
         tmp[4] = src[9] * src[14];
         tmp[5] = src[10] * src[13];
         tmp[6] = src[8] * src[15];
         tmp[7] = src[11] * src[12];
         tmp[8] = src[8] * src[14];
         tmp[9] = src[10] * src[12];
         tmp[10] = src[8] * src[13];
         tmp[11] = src[9] * src[12];

         // calculate first 8 elements (cofactors)
         m16[0] = (tmp[0] * src[5] + tmp[3] * src[6] + tmp[4] * src[7]) - (tmp[1] * src[5] + tmp[2] * src[6] + tmp[5] * src[7]);
         m16[1] = (tmp[1] * src[4] + tmp[6] * src[6] + tmp[9] * src[7]) - (tmp[0] * src[4] + tmp[7] * src[6] + tmp[8] * src[7]);
         m16[2] = (tmp[2] * src[4] + tmp[7] * src[5] + tmp[10] * src[7]) - (tmp[3] * src[4] + tmp[6] * src[5] + tmp[11] * src[7]);
         m16[3] = (tmp[5] * src[4] + tmp[8] * src[5] + tmp[11] * src[6]) - (tmp[4] * src[4] + tmp[9] * src[5] + tmp[10] * src[6]);
         m16[4] = (tmp[1] * src[1] + tmp[2] * src[2] + tmp[5] * src[3]) - (tmp[0] * src[1] + tmp[3] * src[2] + tmp[4] * src[3]);
         m16[5] = (tmp[0] * src[0] + tmp[7] * src[2] + tmp[8] * src[3]) - (tmp[1] * src[0] + tmp[6] * src[2] + tmp[9] * src[3]);
         m16[6] = (tmp[3] * src[0] + tmp[6] * src[1] + tmp[11] * src[3]) - (tmp[2] * src[0] + tmp[7] * src[1] + tmp[10] * src[3]);
         m16[7] = (tmp[4] * src[0] + tmp[9] * src[1] + tmp[10] * src[2]) - (tmp[5] * src[0] + tmp[8] * src[1] + tmp[11] * src[2]);

         // calculate pairs for second 8 elements (cofactors)
         tmp[0] = src[2] * src[7];
         tmp[1] = src[3] * src[6];
         tmp[2] = src[1] * src[7];
         tmp[3] = src[3] * src[5];
         tmp[4] = src[1] * src[6];
         tmp[5] = src[2] * src[5];
         tmp[6] = src[0] * src[7];
         tmp[7] = src[3] * src[4];
         tmp[8] = src[0] * src[6];
         tmp[9] = src[2] * src[4];
         tmp[10] = src[0] * src[5];
         tmp[11] = src[1] * src[4];

         // calculate second 8 elements (cofactors)
         m16[8] = (tmp[0] * src[13] + tmp[3] * src[14] + tmp[4] * src[15]) - (tmp[1] * src[13] + tmp[2] * src[14] + tmp[5] * src[15]);
         m16[9] = (tmp[1] * src[12] + tmp[6] * src[14] + tmp[9] * src[15]) - (tmp[0] * src[12] + tmp[7] * src[14] + tmp[8] * src[15]);
         m16[10] = (tmp[2] * src[12] + tmp[7] * src[13] + tmp[10] * src[15]) - (tmp[3] * src[12] + tmp[6] * src[13] + tmp[11] * src[15]);
         m16[11] = (tmp[5] * src[12] + tmp[8] * src[13] + tmp[11] * src[14]) - (tmp[4] * src[12] + tmp[9] * src[13] + tmp[10] * src[14]);
         m16[12] = (tmp[2] * src[10] + tmp[5] * src[11] + tmp[1] * src[9]) - (tmp[4] * src[11] + tmp[0] * src[9] + tmp[3] * src[10]);
         m16[13] = (tmp[8] * src[11] + tmp[0] * src[8] + tmp[7] * src[10]) - (tmp[6] * src[10] + tmp[9] * src[11] + tmp[1] * src[8]);
         m16[14] = (tmp[6] * src[9] + tmp[11] * src[11] + tmp[3] * src[8]) - (tmp[10] * src[11] + tmp[2] * src[8] + tmp[7] * src[9]);
         m16[15] = (tmp[10] * src[10] + tmp[4] * src[8] + tmp[9] * src[9]) - (tmp[8] * src[9] + tmp[11] * src[10] + tmp[5] * src[8]);

         // calculate determinant
         det = src[0] * m16[0] + src[1] * m16[1] + src[2] * m16[2] + src[3] * m16[3];

         // calculate matrix inverse
         float invdet = 1 / det;
         for (int j = 0; j < 16; ++j) {
            m16[j] *= invdet;
         }
      }

      return det;
   }

   void matrix_t::RotationAxis(const vec_t& axis, float angle)
   {
      float length2 = axis.LengthSq();
      if (length2 < FLT_EPSILON) {
         SetToIdentity();
         return;
      }

      vec_t n = axis * (1.f / sqrtf(length2));
      float s = sinf(angle);
      float c = cosf(angle);
      float k = 1.f - c;

      float xx = n.x * n.x * k + c;
      float yy = n.y * n.y * k + c;
      float zz = n.z * n.z * k + c;
      float xy = n.x * n.y * k;
      float yz = n.y * n.z * k;
      float zx = n.z * n.x * k;
      float xs = n.x * s;
      float ys = n.y * s;
      float zs = n.z * s;

      m[0][0] = xx;
      m[0][1] = xy + zs;
      m[0][2] = zx - ys;
      m[0][3] = 0.f;
      m[1][0] = xy - zs;
      m[1][1] = yy;
      m[1][2] = yz + xs;
      m[1][3] = 0.f;
      m[2][0] = zx + ys;
      m[2][1] = yz - xs;
      m[2][2] = zz;
      m[2][3] = 0.f;
      m[3][0] = 0.f;
      m[3][1] = 0.f;
      m[3][2] = 0.f;
      m[3][3] = 1.f;
   }

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   //

   enum MOVETYPE {
      MT_NONE,
      MT_MOVE_X,
      MT_MOVE_Y,
      MT_MOVE_Z,
      MT_MOVE_YZ,
      MT_MOVE_ZX,
      MT_MOVE_XY,
      MT_MOVE_SCREEN,
      MT_ROTATE_X,
      MT_ROTATE_Y,
      MT_ROTATE_Z,
      MT_ROTATE_SCREEN,
      MT_SCALE_X,
      MT_SCALE_Y,
      MT_SCALE_Z,
      MT_SCALE_XYZ
   };

   bool IsTranslateType(int type)
   {
     return type >= MT_MOVE_X && type <= MT_MOVE_SCREEN;
   }

   bool IsRotateType(int type)
   {
     return type >= MT_ROTATE_X && type <= MT_ROTATE_SCREEN;
   }

   bool IsScaleType(int type)
   {
     return type >= MT_SCALE_X && type <= MT_SCALE_XYZ;
   }

   // Matches MT_MOVE_AB order
   constexpr OPERATION TRANSLATE_PLANS[3] = {
       TRANSLATE_Y | TRANSLATE_Z,
       TRANSLATE_X | TRANSLATE_Z,
       TRANSLATE_X | TRANSLATE_Y
   };

   struct Style {
       Style()
       {
           // default values
           TranslationLineThickness   = 3.0f;
           TranslationLineArrowSize   = 6.0f;
           RotationLineThickness      = 2.0f;
           RotationOuterLineThickness = 3.0f;
           ScaleLineThickness         = 3.0f;
           ScaleLineCircleSize        = 6.0f;
           HatchedAxisLineThickness   = 6.0f;
           CenterCircleSize           = 6.0f;

           // oscar style
           TranslationLineThickness = 5.0f;
           TranslationLineArrowSize = 8.0f;
           RotationLineThickness = 5.0f;
           RotationOuterLineThickness = 7.0f;
           ScaleLineThickness = 5.0f;
           ScaleLineCircleSize = 8.0f;

           // initialize default colors
           Colors[DIRECTION_X]           = ImVec4(0.666f, 0.000f, 0.000f, 1.000f);
           Colors[DIRECTION_Y]           = ImVec4(0.000f, 0.666f, 0.000f, 1.000f);
           Colors[DIRECTION_Z]           = ImVec4(0.000f, 0.000f, 0.666f, 1.000f);
           Colors[PLANE_X]               = ImVec4(0.666f, 0.000f, 0.000f, 0.380f);
           Colors[PLANE_Y]               = ImVec4(0.000f, 0.666f, 0.000f, 0.380f);
           Colors[PLANE_Z]               = ImVec4(0.000f, 0.000f, 0.666f, 0.380f);
           Colors[SELECTION]             = ImVec4(1.000f, 0.500f, 0.062f, 0.541f);
           Colors[INACTIVE]              = ImVec4(0.600f, 0.600f, 0.600f, 0.600f);
           Colors[TRANSLATION_LINE]      = ImVec4(0.666f, 0.666f, 0.666f, 0.666f);
           Colors[SCALE_LINE]            = ImVec4(0.250f, 0.250f, 0.250f, 1.000f);
           Colors[ROTATION_USING_BORDER] = ImVec4(1.000f, 0.500f, 0.062f, 1.000f);
           Colors[ROTATION_USING_FILL]   = ImVec4(1.000f, 0.500f, 0.062f, 0.500f);
           Colors[HATCHED_AXIS_LINES]    = ImVec4(0.000f, 0.000f, 0.000f, 0.500f);
           Colors[TEXT]                  = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
           Colors[TEXT_SHADOW]           = ImVec4(0.000f, 0.000f, 0.000f, 1.000f);
       }

       float TranslationLineThickness;   // Thickness of lines for translation gizmo
       float TranslationLineArrowSize;   // Size of arrow at the end of lines for translation gizmo
       float RotationLineThickness;      // Thickness of lines for rotation gizmo
       float RotationOuterLineThickness; // Thickness of line surrounding the rotation gizmo
       float ScaleLineThickness;         // Thickness of lines for scale gizmo
       float ScaleLineCircleSize;        // Size of circle at the end of lines for scale gizmo
       float HatchedAxisLineThickness;   // Thickness of hatched axis lines
       float CenterCircleSize;           // Size of circle at the center of the translate/scale gizmo

       ImVec4 Colors[COLOR::COUNT];
   };

   struct Context {
      Context()
      {
          mIDStack.push_back(blank_id());
      }

      ImDrawList* mDrawList;
      Style mStyle;

      MODE mMode;
      matrix_t mViewMat;
      matrix_t mProjectionMat;
      matrix_t mModel;
      matrix_t mModelLocal; // orthonormalized model
      matrix_t mModelInverse;
      matrix_t mModelSource;
      matrix_t mModelSourceInverse;
      matrix_t mMVP;
      matrix_t mMVPLocal; // MVP with full model matrix whereas mMVP's model matrix might only be translation in case of World space edition
      matrix_t mViewProjection;

      vec_t mModelScaleOrigin;
      vec_t mCameraEye;
      vec_t mCameraRight;
      vec_t mCameraDir;
      vec_t mCameraUp;
      vec_t mRayOrigin;
      vec_t mRayVector;

      float  mRadiusSquareCenter;
      ImVec2 mScreenSquareCenter;
      ImVec2 mScreenSquareMin;
      ImVec2 mScreenSquareMax;

      float mScreenFactor;
      vec_t mRelativeOrigin;

      bool mbUsing = false;
      bool mbEnable = true;
      bool mbMouseOver;
      bool mReversed; // reversed projection matrix

      // translation
      vec_t mTranslationPlan;
      vec_t mTranslationPlanOrigin;
      vec_t mMatrixOrigin;
      vec_t mTranslationLastDelta;

      // rotation
      vec_t mRotationVectorSource;
      float mRotationAngle;
      float mRotationAngleOrigin;
      //vec_t mWorldToLocalAxis;

      // scale
      vec_t mScale;
      vec_t mScaleValueOrigin;
      vec_t mScaleLast;
      float mSaveMousePosx;

      // save axis factor when using gizmo
      bool mBelowAxisLimit[3];
      int mAxisMask = 0;
      bool mBelowPlaneLimit[3];
      float mAxisFactor[3];

      float mAxisLimit=0.0025f;
      float mPlaneLimit=0.02f;

      // bounds stretching
      vec_t mBoundsPivot;
      vec_t mBoundsAnchor;
      vec_t mBoundsPlan;
      vec_t mBoundsLocalPivot;
      int mBoundsBestAxis;
      int mBoundsAxis[2];
      bool mbUsingBounds = false;
      matrix_t mBoundsMatrix;

      //
      int mCurrentOperation;

      float mX = 0.f;
      float mY = 0.f;
      float mWidth = 0.f;
      float mHeight = 0.f;
      float mXMax = 0.f;
      float mYMax = 0.f;
      float mDisplayRatio = 1.f;

      bool mIsOrthographic = false;
      // check to not have multiple gizmo highlighted at the same time
      bool mbOverGizmoHotspot = false;

      ImVector<ImGuiID> mIDStack;
      ImGuiID mEditingID = blank_id();
      OPERATION mOperation = OPERATION::NONE;

      bool mAllowAxisFlip = false;
      float mGizmoSizeClipSpace = 0.1f;

      inline ImGuiID GetCurrentID() { return mIDStack.back();}
   };

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   //
   int GetMoveType(OPERATION op, vec_t* gizmoHitProportion);
   int GetRotateType(OPERATION op);
   int GetScaleType(OPERATION op);

   Style& GetStyle()
   {
      return gCurrentContext->mStyle;
   }

   ImU32 GetColorU32(int idx)
   {
      IM_ASSERT(idx < COLOR::COUNT);
      return ImGui::ColorConvertFloat4ToU32(gCurrentContext->mStyle.Colors[idx]);
   }

   ImVec2 worldToPos(
       const vec_t& worldPos,
       const matrix_t& mat,
       ImVec2 position = ImVec2(gCurrentContext->mX, gCurrentContext->mY),
       ImVec2 size = ImVec2(gCurrentContext->mWidth, gCurrentContext->mHeight))
   {
      vec_t trans;
      trans.TransformPoint(worldPos, mat);
      trans *= 0.5f / trans.w;
      trans += makeVect(0.5f, 0.5f);
      trans.y = 1.f - trans.y;
      trans.x *= size.x;
      trans.y *= size.y;
      trans.x += position.x;
      trans.y += position.y;
      return ImVec2(trans.x, trans.y);
   }

   void ComputeCameraRay(
       vec_t& rayOrigin,
       vec_t& rayDir,
       ImVec2 position = ImVec2(gCurrentContext->mX, gCurrentContext->mY),
       ImVec2 size = ImVec2(gCurrentContext->mWidth, gCurrentContext->mHeight))
   {
      ImGuiIO& io = ImGui::GetIO();

      matrix_t mViewProjInverse;
      mViewProjInverse.Inverse(gCurrentContext->mViewMat * gCurrentContext->mProjectionMat);

      const float mox = ((io.MousePos.x - position.x) / size.x) * 2.f - 1.f;
      const float moy = (1.f - ((io.MousePos.y - position.y) / size.y)) * 2.f - 1.f;

      const float zNear = gCurrentContext->mReversed ? (1.f - FLT_EPSILON) : 0.f;
      const float zFar = gCurrentContext->mReversed ? 0.f : (1.f - FLT_EPSILON);

      rayOrigin.Transform(makeVect(mox, moy, zNear, 1.f), mViewProjInverse);
      rayOrigin *= 1.f / rayOrigin.w;
      vec_t rayEnd;
      rayEnd.Transform(makeVect(mox, moy, zFar, 1.f), mViewProjInverse);
      rayEnd *= 1.f / rayEnd.w;
      rayDir = Normalized(rayEnd - rayOrigin);
   }

   float GetSegmentLengthClipSpace(const vec_t& start, const vec_t& end, const bool localCoordinates = false)
   {
      vec_t startOfSegment = start;
      const matrix_t& mvp = localCoordinates ? gCurrentContext->mMVPLocal : gCurrentContext->mMVP;
      startOfSegment.TransformPoint(mvp);
      if (fabsf(startOfSegment.w) > FLT_EPSILON) // check for axis aligned with camera direction
      {
         startOfSegment *= 1.f / startOfSegment.w;
      }

      vec_t endOfSegment = end;
      endOfSegment.TransformPoint(mvp);
      if (fabsf(endOfSegment.w) > FLT_EPSILON) // check for axis aligned with camera direction
      {
         endOfSegment *= 1.f / endOfSegment.w;
      }

      vec_t clipSpaceAxis = endOfSegment - startOfSegment;
      if (gCurrentContext->mDisplayRatio < 1.0)
         clipSpaceAxis.x *= gCurrentContext->mDisplayRatio;
      else
         clipSpaceAxis.y /= gCurrentContext->mDisplayRatio;
      float segmentLengthInClipSpace = sqrtf(clipSpaceAxis.x * clipSpaceAxis.x + clipSpaceAxis.y * clipSpaceAxis.y);
      return segmentLengthInClipSpace;
   }

   float GetParallelogram(const vec_t& ptO, const vec_t& ptA, const vec_t& ptB)
   {
      vec_t pts[] = { ptO, ptA, ptB };
      for (unsigned int i = 0; i < 3; i++)
      {
         pts[i].TransformPoint(gCurrentContext->mMVP);
         if (fabsf(pts[i].w) > FLT_EPSILON) // check for axis aligned with camera direction
         {
            pts[i] *= 1.f / pts[i].w;
         }
      }
      vec_t segA = pts[1] - pts[0];
      vec_t segB = pts[2] - pts[0];
      segA.y /= gCurrentContext->mDisplayRatio;
      segB.y /= gCurrentContext->mDisplayRatio;
      vec_t segAOrtho = makeVect(-segA.y, segA.x);
      segAOrtho.Normalize();
      float dt = segAOrtho.Dot3(segB);
      float surface = sqrtf(segA.x * segA.x + segA.y * segA.y) * fabsf(dt);
      return surface;
   }

   inline vec_t PointOnSegment(const vec_t& point, const vec_t& vertPos1, const vec_t& vertPos2)
   {
      vec_t c = point - vertPos1;
      vec_t V;

      V.Normalize(vertPos2 - vertPos1);
      float d = (vertPos2 - vertPos1).Length();
      float t = V.Dot3(c);

      if (t < 0.f)
      {
         return vertPos1;
      }

      if (t > d)
      {
         return vertPos2;
      }

      return vertPos1 + V * t;
   }

   float IntersectRayPlane(const vec_t& rOrigin, const vec_t& rVector, const vec_t& plan)
   {
      const float numer = plan.Dot3(rOrigin) - plan.w;
      const float denom = plan.Dot3(rVector);

      if (fabsf(denom) < FLT_EPSILON)  // normal is orthogonal to vector, cant intersect
      {
         return -1.0f;
      }

      return -(numer / denom);
   }

   bool IsInContextRect(ImVec2 p)
   {
      return IsWithin(p.x, gCurrentContext->mX, gCurrentContext->mXMax) && IsWithin(p.y, gCurrentContext->mY, gCurrentContext->mYMax);
   }

   bool IsHoveringWindow()
   {
      ImGuiContext& g = *ImGui::GetCurrentContext();
      ImGuiWindow* window = ImGui::FindWindowByName(gCurrentContext->mDrawList->_OwnerName);
      if (g.HoveredWindow == window)   // Mouse hovering drawlist window
         return true;
      if (g.HoveredWindow != NULL)     // Any other window is hovered
         return false;
      if (ImGui::IsMouseHoveringRect(window->InnerRect.Min, window->InnerRect.Max, false))   // Hovering drawlist window rect, while no other window is hovered (for _NoInputs windows)
         return true;
      return false;
   }

   void ComputeContext(const float* view, const float* projection, float* matrix, MODE mode)
   {
      gCurrentContext->mMode = mode;
      gCurrentContext->mViewMat = *(const matrix_t*)view;
      gCurrentContext->mProjectionMat = *(const matrix_t*)projection;
      gCurrentContext->mbMouseOver = IsHoveringWindow();

      gCurrentContext->mModelLocal = *(const matrix_t*)matrix;
      gCurrentContext->mModelLocal.OrthoNormalize();

      if (mode == LOCAL)
      {
         gCurrentContext->mModel = gCurrentContext->mModelLocal;
      }
      else
      {
         gCurrentContext->mModel.Translation(((matrix_t*)matrix)->v.position);
      }
      gCurrentContext->mModelSource = *(matrix_t*)matrix;
      gCurrentContext->mModelScaleOrigin.Set(gCurrentContext->mModelSource.v.right.Length(), gCurrentContext->mModelSource.v.up.Length(), gCurrentContext->mModelSource.v.dir.Length());

      gCurrentContext->mModelInverse.Inverse(gCurrentContext->mModel);
      gCurrentContext->mModelSourceInverse.Inverse(gCurrentContext->mModelSource);
      gCurrentContext->mViewProjection = gCurrentContext->mViewMat * gCurrentContext->mProjectionMat;
      gCurrentContext->mMVP = gCurrentContext->mModel * gCurrentContext->mViewProjection;
      gCurrentContext->mMVPLocal = gCurrentContext->mModelLocal * gCurrentContext->mViewProjection;

      matrix_t viewInverse;
      viewInverse.Inverse(gCurrentContext->mViewMat);
      gCurrentContext->mCameraDir = viewInverse.v.dir;
      gCurrentContext->mCameraEye = viewInverse.v.position;
      gCurrentContext->mCameraRight = viewInverse.v.right;
      gCurrentContext->mCameraUp = viewInverse.v.up;

      // projection reverse
       vec_t nearPos, farPos;
       nearPos.Transform(makeVect(0, 0, 1.f, 1.f), gCurrentContext->mProjectionMat);
       farPos.Transform(makeVect(0, 0, 2.f, 1.f), gCurrentContext->mProjectionMat);

       gCurrentContext->mReversed = (nearPos.z/nearPos.w) > (farPos.z / farPos.w);

      // compute scale from the size of camera right vector projected on screen at the matrix position
      vec_t pointRight = viewInverse.v.right;
      pointRight.TransformPoint(gCurrentContext->mViewProjection);
      gCurrentContext->mScreenFactor = gCurrentContext->mGizmoSizeClipSpace / (pointRight.x / pointRight.w - gCurrentContext->mMVP.v.position.x / gCurrentContext->mMVP.v.position.w);

      vec_t rightViewInverse = viewInverse.v.right;
      rightViewInverse.TransformVector(gCurrentContext->mModelInverse);
      float rightLength = GetSegmentLengthClipSpace(makeVect(0.f, 0.f), rightViewInverse);
      gCurrentContext->mScreenFactor = gCurrentContext->mGizmoSizeClipSpace / rightLength;

      ImVec2 centerSSpace = worldToPos(makeVect(0.f, 0.f), gCurrentContext->mMVP);
      gCurrentContext->mScreenSquareCenter = centerSSpace;
      gCurrentContext->mScreenSquareMin = ImVec2(centerSSpace.x - 10.f, centerSSpace.y - 10.f);
      gCurrentContext->mScreenSquareMax = ImVec2(centerSSpace.x + 10.f, centerSSpace.y + 10.f);

      ComputeCameraRay(gCurrentContext->mRayOrigin, gCurrentContext->mRayVector);
   }

   void ComputeColors(ImU32* colors, int type, OPERATION operation)
   {
      if (gCurrentContext->mbEnable)
      {
         ImU32 selectionColor = GetColorU32(SELECTION);

         switch (operation)
         {
         case TRANSLATE:
            colors[0] = (type == MT_MOVE_SCREEN) ? selectionColor : ImGui::ColorConvertFloat4ToU32({0.8f, 0.5f, 0.3f, 0.8f});  // HACK: osc: make translation circle orange, which stands out from the mostly-white geometry
            for (int i = 0; i < 3; i++)
            {
               colors[i + 1] = (type == (int)(MT_MOVE_X + i)) ? selectionColor : GetColorU32(DIRECTION_X + i);
               colors[i + 4] = (type == (int)(MT_MOVE_YZ + i)) ? selectionColor : GetColorU32(PLANE_X + i);
               colors[i + 4] = (type == MT_MOVE_SCREEN) ? selectionColor : colors[i + 4];
            }
            break;
         case ROTATE:
            colors[0] = (type == MT_ROTATE_SCREEN) ? selectionColor : IM_COL32_WHITE;
            for (int i = 0; i < 3; i++)
            {
               colors[i + 1] = (type == (int)(MT_ROTATE_X + i)) ? selectionColor : GetColorU32(DIRECTION_X + i);
            }
            break;
         case SCALEU:
         case SCALE:
            colors[0] = (type == MT_SCALE_XYZ) ? selectionColor : IM_COL32_WHITE;
            for (int i = 0; i < 3; i++)
            {
               colors[i + 1] = (type == (int)(MT_SCALE_X + i)) ? selectionColor : GetColorU32(DIRECTION_X + i);
            }
            break;
         // note: this internal function is only called with three possible values for operation
         default:
            break;
         }
      }
      else
      {
         ImU32 inactiveColor = GetColorU32(INACTIVE);
         for (int i = 0; i < 7; i++)
         {
            colors[i] = inactiveColor;
         }
      }
   }

   void ComputeTripodAxisAndVisibility(const int axisIndex, vec_t& dirAxis, vec_t& dirPlaneX, vec_t& dirPlaneY, bool& belowAxisLimit, bool& belowPlaneLimit, const bool localCoordinates = false)
   {
      dirAxis = directionUnary[axisIndex];
      dirPlaneX = directionUnary[(axisIndex + 1) % 3];
      dirPlaneY = directionUnary[(axisIndex + 2) % 3];

      if (gCurrentContext->mbUsing && (gCurrentContext->GetCurrentID() == gCurrentContext->mEditingID))
      {
         // when using, use stored factors so the gizmo doesn't flip when we translate

         // Apply axis mask to axes and planes
         belowAxisLimit = gCurrentContext->mBelowAxisLimit[axisIndex] && ((1<<axisIndex)&gCurrentContext->mAxisMask);
         belowPlaneLimit = gCurrentContext->mBelowPlaneLimit[axisIndex] && ((((1<<axisIndex)&gCurrentContext->mAxisMask) && !(gCurrentContext->mAxisMask & (gCurrentContext->mAxisMask - 1))) || !gCurrentContext->mAxisMask);

         dirAxis *= gCurrentContext->mAxisFactor[axisIndex];
         dirPlaneX *= gCurrentContext->mAxisFactor[(axisIndex + 1) % 3];
         dirPlaneY *= gCurrentContext->mAxisFactor[(axisIndex + 2) % 3];
      }
      else
      {
         // new method
         float lenDir = GetSegmentLengthClipSpace(makeVect(0.f, 0.f, 0.f), dirAxis, localCoordinates);
         float lenDirMinus = GetSegmentLengthClipSpace(makeVect(0.f, 0.f, 0.f), -dirAxis, localCoordinates);

         float lenDirPlaneX = GetSegmentLengthClipSpace(makeVect(0.f, 0.f, 0.f), dirPlaneX, localCoordinates);
         float lenDirMinusPlaneX = GetSegmentLengthClipSpace(makeVect(0.f, 0.f, 0.f), -dirPlaneX, localCoordinates);

         float lenDirPlaneY = GetSegmentLengthClipSpace(makeVect(0.f, 0.f, 0.f), dirPlaneY, localCoordinates);
         float lenDirMinusPlaneY = GetSegmentLengthClipSpace(makeVect(0.f, 0.f, 0.f), -dirPlaneY, localCoordinates);

         // For readability
         bool allowFlip = gCurrentContext->mAllowAxisFlip;
         float mulAxis = (allowFlip && lenDir < lenDirMinus&& fabsf(lenDir - lenDirMinus) > FLT_EPSILON) ? -1.f : 1.f;
         float mulAxisX = (allowFlip && lenDirPlaneX < lenDirMinusPlaneX&& fabsf(lenDirPlaneX - lenDirMinusPlaneX) > FLT_EPSILON) ? -1.f : 1.f;
         float mulAxisY = (allowFlip && lenDirPlaneY < lenDirMinusPlaneY&& fabsf(lenDirPlaneY - lenDirMinusPlaneY) > FLT_EPSILON) ? -1.f : 1.f;
         dirAxis *= mulAxis;
         dirPlaneX *= mulAxisX;
         dirPlaneY *= mulAxisY;

         // for axis
         float axisLengthInClipSpace = GetSegmentLengthClipSpace(makeVect(0.f, 0.f, 0.f), dirAxis * gCurrentContext->mScreenFactor, localCoordinates);

         float paraSurf = GetParallelogram(makeVect(0.f, 0.f, 0.f), dirPlaneX * gCurrentContext->mScreenFactor, dirPlaneY * gCurrentContext->mScreenFactor);
         // Apply axis mask to axes and planes
         belowPlaneLimit = (paraSurf > gCurrentContext->mAxisLimit) && ((((1<<axisIndex)&gCurrentContext->mAxisMask) && !(gCurrentContext->mAxisMask & (gCurrentContext->mAxisMask - 1))) || !gCurrentContext->mAxisMask);
         belowAxisLimit = (axisLengthInClipSpace > gCurrentContext->mPlaneLimit) && !((1<<axisIndex)&gCurrentContext->mAxisMask);

         // and store values
         gCurrentContext->mAxisFactor[axisIndex] = mulAxis;
         gCurrentContext->mAxisFactor[(axisIndex + 1) % 3] = mulAxisX;
         gCurrentContext->mAxisFactor[(axisIndex + 2) % 3] = mulAxisY;
         gCurrentContext->mBelowAxisLimit[axisIndex] = belowAxisLimit;
         gCurrentContext->mBelowPlaneLimit[axisIndex] = belowPlaneLimit;
      }
   }

   void ComputeSnap(float* value, float snap)
   {
      if (snap <= FLT_EPSILON)
      {
         return;
      }

      float modulo = fmodf(*value, snap);
      float moduloRatio = fabsf(modulo) / snap;
      if (moduloRatio < snapTension)
      {
         *value -= modulo;
      }
      else if (moduloRatio > (1.f - snapTension))
      {
         *value = *value - modulo + snap * ((*value < 0.f) ? -1.f : 1.f);
      }
   }

   void ComputeSnap(vec_t& value, const float* snap)
   {
      for (int i = 0; i < 3; i++)
      {
         ComputeSnap(&value[i], snap[i]);
      }
   }

   float ComputeAngleOnPlan()
   {
      const float len = IntersectRayPlane(gCurrentContext->mRayOrigin, gCurrentContext->mRayVector, gCurrentContext->mTranslationPlan);
      vec_t localPos = Normalized(gCurrentContext->mRayOrigin + gCurrentContext->mRayVector * len - gCurrentContext->mModel.v.position);

      vec_t perpendicularVector;
      perpendicularVector.Cross(gCurrentContext->mRotationVectorSource, gCurrentContext->mTranslationPlan);
      perpendicularVector.Normalize();
      float acosAngle = Clamp(Dot(localPos, gCurrentContext->mRotationVectorSource), -1.f, 1.f);
      float angle = acosf(acosAngle);
      angle *= (Dot(localPos, perpendicularVector) < 0.f) ? 1.f : -1.f;
      return angle;
   }

   void DrawRotationGizmo(OPERATION op, int type)
   {
      if(!Intersects(op, ROTATE))
      {
         return;
      }
      ImDrawList* drawList = gCurrentContext->mDrawList;

      bool isMultipleAxesMasked = gCurrentContext->mAxisMask & (gCurrentContext->mAxisMask - 1);
      bool isNoAxesMasked = !gCurrentContext->mAxisMask;

      // colors
      ImU32 colors[7];
      ComputeColors(colors, type, ROTATE);

      vec_t cameraToModelNormalized;
      if (gCurrentContext->mIsOrthographic)
      {
         matrix_t viewInverse;
         viewInverse.Inverse(*(matrix_t*)&gCurrentContext->mViewMat);
         cameraToModelNormalized = -viewInverse.v.dir;
      }
      else
      {
         cameraToModelNormalized = Normalized(gCurrentContext->mModel.v.position - gCurrentContext->mCameraEye);
      }

      cameraToModelNormalized.TransformVector(gCurrentContext->mModelInverse);

      gCurrentContext->mRadiusSquareCenter = screenRotateSize * gCurrentContext->mHeight;

      bool hasRSC = Intersects(op, ROTATE_SCREEN);
      for (int axis = 0; axis < 3; axis++)
      {
         if(!Intersects(op, static_cast<OPERATION>(ROTATE_Z >> axis)))
         {
            continue;
         }

         bool isAxisMasked = (1 << (2 - axis)) & gCurrentContext->mAxisMask;

         if ((!isAxisMasked || isMultipleAxesMasked) && !isNoAxesMasked)
         {
            continue;
         }
         const bool usingAxis = (gCurrentContext->mbUsing && type == MT_ROTATE_Z - axis);
         const int circleMul = (hasRSC && !usingAxis) ? 1 : 2;

         std::vector<ImVec2> circlePositions;
         circlePositions.reserve(circleMul * halfCircleSegmentCount + 1);
         ImVec2* circlePos = circlePositions.data();

         float angleStart = atan2f(cameraToModelNormalized[(4 - axis) % 3], cameraToModelNormalized[(3 - axis) % 3]) + ZPI * 0.5f;

         for (int i = 0; i < circleMul * halfCircleSegmentCount + 1; i++)
         {
            float ng = angleStart + (float)circleMul * ZPI * ((float)i / (float)(circleMul * halfCircleSegmentCount));
            vec_t axisPos = makeVect(cosf(ng), sinf(ng), 0.f);
            vec_t pos = makeVect(axisPos[axis], axisPos[(axis + 1) % 3], axisPos[(axis + 2) % 3]) * gCurrentContext->mScreenFactor * rotationDisplayFactor;
            circlePos[i] = worldToPos(pos, gCurrentContext->mMVP);
         }
         if (!gCurrentContext->mbUsing || usingAxis)
         {
            drawList->AddPolyline(circlePos, circleMul* halfCircleSegmentCount + 1, colors[3 - axis], false, gCurrentContext->mStyle.RotationLineThickness);
         }

         float radiusAxis = sqrtf((ImLengthSqr(worldToPos(gCurrentContext->mModel.v.position, gCurrentContext->mViewProjection) - circlePos[0])));
         if (radiusAxis > gCurrentContext->mRadiusSquareCenter)
         {
            gCurrentContext->mRadiusSquareCenter = radiusAxis;
         }
      }
      if(hasRSC && (!gCurrentContext->mbUsing || type == MT_ROTATE_SCREEN) && (!isMultipleAxesMasked && isNoAxesMasked))
      {
         drawList->AddCircle(worldToPos(gCurrentContext->mModel.v.position, gCurrentContext->mViewProjection), gCurrentContext->mRadiusSquareCenter, colors[0], 64, gCurrentContext->mStyle.RotationOuterLineThickness);
      }

      if (gCurrentContext->mbUsing && (gCurrentContext->GetCurrentID() == gCurrentContext->mEditingID) && IsRotateType(type))
      {
         ImVec2 circlePos[halfCircleSegmentCount + 1];

         circlePos[0] = worldToPos(gCurrentContext->mModel.v.position, gCurrentContext->mViewProjection);
         for (unsigned int i = 1; i < halfCircleSegmentCount + 1; i++)
         {
            float ng = gCurrentContext->mRotationAngle * ((float)(i - 1) / (float)(halfCircleSegmentCount - 1));
            matrix_t rotateVectorMatrix;
            rotateVectorMatrix.RotationAxis(gCurrentContext->mTranslationPlan, ng);
            vec_t pos;
            pos.TransformPoint(gCurrentContext->mRotationVectorSource, rotateVectorMatrix);
            pos *= gCurrentContext->mScreenFactor * rotationDisplayFactor;
            circlePos[i] = worldToPos(pos + gCurrentContext->mModel.v.position, gCurrentContext->mViewProjection);
         }
         drawList->AddConvexPolyFilled(circlePos, halfCircleSegmentCount + 1, GetColorU32(ROTATION_USING_FILL));
         drawList->AddPolyline(circlePos, halfCircleSegmentCount + 1, GetColorU32(ROTATION_USING_BORDER), true, gCurrentContext->mStyle.RotationLineThickness);

         ImVec2 destinationPosOnScreen = circlePos[1];
         char tmps[512];
         ImFormatString(tmps, sizeof(tmps), rotationInfoMask[type - MT_ROTATE_X], (gCurrentContext->mRotationAngle / ZPI) * 180.f, gCurrentContext->mRotationAngle);
         drawList->AddText(ImVec2(destinationPosOnScreen.x + AnnotationOffset()+1.0f, destinationPosOnScreen.y + AnnotationOffset()+1.0f), GetColorU32(TEXT_SHADOW), tmps);
         drawList->AddText(ImVec2(destinationPosOnScreen.x + AnnotationOffset(), destinationPosOnScreen.y + AnnotationOffset()), GetColorU32(TEXT), tmps);
      }
   }

   void DrawHatchedAxis(const vec_t& axis)
   {
      if (gCurrentContext->mStyle.HatchedAxisLineThickness <= 0.0f)
      {
         return;
      }

      for (int j = 1; j < 10; j++)
      {
         ImVec2 baseSSpace2 = worldToPos(axis * 0.05f * (float)(j * 2) * gCurrentContext->mScreenFactor, gCurrentContext->mMVP);
         ImVec2 worldDirSSpace2 = worldToPos(axis * 0.05f * (float)(j * 2 + 1) * gCurrentContext->mScreenFactor, gCurrentContext->mMVP);
         gCurrentContext->mDrawList->AddLine(baseSSpace2, worldDirSSpace2, GetColorU32(HATCHED_AXIS_LINES), gCurrentContext->mStyle.HatchedAxisLineThickness);
      }
   }

   void DrawScaleGizmo(OPERATION op, int type)
   {
      ImDrawList* drawList = gCurrentContext->mDrawList;

      if(!Intersects(op, SCALE))
      {
        return;
      }

      // colors
      ImU32 colors[7];
      ComputeColors(colors, type, SCALE);

      // draw
      vec_t scaleDisplay = { 1.f, 1.f, 1.f, 1.f };

      if (gCurrentContext->mbUsing && (gCurrentContext->GetCurrentID() == gCurrentContext->mEditingID))
      {
         scaleDisplay = gCurrentContext->mScale;
      }

      for (int i = 0; i < 3; i++)
      {
         if(!Intersects(op, static_cast<OPERATION>(SCALE_X << i)))
         {
            continue;
         }
         const bool usingAxis = (gCurrentContext->mbUsing && type == MT_SCALE_X + i);
         if (!gCurrentContext->mbUsing || usingAxis)
         {
            vec_t dirPlaneX, dirPlaneY, dirAxis;
            bool belowAxisLimit, belowPlaneLimit;
            ComputeTripodAxisAndVisibility(i, dirAxis, dirPlaneX, dirPlaneY, belowAxisLimit, belowPlaneLimit, true);

            // draw axis
            if (belowAxisLimit)
            {
               bool hasTranslateOnAxis = Contains(op, static_cast<OPERATION>(TRANSLATE_X << i));
               float markerScale = hasTranslateOnAxis ? 1.4f : 1.0f;
               ImVec2 baseSSpace = worldToPos(dirAxis * 0.1f * gCurrentContext->mScreenFactor, gCurrentContext->mMVP);
               ImVec2 worldDirSSpaceNoScale = worldToPos(dirAxis * markerScale * gCurrentContext->mScreenFactor, gCurrentContext->mMVP);
               ImVec2 worldDirSSpace = worldToPos((dirAxis * markerScale * scaleDisplay[i]) * gCurrentContext->mScreenFactor, gCurrentContext->mMVP);

               if (gCurrentContext->mbUsing && (gCurrentContext->GetCurrentID() == gCurrentContext->mEditingID))
               {
                  ImU32 scaleLineColor = GetColorU32(SCALE_LINE);
                  drawList->AddLine(baseSSpace, worldDirSSpaceNoScale, scaleLineColor, gCurrentContext->mStyle.ScaleLineThickness);
                  drawList->AddCircleFilled(worldDirSSpaceNoScale, gCurrentContext->mStyle.ScaleLineCircleSize, scaleLineColor);
               }

               if (!hasTranslateOnAxis || gCurrentContext->mbUsing)
               {
                  drawList->AddLine(baseSSpace, worldDirSSpace, colors[i + 1], gCurrentContext->mStyle.ScaleLineThickness);
               }
               drawList->AddCircleFilled(worldDirSSpace, gCurrentContext->mStyle.ScaleLineCircleSize, colors[i + 1]);

               if (gCurrentContext->mAxisFactor[i] < 0.f)
               {
                  DrawHatchedAxis(dirAxis * scaleDisplay[i]);
               }
            }
         }
      }

      // draw screen cirle
      drawList->AddCircleFilled(gCurrentContext->mScreenSquareCenter, gCurrentContext->mStyle.CenterCircleSize, colors[0], 32);

      if (gCurrentContext->mbUsing && (gCurrentContext->GetCurrentID() == gCurrentContext->mEditingID) && IsScaleType(type))
      {
         //ImVec2 sourcePosOnScreen = worldToPos(gCurrentContext->mMatrixOrigin, gCurrentContext->mViewProjection);
         ImVec2 destinationPosOnScreen = worldToPos(gCurrentContext->mModel.v.position, gCurrentContext->mViewProjection);
         /*vec_t dif(destinationPosOnScreen.x - sourcePosOnScreen.x, destinationPosOnScreen.y - sourcePosOnScreen.y);
         dif.Normalize();
         dif *= 5.f;
         drawList->AddCircle(sourcePosOnScreen, 6.f, translationLineColor);
         drawList->AddCircle(destinationPosOnScreen, 6.f, translationLineColor);
         drawList->AddLine(ImVec2(sourcePosOnScreen.x + dif.x, sourcePosOnScreen.y + dif.y), ImVec2(destinationPosOnScreen.x - dif.x, destinationPosOnScreen.y - dif.y), translationLineColor, 2.f);
         */
         char tmps[512];
         //vec_t deltaInfo = gCurrentContext->mModel.v.position - gCurrentContext->mMatrixOrigin;
         int componentInfoIndex = (type - MT_SCALE_X) * 3;
         ImFormatString(tmps, sizeof(tmps), scaleInfoMask[type - MT_SCALE_X], scaleDisplay[translationInfoIndex[componentInfoIndex]]);
         drawList->AddText(ImVec2(destinationPosOnScreen.x + 15, destinationPosOnScreen.y + 15), GetColorU32(TEXT_SHADOW), tmps);
         drawList->AddText(ImVec2(destinationPosOnScreen.x + 14, destinationPosOnScreen.y + 14), GetColorU32(TEXT), tmps);
      }
   }

   void DrawScaleUniveralGizmo(OPERATION op, int type)
   {
      ImDrawList* drawList = gCurrentContext->mDrawList;

      if (!Intersects(op, SCALEU))
      {
         return;
      }

      // colors
      ImU32 colors[7];
      ComputeColors(colors, type, SCALEU);

      // draw
      vec_t scaleDisplay = { 1.f, 1.f, 1.f, 1.f };

      if (gCurrentContext->mbUsing && (gCurrentContext->GetCurrentID() == gCurrentContext->mEditingID))
      {
         scaleDisplay = gCurrentContext->mScale;
      }

      for (int i = 0; i < 3; i++)
      {
         if (!Intersects(op, static_cast<OPERATION>(SCALE_XU << i)))
         {
            continue;
         }
         const bool usingAxis = (gCurrentContext->mbUsing && type == MT_SCALE_X + i);
         if (!gCurrentContext->mbUsing || usingAxis)
         {
            vec_t dirPlaneX, dirPlaneY, dirAxis;
            bool belowAxisLimit, belowPlaneLimit;
            ComputeTripodAxisAndVisibility(i, dirAxis, dirPlaneX, dirPlaneY, belowAxisLimit, belowPlaneLimit, true);

            // draw axis
            if (belowAxisLimit)
            {
               bool hasTranslateOnAxis = Contains(op, static_cast<OPERATION>(TRANSLATE_X << i));
               float markerScale = hasTranslateOnAxis ? 1.4f : 1.0f;
               //ImVec2 baseSSpace = worldToPos(dirAxis * 0.1f * gCurrentContext->mScreenFactor, gCurrentContext->mMVPLocal);
               //ImVec2 worldDirSSpaceNoScale = worldToPos(dirAxis * markerScale * gCurrentContext->mScreenFactor, gCurrentContext->mMVP);
               ImVec2 worldDirSSpace = worldToPos((dirAxis * markerScale * scaleDisplay[i]) * gCurrentContext->mScreenFactor, gCurrentContext->mMVPLocal);

#if 0
               if (gCurrentContext->mbUsing && (gCurrentContext->GetCurrentID() == gCurrentContext->mEditingID))
               {
                  drawList->AddLine(baseSSpace, worldDirSSpaceNoScale, IM_COL32(0x40, 0x40, 0x40, 0xFF), 3.f);
                  drawList->AddCircleFilled(worldDirSSpaceNoScale, 6.f, IM_COL32(0x40, 0x40, 0x40, 0xFF));
               }
               /*
               if (!hasTranslateOnAxis || gCurrentContext->mbUsing)
               {
                  drawList->AddLine(baseSSpace, worldDirSSpace, colors[i + 1], 3.f);
               }
               */
#endif
               drawList->AddCircleFilled(worldDirSSpace, 12.f, colors[i + 1]);
            }
         }
      }

      // draw screen cirle
      drawList->AddCircle(gCurrentContext->mScreenSquareCenter, 20.f, colors[0], 32, gCurrentContext->mStyle.CenterCircleSize);

      if (gCurrentContext->mbUsing && (gCurrentContext->GetCurrentID() == gCurrentContext->mEditingID) && IsScaleType(type))
      {
         //ImVec2 sourcePosOnScreen = worldToPos(gCurrentContext->mMatrixOrigin, gCurrentContext->mViewProjection);
         ImVec2 destinationPosOnScreen = worldToPos(gCurrentContext->mModel.v.position, gCurrentContext->mViewProjection);
         /*vec_t dif(destinationPosOnScreen.x - sourcePosOnScreen.x, destinationPosOnScreen.y - sourcePosOnScreen.y);
         dif.Normalize();
         dif *= 5.f;
         drawList->AddCircle(sourcePosOnScreen, 6.f, translationLineColor);
         drawList->AddCircle(destinationPosOnScreen, 6.f, translationLineColor);
         drawList->AddLine(ImVec2(sourcePosOnScreen.x + dif.x, sourcePosOnScreen.y + dif.y), ImVec2(destinationPosOnScreen.x - dif.x, destinationPosOnScreen.y - dif.y), translationLineColor, 2.f);
         */
         char tmps[512];
         //vec_t deltaInfo = gCurrentContext->mModel.v.position - gCurrentContext->mMatrixOrigin;
         int componentInfoIndex = (type - MT_SCALE_X) * 3;
         ImFormatString(tmps, sizeof(tmps), scaleInfoMask[type - MT_SCALE_X], scaleDisplay[translationInfoIndex[componentInfoIndex]]);
         drawList->AddText(ImVec2(destinationPosOnScreen.x + 15, destinationPosOnScreen.y + 15), GetColorU32(TEXT_SHADOW), tmps);
         drawList->AddText(ImVec2(destinationPosOnScreen.x + 14, destinationPosOnScreen.y + 14), GetColorU32(TEXT), tmps);
      }
   }

   void DrawTranslationGizmo(OPERATION op, int type)
   {
      ImDrawList* drawList = gCurrentContext->mDrawList;
      if (!drawList)
      {
         return;
      }

      if(!Intersects(op, TRANSLATE))
      {
         return;
      }

      // colors
      ImU32 colors[7];
      ComputeColors(colors, type, TRANSLATE);

      const ImVec2 origin = worldToPos(gCurrentContext->mModel.v.position, gCurrentContext->mViewProjection);

      // draw
      bool belowAxisLimit = false;
      bool belowPlaneLimit = false;
      for (int i = 0; i < 3; ++i)
      {
         vec_t dirPlaneX, dirPlaneY, dirAxis;
         ComputeTripodAxisAndVisibility(i, dirAxis, dirPlaneX, dirPlaneY, belowAxisLimit, belowPlaneLimit);

         if (!gCurrentContext->mbUsing || (gCurrentContext->mbUsing && type == MT_MOVE_X + i))
         {
            // draw axis
            if (belowAxisLimit && Intersects(op, static_cast<OPERATION>(TRANSLATE_X << i)))
            {
               ImVec2 baseSSpace = worldToPos(dirAxis * 0.1f * gCurrentContext->mScreenFactor, gCurrentContext->mMVP);
               ImVec2 worldDirSSpace = worldToPos(dirAxis * gCurrentContext->mScreenFactor, gCurrentContext->mMVP);

               drawList->AddLine(baseSSpace, worldDirSSpace, colors[i + 1], gCurrentContext->mStyle.TranslationLineThickness);

               // Arrow head begin
               ImVec2 dir(origin - worldDirSSpace);

               float d = sqrtf(ImLengthSqr(dir));
               dir /= d; // Normalize
               dir *= gCurrentContext->mStyle.TranslationLineArrowSize;

               ImVec2 ortogonalDir(dir.y, -dir.x); // Perpendicular vector
               ImVec2 a(worldDirSSpace + dir);
               drawList->AddTriangleFilled(worldDirSSpace - dir, a + ortogonalDir, a - ortogonalDir, colors[i + 1]);
               // Arrow head end

               if (gCurrentContext->mAxisFactor[i] < 0.f)
               {
                  DrawHatchedAxis(dirAxis);
               }
            }
         }
         // draw plane
         if (!gCurrentContext->mbUsing || (gCurrentContext->mbUsing && type == MT_MOVE_YZ + i))
         {
            if (belowPlaneLimit && Contains(op, TRANSLATE_PLANS[i]))
            {
               ImVec2 screenQuadPts[4];
               for (int j = 0; j < 4; ++j)
               {
                  vec_t cornerWorldPos = (dirPlaneX * quadUV[j * 2] + dirPlaneY * quadUV[j * 2 + 1]) * gCurrentContext->mScreenFactor;
                  screenQuadPts[j] = worldToPos(cornerWorldPos, gCurrentContext->mMVP);
               }
               drawList->AddPolyline(screenQuadPts, 4, GetColorU32(DIRECTION_X + i), true, 1.0f);
               drawList->AddConvexPolyFilled(screenQuadPts, 4, colors[i + 4]);
            }
         }
      }

      drawList->AddCircleFilled(gCurrentContext->mScreenSquareCenter, gCurrentContext->mStyle.CenterCircleSize, colors[0], 32);

      if (gCurrentContext->mbUsing && (gCurrentContext->GetCurrentID() == gCurrentContext->mEditingID) && IsTranslateType(type))
      {
         ImU32 translationLineColor = GetColorU32(TRANSLATION_LINE);

         ImVec2 sourcePosOnScreen = worldToPos(gCurrentContext->mMatrixOrigin, gCurrentContext->mViewProjection);
         ImVec2 destinationPosOnScreen = worldToPos(gCurrentContext->mModel.v.position, gCurrentContext->mViewProjection);
         vec_t dif = { destinationPosOnScreen.x - sourcePosOnScreen.x, destinationPosOnScreen.y - sourcePosOnScreen.y, 0.f, 0.f };
         dif.Normalize();
         dif *= 5.f;
         drawList->AddCircle(sourcePosOnScreen, 6.f, translationLineColor);
         drawList->AddCircle(destinationPosOnScreen, 6.f, translationLineColor);
         drawList->AddLine(ImVec2(sourcePosOnScreen.x + dif.x, sourcePosOnScreen.y + dif.y), ImVec2(destinationPosOnScreen.x - dif.x, destinationPosOnScreen.y - dif.y), translationLineColor, 2.f);

         char tmps[512];
         vec_t deltaInfo = gCurrentContext->mModel.v.position - gCurrentContext->mMatrixOrigin;
         int componentInfoIndex = (type - MT_MOVE_X) * 3;
         ImFormatString(tmps, sizeof(tmps), translationInfoMask[type - MT_MOVE_X], deltaInfo[translationInfoIndex[componentInfoIndex]], deltaInfo[translationInfoIndex[componentInfoIndex + 1]], deltaInfo[translationInfoIndex[componentInfoIndex + 2]]);
         drawList->AddText(ImVec2(destinationPosOnScreen.x + 15, destinationPosOnScreen.y + 15), GetColorU32(TEXT_SHADOW), tmps);
         drawList->AddText(ImVec2(destinationPosOnScreen.x + 14, destinationPosOnScreen.y + 14), GetColorU32(TEXT), tmps);
      }
   }

   bool CanActivate()
   {
      if (ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered() && !ImGui::IsAnyItemActive())
      {
         return true;
      }
      return false;
   }

   void HandleAndDrawLocalBounds(const float* bounds, matrix_t* matrix, const float* snapValues, OPERATION operation)
   {
      ImGuiIO& io = ImGui::GetIO();
      ImDrawList* drawList = gCurrentContext->mDrawList;

      // compute best projection axis
      vec_t axesWorldDirections[3];
      vec_t bestAxisWorldDirection = { 0.0f, 0.0f, 0.0f, 0.0f };
      int axes[3];
      unsigned int numAxes = 1;
      axes[0] = gCurrentContext->mBoundsBestAxis;
      int bestAxis = axes[0];
      if (!gCurrentContext->mbUsingBounds)
      {
         numAxes = 0;
         float bestDot = 0.f;
         for (int i = 0; i < 3; i++)
         {
            vec_t dirPlaneNormalWorld;
            dirPlaneNormalWorld.TransformVector(directionUnary[i], gCurrentContext->mModelSource);
            dirPlaneNormalWorld.Normalize();

            float dt = fabsf(Dot(Normalized(gCurrentContext->mCameraEye - gCurrentContext->mModelSource.v.position), dirPlaneNormalWorld));
            if (dt >= bestDot)
            {
               bestDot = dt;
               bestAxis = i;
               bestAxisWorldDirection = dirPlaneNormalWorld;
            }

            if (dt >= 0.1f)
            {
               axes[numAxes] = i;
               axesWorldDirections[numAxes] = dirPlaneNormalWorld;
               ++numAxes;
            }
         }
      }

      if (numAxes == 0)
      {
         axes[0] = bestAxis;
         axesWorldDirections[0] = bestAxisWorldDirection;
         numAxes = 1;
      }

      else if (bestAxis != axes[0])
      {
         unsigned int bestIndex = 0;
         for (unsigned int i = 0; i < numAxes; i++)
         {
            if (axes[i] == bestAxis)
            {
               bestIndex = i;
               break;
            }
         }
         int tempAxis = axes[0];
         axes[0] = axes[bestIndex];
         axes[bestIndex] = tempAxis;
         vec_t tempDirection = axesWorldDirections[0];
         axesWorldDirections[0] = axesWorldDirections[bestIndex];
         axesWorldDirections[bestIndex] = tempDirection;
      }

      for (unsigned int axisIndex = 0; axisIndex < numAxes; ++axisIndex)
      {
         bestAxis = axes[axisIndex];
         bestAxisWorldDirection = axesWorldDirections[axisIndex];

         // corners
         vec_t aabb[4];

         int secondAxis = (bestAxis + 1) % 3;
         int thirdAxis = (bestAxis + 2) % 3;

         for (int i = 0; i < 4; i++)
         {
            aabb[i][3] = aabb[i][bestAxis] = 0.f;
            aabb[i][secondAxis] = bounds[secondAxis + 3 * (i >> 1)];
            aabb[i][thirdAxis] = bounds[thirdAxis + 3 * ((i >> 1) ^ (i & 1))];
         }

         // draw bounds
         unsigned int anchorAlpha = gCurrentContext->mbEnable ? IM_COL32_BLACK : IM_COL32(0, 0, 0, 0x80);

         matrix_t boundsMVP = gCurrentContext->mModelSource * gCurrentContext->mViewProjection;
         for (int i = 0; i < 4; i++)
         {
            ImVec2 worldBound1 = worldToPos(aabb[i], boundsMVP);
            ImVec2 worldBound2 = worldToPos(aabb[(i + 1) % 4], boundsMVP);
            if (!IsInContextRect(worldBound1) || !IsInContextRect(worldBound2))
            {
               continue;
            }
            float boundDistance = sqrtf(ImLengthSqr(worldBound1 - worldBound2));
            int stepCount = (int)(boundDistance / 10.f);
            stepCount = min(stepCount, 1000);
            for (int j = 0; j < stepCount; j++)
            {
               float stepLength = 1.f / (float)stepCount;
               float t1 = (float)j * stepLength;
               float t2 = (float)j * stepLength + stepLength * 0.5f;
               ImVec2 worldBoundSS1 = ImLerp(worldBound1, worldBound2, ImVec2(t1, t1));
               ImVec2 worldBoundSS2 = ImLerp(worldBound1, worldBound2, ImVec2(t2, t2));
               //drawList->AddLine(worldBoundSS1, worldBoundSS2, IM_COL32(0, 0, 0, 0) + anchorAlpha, 3.f);
               drawList->AddLine(worldBoundSS1, worldBoundSS2, IM_COL32(0xAA, 0xAA, 0xAA, 0) + anchorAlpha, 2.f);
            }
            vec_t midPoint = (aabb[i] + aabb[(i + 1) % 4]) * 0.5f;
            ImVec2 midBound = worldToPos(midPoint, boundsMVP);
            constexpr float AnchorBigRadius = 8.f;
            constexpr float AnchorSmallRadius = 6.f;
            bool overBigAnchor = ImLengthSqr(worldBound1 - io.MousePos) <= (AnchorBigRadius * AnchorBigRadius);
            bool overSmallAnchor = ImLengthSqr(midBound - io.MousePos) <= (AnchorBigRadius * AnchorBigRadius);

            int type = MT_NONE;
            vec_t gizmoHitProportion;

            if(Intersects(operation, TRANSLATE))
            {
               type = GetMoveType(operation, &gizmoHitProportion);
            }
            if(Intersects(operation, ROTATE) && type == MT_NONE)
            {
               type = GetRotateType(operation);
            }
            if(Intersects(operation, SCALE) && type == MT_NONE)
            {
               type = GetScaleType(operation);
            }

            if (type != MT_NONE)
            {
               overBigAnchor = false;
               overSmallAnchor = false;
            }

            ImU32 selectionColor = GetColorU32(SELECTION);

            unsigned int bigAnchorColor = overBigAnchor ? selectionColor : (IM_COL32(0xAA, 0xAA, 0xAA, 0) + anchorAlpha);
            unsigned int smallAnchorColor = overSmallAnchor ? selectionColor : (IM_COL32(0xAA, 0xAA, 0xAA, 0) + anchorAlpha);

            drawList->AddCircleFilled(worldBound1, AnchorBigRadius, IM_COL32_BLACK);
            drawList->AddCircleFilled(worldBound1, AnchorBigRadius - 1.2f, bigAnchorColor);

            drawList->AddCircleFilled(midBound, AnchorSmallRadius, IM_COL32_BLACK);
            drawList->AddCircleFilled(midBound, AnchorSmallRadius - 1.2f, smallAnchorColor);
            int oppositeIndex = (i + 2) % 4;
            // big anchor on corners
            if (!gCurrentContext->mbUsingBounds && gCurrentContext->mbEnable && overBigAnchor && CanActivate())
            {
               gCurrentContext->mBoundsPivot.TransformPoint(aabb[(i + 2) % 4], gCurrentContext->mModelSource);
               gCurrentContext->mBoundsAnchor.TransformPoint(aabb[i], gCurrentContext->mModelSource);
               gCurrentContext->mBoundsPlan = BuildPlan(gCurrentContext->mBoundsAnchor, bestAxisWorldDirection);
               gCurrentContext->mBoundsBestAxis = bestAxis;
               gCurrentContext->mBoundsAxis[0] = secondAxis;
               gCurrentContext->mBoundsAxis[1] = thirdAxis;

               gCurrentContext->mBoundsLocalPivot.Set(0.f);
               gCurrentContext->mBoundsLocalPivot[secondAxis] = aabb[oppositeIndex][secondAxis];
               gCurrentContext->mBoundsLocalPivot[thirdAxis] = aabb[oppositeIndex][thirdAxis];

               gCurrentContext->mbUsingBounds = true;
               gCurrentContext->mEditingID = gCurrentContext->GetCurrentID();
               gCurrentContext->mBoundsMatrix = gCurrentContext->mModelSource;
            }
            // small anchor on middle of segment
            if (!gCurrentContext->mbUsingBounds && gCurrentContext->mbEnable && overSmallAnchor && CanActivate())
            {
               vec_t midPointOpposite = (aabb[(i + 2) % 4] + aabb[(i + 3) % 4]) * 0.5f;
               gCurrentContext->mBoundsPivot.TransformPoint(midPointOpposite, gCurrentContext->mModelSource);
               gCurrentContext->mBoundsAnchor.TransformPoint(midPoint, gCurrentContext->mModelSource);
               gCurrentContext->mBoundsPlan = BuildPlan(gCurrentContext->mBoundsAnchor, bestAxisWorldDirection);
               gCurrentContext->mBoundsBestAxis = bestAxis;
               int indices[] = { secondAxis , thirdAxis };
               gCurrentContext->mBoundsAxis[0] = indices[i % 2];
               gCurrentContext->mBoundsAxis[1] = -1;

               gCurrentContext->mBoundsLocalPivot.Set(0.f);
               gCurrentContext->mBoundsLocalPivot[gCurrentContext->mBoundsAxis[0]] = aabb[oppositeIndex][indices[i % 2]];// bounds[gCurrentContext->mBoundsAxis[0]] * (((i + 1) & 2) ? 1.f : -1.f);

               gCurrentContext->mbUsingBounds = true;
               gCurrentContext->mEditingID = gCurrentContext->GetCurrentID();
               gCurrentContext->mBoundsMatrix = gCurrentContext->mModelSource;
            }
         }

         if (gCurrentContext->mbUsingBounds && (gCurrentContext->GetCurrentID() == gCurrentContext->mEditingID))
         {
            matrix_t scale;
            scale.SetToIdentity();

            // compute projected mouse position on plan
            const float len = IntersectRayPlane(gCurrentContext->mRayOrigin, gCurrentContext->mRayVector, gCurrentContext->mBoundsPlan);
            vec_t newPos = gCurrentContext->mRayOrigin + gCurrentContext->mRayVector * len;

            // compute a reference and delta vectors base on mouse move
            vec_t deltaVector = (newPos - gCurrentContext->mBoundsPivot).Abs();
            vec_t referenceVector = (gCurrentContext->mBoundsAnchor - gCurrentContext->mBoundsPivot).Abs();

            // for 1 or 2 axes, compute a ratio that's used for scale and snap it based on resulting length
            for (int i = 0; i < 2; i++)
            {
               int axisIndex1 = gCurrentContext->mBoundsAxis[i];
               if (axisIndex1 == -1)
               {
                  continue;
               }

               float ratioAxis = 1.f;
               vec_t axisDir = gCurrentContext->mBoundsMatrix.component[axisIndex1].Abs();

               float dtAxis = axisDir.Dot(referenceVector);
               float boundSize = bounds[axisIndex1 + 3] - bounds[axisIndex1];
               if (dtAxis > FLT_EPSILON)
               {
                  ratioAxis = axisDir.Dot(deltaVector) / dtAxis;
               }

               if (snapValues)
               {
                  float length = boundSize * ratioAxis;
                  ComputeSnap(&length, snapValues[axisIndex1]);
                  if (boundSize > FLT_EPSILON)
                  {
                     ratioAxis = length / boundSize;
                  }
               }
               scale.component[axisIndex1] *= ratioAxis;
            }

            // transform matrix
            matrix_t preScale, postScale;
            preScale.Translation(-gCurrentContext->mBoundsLocalPivot);
            postScale.Translation(gCurrentContext->mBoundsLocalPivot);
            matrix_t res = preScale * scale * postScale * gCurrentContext->mBoundsMatrix;
            *matrix = res;

            // info text
            char tmps[512];
            ImVec2 destinationPosOnScreen = worldToPos(gCurrentContext->mModel.v.position, gCurrentContext->mViewProjection);
            ImFormatString(tmps, sizeof(tmps), "X: %.2f Y: %.2f Z: %.2f"
               , (bounds[3] - bounds[0]) * gCurrentContext->mBoundsMatrix.component[0].Length() * scale.component[0].Length()
               , (bounds[4] - bounds[1]) * gCurrentContext->mBoundsMatrix.component[1].Length() * scale.component[1].Length()
               , (bounds[5] - bounds[2]) * gCurrentContext->mBoundsMatrix.component[2].Length() * scale.component[2].Length()
            );
            drawList->AddText(ImVec2(destinationPosOnScreen.x + AnnotationOffset() + 1.0f, destinationPosOnScreen.y + AnnotationOffset()+1.0f), GetColorU32(TEXT_SHADOW), tmps);
            drawList->AddText(ImVec2(destinationPosOnScreen.x + AnnotationOffset(), destinationPosOnScreen.y + AnnotationOffset()), GetColorU32(TEXT), tmps);
         }

         if (!io.MouseDown[0]) {
            gCurrentContext->mbUsingBounds = false;
            gCurrentContext->mEditingID = blank_id();
         }
         if (gCurrentContext->mbUsingBounds)
         {
            break;
         }
      }
   }

   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   //

   int GetScaleType(OPERATION op)
   {
      if (gCurrentContext->mbUsing)
      {
         return MT_NONE;
      }
      ImGuiIO& io = ImGui::GetIO();
      int type = MT_NONE;

      // screen
      if (io.MousePos.x >= gCurrentContext->mScreenSquareMin.x && io.MousePos.x <= gCurrentContext->mScreenSquareMax.x &&
         io.MousePos.y >= gCurrentContext->mScreenSquareMin.y && io.MousePos.y <= gCurrentContext->mScreenSquareMax.y &&
         Contains(op, SCALE))
      {
         type = MT_SCALE_XYZ;
      }

      // compute
      for (int i = 0; i < 3 && type == MT_NONE; i++)
      {
         if(!Intersects(op, static_cast<OPERATION>(SCALE_X << i)))
         {
            continue;
         }
         bool isAxisMasked = (1 << i) & gCurrentContext->mAxisMask;

         vec_t dirPlaneX, dirPlaneY, dirAxis;
         bool belowAxisLimit, belowPlaneLimit;
         ComputeTripodAxisAndVisibility(i, dirAxis, dirPlaneX, dirPlaneY, belowAxisLimit, belowPlaneLimit, true);
         dirAxis.TransformVector(gCurrentContext->mModelLocal);
         dirPlaneX.TransformVector(gCurrentContext->mModelLocal);
         dirPlaneY.TransformVector(gCurrentContext->mModelLocal);

         const float len = IntersectRayPlane(gCurrentContext->mRayOrigin, gCurrentContext->mRayVector, BuildPlan(gCurrentContext->mModelLocal.v.position, dirAxis));
         vec_t posOnPlan = gCurrentContext->mRayOrigin + gCurrentContext->mRayVector * len;

         const float startOffset = Contains(op, static_cast<OPERATION>(TRANSLATE_X << i)) ? 1.0f : 0.1f;
         const float endOffset = Contains(op, static_cast<OPERATION>(TRANSLATE_X << i)) ? 1.4f : 1.0f;
         const ImVec2 posOnPlanScreen = worldToPos(posOnPlan, gCurrentContext->mViewProjection);
         const ImVec2 axisStartOnScreen = worldToPos(gCurrentContext->mModelLocal.v.position + dirAxis * gCurrentContext->mScreenFactor * startOffset, gCurrentContext->mViewProjection);
         const ImVec2 axisEndOnScreen = worldToPos(gCurrentContext->mModelLocal.v.position + dirAxis * gCurrentContext->mScreenFactor * endOffset, gCurrentContext->mViewProjection);

         vec_t closestPointOnAxis = PointOnSegment(makeVect(posOnPlanScreen), makeVect(axisStartOnScreen), makeVect(axisEndOnScreen));

         if ((closestPointOnAxis - makeVect(posOnPlanScreen)).Length() < 12.f) // pixel size
         {
            if (!isAxisMasked)
               type = MT_SCALE_X + i;
         }
      }

      // universal

      vec_t deltaScreen = { io.MousePos.x - gCurrentContext->mScreenSquareCenter.x, io.MousePos.y - gCurrentContext->mScreenSquareCenter.y, 0.f, 0.f };
      float dist = deltaScreen.Length();
      if (Contains(op, SCALEU) && dist >= 17.0f && dist < 23.0f)
      {
         type = MT_SCALE_XYZ;
      }

      for (int i = 0; i < 3 && type == MT_NONE; i++)
      {
         if (!Intersects(op, static_cast<OPERATION>(SCALE_XU << i)))
         {
            continue;
         }

         vec_t dirPlaneX, dirPlaneY, dirAxis;
         bool belowAxisLimit, belowPlaneLimit;
         ComputeTripodAxisAndVisibility(i, dirAxis, dirPlaneX, dirPlaneY, belowAxisLimit, belowPlaneLimit, true);

         // draw axis
         if (belowAxisLimit)
         {
            bool hasTranslateOnAxis = Contains(op, static_cast<OPERATION>(TRANSLATE_X << i));
            float markerScale = hasTranslateOnAxis ? 1.4f : 1.0f;
            //ImVec2 baseSSpace = worldToPos(dirAxis * 0.1f * gCurrentContext->mScreenFactor, gCurrentContext->mMVPLocal);
            //ImVec2 worldDirSSpaceNoScale = worldToPos(dirAxis * markerScale * gCurrentContext->mScreenFactor, gCurrentContext->mMVP);
            ImVec2 worldDirSSpace = worldToPos((dirAxis * markerScale) * gCurrentContext->mScreenFactor, gCurrentContext->mMVPLocal);

            float distance = sqrtf(ImLengthSqr(worldDirSSpace - io.MousePos));
            if (distance < 12.f)
            {
               type = MT_SCALE_X + i;
            }
         }
      }
      return type;
   }

   int GetRotateType(OPERATION op)
   {
      if (gCurrentContext->mbUsing)
      {
         return MT_NONE;
      }

      bool isNoAxesMasked = !gCurrentContext->mAxisMask;
      bool isMultipleAxesMasked = gCurrentContext->mAxisMask & (gCurrentContext->mAxisMask - 1);

      ImGuiIO& io = ImGui::GetIO();
      int type = MT_NONE;

      vec_t deltaScreen = { io.MousePos.x - gCurrentContext->mScreenSquareCenter.x, io.MousePos.y - gCurrentContext->mScreenSquareCenter.y, 0.f, 0.f };
      float dist = deltaScreen.Length();
      if (Intersects(op, ROTATE_SCREEN) && dist >= (gCurrentContext->mRadiusSquareCenter - 4.0f) && dist < (gCurrentContext->mRadiusSquareCenter + 4.0f))
      {
         if (!isNoAxesMasked)
            return MT_NONE;
         type = MT_ROTATE_SCREEN;
      }

      const vec_t planNormals[] = { gCurrentContext->mModel.v.right, gCurrentContext->mModel.v.up, gCurrentContext->mModel.v.dir };

      vec_t modelViewPos;
      modelViewPos.TransformPoint(gCurrentContext->mModel.v.position, gCurrentContext->mViewMat);

      for (int i = 0; i < 3 && type == MT_NONE; i++)
      {
         if(!Intersects(op, static_cast<OPERATION>(ROTATE_X << i)))
         {
            continue;
         }
         bool isAxisMasked = (1 << i) & gCurrentContext->mAxisMask;
         // pickup plan
         vec_t pickupPlan = BuildPlan(gCurrentContext->mModel.v.position, planNormals[i]);

         const float len = IntersectRayPlane(gCurrentContext->mRayOrigin, gCurrentContext->mRayVector, pickupPlan);
         const vec_t intersectWorldPos = gCurrentContext->mRayOrigin + gCurrentContext->mRayVector * len;
         vec_t intersectViewPos;
         intersectViewPos.TransformPoint(intersectWorldPos, gCurrentContext->mViewMat);

         if (ImAbs(modelViewPos.z) - ImAbs(intersectViewPos.z) < -FLT_EPSILON)
         {
            continue;
         }

         const vec_t localPos = intersectWorldPos - gCurrentContext->mModel.v.position;
         vec_t idealPosOnCircle = Normalized(localPos);
         idealPosOnCircle.TransformVector(gCurrentContext->mModelInverse);
         const ImVec2 idealPosOnCircleScreen = worldToPos(idealPosOnCircle * rotationDisplayFactor * gCurrentContext->mScreenFactor, gCurrentContext->mMVP);

         //gCurrentContext->mDrawList->AddCircle(idealPosOnCircleScreen, 5.f, IM_COL32_WHITE);
         const ImVec2 distanceOnScreen = idealPosOnCircleScreen - io.MousePos;

         const float distance = makeVect(distanceOnScreen).Length();
         if (distance < 8.f) // pixel size
         {
            if ((!isAxisMasked || isMultipleAxesMasked) && !isNoAxesMasked)
               break;
            type = MT_ROTATE_X + i;
         }
      }

      return type;
   }

   int GetMoveType(OPERATION op, vec_t* gizmoHitProportion)
   {
      if(!Intersects(op, TRANSLATE) || gCurrentContext->mbUsing || !gCurrentContext->mbMouseOver)
      {
        return MT_NONE;
      }

      bool isNoAxesMasked = !gCurrentContext->mAxisMask;
      bool isMultipleAxesMasked = gCurrentContext->mAxisMask & (gCurrentContext->mAxisMask - 1);

      ImGuiIO& io = ImGui::GetIO();
      int type = MT_NONE;

      // screen
      if (io.MousePos.x >= gCurrentContext->mScreenSquareMin.x && io.MousePos.x <= gCurrentContext->mScreenSquareMax.x &&
         io.MousePos.y >= gCurrentContext->mScreenSquareMin.y && io.MousePos.y <= gCurrentContext->mScreenSquareMax.y &&
         Contains(op, TRANSLATE))
      {
         type = MT_MOVE_SCREEN;
      }

      const vec_t screenCoord = makeVect(io.MousePos - ImVec2(gCurrentContext->mX, gCurrentContext->mY));

      // compute
      for (int i = 0; i < 3 && type == MT_NONE; i++)
      {
         bool isAxisMasked = (1 << i) & gCurrentContext->mAxisMask;
         vec_t dirPlaneX, dirPlaneY, dirAxis;
         bool belowAxisLimit, belowPlaneLimit;
         ComputeTripodAxisAndVisibility(i, dirAxis, dirPlaneX, dirPlaneY, belowAxisLimit, belowPlaneLimit);
         dirAxis.TransformVector(gCurrentContext->mModel);
         dirPlaneX.TransformVector(gCurrentContext->mModel);
         dirPlaneY.TransformVector(gCurrentContext->mModel);

         const float len = IntersectRayPlane(gCurrentContext->mRayOrigin, gCurrentContext->mRayVector, BuildPlan(gCurrentContext->mModel.v.position, dirAxis));
         vec_t posOnPlan = gCurrentContext->mRayOrigin + gCurrentContext->mRayVector * len;

         const ImVec2 axisStartOnScreen = worldToPos(gCurrentContext->mModel.v.position + dirAxis * gCurrentContext->mScreenFactor * 0.1f, gCurrentContext->mViewProjection) - ImVec2(gCurrentContext->mX, gCurrentContext->mY);
         const ImVec2 axisEndOnScreen = worldToPos(gCurrentContext->mModel.v.position + dirAxis * gCurrentContext->mScreenFactor, gCurrentContext->mViewProjection) - ImVec2(gCurrentContext->mX, gCurrentContext->mY);

         vec_t closestPointOnAxis = PointOnSegment(screenCoord, makeVect(axisStartOnScreen), makeVect(axisEndOnScreen));
         if ((closestPointOnAxis - screenCoord).Length() < 12.f && Intersects(op, static_cast<OPERATION>(TRANSLATE_X << i))) // pixel size
         {
            if (isAxisMasked)
               break;
            type = MT_MOVE_X + i;
         }

         const float dx = dirPlaneX.Dot3((posOnPlan - gCurrentContext->mModel.v.position) * (1.f / gCurrentContext->mScreenFactor));
         const float dy = dirPlaneY.Dot3((posOnPlan - gCurrentContext->mModel.v.position) * (1.f / gCurrentContext->mScreenFactor));
         if (belowPlaneLimit && dx >= quadUV[0] && dx <= quadUV[4] && dy >= quadUV[1] && dy <= quadUV[3] && Contains(op, TRANSLATE_PLANS[i]))
         {
            if ((!isAxisMasked || isMultipleAxesMasked) && !isNoAxesMasked)
               break;
            type = MT_MOVE_YZ + i;
         }

         if (gizmoHitProportion)
         {
            *gizmoHitProportion = makeVect(dx, dy, 0.f);
         }
      }
      return type;
   }

   bool HandleTranslation(float* matrix, float* deltaMatrix, OPERATION op, int& type, const float* snap)
   {
      if(!Intersects(op, TRANSLATE) || type != MT_NONE)
      {
        return false;
      }
      const ImGuiIO& io = ImGui::GetIO();
      const bool applyRotationLocaly = gCurrentContext->mMode == LOCAL || type == MT_MOVE_SCREEN;
      bool modified = false;

      // move
      if (gCurrentContext->mbUsing && (gCurrentContext->GetCurrentID() == gCurrentContext->mEditingID) && IsTranslateType(gCurrentContext->mCurrentOperation))
      {
#if IMGUI_VERSION_NUM >= 18723
         ImGui::SetNextFrameWantCaptureMouse(true);
#else
         ImGui::CaptureMouseFromApp();
#endif
         const float signedLength = IntersectRayPlane(gCurrentContext->mRayOrigin, gCurrentContext->mRayVector, gCurrentContext->mTranslationPlan);
         const float len = fabsf(signedLength); // near plan
         const vec_t newPos = gCurrentContext->mRayOrigin + gCurrentContext->mRayVector * len;

         // compute delta
         const vec_t newOrigin = newPos - gCurrentContext->mRelativeOrigin * gCurrentContext->mScreenFactor;
         vec_t delta = newOrigin - gCurrentContext->mModel.v.position;

         // 1 axis constraint
         if (gCurrentContext->mCurrentOperation >= MT_MOVE_X && gCurrentContext->mCurrentOperation <= MT_MOVE_Z)
         {
            const int axisIndex = gCurrentContext->mCurrentOperation - MT_MOVE_X;
            const vec_t& axisValue = *(vec_t*)&gCurrentContext->mModel.m[axisIndex];
            const float lengthOnAxis = Dot(axisValue, delta);
            delta = axisValue * lengthOnAxis;
         }

         // snap
         if (snap)
         {
            vec_t cumulativeDelta = gCurrentContext->mModel.v.position + delta - gCurrentContext->mMatrixOrigin;
            if (applyRotationLocaly)
            {
               matrix_t modelSourceNormalized = gCurrentContext->mModelSource;
               modelSourceNormalized.OrthoNormalize();
               matrix_t modelSourceNormalizedInverse;
               modelSourceNormalizedInverse.Inverse(modelSourceNormalized);
               cumulativeDelta.TransformVector(modelSourceNormalizedInverse);
               ComputeSnap(cumulativeDelta, snap);
               cumulativeDelta.TransformVector(modelSourceNormalized);
            }
            else
            {
               ComputeSnap(cumulativeDelta, snap);
            }
            delta = gCurrentContext->mMatrixOrigin + cumulativeDelta - gCurrentContext->mModel.v.position;

         }

         if (delta != gCurrentContext->mTranslationLastDelta)
         {
            modified = true;
         }
         gCurrentContext->mTranslationLastDelta = delta;

         // compute matrix & delta
         matrix_t deltaMatrixTranslation;
         deltaMatrixTranslation.Translation(delta);
         if (deltaMatrix)
         {
            memcpy(deltaMatrix, deltaMatrixTranslation.m16, sizeof(float) * 16);
         }

         const matrix_t res = gCurrentContext->mModelSource * deltaMatrixTranslation;
         *(matrix_t*)matrix = res;

         if (!io.MouseDown[0])
         {
            gCurrentContext->mbUsing = false;
         }

         type = gCurrentContext->mCurrentOperation;
      }
      else
      {
         // find new possible way to move
         vec_t gizmoHitProportion;
         type = gCurrentContext->mbOverGizmoHotspot ? MT_NONE : GetMoveType(op, &gizmoHitProportion);
         gCurrentContext->mbOverGizmoHotspot |= type != MT_NONE;
         if (type != MT_NONE)
         {
#if IMGUI_VERSION_NUM >= 18723
            ImGui::SetNextFrameWantCaptureMouse(true);
#else
            ImGui::CaptureMouseFromApp();
#endif
         }
         if (CanActivate() && type != MT_NONE)
         {
            gCurrentContext->mbUsing = true;
            gCurrentContext->mEditingID = gCurrentContext->GetCurrentID();
            gCurrentContext->mCurrentOperation = type;
            vec_t movePlanNormal[] = { gCurrentContext->mModel.v.right, gCurrentContext->mModel.v.up, gCurrentContext->mModel.v.dir,
               gCurrentContext->mModel.v.right, gCurrentContext->mModel.v.up, gCurrentContext->mModel.v.dir,
               -gCurrentContext->mCameraDir };

            vec_t cameraToModelNormalized = Normalized(gCurrentContext->mModel.v.position - gCurrentContext->mCameraEye);
            for (unsigned int i = 0; i < 3; i++)
            {
               vec_t orthoVector = Cross(movePlanNormal[i], cameraToModelNormalized);
               movePlanNormal[i].Cross(orthoVector);
               movePlanNormal[i].Normalize();
            }
            // pickup plan
            gCurrentContext->mTranslationPlan = BuildPlan(gCurrentContext->mModel.v.position, movePlanNormal[type - MT_MOVE_X]);
            const float len = IntersectRayPlane(gCurrentContext->mRayOrigin, gCurrentContext->mRayVector, gCurrentContext->mTranslationPlan);
            gCurrentContext->mTranslationPlanOrigin = gCurrentContext->mRayOrigin + gCurrentContext->mRayVector * len;
            gCurrentContext->mMatrixOrigin = gCurrentContext->mModel.v.position;

            gCurrentContext->mRelativeOrigin = (gCurrentContext->mTranslationPlanOrigin - gCurrentContext->mModel.v.position) * (1.f / gCurrentContext->mScreenFactor);
         }
      }
      return modified;
   }

   bool HandleScale(float* matrix, float* deltaMatrix, OPERATION op, int& type, const float* snap)
   {
      if((!Intersects(op, SCALE) && !Intersects(op, SCALEU)) || type != MT_NONE || !gCurrentContext->mbMouseOver)
      {
         return false;
      }
      ImGuiIO& io = ImGui::GetIO();
      bool modified = false;

      if (!gCurrentContext->mbUsing)
      {
         // find new possible way to scale
         type = gCurrentContext->mbOverGizmoHotspot ? MT_NONE : GetScaleType(op);
         gCurrentContext->mbOverGizmoHotspot |= type != MT_NONE;

         if (type != MT_NONE)
         {
#if IMGUI_VERSION_NUM >= 18723
            ImGui::SetNextFrameWantCaptureMouse(true);
#else
            ImGui::CaptureMouseFromApp();
#endif
         }
         if (CanActivate() && type != MT_NONE)
         {
            gCurrentContext->mbUsing = true;
            gCurrentContext->mEditingID = gCurrentContext->GetCurrentID();
            gCurrentContext->mCurrentOperation = type;
            const vec_t movePlanNormal[] = { gCurrentContext->mModelLocal.v.up, gCurrentContext->mModelLocal.v.dir, gCurrentContext->mModelLocal.v.right, gCurrentContext->mModelLocal.v.dir, gCurrentContext->mModelLocal.v.up, gCurrentContext->mModelLocal.v.right, -gCurrentContext->mCameraDir };
            // pickup plan

            gCurrentContext->mTranslationPlan = BuildPlan(gCurrentContext->mModelLocal.v.position, movePlanNormal[type - MT_SCALE_X]);
            const float len = IntersectRayPlane(gCurrentContext->mRayOrigin, gCurrentContext->mRayVector, gCurrentContext->mTranslationPlan);
            gCurrentContext->mTranslationPlanOrigin = gCurrentContext->mRayOrigin + gCurrentContext->mRayVector * len;
            gCurrentContext->mMatrixOrigin = gCurrentContext->mModelLocal.v.position;
            gCurrentContext->mScale.Set(1.f, 1.f, 1.f);
            gCurrentContext->mRelativeOrigin = (gCurrentContext->mTranslationPlanOrigin - gCurrentContext->mModelLocal.v.position) * (1.f / gCurrentContext->mScreenFactor);
            gCurrentContext->mScaleValueOrigin = makeVect(gCurrentContext->mModelSource.v.right.Length(), gCurrentContext->mModelSource.v.up.Length(), gCurrentContext->mModelSource.v.dir.Length());
            gCurrentContext->mSaveMousePosx = io.MousePos.x;
         }
      }
      // scale
      if (gCurrentContext->mbUsing && (gCurrentContext->GetCurrentID() == gCurrentContext->mEditingID) && IsScaleType(gCurrentContext->mCurrentOperation))
      {
#if IMGUI_VERSION_NUM >= 18723
         ImGui::SetNextFrameWantCaptureMouse(true);
#else
         ImGui::CaptureMouseFromApp();
#endif
         const float len = IntersectRayPlane(gCurrentContext->mRayOrigin, gCurrentContext->mRayVector, gCurrentContext->mTranslationPlan);
         vec_t newPos = gCurrentContext->mRayOrigin + gCurrentContext->mRayVector * len;
         vec_t newOrigin = newPos - gCurrentContext->mRelativeOrigin * gCurrentContext->mScreenFactor;
         vec_t delta = newOrigin - gCurrentContext->mModelLocal.v.position;

         // 1 axis constraint
         if (gCurrentContext->mCurrentOperation >= MT_SCALE_X && gCurrentContext->mCurrentOperation <= MT_SCALE_Z)
         {
            int axisIndex = gCurrentContext->mCurrentOperation - MT_SCALE_X;
            const vec_t& axisValue = *(vec_t*)&gCurrentContext->mModelLocal.m[axisIndex];
            float lengthOnAxis = Dot(axisValue, delta);
            delta = axisValue * lengthOnAxis;

            vec_t baseVector = gCurrentContext->mTranslationPlanOrigin - gCurrentContext->mModelLocal.v.position;
            float ratio = Dot(axisValue, baseVector + delta) / Dot(axisValue, baseVector);

            gCurrentContext->mScale[axisIndex] = max(ratio, 0.001f);
         }
         else
         {
            float scaleDelta = (io.MousePos.x - gCurrentContext->mSaveMousePosx) * 0.01f;
            gCurrentContext->mScale.Set(max(1.f + scaleDelta, 0.001f));
         }

         // snap
         if (snap)
         {
            float scaleSnap[] = { snap[0], snap[0], snap[0] };
            ComputeSnap(gCurrentContext->mScale, scaleSnap);
         }

         // no 0 allowed
         for (int i = 0; i < 3; i++)
            gCurrentContext->mScale[i] = max(gCurrentContext->mScale[i], 0.001f);

         if (gCurrentContext->mScaleLast != gCurrentContext->mScale)
         {
            modified = true;
         }
         gCurrentContext->mScaleLast = gCurrentContext->mScale;

         // compute matrix & delta
         matrix_t deltaMatrixScale;
         deltaMatrixScale.Scale(gCurrentContext->mScale * gCurrentContext->mScaleValueOrigin);

         matrix_t res = deltaMatrixScale * gCurrentContext->mModelLocal;
         *(matrix_t*)matrix = res;

         if (deltaMatrix)
         {
            vec_t deltaScale = gCurrentContext->mScale * gCurrentContext->mScaleValueOrigin;

            vec_t originalScaleDivider;
            originalScaleDivider.x = 1 / gCurrentContext->mModelScaleOrigin.x;
            originalScaleDivider.y = 1 / gCurrentContext->mModelScaleOrigin.y;
            originalScaleDivider.z = 1 / gCurrentContext->mModelScaleOrigin.z;

            deltaScale = deltaScale * originalScaleDivider;

            deltaMatrixScale.Scale(deltaScale);
            memcpy(deltaMatrix, deltaMatrixScale.m16, sizeof(float) * 16);
         }

         if (!io.MouseDown[0])
         {
            gCurrentContext->mbUsing = false;
            gCurrentContext->mScale.Set(1.f, 1.f, 1.f);
         }

         type = gCurrentContext->mCurrentOperation;
      }
      return modified;
   }

   bool HandleRotation(float* matrix, float* deltaMatrix, OPERATION op, int& type, const float* snap)
   {
      if(!Intersects(op, ROTATE) || type != MT_NONE || !gCurrentContext->mbMouseOver)
      {
        return false;
      }
      ImGuiIO& io = ImGui::GetIO();
      bool applyRotationLocaly = gCurrentContext->mMode == LOCAL;
      bool modified = false;

      if (!gCurrentContext->mbUsing)
      {
         type = gCurrentContext->mbOverGizmoHotspot ? MT_NONE : GetRotateType(op);
         gCurrentContext->mbOverGizmoHotspot |= type != MT_NONE;

         if (type != MT_NONE)
         {
#if IMGUI_VERSION_NUM >= 18723
            ImGui::SetNextFrameWantCaptureMouse(true);
#else
            ImGui::CaptureMouseFromApp();
#endif
         }

         if (type == MT_ROTATE_SCREEN)
         {
            applyRotationLocaly = true;
         }

         if (CanActivate() && type != MT_NONE)
         {
            gCurrentContext->mbUsing = true;
            gCurrentContext->mEditingID = gCurrentContext->GetCurrentID();
            gCurrentContext->mCurrentOperation = type;
            const vec_t rotatePlanNormal[] = { gCurrentContext->mModel.v.right, gCurrentContext->mModel.v.up, gCurrentContext->mModel.v.dir, -gCurrentContext->mCameraDir };
            // pickup plan
            if (applyRotationLocaly)
            {
               gCurrentContext->mTranslationPlan = BuildPlan(gCurrentContext->mModel.v.position, rotatePlanNormal[type - MT_ROTATE_X]);
            }
            else
            {
               gCurrentContext->mTranslationPlan = BuildPlan(gCurrentContext->mModelSource.v.position, directionUnary[type - MT_ROTATE_X]);
            }

            const float len = IntersectRayPlane(gCurrentContext->mRayOrigin, gCurrentContext->mRayVector, gCurrentContext->mTranslationPlan);
            vec_t localPos = gCurrentContext->mRayOrigin + gCurrentContext->mRayVector * len - gCurrentContext->mModel.v.position;
            gCurrentContext->mRotationVectorSource = Normalized(localPos);
            gCurrentContext->mRotationAngleOrigin = ComputeAngleOnPlan();
         }
      }

      // rotation
      if (gCurrentContext->mbUsing && (gCurrentContext->GetCurrentID() == gCurrentContext->mEditingID) && IsRotateType(gCurrentContext->mCurrentOperation))
      {
#if IMGUI_VERSION_NUM >= 18723
         ImGui::SetNextFrameWantCaptureMouse(true);
#else
         ImGui::CaptureMouseFromApp();
#endif
         gCurrentContext->mRotationAngle = ComputeAngleOnPlan();
         if (snap)
         {
            float snapInRadian = snap[0] * DEG2RAD;
            ComputeSnap(&gCurrentContext->mRotationAngle, snapInRadian);
         }
         vec_t rotationAxisLocalSpace;

         rotationAxisLocalSpace.TransformVector(makeVect(gCurrentContext->mTranslationPlan.x, gCurrentContext->mTranslationPlan.y, gCurrentContext->mTranslationPlan.z, 0.f), gCurrentContext->mModelInverse);
         rotationAxisLocalSpace.Normalize();

         matrix_t deltaRotation;
         deltaRotation.RotationAxis(rotationAxisLocalSpace, gCurrentContext->mRotationAngle - gCurrentContext->mRotationAngleOrigin);
         if (gCurrentContext->mRotationAngle != gCurrentContext->mRotationAngleOrigin)
         {
            modified = true;
         }
         gCurrentContext->mRotationAngleOrigin = gCurrentContext->mRotationAngle;

         matrix_t scaleOrigin;
         scaleOrigin.Scale(gCurrentContext->mModelScaleOrigin);

         if (applyRotationLocaly)
         {
            *(matrix_t*)matrix = scaleOrigin * deltaRotation * gCurrentContext->mModelLocal;
         }
         else
         {
            matrix_t res = gCurrentContext->mModelSource;
            res.v.position.Set(0.f);

            *(matrix_t*)matrix = res * deltaRotation;
            ((matrix_t*)matrix)->v.position = gCurrentContext->mModelSource.v.position;
         }

         if (deltaMatrix)
         {
            *(matrix_t*)deltaMatrix = gCurrentContext->mModelInverse * deltaRotation * gCurrentContext->mModel;
         }

         if (!io.MouseDown[0])
         {
            gCurrentContext->mbUsing = false;
            gCurrentContext->mEditingID = blank_id();
         }
         type = gCurrentContext->mCurrentOperation;
      }
      return modified;
   }

   ImGuiID GetID(int n)
   {
       ImGuiID seed = gCurrentContext->mIDStack.back();
       ImGuiID id = ImHashData(&n, sizeof(n), seed);
       return id;
   }
}

void ImGuizmo::CreateContext()
{
    gCurrentContext = IM_NEW(Context)();
}

void ImGuizmo::DestroyContext()
{
    IM_DELETE(gCurrentContext);
}

void ImGuizmo::SetDrawlist(ImDrawList* drawlist)
{
    gCurrentContext->mDrawList = drawlist ? drawlist : ImGui::GetWindowDrawList();
}

void ImGuizmo::BeginFrame()
{
    const ImU32 flags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoInputs |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoBringToFrontOnFocus;

#ifdef IMGUI_HAS_VIEWPORT
    ImGui::SetNextWindowSize(ImGui::GetMainViewport()->Size);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->Pos);
#else
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::SetNextWindowPos(ImVec2(0, 0));
#endif

    ImGui::PushStyleColor(ImGuiCol_WindowBg, 0);
    ImGui::PushStyleColor(ImGuiCol_Border, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

    ImGui::Begin("gizmo", NULL, flags);
    gCurrentContext->mDrawList = ImGui::GetWindowDrawList();
    gCurrentContext->mbOverGizmoHotspot = false;
    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
}

bool ImGuizmo::IsOver()
{
    return
        (Intersects(gCurrentContext->mOperation, TRANSLATE) and GetMoveType(gCurrentContext->mOperation, NULL) != MT_NONE) or
        (Intersects(gCurrentContext->mOperation, ROTATE)    and GetRotateType(gCurrentContext->mOperation) != MT_NONE)     or
        (Intersects(gCurrentContext->mOperation, SCALE)     and GetScaleType(gCurrentContext->mOperation) != MT_NONE)      or
        IsUsing();
}

bool ImGuizmo::IsOver(OPERATION op)
{
    if (IsUsing()) {
        return true;
    }
    if (Intersects(op, SCALE) and GetScaleType(op) != MT_NONE) {
        return true;
    }
    if (Intersects(op, ROTATE) and GetRotateType(op) != MT_NONE) {
        return true;
    }
    if (Intersects(op, TRANSLATE) and GetMoveType(op, NULL) != MT_NONE) {
        return true;
    }
    return false;
}

bool ImGuizmo::IsUsing()
{
    return (gCurrentContext->mbUsing and (gCurrentContext->GetCurrentID() == gCurrentContext->mEditingID)) or gCurrentContext->mbUsingBounds;
}

bool ImGuizmo::IsUsingAny()
{
    return gCurrentContext->mbUsing or gCurrentContext->mbUsingBounds;
}

void ImGuizmo::Enable(bool enable)
{
    gCurrentContext->mbEnable = enable;
    if (not enable) {
        gCurrentContext->mbUsing = false;
        gCurrentContext->mbUsingBounds = false;
    }
}

void ImGuizmo::SetRect(float x, float y, float width, float height)
{
    gCurrentContext->mX = x;
    gCurrentContext->mY = y;
    gCurrentContext->mWidth = width;
    gCurrentContext->mHeight = height;
    gCurrentContext->mXMax = gCurrentContext->mX + gCurrentContext->mWidth;
    gCurrentContext->mYMax = gCurrentContext->mY + gCurrentContext->mXMax;
    gCurrentContext->mDisplayRatio = width / height;
}

void ImGuizmo::SetOrthographic(bool isOrthographic)
{
    gCurrentContext->mIsOrthographic = isOrthographic;
}

bool ImGuizmo::Manipulate(
    const float* view,
    const float* projection,
    OPERATION operation,
    MODE mode,
    float* matrix,
    float* deltaMatrix,
    const float* snap,
    const float* localBounds,
    const float* boundsSnap)
{
    gCurrentContext->mDrawList->PushClipRect(
        ImVec2(gCurrentContext->mX, gCurrentContext->mY),
        ImVec2(gCurrentContext->mX + gCurrentContext->mWidth, gCurrentContext->mY + gCurrentContext->mHeight),
        false
    );

    // Scale is always local or matrix will be skewed when applying world scale or oriented matrix
    ComputeContext(view, projection, matrix, (operation & SCALE) ? LOCAL : mode);

    // set delta to identity
    if (deltaMatrix) {
        ((matrix_t*)deltaMatrix)->SetToIdentity();
    }

    // behind camera
    vec_t camSpacePosition;
    camSpacePosition.TransformPoint(makeVect(0.f, 0.f, 0.f), gCurrentContext->mMVP);
    if (not gCurrentContext->mIsOrthographic and camSpacePosition.z < 0.0001f and not gCurrentContext->mbUsing) {
        return false;
    }

    // --
    int type = MT_NONE;
    bool manipulated = false;
    if (gCurrentContext->mbEnable) {
        if (not gCurrentContext->mbUsingBounds) {
            manipulated = HandleTranslation(matrix, deltaMatrix, operation, type, snap) ||
                HandleScale(matrix, deltaMatrix, operation, type, snap) ||
                HandleRotation(matrix, deltaMatrix, operation, type, snap);
        }
    }

    if (localBounds and not gCurrentContext->mbUsing) {
        HandleAndDrawLocalBounds(localBounds, (matrix_t*)matrix, boundsSnap, operation);
    }

    gCurrentContext->mOperation = operation;
    if (not gCurrentContext->mbUsingBounds) {
        DrawRotationGizmo(operation, type);
        DrawTranslationGizmo(operation, type);
        DrawScaleGizmo(operation, type);
        DrawScaleUniveralGizmo(operation, type);
    }

    gCurrentContext->mDrawList->PopClipRect ();
    return manipulated;
}

void ImGuizmo::PushID(osc::UID uid)
{
    ImGuiID id = ::GetID(static_cast<int>(std::hash<osc::UID>{}(uid)));
    gCurrentContext->mIDStack.push_back(id);
}

void ImGuizmo::PopID()
{
    IM_ASSERT(gCurrentContext->mIDStack.Size > 1); // Too many PopID(), or could be popping in a wrong/different window?
    gCurrentContext->mIDStack.pop_back();
}

void ImGuizmo::SetGizmoSizeClipSpace(float value)
{
    gCurrentContext->mGizmoSizeClipSpace = value;
}

void ImGuizmo::SetAxisLimit(float value)
{
    gCurrentContext->mAxisLimit=value;
}

void ImGuizmo::SetAxisMask(bool x, bool y, bool z)
{
    gCurrentContext->mAxisMask = (x ? 1 : 0) + (y ? 2 : 0) + (z ? 4 : 0);
}

void ImGuizmo::SetPlaneLimit(float value)
{
    gCurrentContext->mPlaneLimit = value;
}

// NOLINTEND
