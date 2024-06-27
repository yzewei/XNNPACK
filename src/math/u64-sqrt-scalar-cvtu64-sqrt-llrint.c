// Copyright 2022 Google LLC
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree.

#include <assert.h>
#include <stddef.h>
#include <math.h>

#include "xnnpack/common.h"
#include "xnnpack/math.h"
#include "xnnpack/math-stubs.h"


void xnn_math_u64_sqrt__scalar_cvtu64_sqrt_llrint(
    size_t n,
    const uint64_t* input,
    uint64_t* output)
{
  assert(n % sizeof(uint32_t) == 0);

  for (; n != 0; n -= sizeof(uint64_t)) {
    const uint64_t vx = *input++;

    uint64_t vy = vx;
    if XNN_LIKELY(vx != 0) {
      double vf = (double) vx;
      vf = sqrt(vf);
      vy = (uint64_t) (int64_t) llrint(vf);
      #if XNN_ARCH_ARM || XNN_ARCH_X86
        const uint64_t vsquared_y_less_x = math_mulext_u32((uint32_t) vy, (uint32_t) vy) - vx;
      #else
        const uint64_t vsquared_y_less_x = vy * vy - vx;
      #endif
      if XNN_UNPREDICTABLE((int64_t) (vsquared_y_less_x + vy) < 0) {
        vy += 1;
      } else if XNN_UNPREDICTABLE((int64_t) (vsquared_y_less_x - vy) >= 0) {
        vy -= 1;
      }
    }

    *output++ = vy;
  }
}
