////////////////////////////////////////////////////////////////////////////////
//
// GLSL-style math library
//
// (C) Andy Thomason 2016 (MIT License)
//
////////////////////////////////////////////////////////////////////////////////

#ifndef INCLUDED_GLSLMATH_MATH
#define INCLUDED_GLSLMATH_MATH

#include <ostream>
#include <cmath>
#include <stdio.h>


namespace glslmath {
    template <class Impl, class Scalar, size_t N> class basic_vec {
        static_assert(N >= 2 && N <= 4, "expected N in range 1..4");
    public:
        typedef basic_vec<Impl, Scalar, N> base_t;
        typedef Scalar scalar_t;
        typedef Impl impl_t;
        static constexpr size_t size() { return N; }
        
        basic_vec() {
        }

        basic_vec(Scalar a) {
            for (size_t i = 0; i != N; ++i) {
                impl[i] = a;
            }
        }
        
        Scalar x() const { return impl[0]; }
        Scalar y() const { return N >= 1 ? impl[1] : 0; }
        Scalar z() const { return N >= 2 ? impl[2] : 0; }
        Scalar w() const { return N >= 3 ? impl[3] : 1; }
        
        template <class F>
        basic_vec map(F f) const {
            basic_vec res;
            for (size_t i = 0; i != N; ++i) {
                res.impl[i] = f(impl[i]);
            }
            return res;
        }

        template <class F>
        basic_vec map(const basic_vec &b, F f) const {
            basic_vec res;
            for (size_t i = 0; i != N; ++i) {
                res.impl[i] = f(impl[i], b.impl[i]);
            }
            return res;
        }

        template <class F>
        basic_vec map(const basic_vec &b, const basic_vec &c, F f) const {
            basic_vec res;
            for (size_t i = 0; i != N; ++i) {
                res.impl[i] = f(impl[i], b.impl[i]);
            }
            return res;
        }

        template <class F, class ACC>
        ACC sum(const basic_vec &b, F f, ACC acc) const {
            for (size_t i = 0; i != N; ++i) {
                acc += f(impl[i], b.impl[i]);
            }
            return acc;
        }
        
        Scalar operator[](size_t i) const {
            return impl[i];
        }

        void set_elem(size_t i, scalar_t v) {
            impl[i] = v;
        }
        
    private:
        Impl impl;
    };
    
    template <class T, class Scalar, size_t N>
    basic_vec<T, Scalar, N> operator+(const basic_vec<T, Scalar, N> &a, const basic_vec<T, Scalar, N> &b) { return a.map(b, [](Scalar a, Scalar b){ return a + b; }); }

    template <class T, class Scalar, size_t N>
    basic_vec<T, Scalar, N> operator-(const basic_vec<T, Scalar, N> &a, const basic_vec<T, Scalar, N> &b) { return a.map(b, [](Scalar a, Scalar b){ return a - b; }); }

    template <class T, class Scalar, size_t N>
    basic_vec<T, Scalar, N> operator*(const basic_vec<T, Scalar, N> &a, const basic_vec<T, Scalar, N> &b) { return a.map(b, [](Scalar a, Scalar b){ return a * b; }); }

    template <class T, class Scalar, size_t N>
    basic_vec<T, Scalar, N> operator/(const basic_vec<T, Scalar, N> &a, const basic_vec<T, Scalar, N> &b) { return a.map(b, [](Scalar a, Scalar b){ return a / b; }); }

    template <class T, class Scalar, size_t N>
    basic_vec<T, Scalar, N> operator+(const basic_vec<T, Scalar, N> &a, Scalar b) { return a.map([b](Scalar a){ return a + b; }); }

    template <class T, class Scalar, size_t N>
    basic_vec<T, Scalar, N> operator-(const basic_vec<T, Scalar, N> &a, Scalar b) { return a.map([b](Scalar a){ return a - b; }); }

    template <class T, class Scalar, size_t N>
    basic_vec<T, Scalar, N> operator*(const basic_vec<T, Scalar, N> &a, Scalar b) { return a.map([b](Scalar a){ return a * b; }); }

    template <class T, class Scalar, size_t N>
    basic_vec<T, Scalar, N> operator/(const basic_vec<T, Scalar, N> &a, Scalar b) { return a.map([b](Scalar a){ return a / b; }); }

    template <class T, class Scalar, size_t N>
    basic_vec<T, Scalar, N> min(const basic_vec<T, Scalar, N> &a, const basic_vec<T, Scalar, N> &b) { return a.map(b, [](Scalar a, Scalar b){ return a < b ? a : b; }); }

    template <class T, class Scalar, size_t N>
    basic_vec<T, Scalar, N> max(const basic_vec<T, Scalar, N> &a, const basic_vec<T, Scalar, N> &b) { return a.map(b, [](Scalar a, Scalar b){ return a > b ? a : b; }); }

    template <class T, class Scalar, size_t N>
    basic_vec<T, Scalar, N> mix(const basic_vec<T, Scalar, N> &a, const basic_vec<T, Scalar, N> &b, const basic_vec<T, Scalar, N> &c) { return a.map(b, c, [](Scalar a, Scalar b, Scalar c){ return a * (1 - c) + b * c; }); }

    template <class T, class Scalar, size_t N>
    bool operator==(const basic_vec<T, Scalar, N> &a, const basic_vec<T, Scalar, N> &b) { return a.sum(b, [](Scalar a, Scalar b){ return a == b; }, 0) == N; }

    template <class T, class Scalar, size_t N>
    bool operator!=(const basic_vec<T, Scalar, N> &a, const basic_vec<T, Scalar, N> &b) { return a.sum(b, [](Scalar a, Scalar b){ return a == b; }, 0) != N; }

    template <class T, class Scalar, size_t N>
    basic_vec<T, Scalar, N> abs(const basic_vec<T, Scalar, N> &a) { return a.map([](Scalar a){ return a < Scalar(0) ? -a : a; }); }

    template <class T, class Scalar, size_t N>
    Scalar dot(const basic_vec<T, Scalar, N> &a, const basic_vec<T, Scalar, N> &b) { return a.sum(b, [](Scalar a, Scalar b){ return a * b; }, Scalar(0)); }
    
    template <class T, class Scalar, size_t N>
    basic_vec<T, Scalar, N> normalized(const basic_vec<T, Scalar, N> &a) {
        float rlen = 1.0f / std::sqrt(dot(a, a));
        return a * rlen;
    }

    template <class Column, size_t M>
    class basic_mat : public basic_vec<Column[M], Column, M> {
    public:
        typedef basic_mat<Column, M> base_t;
        typedef Column column_t;
        typedef typename column_t::scalar_t scalar_t;
        static constexpr size_t num_rows() { return column_t::size(); }
        static constexpr size_t num_cols() { return M; }

        basic_mat() {
        }

        basic_mat(scalar_t value) {
            for (size_t i = 0; i != M; ++i) {
                set_elem(i, column_t(value));
            }
        }

        basic_mat(column_t *cols) {
            for (size_t i = 0; i != M; ++i) {
                set_elem(i, cols[i]);
            }
        }
        
        void set_col(size_t i, column_t c) {
            set_elem(i, c);
        }
    };

    template <class Column, size_t M>
    basic_mat<Column, M> operator*(const basic_mat<Column, M> &a, const basic_mat<Column, M> &b) {
        basic_mat<Column, M> result;
        for (size_t c = 0; c != result.num_cols(); ++c) {
            Column sum = a[0] * b[c][0];
            for (size_t r = 1; r != result.num_rows(); ++r) {
                sum += a[r] * b[c][r];
            }
            result.set_elem(c, sum);
        }
        return result;
    }
    
    #define MATH_VEC_BOILERPLATE(C) \
        typedef C this_t; \
        C() {} \
        C(const base_t &b) : base_t(b) {} \
        C(const this_t &b) : base_t(b) {} \
        C &operator+=(C b) { return (*this) = (*this) + b; } \
        C &operator-=(C b) { return (*this) = (*this) - b; } \
        C &operator*=(C b) { return (*this) = (*this) * b; } \
        C &operator/=(C b) { return (*this) = (*this) / b; }
    

    class vec2 : public basic_vec<float[2], float, 2> {
    public:
        MATH_VEC_BOILERPLATE(vec2)

        vec2(float x, float y) {
            set_elem(0, x);
            set_elem(1, y);
        }
    };
    
    class vec3 : public basic_vec<float[3], float, 3> {
    public:
        MATH_VEC_BOILERPLATE(vec3)

        vec3(float x, float y, float z) {
            set_elem(0, x);
            set_elem(1, y);
            set_elem(2, z);
        }
        
        vec3(vec2 a, scalar_t b) {
            set_elem(0, a[0]);
            set_elem(1, a[1]);
            set_elem(2, b);
        }

        vec3(float a, vec2 b) {
            set_elem(0, a);
            set_elem(1, b[0]);
            set_elem(2, b[1]);
        }
    };
    
    class vec4 : public basic_vec<float[4], float, 4> {
    public:
        MATH_VEC_BOILERPLATE(vec4)

        vec4(float x, float y, float z, float w) {
            set_elem(0, x);
            set_elem(1, y);
            set_elem(2, z);
            set_elem(3, w);
        }
    };
    
    class ivec2 : public basic_vec<int[2], int, 2> {
    public:
        MATH_VEC_BOILERPLATE(ivec2)

        ivec2(int x, int y) {
            set_elem(0, x);
            set_elem(1, y);
        }
    };
    
    class ivec3 : public basic_vec<int[3], int, 3> {
    public:
        MATH_VEC_BOILERPLATE(ivec3)

        ivec3(int x, int y, int z) {
            set_elem(0, x);
            set_elem(1, y);
            set_elem(2, z);
        }
    };
    
    class ivec4 : public basic_vec<int[4], int, 4> {
    public:
        MATH_VEC_BOILERPLATE(ivec4)

        ivec4(int x, int y, int z, int w) {
            set_elem(0, x);
            set_elem(1, y);
            set_elem(2, z);
            set_elem(3, w);
        }
    };
    
    #undef MATH_VEC_BOILERPLATE

    #define MATH_MAT_BOILERPLATE(C) \
        typedef C this_t; \
        C() {} \
        C(const base_t &b) : base_t(b) {} \
        C(const this_t &b) : base_t(b) {}

    class mat2 : public basic_mat<vec2, 2> {
    public:
        MATH_MAT_BOILERPLATE(mat2)

        mat2(column_t x, column_t y) {
            set_elem(0, x);
            set_elem(1, y);
        }

        mat2(float xx, float xy, float yx, float yy) {
            set_elem(0, vec2(xx, xy));
            set_elem(1, vec2(yx, yy));
        }
    };
    
    class mat3 : public basic_mat<vec3, 3> {
    public:
        MATH_MAT_BOILERPLATE(mat3)

        mat3(column_t x, column_t y, column_t z) {
            set_elem(0, x);
            set_elem(1, y);
            set_elem(2, z);
        }
    };
    
    class mat4 : public basic_mat<vec4, 4> {
    public:
        MATH_MAT_BOILERPLATE(mat4)

        mat4(column_t x, column_t y, column_t z, column_t w) {
            set_elem(0, x);
            set_elem(1, y);
            set_elem(2, z);
            set_elem(3, w);
        }
    };
    
    
    #undef MATH_MAT_BOILERPLATE

    vec3 cross(vec3 a, vec3 b) {
        return vec3(a.y() * b.z() - a.z() * b.y(), a.z() * b.x() - a.x() * b.z(), a.x() * b.y() - a.y() * b.x());
    }
    
    vec3 cwoss(vec4 a, vec4 b) {
        return vec3(a.y() * b.w() - a.w() * b.y(), a.w() * b.x() - a.x() * b.w(), a.x() * b.y() - a.y() * b.x());
    }
    
    vec3 persp(vec4 a) {
        float rw = 1.0f / a.w();
        return vec3(a.x() * rw, a.y() * rw, a.z() * rw);
    }
    
    std::ostream &operator << (std::ostream &os, vec2 v) {
        char tmp[64];
        snprintf(tmp, sizeof(tmp), "vec2(%g, %g)", v.x(), v.y());
        os << tmp;
        return os;
    }
    
    std::ostream &operator << (std::ostream &os, vec3 v) {
        char tmp[64];
        snprintf(tmp, sizeof(tmp), "vec3(%g, %g, %g)", v.x(), v.y(), v.z());
        os << tmp;
        return os;
    }
    
    std::ostream &operator << (std::ostream &os, vec4 v) {
        char tmp[64];
        snprintf(tmp, sizeof(tmp), "vec4(%g, %g, %g, %g)", v.x(), v.y(), v.z(), v.w());
        os << tmp;
        return os;
    }
    
    std::ostream &operator << (std::ostream &os, mat2 v) {
        char tmp[64];
        snprintf(tmp, sizeof(tmp), "mat2(%g, %g, %g, %g)", v[0][0], v[0][1], v[1][0], v[1][1]);
        os << tmp;
        return os;
    }
    
}

#endif
