/// Copyright (c) 2019 Chris Ohk, Justin Kim

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#include "CpuComputeTest.hpp"
#include "doctest.h"

#include <Takion/Computations/Computations.hpp>
#include <array>

namespace Takion::Test
{
void TestMatMul()
{
    Compute::Device device(0, Compute::DeviceType::CPU, "testDevice");
    const auto batchSize = 3;

    Tensor<float> tensorA({ 3, 2 }, device);
    Tensor<float> tensorB({ 2, 3 }, device);

    Tensor<float> output({ 3, 3 }, batchSize, device);

    for (std::size_t batchIdx = 0; batchIdx < batchSize; ++batchIdx)
    {
        tensorA.At(batchIdx, { 0, 0 }) = 1.0f;
        tensorA.At(batchIdx, { 0, 1 }) = 2.0f;
        tensorA.At(batchIdx, { 1, 0 }) = 3.0f;
        tensorA.At(batchIdx, { 1, 1 }) = 4.0f;
        tensorA.At(batchIdx, { 2, 0 }) = 5.0f;
        tensorA.At(batchIdx, { 2, 1 }) = 6.0f;

        tensorB.At(batchIdx, { 0, 0 }) = 1.0f;
        tensorB.At(batchIdx, { 0, 1 }) = 3.0f;
        tensorB.At(batchIdx, { 0, 2 }) = 5.0f;
        tensorB.At(batchIdx, { 1, 0 }) = 2.0f;
        tensorB.At(batchIdx, { 1, 1 }) = 4.0f;
        tensorB.At(batchIdx, { 1, 2 }) = 6.0f;
    }

    Compute::Multiply(tensorA, tensorB, output);

    std::array<std::array<float, 3>, 3> answer = { { { 5.0f, 11.0f, 17.0f },
                                                     { 11.0f, 25.0f, 39.0f },
                                                     { 17.0f, 39.0f,
                                                       61.0f } } };

    for (std::size_t batchIdx = 0; batchIdx < batchSize; ++batchIdx)
        for (std::size_t i = 0; i < 3; i++)
        {
            for (std::size_t j = 0; j < 3; j++)
            {
                auto ans = answer[i][j];
                auto num = output.At(batchIdx, { i, j });

                CHECK(ans == num);
            }
        }
}

void TestMatMul2()
{
    Compute::Device device(0, Compute::DeviceType::CPU, "testDevice");

    const auto batchSize = 3;
    const auto size = 150;

    Tensor<float> tensorA({ size, size }, batchSize, device);
    Tensor<float> tensorB({ size, size }, batchSize, device);

    Tensor<float> output({ size, size }, device);

    for (std::size_t batchIdx = 0; batchIdx < batchSize; ++batchIdx)
        for (std::size_t i = 0; i < size; i++)
        {
            for (std::size_t j = 0; j < size; j++)
            {
                tensorA.At(batchIdx, { i, j }) = 2.0f;
                tensorB.At(batchIdx, { i, j }) = 2.0f;
            }
        }

    Compute::Multiply(tensorA, tensorB, output);

    for (std::size_t batchIdx = 0; batchIdx < batchSize; ++batchIdx)
        for (std::size_t i = 0; i < size; i++)
        {
            for (std::size_t j = 0; j < size; j++)
            {
                const auto num = output.At(batchIdx, { i, j });
                CHECK(num == 4 * size);
            }
        }
}

void TestMatMulWithTranspose()
{
    Compute::Device device(0, Compute::DeviceType::CPU, "testDevice");
    const auto batchSize = 3;

    Tensor<float> tensorA({ 3, 3 }, batchSize, device);
    Tensor<float> tensorB({ 3, 3 }, batchSize, device);
    Tensor<float> tensorBTranspose({ 3, 3 }, batchSize, device);

    Tensor<float> output({ 3, 3 }, batchSize, device);

    for (std::size_t batchIdx = 0; batchIdx < batchSize; ++batchIdx)
    {
        tensorA.At(batchIdx, { 0, 0 }) = 1.0f;
        tensorA.At(batchIdx, { 0, 1 }) = 2.0f;
        tensorA.At(batchIdx, { 0, 2 }) = 3.0f;
        tensorA.At(batchIdx, { 1, 0 }) = 4.0f;
        tensorA.At(batchIdx, { 1, 1 }) = 5.0f;
        tensorA.At(batchIdx, { 1, 2 }) = 6.0f;
        tensorA.At(batchIdx, { 2, 0 }) = 7.0f;
        tensorA.At(batchIdx, { 2, 1 }) = 8.0f;
        tensorA.At(batchIdx, { 2, 2 }) = 9.0f;

        tensorB.At(batchIdx, { 0, 0 }) = 1.0f;
        tensorB.At(batchIdx, { 0, 1 }) = 2.0f;
        tensorB.At(batchIdx, { 0, 2 }) = 3.0f;
        tensorB.At(batchIdx, { 1, 0 }) = 4.0f;
        tensorB.At(batchIdx, { 1, 1 }) = 5.0f;
        tensorB.At(batchIdx, { 1, 2 }) = 6.0f;
        tensorB.At(batchIdx, { 2, 0 }) = 7.0f;
        tensorB.At(batchIdx, { 2, 1 }) = 8.0f;
        tensorB.At(batchIdx, { 2, 2 }) = 9.0f;
    }

    Compute::Transpose(tensorB, tensorBTranspose);
    Compute::Multiply(tensorA, tensorB, output);

    std::array<std::array<float, 3>, 3> answer = { { { 14.0f, 32.0f, 50.0f },
                                                     { 32.0f, 77.0f, 122.0f },
                                                     { 50.0f, 122.0f,
                                                       194.0f } } };

    for (std::size_t batchIdx = 0; batchIdx < batchSize; ++batchIdx)
        for (std::size_t i = 0; i < 3; i++)
        {
            for (std::size_t j = 0; j < 3; j++)
            {
                auto ans = answer[i][j];
                auto num = output.At(batchIdx, { i, j });

                CHECK(ans == num);
            }
        }
}


void TestMatAdd()
{
    Compute::Device device(0, Compute::DeviceType::CPU, "testDevice");
    const auto batchSize = 3;
    const auto rowSize = 2;
    const auto colSize = 5;

    Tensor<float> tensorA({ rowSize, colSize }, batchSize, device);
    Tensor<float> tensorB({ rowSize, colSize }, batchSize, device);

    Tensor<float> output({ rowSize, colSize }, batchSize, device);

    for (std::size_t batch = 0; batch < batchSize; ++batch)
        for (std::size_t i = 0; i < rowSize; i++)
        {
            for (std::size_t j = 0; j < colSize; j++)
            {
                tensorA.At(batch, { i, j }) = 4.0f;
                tensorB.At(batch, { i, j }) = 4.0f;
            }
        }

    Compute::Add(tensorA, tensorB, output);

    for (std::size_t batch = 0; batch < batchSize; ++batch)
        for (std::size_t i = 0; i < rowSize; i++)
        {
            for (std::size_t j = 0; j < colSize; j++)
            {
                const auto num = output.At(batch, { i, j });
                CHECK(num == 8);
            }
        }
}

void TestMatDot()
{
    Compute::Device device(0, Compute::DeviceType::CPU, "testDevice");
    const auto batchSize = 3;
    const auto rowSize = 2;
    const auto colSize = 5;

    Tensor<float> tensorA({ rowSize, colSize }, batchSize, device);
    Tensor<float> tensorB({ rowSize, colSize }, batchSize, device);

    Tensor<float> output({ rowSize, colSize }, batchSize, device);

    for (std::size_t batch = 0; batch < batchSize; ++batch)
        for (std::size_t i = 0; i < rowSize; i++)
        {
            for (std::size_t j = 0; j < colSize; j++)
            {
                tensorA.At(batch, { i, j }) = 4.0f;
                tensorB.At(batch, { i, j }) = 4.0f;
            }
        }

    Compute::Dot(tensorA, tensorB, output);

    for (std::size_t batch = 0; batch < batchSize; ++batch)
        for (std::size_t i = 0; i < rowSize; i++)
        {
            for (std::size_t j = 0; j < colSize; j++)
            {
                const auto num = output.At(batch, { i, j });
                CHECK(num == 16);
            }
        }
}

void TestShrink()
{
    Compute::Device device(0, Compute::DeviceType::CPU, "testDevice");
    const auto batchSize = 3;

    Tensor<float> tensorA({ 3, 3 }, batchSize, device);
    Tensor<float> output({ 3, 3 }, device);

    for (std::size_t batchIdx = 0; batchIdx < batchSize; ++batchIdx)
    {
        tensorA.At(batchIdx, { 0, 0 }) = 1.0f;
        tensorA.At(batchIdx, { 0, 1 }) = 2.0f;
        tensorA.At(batchIdx, { 0, 2 }) = 3.0f;
        tensorA.At(batchIdx, { 1, 0 }) = 4.0f;
        tensorA.At(batchIdx, { 1, 1 }) = 5.0f;
        tensorA.At(batchIdx, { 1, 2 }) = 6.0f;
        tensorA.At(batchIdx, { 2, 0 }) = 7.0f;
        tensorA.At(batchIdx, { 2, 1 }) = 8.0f;
        tensorA.At(batchIdx, { 2, 2 }) = 9.0f;
    }

    Compute::Shrink(tensorA, output);

    std::array<std::array<float, 3>, 3> answer = { { { 1.0f, 2.0f, 3.0f },
                                                     { 4.0f, 5.0f, 6.0f },
                                                     { 7.0f, 8.0f,
                                                       9.0f } } };

    for (std::size_t i = 0; i < 3; i++)
    {
        for (std::size_t j = 0; j < 3; j++)
        {
            auto ans = answer[i][j];
            auto num = output.At(0, { i, j });

            CHECK(ans == num);
        }
    }
}

void TestShrink2()
{
    Compute::Device device(0, Compute::DeviceType::CPU, "testDevice");
    const auto batchSize = 3;

    Tensor<float> tensor({ 3 }, batchSize, device);
    Tensor<float> output({ 3 }, device);

    for (std::size_t batchIdx = 0; batchIdx < batchSize; ++batchIdx)
    {
        tensor.At(batchIdx, { 0, 0 }) = 1.0f;
        tensor.At(batchIdx, { 0, 1 }) = 2.0f;
        tensor.At(batchIdx, { 0, 2 }) = 3.0f;
        tensor.At(batchIdx, { 1, 0 }) = 4.0f;
        tensor.At(batchIdx, { 1, 1 }) = 5.0f;
        tensor.At(batchIdx, { 1, 2 }) = 6.0f;
        tensor.At(batchIdx, { 2, 0 }) = 7.0f;
        tensor.At(batchIdx, { 2, 1 }) = 8.0f;
        tensor.At(batchIdx, { 2, 2 }) = 9.0f;
    }

    Compute::Shrink(tensor, output);

    std::array<float, 3> answer = { { 4.0f, 5.0f, 6.0f } };

    for (std::size_t j = 0; j < 3; j++)
    {
        auto ans = answer[j];
        auto num = output.At(0, { j });

        CHECK(ans == num);
    }
}

void TestScalarMul()
{
    const Compute::Device device(0, Compute::DeviceType::CPU, "testDevice");
    const auto batchSize = 3;

    Tensor<float> tensor({ 3, 3 }, batchSize, device);

    for (std::size_t batchIdx = 0; batchIdx < batchSize; ++batchIdx)
    {
        tensor.At(batchIdx, { 0, 0 }) = 1.0f;
        tensor.At(batchIdx, { 0, 1 }) = 2.0f;
        tensor.At(batchIdx, { 0, 2 }) = 3.0f;
        tensor.At(batchIdx, { 1, 0 }) = 4.0f;
        tensor.At(batchIdx, { 1, 1 }) = 5.0f;
        tensor.At(batchIdx, { 1, 2 }) = 6.0f;
        tensor.At(batchIdx, { 2, 0 }) = 7.0f;
        tensor.At(batchIdx, { 2, 1 }) = 8.0f;
        tensor.At(batchIdx, { 2, 2 }) = 9.0f;
    }

    Compute::ScalarMul(tensor, 0.5f, tensor);

    std::array<std::array<float, 3>, 3> answer = {
        { { 0.5f, 1.0f, 1.5f },
          { 2.0f, 2.5f, 3.0f },
          { 3.5f, 4.0f, 4.5f } }
    };

    for (std::size_t batch = 0; batch < batchSize; ++batch)
        for (std::size_t i = 0; i < 3; i++)
        {
            for (std::size_t j = 0; j < 3; j++)
            {
                auto ans = answer[i][j];
                auto num = tensor.At(batch, { i, j });

                CHECK(ans == num);
            }
        }
}
} // namespace Takion