// Copyright (c) 2019 Chris Ohk, Justin Kim

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#include <cubbydnn/Engine/Engine.hpp>
#include <cassert>

namespace CubbyDNN
{
std::thread Engine::m_scanMainThread;

std::thread Engine::m_scanCopyThread;

std::vector<std::thread> Engine::m_mainThreadPool = std::vector<std::thread>();

std::vector<std::thread> Engine::m_copyThreadPool;

SpinLockQueue<TaskWrapper> Engine::m_taskQueue(20);

bool Engine::m_active = true;

std::vector<SharedPtr<SourceUnit>> Engine::m_sourceUnitVector;

std::vector<SharedPtr<SinkUnit>> Engine::m_sinkUnitVector;

std::vector<SharedPtr<HiddenUnit>> Engine::m_hiddenUnitVector;

std::vector<SharedPtr<CopyUnit>> Engine::m_copyUnitVector;

std::size_t Engine::m_maxEpochs = 0;

std::atomic_bool Engine::m_ready = false;

void Engine::Execute(std::size_t epochs)
{
    bool isFinished = false;
    m_maxEpochs = epochs;

    while (m_active && !isFinished)
    {
        isFinished = true;
        for (auto& sourceUnit : m_sourceUnitVector)
        {
            if (sourceUnit->IsReady() && sourceUnit->GetStateNum() < epochs)
            {
                sourceUnit->Compute();
                sourceUnit->ReleaseUnit();
                isFinished = false;
            }
        }

        for (auto& hiddenUnit : m_hiddenUnitVector)
        {
            if (hiddenUnit->IsReady() && hiddenUnit->GetStateNum() < epochs)
            {
                hiddenUnit->Compute();
                hiddenUnit->ReleaseUnit();
                isFinished = false;
            }
        }

        for (auto& sinkUnit : m_sinkUnitVector)
        {
            if (sinkUnit->IsReady() && sinkUnit->GetStateNum() < epochs)
            {
                sinkUnit->Compute();
                sinkUnit->ReleaseUnit();
                isFinished = false;
            }
        }

        for (auto& copyUnit : m_copyUnitVector)
        {
            if (copyUnit->IsReady() && copyUnit->GetStateNum() < epochs)
            {
                copyUnit->Compute();
                copyUnit->ReleaseUnit();
                isFinished = false;
            }
        }
    }
}

void Engine::ExecuteParallel(std::size_t workers, std::size_t epochs)
{
    const auto hardwareConcurrency = std::thread::hardware_concurrency();
    if (workers + 1 > hardwareConcurrency)
    {
        std::cout << "This computer has only " << hardwareConcurrency
            << " threads available, but total " << workers
            << " threads were requested" << std::endl;
    }

    if (hardwareConcurrency < workers)
        workers = hardwareConcurrency;

    std::cout << "Creating " << workers << " workers" << std::endl;

    for (std::size_t count = 0; count < workers; ++count)
        m_mainThreadPool.emplace_back(std::thread(m_run));

    while (!m_IsComplete(epochs))
    {
        m_executeComputeUnits();
        m_executeCopyUnits();
    }

    for (std::size_t count = 0; count < m_mainThreadPool.size(); ++count)
    {
        TaskWrapper taskWrapper(TaskType::Join);
        m_taskQueue.Enqueue(std::move(taskWrapper));
    }
}

UnitIdentifier Engine::Source(const TensorInfo& outputTensorInfo,
                              std::size_t numberOfOutputs)
{
    const auto unitId = m_sourceUnitVector.size();
    m_sourceUnitVector.emplace_back(
        SharedPtr<SourceUnit>::Make(outputTensorInfo, numberOfOutputs));
    return { UnitType::Source, unitId };
}

UnitIdentifier Engine::Constant(const TensorInfo& output, void* dataPtr,
                                int numberOfOutputs)
{
    const auto unitId = m_sourceUnitVector.size();
    m_sourceUnitVector.emplace_back(
        SharedPtr<ConstantUnit>::Make(output, numberOfOutputs, dataPtr));
    return { UnitType::Source, unitId };
}

UnitIdentifier Engine::Hidden(
    const std::vector<UnitIdentifier>& previousUnitVector,
    TensorInfo outputTensorInfo, std::size_t numberOfOutputs)
{
    std::vector<TensorInfo> inputTensorInfoVector;
    inputTensorInfoVector.reserve(previousUnitVector.size());
    for (const auto& unitIdentifier : previousUnitVector)
    {
        if (unitIdentifier.Type == UnitType::Hidden)
            inputTensorInfoVector.emplace_back(
                m_hiddenUnitVector.at(unitIdentifier.ID)
                                  ->GetOutputTensorInfo());
        if (unitIdentifier.Type == UnitType::Source)
            inputTensorInfoVector.emplace_back(
                m_sourceUnitVector.at(unitIdentifier.ID)
                                  ->GetOutputTensorInfo());
    }

    const auto unitId = m_hiddenUnitVector.size();
    m_hiddenUnitVector.emplace_back(SharedPtr<HiddenUnit>::Make(
        inputTensorInfoVector, outputTensorInfo, numberOfOutputs));
    const UnitIdentifier unitIdentifier = { UnitType::Hidden, unitId };
    m_connectWithPreviousUnit(previousUnitVector, unitIdentifier);
    return unitIdentifier;
}

UnitIdentifier Engine::Multiply(const UnitIdentifier& unitA,
                                const UnitIdentifier& unitB,
                                std::size_t numberOfOutputs)
{
    const auto unitId = m_hiddenUnitVector.size();

    TensorInfo tensorInfoA;
    TensorInfo tensorInfoB;

    if (unitA.Type == UnitType::Hidden)
        tensorInfoA = m_hiddenUnitVector.at(unitA.ID)->GetOutputTensorInfo();
    if (unitA.Type == UnitType::Source)
        tensorInfoA = m_sourceUnitVector.at(unitA.ID)->GetOutputTensorInfo();

    if (unitB.Type == UnitType::Hidden)
        tensorInfoB = m_hiddenUnitVector.at(unitB.ID)->GetOutputTensorInfo();
    if (unitB.Type == UnitType::Source)
        tensorInfoB = m_sourceUnitVector.at(unitB.ID)->GetOutputTensorInfo();

    const auto shapeA = tensorInfoA.GetShape();
    const auto shapeB = tensorInfoB.GetShape();

    if (shapeA.Col() != shapeB.Row())
        throw std::runtime_error("Multiply-shape mismatch");
    if (shapeA.BatchSize() != shapeB.BatchSize())
        throw std::runtime_error("Batch size mismatch");

    const Shape outputShape = shapeA * shapeB;

    m_hiddenUnitVector.emplace_back(SharedPtr<MatMul>::Make(
        tensorInfoA, tensorInfoB,TensorInfo(outputShape) , numberOfOutputs));

    const UnitIdentifier unitIdentifier = { UnitType::Hidden, unitId };
    m_connectWithPreviousUnit({ unitA, unitB }, unitIdentifier);
    return unitIdentifier;
}

void Engine::Sink(const std::vector<UnitIdentifier>& previousUnit,
                  const std::vector<TensorInfo>& inputTensorInfoVector)
{
    const auto unitId = m_sinkUnitVector.size();
    m_sinkUnitVector.emplace_back(
        SharedPtr<SinkUnit>::Make(inputTensorInfoVector));
    const UnitIdentifier unitIdentifier = { UnitType::Sink, unitId };
    m_connectWithPreviousUnit(previousUnit, unitIdentifier);
}

UnitIdentifier Engine::OutputTest(
    const UnitIdentifier& previousUnit,
    const std::function<void(const Tensor&, std::size_t)>& testFunction)
{
    const auto unitId = m_sinkUnitVector.size();
    TensorInfo previousTensorInfo;

    if (previousUnit.Type == UnitType::Hidden)
        previousTensorInfo =
            m_hiddenUnitVector.at(previousUnit.ID)->GetOutputTensorInfo();
    else
        previousTensorInfo =
            m_sourceUnitVector.at(previousUnit.ID)->GetOutputTensorInfo();

    const auto sinkUnitPtr =
        SharedPtr<SinkTestUnit>::Make(previousTensorInfo, testFunction);
    m_sinkUnitVector.emplace_back(sinkUnitPtr);
    const UnitIdentifier unitIdentifier = { UnitType::Sink, unitId };
    m_connectWithPreviousUnit({ previousUnit }, unitIdentifier);
    return unitIdentifier;
}

void Engine::m_connectSourceToHidden(std::size_t originID, std::size_t destID,
                                     std::size_t destInputIndex)
{
    assert(originID < m_sourceUnitVector.size());
    assert(destID < m_hiddenUnitVector.size());
    auto sourceUnit = m_sourceUnitVector.at(originID);
    auto intermediateUnit = m_hiddenUnitVector.at(destID);
    m_copyUnitVector.emplace_back(SharedPtr<CopyUnit>::Make());
    auto copyUnit = m_copyUnitVector.at(m_copyUnitVector.size() - 1);
    copyUnit->SetInputPtr(sourceUnit);
    copyUnit->SetOutputPtr(intermediateUnit);
    const auto inputIndex = sourceUnit->AddOutputPtr(copyUnit);
    intermediateUnit->AddInputPtr(copyUnit, destInputIndex);
    copyUnit->SetInputTensorIndex(inputIndex);
    copyUnit->SetOutputTensorIndex(destInputIndex);
}

void Engine::m_connectHiddenToHidden(std::size_t originID, std::size_t destID,
                                     std::size_t destInputIndex)
{
    assert(originID < m_hiddenUnitVector.size());
    assert(destID < m_hiddenUnitVector.size());
    auto originIntermediateUnit = m_hiddenUnitVector.at(originID);
    auto destIntermediateUnit = m_hiddenUnitVector.at(destID);
    m_copyUnitVector.emplace_back(SharedPtr<CopyUnit>::Make());
    auto copyUnit = m_copyUnitVector.at(m_copyUnitVector.size() - 1);
    copyUnit->SetInputPtr(originIntermediateUnit);
    copyUnit->SetOutputPtr(destIntermediateUnit);
    const auto inputIndex = originIntermediateUnit->AddOutputPtr(copyUnit);
    destIntermediateUnit->AddInputPtr(copyUnit, destInputIndex);
    copyUnit->SetInputTensorIndex(inputIndex);
    copyUnit->SetOutputTensorIndex(destInputIndex);
}

void Engine::m_connectHiddenToSink(std::size_t originID, std::size_t destID,
                                   std::size_t destInputIndex)
{
    assert(originID < m_hiddenUnitVector.size());
    assert(destID < m_sinkUnitVector.size());
    auto hiddenUnit = m_hiddenUnitVector.at(originID);
    auto sinkUnit = m_sinkUnitVector.at(destID);
    m_copyUnitVector.emplace_back(SharedPtr<CopyUnit>::Make());
    auto copyUnit = m_copyUnitVector.at(m_copyUnitVector.size() - 1);
    copyUnit->SetInputPtr(hiddenUnit);
    copyUnit->SetOutputPtr(sinkUnit);
    const auto inputIndex = hiddenUnit->AddOutputPtr(copyUnit);
    sinkUnit->AddInputPtr(copyUnit, destInputIndex);
    copyUnit->SetInputTensorIndex(inputIndex);
    copyUnit->SetOutputTensorIndex(destInputIndex);
}

void Engine::m_connectWithPreviousUnit(
    const std::vector<UnitIdentifier>& previousUnitVector,
    UnitIdentifier subjectUnitIdentifier)
{
    std::size_t inputIdx = 0;
    if (subjectUnitIdentifier.Type == UnitType::Hidden)
        for (const auto& unit : previousUnitVector)
        {
            if (unit.Type == UnitType::Source)
            {
                m_connectSourceToHidden(unit.ID, subjectUnitIdentifier.ID,
                                        inputIdx++);
            }
            else if (unit.Type == UnitType::Hidden)
            {
                m_connectHiddenToHidden(unit.ID, subjectUnitIdentifier.ID,
                                        inputIdx++);
            }
            else
                assert("Unsupported type of unit");
        }

    else if (subjectUnitIdentifier.Type == UnitType::Sink)
        for (const auto& unit : previousUnitVector)
        {
            if (unit.Type == UnitType::Hidden)
            {
                m_connectHiddenToSink(unit.ID, subjectUnitIdentifier.ID,
                                      inputIdx++);
            }
            else
                assert("Unsupported type of unit");
        }
    else
        assert("Unsupported type of unit");
}

void Engine::m_run()
{
    // while (!m_ready)
    //    ;
    TaskWrapper taskWrapper = m_taskQueue.Dequeue();
    while (taskWrapper.Type != TaskType::Join)
    {
        //if (m_ready)
        //{
        auto task = taskWrapper.GetTask();
        task();
        //std::cout << "Execute" << std::endl;
        taskWrapper = m_taskQueue.Dequeue();
        // }
    }
}

void Engine::EnqueueTask(TaskWrapper&& task)
{
    m_taskQueue.Enqueue(std::move(task));
}

TaskWrapper Engine::DequeueTask()
{
    return m_taskQueue.Dequeue();
}

void Engine::JoinThreads()
{
    for (auto& thread : m_mainThreadPool)
    {
        if (thread.joinable())
        {
            thread.join();
            std::cout << "Joined main" << std::endl;
        }
    }

    if (m_scanMainThread.joinable())
        m_scanMainThread.join();
    if (m_scanCopyThread.joinable())
        m_scanCopyThread.join();
}

void Engine::Abort()
{
    for (std::size_t count = 0; count < m_mainThreadPool.size(); ++count)
        m_taskQueue.Enqueue(TaskWrapper(
            TaskType::Join, []()
            {
            }, []()
            {
            }));

    for (auto& thread : m_mainThreadPool)
    {
        if (thread.joinable())
            thread.join();
    }
    m_active = false;
    if (m_scanMainThread.joinable())
        m_scanMainThread.join();
    if (m_scanCopyThread.joinable())
        m_scanCopyThread.join();
}

void Engine::m_executeComputeUnits()
{
    int desired = 0;
    std::atomic_int count = 0;

    for (auto& sourceUnit : m_sourceUnitVector)
    {
        if (sourceUnit->IsReady())
        {
            const auto computeFunc = [&sourceUnit]() { sourceUnit->Compute(); };
            const auto updateState = [&sourceUnit, &count]()
            {
                count.fetch_add(1);
                sourceUnit->ReleaseUnit();
            };
            TaskWrapper taskWrapper(TaskType::ComputeSource, computeFunc,
                                    updateState);
            //std::cout << "Enqueue source" << std::endl;
            m_taskQueue.Enqueue(std::move(taskWrapper));
            ++desired;
        }
    }
    for (auto& hiddenUnit : m_hiddenUnitVector)
    {
        if (hiddenUnit->IsReady())
        {
            const auto computeFunc = [&hiddenUnit]() { hiddenUnit->Compute(); };
            const auto updateState = [&hiddenUnit, &count]()
            {
                count.fetch_add(1);
                hiddenUnit->ReleaseUnit();
            };
            TaskWrapper taskWrapper(TaskType::ComputeHidden, computeFunc,
                                    updateState);
            //std::cout << "Enqueue hidden" << std::endl;
            m_taskQueue.Enqueue(std::move(taskWrapper));
            ++desired;
        }
    }
    for (auto& sinkUnit : m_sinkUnitVector)
    {
        if (sinkUnit->IsReady())
        {
            const auto computeFunc = [&sinkUnit]() { sinkUnit->Compute(); };
            const auto updateState = [&sinkUnit, &count]()
            {
                count.fetch_add(1);
                sinkUnit->ReleaseUnit();
            };
            TaskWrapper taskWrapper(TaskType::ComputeSink, computeFunc,
                                    updateState);
            //std::cout << "Enqueue sink" << std::endl;
            m_taskQueue.Enqueue(std::move(taskWrapper));
            ++desired;
        }
    }

    m_ready.exchange(true);
    while (count.load(std::memory_order_acquire) != desired)
        std::this_thread::yield();
    m_ready.exchange(false);
}

void Engine::m_executeCopyUnits()
{
    int desired = 0;
    std::atomic_int count = 0;
    for (auto& copyUnit : m_copyUnitVector)
    {
        if (copyUnit->IsReady())
        {
            const auto computeFunc = [&copyUnit]() { copyUnit->Compute(); };
            const auto updateState = [&copyUnit, &count]()
            {
                count.fetch_add(1);
                copyUnit->ReleaseUnit();
            };

            TaskWrapper taskWrapper(TaskType::Copy, computeFunc, updateState);
            //std::cout << "Enqueue Copy" << std::endl;
            m_taskQueue.Enqueue(std::move(taskWrapper));
            ++desired;
        }
    }

    m_ready.exchange(true);
    while (count.load(std::memory_order_acquire) != desired)
        std::this_thread::yield();
    m_ready.exchange(false);
}

bool Engine::m_IsComplete(std::size_t epochs)
{
    bool isComplete = true;
    for (auto& sourceUnit : m_sourceUnitVector)
        if (sourceUnit->GetStateNum() < epochs)
            isComplete = false;
    for (auto& hiddenUnit : m_hiddenUnitVector)
        if (hiddenUnit->GetStateNum() < epochs)
            isComplete = false;
    for (auto& sinkUnit : m_sinkUnitVector)
        if (sinkUnit->GetStateNum() < epochs)
            isComplete = false;
    for (auto& copyUnit : m_copyUnitVector)
        if (copyUnit->GetStateNum() < epochs)
            isComplete = false;

    return isComplete;
}
} // namespace CubbyDNN