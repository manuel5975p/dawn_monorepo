// Copyright 2025 The Dawn & Tint Authors
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef SRC_TINT_LANG_CORE_IR_TRANSFORM_BUILTIN_SCALARIZE_H_
#define SRC_TINT_LANG_CORE_IR_TRANSFORM_BUILTIN_SCALARIZE_H_

#include "src/tint/lang/core/ir/validator.h"
#include "src/tint/utils/reflection.h"
#include "src/tint/utils/result.h"

// Forward declarations.
namespace tint::core::ir {
class Module;
}

namespace tint::core::ir::transform {

/// The capabilities that the transform can support.
const Capabilities kBuiltinScalarizeCapabilities{
    core::ir::Capability::kAllowDuplicateBindings,
    core::ir::Capability::kAllow8BitIntegers,
    core::ir::Capability::kAllow64BitIntegers,
    core::ir::Capability::kAllowPointersAndHandlesInStructures,
    core::ir::Capability::kAllowVectorElementPointer,
    core::ir::Capability::kAllowHandleVarsWithoutBindings,
    core::ir::Capability::kAllowClipDistancesOnF32,
    core::ir::Capability::kAllowPrivateVarsInFunctions,
    core::ir::Capability::kAllowAnyLetType,
    core::ir::Capability::kAllowWorkspacePointerInputToEntryPoint,
    core::ir::Capability::kAllowModuleScopeLets,
    core::ir::Capability::kAllowAnyInputAttachmentIndexType,
    core::ir::Capability::kAllowNonCoreTypes,
};

/// The scalarizer configuration options
struct BuiltinScalarizeConfig {
    // Set to true to scalarize clamp builtin
    bool scalarize_clamp = false;

    // Set to true to scalarize max builtin
    bool scalarize_max = false;

    // Set to true to scalarize min builtin
    bool scalarize_min = false;

    /// Reflection for this class
    TINT_REFLECT(BuiltinScalarizeConfig, scalarize_clamp, scalarize_max, scalarize_min);
};

/// BuiltinScalarize is a transform that replaces calls to builtin vector functions with scalar
/// equivalent alternatives.
/// @param module the module to transform
/// @param config the polyfill configuration
/// @returns success or failure
Result<SuccessType> BuiltinScalarize(Module& module, const BuiltinScalarizeConfig& config);

}  // namespace tint::core::ir::transform

#endif  // SRC_TINT_LANG_CORE_IR_TRANSFORM_BUILTIN_SCALARIZE_H_
