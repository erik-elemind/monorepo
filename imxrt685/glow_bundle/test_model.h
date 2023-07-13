// Bundle API auto-generated header file. Do not edit!
// Glow Tools version: 2022-05-19 (2ee55ec50) (Glow_Release_MCUX_SDK_2.12.0)

#ifndef _GLOW_BUNDLE_TEST_MODEL_H
#define _GLOW_BUNDLE_TEST_MODEL_H

#include <stdint.h>

// ---------------------------------------------------------------
//                       Common definitions
// ---------------------------------------------------------------
#ifndef _GLOW_BUNDLE_COMMON_DEFS
#define _GLOW_BUNDLE_COMMON_DEFS

// Glow bundle error code for correct execution.
#define GLOW_SUCCESS 0

// Memory alignment definition with given alignment size
// for static allocation of memory.
#define GLOW_MEM_ALIGN(size)  __attribute__((aligned(size)))

// Macro function to get the absolute address of a
// placeholder using the base address of the mutable
// weight buffer and placeholder offset definition.
#define GLOW_GET_ADDR(mutableBaseAddr, placeholderOff)  (((uint8_t*)(mutableBaseAddr)) + placeholderOff)

#endif

// ---------------------------------------------------------------
//                          Bundle API
// ---------------------------------------------------------------
// Model name: "test_model"
// Total data size: 151104 (bytes)
// Activations allocation efficiency: 1.0000
// Placeholders:
//
//   Name: "serving_default_input_0"
//   Type: float<1 x 3750 x 5>
//   Size: 18750 (elements)
//   Size: 75000 (bytes)
//   Offset: 0 (bytes)
//
//   Name: "StatefulPartitionedCall_0"
//   Type: float<1 x 5>
//   Size: 5 (elements)
//   Size: 20 (bytes)
//   Offset: 75008 (bytes)
//
// NOTE: Placeholders are allocated within the "mutableWeight"
// buffer and are identified using an offset relative to base.
// ---------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif

// Placeholder address offsets within mutable buffer (bytes).
#define TEST_MODEL_serving_default_input_0    0
#define TEST_MODEL_StatefulPartitionedCall_0  75008

// Memory sizes (bytes).
#define TEST_MODEL_CONSTANT_MEM_SIZE     832
#define TEST_MODEL_MUTABLE_MEM_SIZE      75072
#define TEST_MODEL_ACTIVATIONS_MEM_SIZE  75200

// Memory alignment (bytes).
#define TEST_MODEL_MEM_ALIGN  64

// Bundle entry point (inference function). Returns 0
// for correct execution or some error code otherwise.
int test_model(uint8_t *constantWeight, uint8_t *mutableWeight, uint8_t *activations);

#ifdef __cplusplus
}
#endif
#endif
