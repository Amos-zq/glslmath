////////////////////////////////////////////////////////////////////////////////
//
// GLSL-style math library
//
// (C) Andy Thomason 2016 (MIT License)
//
////////////////////////////////////////////////////////////////////////////////

#ifndef INCLUDED_GLSLMATH_MESH
#define INCLUDED_GLSLMATH_MESH

#include <algorithm>
#include <cstdint>
#include <sstream>
#include "math.hpp"

namespace glslmath {

// sizer class for serialization  
class sizer {
public:
  typedef std::uint8_t value_type;

  sizer() : size_(0) {
  }
  
  size_t size() const { return size_; }
  
  std::uint8_t &operator[](size_t i) { return sink; }
  std::uint8_t &operator*() { return sink; }
  sizer &operator++(int) { size_++; return *this; }
  sizer &operator++() { size_++; return *this; }
  std::ptrdiff_t operator-(const sizer &rhs) { return size_ - rhs.size_; }
private:
  size_t size_;
  std::uint8_t sink;
};

struct serial {
  template <class Iter>
  static Iter wr32(Iter p, size_t value) {
    *p++ = value & 0xff;
    *p++ = (value >> 8) & 0xff;
    *p++ = (value >> 16) & 0xff;
    *p++ = (value >> 24) & 0xff;
    return p;
  }
  
  template <class Iter>
  static Iter wr16(Iter p, size_t value) {
    *p++ = value & 0xff;
    *p++ = (value >> 8) & 0xff;
    return p;
  }
  
  template <class Iter>
  static Iter wrtxt(Iter p, const char *text) {
    Iter b = p;
    while (*text) *p++ = *text++;
    *p++ = 0;
    p = align4(p, b);
    return p;
  }
  
  template <class Iter>
  static Iter wr(Iter p, const char *text) {
    while (*text) *p++ = *text++;
  }

  template <class Iter>
  static Iter wr(Iter p, int value) {
    char buf[32];
    sprintf(buf, "%d", value);
    return wr(p, buf);
  }

  template <class Iter>
  static Iter wr(Iter p, float value) {
    char buf[32];
    sprintf(buf, "%g", value);
    return wr(p, buf);
  }

  template <class Iter>
  static Iter align4(Iter p, Iter ref) {
    while ((p - ref) & 3) *p++ = 0;
    return p;
  }
  
  template <class Iter>
  struct chunk {
    Iter len;
    Iter &p;

    chunk(Iter &p, const char *tag) : p(p) {
      p = wrtxt(p, tag);
      len = p;
      p = wr32(p, 0);
    }
    
    ~chunk() {
      wr32(len, (int)(p - len));
      p = align4(p, len);
    }
  };
};

class attribute : public serial {
public:
  attribute(const std::string &name="", size_t num_elems=3, size_t element_size=4, bool is_float=true, bool is_unsigned=false, bool is_normalized=false)
  : name_(name), vector_elems_(num_elems), scalar_size_(element_size), is_float_(is_float), is_unsigned_(is_unsigned), is_normalized_(is_normalized) {
  }
  
  const std::string &name() const { return name_; }

  std::vector<vec4> &data() { return vertices_; }
  const std::vector<vec4> &data() const { return vertices_; }
  
  vec4 operator[](size_t i) const { return vertices_[i]; }
  size_t vertex_count() const { return vertices_.size(); }
  
  void resize(size_t size) { vertices_.resize(size * vertex_size()); }
  
  size_t vector_elems() const { return vector_elems_; }
  size_t vertex_size() const { return vector_elems_ * scalar_size_; }
  size_t scalar_size() const { return scalar_size_; }
  
  bool is_float() const { return is_float_; }
  bool is_normalized() const { return is_normalized_; }
  bool is_unsigned() const { return is_unsigned_; }

  // clone another attribute except for the data.
  void copy_params(const attribute &rhs) {
    name_ = rhs.name();
    vector_elems_ = rhs.vector_elems_;
    scalar_size_ = rhs.scalar_size_;
    is_float_ = rhs.is_float_;
    is_unsigned_ = rhs.is_unsigned_;
    is_normalized_ = rhs.is_normalized_;
  }

  void push(const vec4 &v) {
    vertices_.push_back(v);
  }

  void push(const vec3 &v) {
    vertices_.push_back(vec4(v, 1));
  }

  void push(const vec2 &v) {
    vertices_.push_back(vec4(v, 0, 1));
  }

  template <class Iter>
  Iter write_binary(Iter p) const {
    {
      chunk<Iter> ATR(p, "ATR");
      {
        chunk<Iter> at1(p, "atn");
        p = wrtxt(p, name_.c_str());
      }
      {
        chunk<Iter> atf(p, "a3f");
        union { std::uint8_t u[4]; float f; } u;
        for (auto v : vertices_) {
          u.f = v[0];
          *p++ = u.u[0];
          *p++ = u.u[1];
          *p++ = u.u[2];
          *p++ = u.u[3];
          u.f = v[1];
          *p++ = u.u[0];
          *p++ = u.u[1];
          *p++ = u.u[2];
          *p++ = u.u[3];
          u.f = v[2];
          *p++ = u.u[0];
          *p++ = u.u[1];
          *p++ = u.u[2];
          *p++ = u.u[3];
        }
      }
    }
    return p;
  }
  
private:
  // name of the mesh.
  std::string name_;
  
  // number of elements in the vector
  size_t vector_elems_;
  
  // number of bytes in the scalar
  size_t scalar_size_;

  // scalar is float 16, 32, 64
  bool is_float_;
  
  // integer scalar is unsigned
  bool is_unsigned_;

  // integer scalar represents a floating point number.
  bool is_normalized_;

  // the data itself.
  std::vector<vec4> vertices_;
};

/// single component mesh
class mesh : public serial {
public:
  typedef std::uint32_t index_type;
  
  static const size_t bad_attr = ~(size_t)0;

  mesh(const std::string &name="mesh", size_t index_size=4)
  : name_(name), index_size_(index_size) {
  }
  
  size_t add_attribute(const std::string &name="", size_t num_elems=3, size_t element_size=4, bool is_float=true, bool is_unsigned=false, bool is_normalized=false) {
    size_t res = attrs_.size();
    attrs_.emplace_back(name, num_elems, element_size, is_float, is_unsigned, is_normalized);
    return res;
  }

  size_t find_attribute(const std::string &name) const {
    for (size_t i = 0; i != attrs_.size(); ++i) {
      if (attrs_[i].name() == name) return i;
    }
    return bad_attr;
  }

  template <class Iter>
  Iter write_binary(Iter p) const {
    {
      chunk<Iter> MSH(p, "MSH");

      {
        chunk<Iter> msh(p, "msh");
        p = wrtxt(p, name_.c_str());
      }

      for (auto &a : attrs_) {
        p = a.write_binary(p);
      }

      {
        if (index_size_ == 2) {
          chunk<Iter> ix2(p, "ix2");
          for (auto idx : indices_) {
            p = wr16(p, idx);
          }
        } else {
          chunk<Iter> ix2(p, "ix4");
          for (auto idx : indices_) {
            p = wr32(p, idx);
          }
        }
      }
    }

    return p;
  }

  // generate normals for this mesh.
  void generate_normals() {
    if (find_attribute("normal") != attributes().size()) return;
    
    size_t normal_attr = add_attribute("normal");
    size_t pos_attr = find_attribute("pos");

    attribute &pos = attrs_[pos_attr];
    
    std::vector<vec3> normals(pos.vertex_count());
    std::fill(normals.begin(), normals.end(), vec3(0));
    size_t icount = indices_.size();
    for (size_t i = 0; i + 2 < icount; i += 3) {
      size_t ai = indices_[i];
      size_t bi = indices_[i+1];
      size_t ci = indices_[i+2];
      vec3 a = pos[ai].xyz();
      vec3 b = pos[bi].xyz();
      vec3 c = pos[ci].xyz();
      vec3 normal = cross((b-a), (c-a));
      //std::cout << a << " " << b << " " << c << " " << normal << "\n";
      normals[ai] += normal;
      normals[bi] += normal;
      normals[ci] += normal;
    }

    attribute &normal = attrs_[normal_attr];
    for (auto &n : normals) {
      if (dot(n, n) >= 1.0e-6f) {
        normal.push(normalized(n));
      } else {
        normal.push(vec3(1, 0, 0));
      }
    }
  }
  
  void write_obj(std::ostream &os) const {
    size_t pos_attr = find_attribute("pos");
    size_t uv_attr = find_attribute("uv");
    size_t normal_attr = find_attribute("normal");

    os << "o " << name() << "\n";
    
    if (pos_attr == bad_attr) return;

    {
      const attribute &attr = attributes()[pos_attr];
      if (attr.vertex_size() != 3 || attr.scalar_size() != 4 || !attr.is_float()) throw(std::range_error("write_obj: wrong pos attr"));
      size_t size = attr.vertex_count();
      for (size_t i = 0; i != size; ++i) {
        vec4 v = attr[i];
        os << "v " << v[0] << " " << v[1] << " " << v[2] << "\n";
      }
    }
    
    if (uv_attr != bad_attr) {
    }

    if (normal_attr != bad_attr) {
    }

    size_t icount = indices_.size();
    for (size_t i = 0; i + 2 < icount; i += 3) {
      index_type ai = indices_[i];
      index_type bi = indices_[i+1];
      index_type ci = indices_[i+2];
      if (uv_attr == bad_attr && normal_attr == bad_attr) {
        os << "f " << ai << " " << bi << " " << ci << "\n";
      } else if (uv_attr != bad_attr && normal_attr == bad_attr) {
        os << "f " << ai << "/" << ai << " " << bi << "/" << bi << " " << ci << "/" << ci << "\n";
      } else if (uv_attr == bad_attr && normal_attr != bad_attr) {
        os << "f " << ai << "//" << ai << " " << bi << "//" << bi << " " << ci << "//" << ci << "\n";
      } else if (uv_attr != bad_attr && normal_attr != bad_attr) {
        os << "f " << ai << "/" << ai << "/" << ai << " " << bi << "/" << bi << "/" << bi << " " << ci << "/" << ci << "/" << ci << "\n";
      }
    }
  }

  void push_index(index_type i) { indices_.push_back(i); }

  std::vector<attribute> &attributes() { return attrs_; }
  std::vector<index_type> &indices()  { return indices_; }
  const std::vector<attribute> &attributes() const { return attrs_; }
  const std::vector<index_type> &indices() const { return indices_; }
  const std::string &name() const { return name_; }
  
private:
  std::string name_;
  std::vector<attribute> attrs_;
  std::vector<index_type> indices_;
  size_t index_size_;
};

/// multi-component mesh
class multi_mesh : public std::vector<mesh>, public serial {
public:
  typedef mesh::index_type index_type;

  // Make a blank multi mesh.
  multi_mesh() {
  }
  
  /// Make a mesh with a single submesh
  multi_mesh(const mesh &src) {
    push_back(src);
  }

  /// Split the mesh into smaller vertex chunks.  
  static multi_mesh split(const mesh &src, size_t max_size = 65500, size_t new_index_size=2) {
    multi_mesh dest;
    bool is_small = true;
    for (auto i : src.indices()) {
      if (i >= max_size) {
        is_small = false;
        break;
      }
    }

    if (is_small) {
      dest.push_back(src);
    } else {
      auto &indices = src.indices();
      std::vector<index_type> fwd(indices.size());
      std::vector<index_type> rev;
      rev.reserve(max_size + 2);
      size_t t = 0;
      int mesh_number = 0;
      for (size_t i = 0; i != indices.size(); ++i) {
        if (fwd[i] == 0) {
          rev.push_back(i);
          fwd[i] = (index_type)rev.size();
        }
        if (++t == 3 || i == indices.size()-1) {
          t = 0;
          if (i == indices.size()-1 || rev.size() >= max_size) {
            std::stringstream ns;
            ns << src.name() << "." << mesh_number++;
            mesh submesh(ns.str(), new_index_size);
            
            for (auto &oldattr : src.attributes()) {
              attribute newattr;
              newattr.copy_params(oldattr);
              for (size_t j = 0; j != rev.size(); ++j) {
                newattr[j] = oldattr[rev[j]];
              }
            }
            submesh.indices() = rev;
            rev.resize(0);
            std::fill(fwd.begin(), fwd.end(), 0);
          }
        }
      }
    }
    
    return dest;
  }

  template <class Iter>
  Iter write_binary(Iter p) const {
    {
      chunk<Iter> MLT(p, "MLT");
      for (auto &m : *this) {
        p = m.write_binary(p);
      }
    }
    return p;
  }

  void write_obj(std::ostream &os) const {
    for (auto &m : *this) {
      m.write_obj(os);
    }
  }
};


}

#endif
