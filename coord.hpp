// -*- mode: c++; coding: utf-8 -*-
/*! @file coord.hpp
    @brief Coordinate system
*/
#pragma once
#ifndef COORD_HPP_
#define COORD_HPP_

#include <cassert>
#include <cmath>
#include <vector>
#include <algorithm>
#include <numeric>
#include <random>


/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////

template <class T>
std::vector<T>& operator+=(std::vector<T>& lhs, const std::vector<T>& rhs) {
    assert(lhs.size() == rhs.size());
    std::transform(lhs.begin(), lhs.end(), rhs.begin(),
                   lhs.begin(), std::plus<T>());
    return lhs;
}

template <class T>
std::vector<T>& operator-=(std::vector<T>& lhs, const std::vector<T>& rhs) {
    assert(lhs.size() == rhs.size());
    std::transform(lhs.begin(), lhs.end(), rhs.begin(),
                   lhs.begin(), std::minus<T>());
    return lhs;
}

template <class T>
std::vector<T> operator+(std::vector<T> lhs, const std::vector<T>& rhs) {
    return lhs += rhs;
}

template <class T>
std::vector<T> operator-(std::vector<T> lhs, const std::vector<T>& rhs) {
    return lhs -= rhs;
}

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////

class Coord {
  public:
    Coord() = delete;
    Coord(const size_t d): dimensions(d) {}
    virtual ~Coord() = default;
    const size_t dimensions;
    const std::vector<std::vector<int>>& directions() const {return directions_;}
    size_t max_neighbors() const {return max_neighbors_;}
    std::vector<int> origin() const {
        return std::vector<int>(dimensions);
    }
    std::vector<std::vector<int>> neighbors(const std::vector<int>& v) const {
        std::vector<std::vector<int>> output = directions_;
        for (auto& d: output) {
            d += v;
        }
        return output;
    }
    template <class RNG> inline
    std::vector<int> random_neighbor(const std::vector<int>& v, RNG& rng) const {
        std::uniform_int_distribution<ptrdiff_t> uniform(0, max_neighbors_ - 1);
        return v + directions_[uniform(rng)];
    }
    std::vector<int> outward(const std::vector<int>& v) const {
        const auto candidates = neighbors(v);
        return *std::max_element(candidates.begin(), candidates.end(),
                                 [this](const std::vector<int>& lhs, const std::vector<int>& rhs) {
            return euclidean_distance(lhs) < euclidean_distance(rhs);
        });
    }
    virtual size_t graph_distance(const std::vector<int>& v) const = 0;
    double euclidean_distance(const std::vector<int>& v) const {
        double result = 0;
        for (const int x: v) {result += x * x;}
        return std::sqrt(result);
    }
    //! @todo
    std::vector<std::vector<int>> layer(const size_t d) const {
        std::vector<std::vector<int>> output;
        // graph_distance(v) == d
        return output;
    }
    virtual std::vector<std::vector<int>> core() const {
        const size_t n = std::pow(2, dimensions);
        std::vector<std::vector<int>> output;
        output.reserve(n);
        for (size_t i=0; i<n; ++i) {
            std::bitset<3> bs(i);
            std::vector<int> v(dimensions);
            for (size_t j=0; j<dimensions; ++j) {
                v[j] = static_cast<int>(bs[j]);
            }
            output.push_back(v);
        }
        return output;
    }
  protected:
    std::vector<std::vector<int>> directions_;
    size_t max_neighbors_;
};

class Neumann: public Coord {
  public:
    Neumann() = delete;
    explicit Neumann(const size_t d): Coord(d) {
        max_neighbors_ = 2 * d;
        directions_.reserve(max_neighbors_);
        std::vector<int> v(dimensions, 0);
        v.back() += 1;
        do {
            directions_.push_back(v);
        } while (std::next_permutation(v.begin(), v.end()));
        v.assign(dimensions, 0);
        v.front() -= 1;
        do {
            directions_.push_back(v);
        } while (std::next_permutation(v.begin(), v.end()));
    }
    //! Manhattan distance
    size_t graph_distance(const std::vector<int>& v) const {
        return std::accumulate(v.begin(), v.end(), 0, [](const int lhs, const int rhs) {
            return std::abs(lhs) + std::abs(rhs);
        });
    }
};

//! Neumann + diagonal cells
class Moore: public Coord {
  public:
    Moore() = delete;
    explicit Moore(const size_t d): Coord(d) {
        max_neighbors_ = std::pow(3, dimensions) - 1;
        directions_.reserve(max_neighbors_);
        for (const int x: {-1, 0, 1}) {
            for (const int y: {-1, 0, 1}) {
                if (dimensions == 2) {
                    if (x == 0 && y == 0) continue;
                    directions_.push_back({x, y});
                    continue;
                }
                for (const int z: {-1, 0, 1}) {
                    if (x == 0 && y == 0 && z == 0) continue;
                    directions_.push_back({x, y, z});
                }
            }
        }
    }
    //! Chebyshev/chessboard distance
    size_t graph_distance(const std::vector<int>& v) const {
        return std::abs(*std::max_element(v.begin(), v.end(), [](const int lhs, const int rhs) {
            return std::abs(lhs) < std::abs(rhs);
        }));
    }
};

class Hexagonal: public Coord {
  public:
    Hexagonal() = delete;
    explicit Hexagonal(const size_t d): Coord(d) {
        max_neighbors_ = 6 * (d - 1);
        std::vector<int> v{-1, 0, 1};
        directions_.reserve(max_neighbors_);
        if (dimensions == 2) {
            do {
                directions_.push_back({v[0], v[1]});
            } while (std::next_permutation(v.begin(), v.end()));
        }
        else {
            do {
                directions_.push_back({v[0], v[1], 0});
            } while (std::next_permutation(v.begin(), v.end()));
            directions_.push_back({0, 0, -1});
            directions_.push_back({1, 0, -1});
            directions_.push_back({1, -1, -1});
            directions_.push_back({0, 0, 1});
            directions_.push_back({-1, 0, 1});
            directions_.push_back({-1, 1, 1});
        }
    }
    size_t graph_distance(const std::vector<int>& v) const {
        std::vector<size_t> absv;
        absv.reserve(v.size() * 2);
        for (auto x: v) {
            absv.push_back(std::abs(x));
        }
        absv.push_back(std::abs(v[0] + v[1]));
        if (v.size() == 3) {
            absv.push_back(std::abs(v[0] + v[2]));
        }
        return *std::max_element(absv.begin(), absv.end());
    }
    //! @todo
    double euclidean_distance(const std::vector<int>& v) const {
        return static_cast<double>(graph_distance(v));
    }

    std::vector<std::vector<int>> core() const {
        std::vector<std::vector<int>> output = Neumann(dimensions).core();
        if (dimensions == 3) {
            output.resize(3);
            output.push_back({1, 0, -1});
        }
        return output;
    }
};



#endif /* COORD_HPP_ */
