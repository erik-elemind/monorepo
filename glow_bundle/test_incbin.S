/*
 * Copyright 2019 NXP
 * All rights reserved.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#if defined(__GNUC__)


        .global weights_section
        .type weights_section, %object
        .align 6
weights_section:
        .incbin "test_model.weights.bin"

        .end

#endif /* defined(__GNUC__) */
