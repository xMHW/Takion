/// Copyright (c) 2019 Chris Ohk, Justin Kim

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#include "MatrixTest.hpp"
#include <cubbydnn/Computations/TensorOperations/Computations.hpp>
#include <iostream>
#include "gtest/gtest.h"

namespace CubbyDNN
{
void TestMatMul()
{
    Compute::Device device(0, Compute::DeviceType::Cpu, "testDevice", 0);

    Tensor tensorA = Tensor::CreateTensor({ 3, 3, 1, 1 }, NumberSystem::Float,
                                          device);
    Tensor tensorB = Tensor::CreateTensor({ 3, 3, 1, 1 }, NumberSystem::Float,
                                          device);

    SetData<float>({ 0, 0, 0, 0 }, tensorA, 4.0f);
    SetData<float>({ 1, 1, 0, 0 }, tensorA, 4.0f);
    SetData<float>({ 2, 2, 0, 0 }, tensorA, 4.0f);

    SetData<float>({ 0, 0, 0, 0 }, tensorB, 4.0f);
    SetData<float>({ 1, 1, 0, 0 }, tensorB, 4.0f);
    SetData<float>({ 2, 2, 0, 0 }, tensorB, 4.0f);

    Tensor output = Tensor::CreateTensor({ 3, 3, 1, 1 }, NumberSystem::Float,
                                         device);

    Compute::Multiply(tensorA, tensorB, output);

    for (std::size_t i = 0; i < 3; i++)
    {
        for (std::size_t j = 0; j < 3; j++)
        {
            const auto num = GetData<float>({ j, i, 0, 0 }, output);
            if (i == j)
                EXPECT_EQ(num, 16);
            else
                EXPECT_EQ(num, 0);
        }
    }
}

void TestMatMul2()
{
    Compute::Device device(0, Compute::DeviceType::Cpu, "testDevice", 0);

    Tensor tensorA = Tensor::CreateTensor({ 1, 1, 3, 3 }, NumberSystem::Float,
                                          device);
    Tensor tensorB = Tensor::CreateTensor({ 1, 1, 3, 3 }, NumberSystem::Float,
                                          device);

    SetData<float>({ 0, 0, 0, 0 }, tensorA, 2.0f);
    SetData<float>({ 0, 0, 0, 1 }, tensorA, 2.0f);
    SetData<float>({ 0, 0, 0, 2 }, tensorA, 2.0f);
    SetData<float>({ 0, 0, 1, 0 }, tensorA, 2.0f);
    SetData<float>({ 0, 0, 1, 1 }, tensorA, 2.0f);
    SetData<float>({ 0, 0, 1, 2 }, tensorA, 2.0f);
    SetData<float>({ 0, 0, 2, 0 }, tensorA, 2.0f);
    SetData<float>({ 0, 0, 2, 1 }, tensorA, 2.0f);
    SetData<float>({ 0, 0, 2, 2 }, tensorA, 2.0f);

    SetData<float>({ 0, 0, 0, 0 }, tensorB, 2.0f);
    SetData<float>({ 0, 0, 0, 1 }, tensorB, 2.0f);
    SetData<float>({ 0, 0, 0, 2 }, tensorB, 2.0f);
    SetData<float>({ 0, 0, 1, 0 }, tensorB, 2.0f);
    SetData<float>({ 0, 0, 1, 1 }, tensorB, 2.0f);
    SetData<float>({ 0, 0, 1, 2 }, tensorB, 2.0f);
    SetData<float>({ 0, 0, 2, 0 }, tensorB, 2.0f);
    SetData<float>({ 0, 0, 2, 1 }, tensorB, 2.0f);
    SetData<float>({ 0, 0, 2, 2 }, tensorB, 2.0f);

    Tensor output = Tensor::CreateTensor({ 1, 1, 3, 3 }, NumberSystem::Float, device);

    Compute::Multiply(tensorA, tensorB, output);

    for (std::size_t i = 0; i < 3; i++)
    {
        for (std::size_t j = 0; j < 3; j++)
        {
            const auto num = GetData<float>({ 0, 0, i, j }, output);
            EXPECT_EQ(num, 12);
            std::cout << num << " ";
        }
        std::cout << std::endl;
    }
}

void TestMatMul3()
{
    Compute::Device device(0, Compute::DeviceType::Cpu, "testDevice", 0);
    Tensor tensorA = Tensor::CreateTensor({ 2, 2, 3, 3 }, NumberSystem::Float,
                                          device);
    Tensor tensorB = Tensor::CreateTensor({ 2, 2, 3, 3 }, NumberSystem::Float,
                                          device);

    SetData<float>({ 0, 0, 0, 0 }, tensorA, 3.0f);
    SetData<float>({ 0, 0, 0, 1 }, tensorA, 3.0f);
    SetData<float>({ 0, 0, 0, 2 }, tensorA, 3.0f);
    SetData<float>({ 0, 0, 1, 0 }, tensorA, 3.0f);
    SetData<float>({ 0, 0, 1, 1 }, tensorA, 3.0f);
    SetData<float>({ 0, 0, 1, 2 }, tensorA, 3.0f);
    SetData<float>({ 0, 0, 2, 0 }, tensorA, 3.0f);
    SetData<float>({ 0, 0, 2, 1 }, tensorA, 3.0f);
    SetData<float>({ 0, 0, 2, 2 }, tensorA, 3.0f);

    SetData<float>({ 0, 0, 0, 0 }, tensorB, 3.0f);
    SetData<float>({ 0, 0, 0, 1 }, tensorB, 3.0f);
    SetData<float>({ 0, 0, 0, 2 }, tensorB, 3.0f);
    SetData<float>({ 0, 0, 1, 0 }, tensorB, 3.0f);
    SetData<float>({ 0, 0, 1, 1 }, tensorB, 3.0f);
    SetData<float>({ 0, 0, 1, 2 }, tensorB, 3.0f);
    SetData<float>({ 0, 0, 2, 0 }, tensorB, 3.0f);
    SetData<float>({ 0, 0, 2, 1 }, tensorB, 3.0f);
    SetData<float>({ 0, 0, 2, 2 }, tensorB, 3.0f);

    SetData<float>({ 0, 1, 0, 0 }, tensorA, 3.0f);
    SetData<float>({ 0, 1, 0, 1 }, tensorA, 3.0f);
    SetData<float>({ 0, 1, 0, 2 }, tensorA, 3.0f);
    SetData<float>({ 0, 1, 1, 0 }, tensorA, 3.0f);
    SetData<float>({ 0, 1, 1, 1 }, tensorA, 3.0f);
    SetData<float>({ 0, 1, 1, 2 }, tensorA, 3.0f);
    SetData<float>({ 0, 1, 2, 0 }, tensorA, 3.0f);
    SetData<float>({ 0, 1, 2, 1 }, tensorA, 3.0f);
    SetData<float>({ 0, 1, 2, 2 }, tensorA, 3.0f);

    SetData<float>({ 0, 1, 0, 0 }, tensorB, 3.0f);
    SetData<float>({ 0, 1, 0, 1 }, tensorB, 3.0f);
    SetData<float>({ 0, 1, 0, 2 }, tensorB, 3.0f);
    SetData<float>({ 0, 1, 1, 0 }, tensorB, 3.0f);
    SetData<float>({ 0, 1, 1, 1 }, tensorB, 3.0f);
    SetData<float>({ 0, 1, 1, 2 }, tensorB, 3.0f);
    SetData<float>({ 0, 1, 2, 0 }, tensorB, 3.0f);
    SetData<float>({ 0, 1, 2, 1 }, tensorB, 3.0f);
    SetData<float>({ 0, 1, 2, 2 }, tensorB, 3.0f);

    SetData<float>({ 1, 0, 0, 0 }, tensorA, 3.0f);
    SetData<float>({ 1, 0, 0, 1 }, tensorA, 3.0f);
    SetData<float>({ 1, 0, 0, 2 }, tensorA, 3.0f);
    SetData<float>({ 1, 0, 1, 0 }, tensorA, 3.0f);
    SetData<float>({ 1, 0, 1, 1 }, tensorA, 3.0f);
    SetData<float>({ 1, 0, 1, 2 }, tensorA, 3.0f);
    SetData<float>({ 1, 0, 2, 0 }, tensorA, 3.0f);
    SetData<float>({ 1, 0, 2, 1 }, tensorA, 3.0f);
    SetData<float>({ 1, 0, 2, 2 }, tensorA, 3.0f);

    SetData<float>({ 1, 0, 0, 0 }, tensorB, 3.0f);
    SetData<float>({ 1, 0, 0, 1 }, tensorB, 3.0f);
    SetData<float>({ 1, 0, 0, 2 }, tensorB, 3.0f);
    SetData<float>({ 1, 0, 1, 0 }, tensorB, 3.0f);
    SetData<float>({ 1, 0, 1, 1 }, tensorB, 3.0f);
    SetData<float>({ 1, 0, 1, 2 }, tensorB, 3.0f);
    SetData<float>({ 1, 0, 2, 0 }, tensorB, 3.0f);
    SetData<float>({ 1, 0, 2, 1 }, tensorB, 3.0f);
    SetData<float>({ 1, 0, 2, 2 }, tensorB, 3.0f);

    SetData<float>({ 1, 1, 0, 0 }, tensorA, 3.0f);
    SetData<float>({ 1, 1, 0, 1 }, tensorA, 3.0f);
    SetData<float>({ 1, 1, 0, 2 }, tensorA, 3.0f);
    SetData<float>({ 1, 1, 1, 0 }, tensorA, 3.0f);
    SetData<float>({ 1, 1, 1, 1 }, tensorA, 3.0f);
    SetData<float>({ 1, 1, 1, 2 }, tensorA, 3.0f);
    SetData<float>({ 1, 1, 2, 0 }, tensorA, 3.0f);
    SetData<float>({ 1, 1, 2, 1 }, tensorA, 3.0f);
    SetData<float>({ 1, 1, 2, 2 }, tensorA, 3.0f);

    SetData<float>({ 1, 1, 0, 0 }, tensorB, 3.0f);
    SetData<float>({ 1, 1, 0, 1 }, tensorB, 3.0f);
    SetData<float>({ 1, 1, 0, 2 }, tensorB, 3.0f);
    SetData<float>({ 1, 1, 1, 0 }, tensorB, 3.0f);
    SetData<float>({ 1, 1, 1, 1 }, tensorB, 3.0f);
    SetData<float>({ 1, 1, 1, 2 }, tensorB, 3.0f);
    SetData<float>({ 1, 1, 2, 0 }, tensorB, 3.0f);
    SetData<float>({ 1, 1, 2, 1 }, tensorB, 3.0f);
    SetData<float>({ 1, 1, 2, 2 }, tensorB, 3.0f);

    Tensor output = Tensor::CreateTensor({ 2, 2, 3, 3 }, NumberSystem::Float,
                                         device);

    Compute::Multiply(tensorA, tensorB, output);

    for (std::size_t batchIdx = 0; batchIdx < 2; ++batchIdx)
        for (std::size_t channelIdx = 0; channelIdx < 2; ++channelIdx)
            for (std::size_t rowIdx = 0; rowIdx < 3; rowIdx++)
            {
                for (std::size_t colIdx = 0; colIdx < 3; colIdx++)
                {
                    const auto num = GetData<float>(
                        { batchIdx, channelIdx, rowIdx, colIdx }, output);
                    EXPECT_EQ(num, 27);
                    std::cout << num << " ";
                }
                std::cout << std::endl;
            }
}

} // namespace CubbyDNN
