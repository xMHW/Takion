// Copyright (c) 2019 Chris Ohk, Justin Kim

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#ifndef CUBBYDNN_COMPUTABLEUNIT_HPP
#define CUBBYDNN_COMPUTABLEUNIT_HPP

#include <atomic>
#include <cstdlib>
#include <iostream>
#include <utility>
#include <vector>

#include <cubbydnn/Tensors/Tensor.hpp>
#include <cubbydnn/Tensors/TensorInfo.hpp>
#include <cubbydnn/Utils/SharedPtr-impl.hpp>

namespace CubbyDNN
{
//! UnitState
//! Wrapper class containing the state and StateNum
//! This represents the execution state of computable Unit
struct UnitState
{
    explicit UnitState();
    /// State number of current
    std::atomic<std::size_t> StateNum = 0;
    /// True if unit is already in the task queue
    std::atomic<bool> IsBusy = false;
};

class ComputableUnit
{
 public:
    //! Default constructor
    //! Initializes unitInfo with pending state
    //! \param inputSize : size of the input for this unit
    //! \param outputSize : size of the output for this unit
    ComputableUnit(size_t inputSize, size_t outputSize);

    //! Move constructor
    //! \param computableUnit : ComputableUnit to move from
    ComputableUnit(ComputableUnit&& computableUnit) noexcept;

    virtual ~ComputableUnit() = default;

    //! Called before computation for acquiring the unit in order to compute
    //! Marks IsBusy as True in order to prevent same tasks being enqueued
    //! multiple times
    void AcquireUnit()
    {
        setBusy();
    }

    //! Called after computation for releasing the unit after computation
    //! Increments the stateNum and marks IsBusy as false
    void ReleaseUnit()
    {
        incrementStateNum();
        setReleased();
    }

    //! Brings back if executableUnit is ready to be executed
    //! \return : whether corresponding unit is ready to be executed
    virtual bool IsReady() = 0;

    //! Method that is executed on the engine
    //! This method must be called after checking computation is ready
    virtual void Compute() = 0;

    //! Brings back reference of the atomic state counter for atomic comparison
    //! of state counter
    //! \return : reference of the state counter
    std::atomic<std::size_t>& GetStateNum();

    virtual Tensor& GetInputTensor(size_t index) = 0;
    virtual Tensor& GetOutputTensor(size_t index) = 0;

 protected:
    //! increments state number after execution
    void incrementStateNum()
    {
        m_unitState.StateNum.fetch_add(1, std::memory_order_relaxed);
        std::cout << "Increment" << std::endl;
    }

    //! Atomically sets isBusy to true
    void setBusy()
    {
        std::atomic_exchange_explicit(&m_unitState.IsBusy, true,
                                      std::memory_order_relaxed);
    }

    //! Atomically sets operation state to pending state (false)
    void setReleased()
    {
        std::atomic_exchange_explicit(&m_unitState.IsBusy, false,
                                      std::memory_order_relaxed);
    }

    /// UnitState object indicates execution state of ComputableUnit
    UnitState m_unitState;
    /// ptr to units to receive result from
    std::vector<ComputableUnit*> m_inputPtrVector;
    /// ptr to units to write result
    std::vector<ComputableUnit*> m_outputPtrVector;
    /// vector to log states for debugging purpose
    std::vector<std::string> m_logVector;

    size_t m_inputIndex = 0;
    size_t m_outputIndex = 0;

    Tensor tensor = Tensor(nullptr, TensorInfo({ 0 }));
};
}  // namespace CubbyDNN

#endif  // CUBBYDNN_COMPUTABLEUNIT_HPP