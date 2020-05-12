// Copyright (c) 2019 Chris Ohk, Justin Kim

// We are making my contributions/submissions to this project solely in our
// personal capacity and are not conveying any rights to any intellectual
// property of any third parties.

#ifndef CUBBYDNN_UNITMETADATA_HPP
#define CUBBYDNN_UNITMETADATA_HPP

#include <cubbydnn/Units/UnitType.hpp>
#include <cubbydnn/Utils/Shape.hpp>
#include <vector>
#include <cubbydnn/Utils/Declarations.hpp>
#include <cubbydnn/Computations/Initializers/InitializerType.hpp>

namespace CubbyDNN::Graph
{
class UnitMetaData
{
public:
    UnitMetaData(UnitId unitId, std::vector<Shape> inputShapeVector,
                 Shape outputShape,
                 std::vector<UnitId> inputUnitIdVector,
                 std::vector<UnitId> outputUnitIdVector,
                 std::vector<Initializer> initializerVector,
                 NumberSystem numericType,
                 std::size_t padSize);

    ~UnitMetaData() = default;

    UnitMetaData(const UnitMetaData& unitMetaData) = default;
    UnitMetaData(UnitMetaData&& unitMetaData) noexcept = default;
    UnitMetaData& operator=(const UnitMetaData& unitMetaData) = default;
    UnitMetaData& operator=(UnitMetaData&& unitMetaData) noexcept = default;

    [[nodiscard]] UnitId Id() const;

    [[nodiscard]] std::vector<Shape> InputShapeVector() const;

    [[nodiscard]] Shape OutputShape() const;

    [[nodiscard]] std::vector<UnitId> InputUnitVector() const;

    [[nodiscard]] std::vector<UnitId> OutputUnitVector() const;

    NumberSystem NumericType;
    std::size_t PadSize;

private:
    UnitId m_unitId;
    std::vector<Shape> m_inputShapeVector;
    Shape m_outputShape;

    std::vector<UnitId> m_inputUnitVector;
    std::vector<UnitId> m_outputUnitVector;
    std::vector<Initializer> m_initializerVector;
};
}
#endif
