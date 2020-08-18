// Copyright (c) 2020, Jaewoo Kim

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#ifndef TAKION_GRAPH_DECL_HPP
#define TAKION_GRAPH_DECL_HPP

#include <Takion/Computations/GEMM/MathKernel.hpp>
#include <Takion/Units/HiddenUnits/Activations/SoftMaxDecl.hpp>

namespace Takion::Graph
{
template <typename T>
SoftMax<T>::SoftMax(const UnitId& unitId, UnitId sourceUnitId,
                    Tensor<T> forwardInput,
                    std::unordered_map<UnitId, Tensor<T>> backwardInputVector,
                    Tensor<T> forwardOutput, Tensor<T> backwardOutput,
                    std::unordered_map<std::string, Tensor<T>>
                    internalTensorMap, Compute::Device device,
                    std::size_t batchSize)
    : ComputableUnit<T>(unitId, { { sourceUnitId, std::move(forwardInput) } },
                        std::move(backwardInputVector),
                        std::move(forwardOutput),
                        { { sourceUnitId, std::move(backwardOutput) } },
                        std::move(internalTensorMap), batchSize),
      m_sourceUnitId(std::move(sourceUnitId)),
      m_device(std::move(device))
{
}

template <typename T>
SoftMax<T> SoftMax<T>::CreateUnit(const FrontEnd::UnitMetaData<T>& unitMetaData)
{
    const auto unitId = unitMetaData.Id();
    const auto batchSize = unitMetaData.BatchSize();
    const auto device = unitMetaData.Device;
    const auto inputShape = unitMetaData.GetInputShape("input");
    const auto outputShape = unitMetaData.GetOutputShape();

    SoftMax<T>::m_checkArguments(inputShape, outputShape, unitId.UnitName);

    auto sourceUnitId = unitMetaData.GetInputUnitId("input");

    Tensor<T> forwardInputTensor(inputShape, batchSize, device);

    std::unordered_map<UnitId, Tensor<T>> backwardInputMap;
    for (const auto& backwardInputUnitId : unitMetaData.OutputUnitVector())
    {
        Tensor<T> tensor(inputShape, batchSize, device);
        backwardInputMap[backwardInputUnitId] = std::move(tensor);
    }

    Tensor<T> forwardOutputTensor(outputShape, batchSize, device);
    Tensor<T> backwardOutputTensor(inputShape, batchSize, device);
    Tensor<T> backwardTempTensor(outputShape, batchSize, device);

    auto activationUnit =
        SoftMax<T>(unitMetaData.Id(), sourceUnitId,
                   std::move(forwardInputTensor),
                   std::move(backwardInputMap), std::move(forwardOutputTensor),
                   std::move(backwardOutputTensor),
                   { { "backwardTemp", backwardTempTensor } }, device,
                   batchSize);

    return activationUnit;
}

template <typename T>
void SoftMax<T>::Forward()
{
    const auto batchSize = ComputableUnit<T>::BatchSize;
    const auto shape = ForwardOutput.TensorShape;
    const auto size = shape.Size();
    const Tensor<T>& inputTensor = ForwardInputMap[m_sourceUnitId];

#pragma omp parallel for schedule(static)
    for (long batchIdx = 0; batchIdx < static_cast<long>(batchSize); ++batchIdx)
    {
        T sum = static_cast<T>(0);
        for (std::size_t idx = 0; idx < size; ++idx)
        {
            const auto index = batchIdx * size + idx;
            sum += static_cast<T>(std::exp(inputTensor.At(index)));
        }
        for (std::size_t idx = 0; idx < size; ++idx)
        {
            const auto index = batchIdx * size + idx;
            ForwardOutput.At(index) =
                static_cast<T>(std::exp(inputTensor.At(index) / sum));
        }
    }
}

template <typename T>
void SoftMax<T>::AsyncForward(std::promise<bool> promise)
{
    const auto batchSize = ComputableUnit<T>::BatchSize;
    const auto shape = ForwardOutput.TensorShape;
    const auto size = shape.Size();
    const Tensor<T>& inputTensor = ForwardInputMap[m_sourceUnitId];

#pragma omp parallel for schedule(static)
    for (long batchIdx = 0; batchIdx < static_cast<long>(batchSize);
         ++batchIdx)
    {
        T sum = static_cast<T>(0);
        for (std::size_t idx = 0; idx < size; ++idx)
        {
            const auto index = batchIdx * size + idx;
            sum += static_cast<T>(std::exp(inputTensor.At(index)));
        }
        for (std::size_t idx = 0; idx < size; ++idx)
        {
            const auto index = batchIdx * size + idx;
            ForwardOutput.At(index) =
                static_cast<T>(std::exp(inputTensor.At(index) / sum));
        }
    }

    promise.set_value(true);
}

template <typename T>
void SoftMax<T>::Backward()
{
    const Zeros<T> zeroInitializer;

    const auto batchSize = ComputableUnit<T>::BatchSize;
    const auto shape = ForwardOutput.TensorShape;
    const auto size = shape.Size();
    Tensor<T>& backwardTemp = InternalTensorMap["backwardTemp"];
    Tensor<T>& backwardOutput = BackwardOutputMap[m_sourceUnitId];

    zeroInitializer.Initialize(backwardTemp);

    for (const auto& [unitId, tensor] : BackwardInputMap)
    {
        Compute::Add(tensor, backwardTemp);
    }

    if (m_device.Type() == Compute::DeviceType::CPU)
    {
        for (std::size_t batchIdx = 0; batchIdx < batchSize; ++batchIdx)
        {
            for (std::size_t idxOut = 0; idxOut < size; ++idxOut)
            {
                const auto backwardOutputIdx = batchIdx * size + idxOut;
                for (std::size_t idxIn = 0; idxIn < size; ++idxIn)
                {
                    const auto backwardInputIdx = batchIdx * size + idxIn;
                    T sum = static_cast<T>(0);

                    if (idxIn == idxOut)
                    {
                        const auto prevOutput =
                            ForwardOutput.At(backwardInputIdx);
                        const auto derivative = prevOutput * (1 - prevOutput);
                        const auto val =
                            backwardTemp.At(backwardInputIdx) * derivative;
                        sum += val;
                    }
                    else
                    {
                        const auto prevOutput =
                            ForwardOutput.At(backwardInputIdx);
                        const auto derivative = -prevOutput * prevOutput;
                        const auto val =
                            backwardTemp.At(backwardInputIdx) * derivative;
                        sum += val;
                    }

                    backwardOutput.At(backwardOutputIdx) = sum;
                }
            }
        }
    }
    else
    {
        throw std::runtime_error("Not implemented");
    }
}

template <typename T>
void SoftMax<T>::AsyncBackward(std::promise<bool> promise)
{
    const Zeros<T> zeroInitializer;

    const auto batchSize = ComputableUnit<T>::BatchSize;
    const auto shape = ForwardOutput.TensorShape;
    const auto size = shape.Size();
    Tensor<T>& backwardTemp = InternalTensorMap["backwardTemp"];
    Tensor<T>& backwardOutput = BackwardOutputMap[m_sourceUnitId];

    zeroInitializer.Initialize(backwardTemp);

    for (const auto& [unitId, tensor] : BackwardInputMap)
    {
        Compute::Add(tensor, backwardTemp);
    }

    if (m_device.Type() == Compute::DeviceType::CPU)
    {
        for (std::size_t batchIdx = 0; batchIdx < batchSize; ++batchIdx)
        {
            for (std::size_t idxOut = 0; idxOut < size; ++idxOut)
            {
                const auto backwardOutputIdx = batchIdx * size + idxOut;
                for (std::size_t idxIn = 0; idxIn < size; ++idxIn)
                {
                    const auto backwardInputIdx = batchIdx * size + idxIn;
                    T sum = static_cast<T>(0);

                    if (idxIn == idxOut)
                    {
                        const auto prevOutput =
                            ForwardOutput.At(backwardInputIdx);
                        const auto derivative = prevOutput * (1 - prevOutput);
                        const auto val =
                            backwardTemp.At(backwardInputIdx) * derivative;
                        sum += val;
                    }
                    else
                    {
                        const auto prevOutput =
                            ForwardOutput.At(backwardInputIdx);
                        const auto derivative = -prevOutput * prevOutput;
                        const auto val =
                            backwardTemp.At(backwardInputIdx) * derivative;
                        sum += val;
                    }

                    backwardOutput.At(backwardOutputIdx) = sum;
                }
            }
        }
    }
    else
    {
        throw std::runtime_error("Not implemented");
    }

    promise.set_value(true);
}

template <typename T>
void SoftMax<T>::m_checkArguments(const Shape& inputShape,
                                  const Shape& outputShape,
                                  const std::string& unitName)
{
    if (inputShape != outputShape)
    {
        const std::string errorMessage =
            std::string("SoftMax " + unitName) +
            " - Shape mismatch between input and output." +
            " input : " + inputShape.ToString() +
            " output : " + outputShape.ToString();

        throw std::runtime_error(errorMessage);
    }
}
}

#endif