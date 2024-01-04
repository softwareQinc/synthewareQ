#include "gtest/gtest.h"

#include "staq/mapping/device.hpp"
#include "staq/synthesis/linear_reversible.hpp"

using namespace staq;
using circuit = std::list<std::pair<int, int>>;

static mapping::Device test_device("Test device", 9,
                                   {
                                       {0, 1, 0, 0, 0, 1, 0, 0, 0},
                                       {1, 0, 1, 0, 1, 0, 0, 0, 0},
                                       {0, 1, 0, 1, 0, 0, 0, 0, 0},
                                       {0, 0, 1, 0, 1, 0, 0, 0, 1},
                                       {0, 1, 0, 1, 0, 1, 0, 1, 0},
                                       {1, 0, 0, 0, 1, 0, 1, 0, 0},
                                       {0, 0, 0, 0, 0, 1, 0, 1, 0},
                                       {0, 0, 0, 0, 1, 0, 1, 0, 1},
                                       {0, 0, 0, 1, 0, 0, 0, 1, 0},
                                   },
                                   {1, 1, 1, 1, 1, 1, 1, 1, 1},
                                   {
                                       {0, 0.9, 0, 0, 0, 0.1, 0, 0, 0},
                                       {0.9, 0, 0.1, 0, 0.9, 0, 0, 0, 0},
                                       {0, 0.1, 0, 0.1, 0, 0, 0, 0, 0},
                                       {0, 0, 0.1, 0, 0.1, 0, 0, 0, 0.1},
                                       {0, 0.9, 0, 0.1, 0, 0.1, 0, 0.9, 0},
                                       {0.1, 0, 0, 0, 0.1, 0, 0.1, 0, 0},
                                       {0, 0, 0, 0, 0, 0.1, 0, 0.1, 0},
                                       {0, 0, 0, 0, 0.9, 0, 0.9, 0, 0.1},
                                       {0, 0, 0, 0.1, 0, 0, 0, 0.11, 0},
                                   });

// Testing linear reversible (cnot) synthesis

TEST(Gaussian_Synthesis, Base) {
    synthesis::linear_op<bool> mat{
        {1, 0},
        {1, 1},
    };
    EXPECT_EQ(synthesis::gauss_jordan(mat), circuit({{0, 1}}));
    EXPECT_EQ(synthesis::gaussian_elim(mat), circuit({{0, 1}}));
}

TEST(Gaussian_Synthesis, Swap) {
    synthesis::linear_op<bool> mat{
        {0, 1},
        {1, 0},
    };
    EXPECT_EQ(synthesis::gauss_jordan(mat), circuit({{1, 0}, {0, 1}, {1, 0}}));
    EXPECT_EQ(synthesis::gaussian_elim(mat), circuit({{1, 0}, {0, 1}, {1, 0}}));
}

TEST(Gaussian_Synthesis, Back_propagation) {
    synthesis::linear_op<bool> mat{
        {1, 1},
        {0, 1},
    };
    EXPECT_EQ(synthesis::gauss_jordan(mat), circuit({{1, 0}}));
    EXPECT_EQ(synthesis::gaussian_elim(mat), circuit({{1, 0}}));
}

TEST(Gaussian_Synthesis, 3_Qubit) {
    synthesis::linear_op<bool> mat{{1, 0, 0}, {1, 1, 0}, {0, 1, 1}};
    EXPECT_EQ(synthesis::gauss_jordan(mat), circuit({{1, 2}, {0, 1}}));
    EXPECT_EQ(synthesis::gaussian_elim(mat), circuit({{1, 2}, {0, 1}}));
}

TEST(Steiner_Gauss, Base) {
    synthesis::linear_op<bool> mat{
        {1, 0, 0, 0, 0, 0, 0, 0}, {1, 1, 0, 0, 0, 0, 0, 0},
        {0, 0, 1, 0, 0, 0, 0, 0}, {0, 0, 0, 1, 0, 0, 0, 0},
        {1, 0, 0, 0, 1, 0, 0, 0}, {0, 0, 0, 0, 0, 1, 0, 0},
        {0, 0, 0, 0, 0, 0, 1, 0}, {0, 0, 0, 0, 0, 0, 0, 1}};

    EXPECT_EQ(synthesis::steiner_gauss(mat, test_device),
              circuit({{1, 4}, {0, 1}, {1, 4}}));
}

TEST(Steiner_Gauss, Base_inv) {
    synthesis::linear_op<bool> mat{
        {1, 1, 0, 0, 0, 0, 0, 0}, {0, 1, 0, 0, 1, 0, 0, 0},
        {0, 0, 1, 0, 0, 0, 0, 0}, {0, 0, 0, 1, 0, 0, 0, 0},
        {0, 0, 0, 0, 1, 0, 0, 0}, {0, 0, 0, 0, 0, 1, 0, 0},
        {0, 0, 0, 0, 0, 0, 1, 0}, {0, 0, 0, 0, 0, 0, 0, 1}};

    EXPECT_EQ(synthesis::steiner_gauss(mat, test_device),
              circuit({{1, 0}, {4, 1}, {1, 0}, {1, 0}}));
}

TEST(Steiner_Gauss, Fill_Flush) {
    synthesis::linear_op<bool> mat{
        {1, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 1, 0, 0, 0, 0, 0, 0, 0},
        {1, 0, 1, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 1, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 1, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 1, 0, 0, 0},
        {1, 0, 0, 0, 0, 0, 1, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 1, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 1}};

    EXPECT_EQ(synthesis::steiner_gauss(mat, test_device), circuit({{1, 4},
                                                                   {4, 7},
                                                                   {7, 6},
                                                                   {1, 2},
                                                                   {4, 7},
                                                                   {1, 4},
                                                                   {0, 1},
                                                                   {1, 4},
                                                                   {4, 7},
                                                                   {7, 6},
                                                                   {1, 2},
                                                                   {4, 7},
                                                                   {1, 4},
                                                                   {0, 1}}));
}

TEST(Steiner_Gauss, Swap_Rows) {
    synthesis::linear_op<bool> mat{
        {0, 1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 1, 0, 0, 0, 0, 0}, {0, 0, 0, 1, 0, 0, 0, 0},
        {0, 0, 0, 0, 1, 0, 0, 0}, {0, 0, 0, 0, 0, 1, 0, 0},
        {0, 0, 0, 0, 0, 0, 1, 0}, {0, 0, 0, 0, 0, 0, 0, 1}};

    EXPECT_EQ(synthesis::steiner_gauss(mat, test_device),
              circuit({{1, 0}, {0, 1}, {1, 0}}));
}

TEST(Steiner_Gauss, Swap_Rows_Nonadjacent) {
    synthesis::linear_op<bool> mat{
        {0, 0, 1, 0, 0, 0, 0, 0}, {0, 1, 0, 0, 0, 0, 0, 0},
        {1, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 1, 0, 0, 0, 0},
        {0, 0, 0, 0, 1, 0, 0, 0}, {0, 0, 0, 0, 0, 1, 0, 0},
        {0, 0, 0, 0, 0, 0, 1, 0}, {0, 0, 0, 0, 0, 0, 0, 1}};

    EXPECT_EQ(
        synthesis::steiner_gauss(mat, test_device),
        circuit(
            {{2, 1}, {1, 0}, {1, 2}, {2, 1}, {0, 1}, {1, 2}, {1, 0}, {2, 1}}));
}
