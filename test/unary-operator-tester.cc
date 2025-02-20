// Copyright 2024 Google LLC
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree.

#include "unary-operator-tester.h"

#include <stdint.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <random>
#include <utility>
#include <vector>

#include <gtest/gtest.h>
#include "xnnpack.h"
#include "xnnpack/buffer.h"
#include "xnnpack/math.h"
#include "replicable_random_device.h"

namespace xnnpack {

void UnaryOperatorTester::TestF16() {
  xnnpack::ReplicableRandomDevice rng;
  std::uniform_real_distribution<float> f32dist(range_f16_.first,
                                                range_f16_.second);

  xnnpack::Buffer<xnn_float16> input(XNN_EXTRA_BYTES / sizeof(xnn_float16) +
                              (batch_size() - 1) * input_stride() + channels());
  xnnpack::Buffer<xnn_float16> output((batch_size() - 1) * output_stride() +
                               channels());
  xnnpack::Buffer<float> output_ref(batch_size() * channels());
  for (size_t iteration = 0; iteration < iterations(); iteration++) {
    std::generate(input.begin(), input.end(),
                  [&]() { return f32dist(rng); });

    // Compute reference results.
    for (size_t i = 0; i < batch_size(); i++) {
      for (size_t c = 0; c < channels(); c++) {
        output_ref[i * channels() + c] =
            RefFunc(input[i * input_stride() + c]);
      }
    }

    // Create, setup, run, and destroy Square operator.
    ASSERT_EQ(xnn_status_success, xnn_initialize(/*allocator=*/nullptr));
    xnn_operator_t op = nullptr;

    const xnn_status status = CreateOpF16(0, &op);
    if (status == xnn_status_unsupported_hardware) {
      GTEST_SKIP();
    }
    ASSERT_EQ(xnn_status_success, status);
    ASSERT_NE(nullptr, op);

    // Smart pointer to automatically delete op.
    std::unique_ptr<xnn_operator, decltype(&xnn_delete_operator)> auto_op(
        op, xnn_delete_operator);

    ASSERT_EQ(xnn_status_success,
              ReshapeOpF16(op, batch_size(), channels(), input_stride(),
                           output_stride(), /*threadpool=*/nullptr));
    ASSERT_EQ(xnn_status_success, SetupOpF16(op, input.data(), output.data()));
    ASSERT_EQ(xnn_status_success, xnn_run_operator(op, /*threadpool=*/nullptr));

    // Verify results.
    for (size_t i = 0; i < batch_size(); i++) {
      for (size_t c = 0; c < channels(); c++) {
        const float y = output[i * output_stride() + c];
        const float y_ref = output_ref[i * channels() + c];
        CheckResultF16(y, y_ref, i, c, input[i * input_stride() + c]);
      }
    }
  }
}

void UnaryOperatorTester::TestF32() {
  xnnpack::ReplicableRandomDevice rng;
  std::uniform_real_distribution<float> f32dist(range_f32_.first,
                                                range_f32_.second);

  xnnpack::Buffer<float> input(XNN_EXTRA_BYTES / sizeof(float) +
                           (batch_size() - 1) * input_stride() + channels());
  xnnpack::Buffer<float> output((batch_size() - 1) * output_stride() + channels());
  xnnpack::Buffer<float> output_ref(batch_size() * channels());
  for (size_t iteration = 0; iteration < iterations(); iteration++) {
    std::generate(input.begin(), input.end(), [&]() { return f32dist(rng); });

    // Compute reference results.
    for (size_t i = 0; i < batch_size(); i++) {
      for (size_t c = 0; c < channels(); c++) {
        output_ref[i * channels() + c] = RefFunc(input[i * input_stride() + c]);
      }
    }

    // Create, setup, run, and destroy Square operator.
    ASSERT_EQ(xnn_status_success, xnn_initialize(/*allocator=*/nullptr));
    xnn_operator_t op = nullptr;

    ASSERT_EQ(xnn_status_success, CreateOpF32(0, &op));
    ASSERT_NE(nullptr, op);

    // Smart pointer to automatically delete op.
    std::unique_ptr<xnn_operator, decltype(&xnn_delete_operator)> auto_op(
        op, xnn_delete_operator);

    ASSERT_EQ(xnn_status_success,
              ReshapeOpF32(op, batch_size(), channels(), input_stride(),
                           output_stride(), /*threadpool=*/nullptr));
    ASSERT_EQ(xnn_status_success, SetupOpF32(op, input.data(), output.data()));
    ASSERT_EQ(xnn_status_success, xnn_run_operator(op, /*threadpool=*/nullptr));

    // Verify results.
    for (size_t i = 0; i < batch_size(); i++) {
      for (size_t c = 0; c < channels(); c++) {
        const float y = output[i * output_stride() + c];
        const float y_ref = output_ref[i * channels() + c];
        CheckResultF32(y, y_ref, i, c, input[i * input_stride() + c]);
      }
    }
  }
}

void UnaryOperatorTester::TestRunF32() {
  xnnpack::ReplicableRandomDevice rng;
  std::uniform_real_distribution<float> f32dist(range_f32_.first,
                                                range_f32_.second);

  xnnpack::Buffer<float> input(XNN_EXTRA_BYTES / sizeof(float) +
                           (batch_size() - 1) * input_stride() + channels());
  xnnpack::Buffer<float> output((batch_size() - 1) * output_stride() + channels());
  xnnpack::Buffer<float> output_ref(batch_size() * channels());
  for (size_t iteration = 0; iteration < iterations(); iteration++) {
    std::generate(input.begin(), input.end(), [&]() { return f32dist(rng); });

    // Compute reference results.
    for (size_t i = 0; i < batch_size(); i++) {
      for (size_t c = 0; c < channels(); c++) {
        output_ref[i * channels() + c] = RefFunc(input[i * input_stride() + c]);
      }
    }

    // Initialize and run Square Root operator.
    ASSERT_EQ(xnn_status_success, xnn_initialize(/*allocator=*/nullptr));

    ASSERT_EQ(
        xnn_status_success,
        RunOpF32(channels(), input_stride(), output_stride(), batch_size(),
                 input.data(), output.data(), 0, /*threadpool=*/nullptr));

    // Verify results.
    for (size_t i = 0; i < batch_size(); i++) {
      for (size_t c = 0; c < channels(); c++) {
        const float y = output[i * output_stride() + c];
        const float y_ref = output_ref[i * channels() + c];
        CheckResultF32(y, y_ref, i, c, input[i * input_stride() + c]);
      }
    }
  }
}

void UnaryOperatorTester::TestQS8() {
  xnnpack::ReplicableRandomDevice rng;
  auto i8rng = [&]() -> int8_t {
    return std::uniform_int_distribution<int32_t>(range_qs8_.first,
                                                  range_qs8_.second)(rng);
  };

  xnnpack::Buffer<int8_t> input((batch_size() - 1) * input_stride() + channels() +
                            XNN_EXTRA_BYTES / sizeof(int8_t));
  xnnpack::Buffer<int8_t> output((batch_size() - 1) * output_stride() + channels());
  xnnpack::Buffer<float> output_ref(batch_size() * channels());
  for (size_t iteration = 0; iteration < iterations(); iteration++) {
    std::generate(input.begin(), input.end(), i8rng);

    // Compute reference results, which are stored as un-truncated quantized
    // values.
    for (size_t i = 0; i < batch_size(); i++) {
      for (size_t c = 0; c < channels(); c++) {
        const float x = FloatFromInputQS8(input[i * input_stride() + c]);
        const float ref_x = RefFunc(x);
        output_ref[i * channels() + c] = QuantizeAsFloatQS8(ref_x);
      }
    }

    // Create, setup, run, and destroy the operator.
    ASSERT_EQ(xnn_status_success, xnn_initialize(nullptr /* allocator */));
    xnn_operator_t op = nullptr;

    ASSERT_EQ(xnn_status_success,
              CreateOpQS8(
                  static_cast<int8_t>(input_zero_point() - 0x80), input_scale(),
                  static_cast<int8_t>(output_zero_point() - 0x80),
                  output_scale(), static_cast<int8_t>(qmin() - 0x80),
                  static_cast<int8_t>(qmax() - 0x80), /*flags=*/0, &op));
    ASSERT_NE(nullptr, op);

    // Smart pointer to automatically delete `op`.
    std::unique_ptr<xnn_operator, decltype(&xnn_delete_operator)> auto_op(
        op, xnn_delete_operator);

    ASSERT_EQ(xnn_status_success, ReshapeOpQS8(op, batch_size(), channels(),
                                               input_stride(), output_stride(),
                                               /*threadpool=*/nullptr));
    ASSERT_EQ(xnn_status_success, SetupOpQS8(op, input.data(), output.data()));
    ASSERT_EQ(xnn_status_success, xnn_run_operator(op, /*threadpool=*/nullptr));

    // Verify results.
    for (size_t i = 0; i < batch_size(); i++) {
      for (size_t c = 0; c < channels(); c++) {
        const int8_t y = output[i * output_stride() + c];
        const float y_ref = output_ref[i * channels() + c];
        CheckResultQS8(y, y_ref, i, c, input[i * input_stride() + c]);
      }
    }
  }
}

void UnaryOperatorTester::TestQU8() {
  xnnpack::ReplicableRandomDevice rng;
  auto u8rng = [&]() -> uint8_t {
    return std::uniform_int_distribution<uint32_t>(range_qu8_.first,
                                                   range_qu8_.second)(rng);
  };

  xnnpack::Buffer<uint8_t> input((batch_size() - 1) * input_stride() + channels() +
                             XNN_EXTRA_BYTES / sizeof(uint8_t));
  xnnpack::Buffer<uint8_t> output((batch_size() - 1) * output_stride() +
                              channels());
  xnnpack::Buffer<float> output_ref(batch_size() * channels());
  for (size_t iteration = 0; iteration < iterations(); iteration++) {
    std::generate(input.begin(), input.end(), u8rng);

    // Compute reference results.
    for (size_t i = 0; i < batch_size(); i++) {
      for (size_t c = 0; c < channels(); c++) {
        const float x = FloatFromInputQU8(input[i * input_stride() + c]);
        const float ref_x = RefFunc(x);
        output_ref[i * channels() + c] = QuantizeAsFloatQU8(ref_x);
      }
    }

    // Create, setup, run, and destroy the operator.
    ASSERT_EQ(xnn_status_success, xnn_initialize(nullptr /* allocator */));
    xnn_operator_t op = nullptr;

    ASSERT_EQ(
        xnn_status_success,
        CreateOpQU8(input_zero_point(), input_scale(), output_zero_point(),
                    output_scale(), qmin(), qmax(), /*flags=*/0, &op));
    ASSERT_NE(nullptr, op);

    // Smart pointer to automatically delete `op`.
    std::unique_ptr<xnn_operator, decltype(&xnn_delete_operator)> auto_op(
        op, xnn_delete_operator);

    ASSERT_EQ(xnn_status_success, ReshapeOpQU8(op, batch_size(), channels(),
                                               input_stride(), output_stride(),
                                               /*threadpool=*/nullptr));
    ASSERT_EQ(xnn_status_success, SetupOpQU8(op, input.data(), output.data()));
    ASSERT_EQ(xnn_status_success, xnn_run_operator(op, /*threadpool=*/nullptr));

    // Verify results.
    for (size_t i = 0; i < batch_size(); i++) {
      for (size_t c = 0; c < channels(); c++) {
        const uint8_t y = output[i * output_stride() + c];
        const float y_ref = output_ref[i * channels() + c];
        CheckResultQU8(y, y_ref, i, c, input[i * input_stride() + c]);
      }
    }
  }
}

};  // namespace xnnpack
