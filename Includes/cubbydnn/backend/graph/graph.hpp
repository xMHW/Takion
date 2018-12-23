
/**
 * Created by Justin on 18. 11. 13.
 *
 * @brief This file contains definitions of methods that adds operations to the
 * graph
 */

#ifndef CUBBYDNN_GENERATOR_TENSOR_HPP
#define CUBBYDNN_GENERATOR_TENSOR_HPP

#include "backend/graph_decl/graph_decl.hpp"

namespace cubby_dnn
{
template <typename T>
tensor<T> generate<T>::placeholder(const tensor_shape& shape, stream<T>& stream,
                                   const std::string& name)
{
    if (!shape::check_shape(shape, name))
    {
        return get_default_tensor();
    }

    long operation_id = operation_management<T>::operation_vector_size();
    tensor<T> output_tensor(shape, operation_id, false);
    /// declare empty operation for later use
    auto new_op = placeholder_op<T>(operation_id, shape, stream, name);
    operation_management<T>::add_operation(new_op);
    adjacency_management<T>::add_operation_to_adjacency(new_op.get_id());

    return output_tensor;
}

template <typename T>
tensor<T> generate<T>::variable(const tensor_shape& shape, bool trainable,
                                const std::string& name)
{
    if (!shape::check_shape(shape, name))
    {
        return get_default_tensor();  // check if shape is valid
    }

    long operation_id = operation_management<T>::operation_vector_size();

    tensor<T> output_tensor(shape, operation_id, false);

    if (!trainable)
        output_tensor.make_constant();

    /// declare empty operation for later use
    auto new_op = weight_op<T>(operation_id, shape, name);
    operation_management<T>::add_operation(new_op);
    adjacency_management<T>::add_operation_to_adjacency(new_op.get_id());

    return output_tensor;
}

template <typename T>
tensor<T> operate<T>::get_default_tensor()
{
    return tensor<T>(tensor_shape(), -1, "default Tensor due to error");
}

template <typename T>
tensor<T> operate<T>::mat_mul(tensor<T>& tensor1, tensor<T>& tensor2,
                              const std::string& name)
{
    std::vector<tensor<T>> tensor_vect;

    if (!tensor1.is_valid() || !tensor2.is_valid())
    {
        return get_default_tensor();
    }

    /// number of rows of first tensor should be identical to number of
    /// columns of second tensor
    if (tensor1.get_shape().cols() != tensor2.get_shape().rows() ||
        tensor1.get_shape().dimension() != tensor2.get_shape().dimension())
    {
        std::string msg = "tensor shapes doesn't match for multiplication";
        terminal::print_error(err_type::shape_mismatch,
                              "operate<T>::mat_mul, " + name, msg);
        return get_default_tensor();
    }

    auto this_id = operation_management<T>::operation_vector_size();

    tensor1.add_to(this_id);
    tensor2.add_to(this_id);
    // TODO: find way to initialize the default data

    auto tensor_data1 =
        tensor_object<T>(tensor1.get_data_size(), tensor1.get_shape(),
                         tensor1.get_from(), this_id);

    auto tensor_data2 =
        tensor_object<T>(tensor2.get_data_size(), tensor2.get_shape(),
                         tensor2.get_from(), this_id);

    if (!tensor1.is_mutable())
        tensor_data1.set_constant();
    if (!tensor2.is_mutable())
        tensor_data1.set_constant();

    long tensor_data1_id =
        tensor_object_management<T>::add_tensor_object(std::move(tensor_data1));
    long tensor_data2_id =
        tensor_object_management<T>::add_tensor_object(std::move(tensor_data2));

    operation_management<T>::get_operation(tensor1.get_from())
        .add_output_tensor(tensor_data1_id);
    operation_management<T>::get_operation(tensor2.get_from())
        .add_output_tensor(tensor_data2_id);

    tensor_shape new_shape(tensor1.get_shape().rows(),
                           tensor2.get_shape().cols(),
                           tensor1.get_shape().dimension());

    tensor<T> output_tensor(new_shape, this_id, false);
    mat_mul_op<T> mat_mul_op(this_id, name);
    mat_mul_op.add_input_tensor(tensor_data1_id);
    mat_mul_op.add_input_tensor(tensor_data2_id);
    operation_management<T>::add_operation(mat_mul_op);
    adjacency_management<T>::add_operation_to_adjacency(mat_mul_op.get_id());

    return output_tensor;
}

template <typename T>
tensor<T> operate<T>::mat_add(tensor<T>& tensor1, tensor<T>& tensor2,
                              const std::string& name)
{
    if (!tensor1.is_valid() || !tensor2.is_valid())
    {
        return get_default_tensor();
    }

    /// check: both tensors should have identical shape
    if (tensor1.get_shape() != tensor2.get_shape())
    {
        std::string msg = "tensor shapes doesn't match for Addition";
        terminal::print_error(err_type::shape_mismatch,
                              "operate<T>::mat_add, " + name, msg);
        return get_default_tensor();
    }

    auto this_id = operation_management<T>::operation_vector_size();

    tensor1.add_to(this_id);
    tensor2.add_to(this_id);
    // TODO: find way to initialize the default data

    auto tensor_data1 =
        tensor_object<T>(tensor1.get_data_size(), tensor1.get_shape(),
                         tensor1.get_from(), this_id);

    auto tensor_data2 =
        tensor_object<T>(tensor2.get_data_size(), tensor2.get_shape(),
                         tensor2.get_from(), this_id);

    if (!tensor1.is_mutable())
        tensor_data1.set_constant();
    if (!tensor2.is_mutable())
        tensor_data1.set_constant();

    long tensor_data1_id =
        tensor_object_management<T>::add_tensor_object(std::move(tensor_data1));
    long tensor_data2_id =
        tensor_object_management<T>::add_tensor_object(std::move(tensor_data2));

    operation_management<T>::get_operation(tensor1.get_from())
        .add_output_tensor(tensor_data1_id);
    operation_management<T>::get_operation(tensor2.get_from())
        .add_output_tensor(tensor_data2_id);

    tensor_shape new_shape = tensor1.get_shape();

    tensor<T> output_tensor(new_shape, this_id, false);

    mat_add_op<T> mat_add_op(this_id, name);
    mat_add_op.add_input_tensor(tensor_data1_id);
    mat_add_op.add_input_tensor(tensor_data2_id);
    operation_management<T>::add_operation(mat_add_op);
    adjacency_management<T>::add_operation_to_adjacency(mat_add_op.get_id());
    return output_tensor;
}

template <typename T>
tensor<T> operate<T>::mat_dot(tensor<T>& tensor1, T multiplier,
                              const std::string& name)
{
    if (!tensor1.is_valid())
    {
        return get_default_tensor();
    }

    auto this_id = operation_management<T>::operation_vector_size();

    tensor1.add_to(this_id);
    // TODO: find way to initialize the default data

    auto tensor_data1 =
        tensor_object<T>(tensor1.get_data_size(), tensor1.get_shape(),
                         tensor1.get_from(), this_id);

    if (!tensor1.is_mutable())
        tensor_data1.set_constant();

    long tensor_data1_id =
        tensor_object_management<T>::add_tensor_object(std::move(tensor_data1));
    operation_management<T>::get_operation(tensor1.get_from())
        .add_output_tensor(tensor_data1_id);

    tensor_shape new_shape = tensor1.get_shape();

    tensor<T> output_tensor(new_shape, this_id, false);
    mat_dot_op<T> mat_dot_op(this_id, name, multiplier);
    mat_dot_op.add_input_tensor(tensor_data1_id);
    operation_management<T>::add_operation(mat_dot_op);
    adjacency_management<T>::add_operation_to_adjacency(mat_dot_op.get_id());

    return output_tensor;
}

template <typename T>
tensor<T> operate<T>::reshape(tensor<T>& tensor1, const tensor_shape& shape,
                              const std::string& name)
{
    if (!tensor1.is_valid())
    {
        return get_default_tensor();
    }

    if (!shape::check_shape(tensor1.get_shape(), name))
    {
        std::string msg = "tensor shapes doesn't match for reshaping";
        terminal::print_error(err_type::shape_mismatch,
                              "operate<T>::reshape, " + name, msg);
        return get_default_tensor();
    }

    if (tensor1.get_data_size() != shape.size())
    {
        std::string msg =
            std::string("size of new shape doesn't match for reshaping") +
            "new size: " + std::to_string(shape.size()) +
            "original size: " + std::to_string(tensor1.get_data_size());
        terminal::print_error(err_type::shape_mismatch,
                              "operate<T>::reshape, " + name, msg);
    }

    auto this_id = operation_management<T>::operation_vector_size();

    tensor1.add_to(this_id);
    // TODO: find way to initialize the default data

    auto tensor_data1 =
        tensor_object<T>(tensor1.get_data_size(), tensor1.get_shape(),
                         tensor1.get_from(), this_id);

    if (!tensor1.is_mutable())
        tensor_data1.set_constant();

    long tensor_data1_id =
        tensor_object_management<T>::add_tensor_object(std::move(tensor_data1));
    operation_management<T>::get_operation(tensor1.get_from())
        .add_output_tensor(tensor_data1_id);

    tensor_shape new_shape = shape;

    tensor<T> output_tensor(new_shape, this_id, false);
    reshape_op<T> reshape_op(this_id, name, shape);
    reshape_op.add_input_tensor(tensor_data1_id);
    operation_management<T>::add_operation(reshape_op);
    adjacency_management<T>::add_operation_to_adjacency(reshape_op.get_id());

    return output_tensor;
}

template <typename T>
tensor<T> operate<T>::one_hot(tensor<T>& tensor1, size_t size,
                              const std::string& name)
{
    if (!tensor1.is_valid())
    {
        return get_default_tensor();
    }

    if (!shape::check_shape(tensor1.get_shape(), name))
    {
        std::string msg = "tensor shapes doesn't match for reshaping";
        terminal::print_error(err_type::shape_mismatch,
                              "operate<T>::reshape, " + name, msg);
        return get_default_tensor();
    }

    if (tensor1.get_data_size() != size)
    {
        std::string msg =
            std::string("size of new shape doesn't match for reshaping") +
            "new size: " + std::to_string(size) +
            "original size: " + std::to_string(tensor1.get_data_size());
        terminal::print_error(err_type::shape_mismatch,
                              "operate<T>::one_hot, " + name, msg);
    }

    auto this_id = operation_management<T>::operation_vector_size();

    auto tensor_data1 =
        tensor_object<T>(tensor1.get_data_size(), tensor1.get_shape(),
                         tensor1.get_from(), this_id);

    if (!tensor1.is_mutable())
        tensor_data1.set_constant();

    long tensor_data1_id =
        tensor_object_management<T>::add_tensor_object(std::move(tensor_data1));
    operation_management<T>::get_operation(tensor1.get_from())
        .add_output_tensor(tensor_data1_id);

    tensor_shape new_shape(size, 1, 1);

    tensor<T> output_tensor(new_shape, this_id, false);
    reshape_op<T> one_hot_op(this_id, name, new_shape);
    one_hot_op.add_input_tensor(tensor_data1_id);
    operation_management<T>::add_operation(one_hot_op);
    adjacency_management<T>::add_operation_to_adjacency(one_hot_op.get_id());

    return output_tensor;
}  // namespace cubby_dnn

template <typename T>
void final<T>::wrapper(tensor<T>& tensor1, const std::string& name)
{
    if (!tensor1.is_valid())
    {
        return;
    }

    auto this_id = operation_management<T>::operation_vector_size();

    tensor1.add_to(this_id);
    // TODO: find way to initialize the default data

    auto tensor_data1 =
        tensor_object<T>(tensor1.get_data_size(), tensor1.get_shape(),
                         tensor1.get_from(), this_id);

    if (!tensor1.is_mutable())
        tensor_data1.set_constant();

    long tensor_data1_id =
        tensor_object_management<T>::add_tensor_object(std::move(tensor_data1));
    operation_management<T>::get_operation(tensor1.get_from())
        .add_output_tensor(tensor_data1_id);

    wrapper_op<T> wrapper_op(this_id, name);
    wrapper_op.add_input_tensor(tensor_data1_id);
    operation_management<T>::add_operation(wrapper_op);
    adjacency_management<T>::add_operation_to_adjacency(wrapper_op.get_id());
}
}  // namespace cubby_dnn

#endif  // CUBBYDNN_GENERATOR_TENSOR_HPP