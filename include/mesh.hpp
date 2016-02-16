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
  attribute(const std::string &name="", size_t num_elems=3, size_t element_size=4, bool is_float=true, bool is_normalized=false)
  : name_(name), num_elems_(num_elems), element_size_(element_size), is_float_(is_float), is_normalized_(is_normalized) {
  }
  
  std::string name() { return name_; }
  std::vector<std::uint8_t> &data() { return data_; }
  size_t size() { return data_.size(); }
  size_t count() { return data_.size() / (num_elems_ * element_size_); }

  template <class Vec>
  void push(Vec v, size_t size = Vec::size()) {
    union { std::uint8_t u[4]; float f; } u;
    for (size_t i = 0; i != size; ++i) {
      u.f = v[i];
      data_.push_back(u.u[0]);
      data_.push_back(u.u[1]);
      data_.push_back(u.u[2]);
      data_.push_back(u.u[3]);
    }
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
        chunk<Iter> at2(p, "atf");
        for (auto c : data_) *p++ = c;
      }
    }
    return p;
  }
  
  size_t size() const { return data_.size() / (num_elems_ * element_size_); }
private:
  std::string name_;
  size_t num_elems_;
  size_t element_size_;
  bool is_float_;
  bool is_normalized_;
  std::vector<std::uint8_t> data_;
};

/// single component mesh
class mesh : public std::vector<attribute>, public serial {
public:
  mesh(const std::string &name="mesh", size_t index_size=4)
  : name_(name) {
  }
  
  size_t add_attribute(const std::string &name="", size_t num_elems=3, size_t element_size=4, bool is_float=true, bool is_normalized=false) {
    size_t res = size();
    emplace_back(name, num_elems, element_size, is_float, is_normalized);
    return res;
  }

  size_t find_attribute(const std::string &name) {
    for (size_t i = 0; i != size(); ++i) {
      if ((*this)[i].name() == name) return i;
    }
    return size();
  }

  template <class Iter>
  Iter write_binary(Iter p) const {
    {
      chunk<Iter> MSH(p, "MSH");

      {
        chunk<Iter> msh(p, "msh");
        p = wrtxt(p, name_.c_str());
      }

      for (auto &a : *this) {
        p = a.write_binary(p);
      }

      {
        if (1 || index_size_ == 2) {
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
    if (find_attribute("normal") != size()) return;
    
    size_t normal_attr = add_attribute("normal");
    size_t pos_attr = find_attribute("pos");

    attribute &pos = (*this)[pos_attr];
    
    std::vector<vec3> normals(pos.count());
    std::fill(normals.begin(), normals.end(), vec3(0));
    size_t icount = indices_.size();
    vec3 *posp = (vec3*)pos.data().data();
    for (size_t i = 0; i + 2 < icount; i += 3) {
      size_t ai = indices_[i];
      size_t bi = indices_[i+1];
      size_t ci = indices_[i+2];
      vec3 a = posp[ai];
      vec3 b = posp[bi];
      vec3 c = posp[ci];
      vec3 normal = cross((b-a), (c-a));
      //std::cout << a << " " << b << " " << c << " " << normal << "\n";
      normals[ai] += normal;
      normals[bi] += normal;
      normals[ci] += normal;
    }

    attribute &normal = (*this)[normal_attr];
    for (auto &n : normals) {
      std::cout << normalized(n) << "\n";
      if (dot(n, n) >= 1.0e-6f) {
        normal.push(normalized(n));
      } else {
        normal.push(vec3(1, 0, 0));
      }
    }
  }
  
  void push_index(std::uint32_t i) { indices_.push_back(i); }
  size_t index_count() const { return indices_.size(); }
  const std::uint32_t *indices() const { return indices_.data(); }
private:
  std::string name_;
  size_t index_size_;
  std::vector<std::uint32_t> indices_;
};

/// multi-component mesh
class multi_mesh : public std::vector<mesh>, public serial {
public:
  multi_mesh() {
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
};


}

#endif
