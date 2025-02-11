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

#include "src/tint/lang/spirv/reader/parser/helper_test.h"

namespace tint::spirv::reader {
namespace {

TEST_F(SpirvParserTest, Dot) {
    EXPECT_IR(R"(
               OpCapability Shader
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
       %void = OpTypeVoid
      %float = OpTypeFloat 32
    %v2float = OpTypeVector %float 2
    %ep_type = OpTypeFunction %void
   %float_50 = OpConstant %float 50
   %float_60 = OpConstant %float 60
%v2float_50_60 = OpConstantComposite %v2float %float_50 %float_60
%v2float_60_50 = OpConstantComposite %v2float %float_60 %float_50
       %main = OpFunction %void None %ep_type
      %entry = OpLabel
          %1 = OpDot %float %v2float_50_60 %v2float_60_50
               OpReturn
               OpFunctionEnd)",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:f32 = dot vec2<f32>(50.0f, 60.0f), vec2<f32>(60.0f, 50.0f)
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, BitCount_Scalar_UnsignedToUnsigned) {
    EXPECT_IR(R"(
               OpCapability Shader
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
       %void = OpTypeVoid
       %uint = OpTypeInt 32 0
    %uint_10 = OpConstant %uint 10
    %ep_type = OpTypeFunction %void
       %main = OpFunction %void None %ep_type
      %entry = OpLabel
          %1 = OpBitCount %uint %uint_10
               OpReturn
               OpFunctionEnd)",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:u32 = spirv.bit_count<u32> 10u
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, BitCount_Scalar_UnsignedToSigned) {
    EXPECT_IR(R"(
               OpCapability Shader
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
       %void = OpTypeVoid
       %uint = OpTypeInt 32 0
        %int = OpTypeInt 32 1
    %uint_10 = OpConstant %uint 10
    %ep_type = OpTypeFunction %void
       %main = OpFunction %void None %ep_type
      %entry = OpLabel
          %1 = OpBitCount %int %uint_10
               OpReturn
               OpFunctionEnd)",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:i32 = spirv.bit_count<i32> 10u
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, BitCount_Scalar_SignedToUnsigned) {
    EXPECT_IR(R"(
               OpCapability Shader
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
       %void = OpTypeVoid
       %uint = OpTypeInt 32 0
        %int = OpTypeInt 32 1
     %int_20 = OpConstant %int 20
    %ep_type = OpTypeFunction %void
       %main = OpFunction %void None %ep_type
      %entry = OpLabel
          %1 = OpBitCount %uint %int_20
               OpReturn
               OpFunctionEnd)",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:u32 = spirv.bit_count<u32> 20i
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, BitCount_Scalar_SignedToSigned) {
    EXPECT_IR(R"(
               OpCapability Shader
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
       %void = OpTypeVoid
        %int = OpTypeInt 32 1
     %int_20 = OpConstant %int 20
    %ep_type = OpTypeFunction %void
       %main = OpFunction %void None %ep_type
      %entry = OpLabel
          %1 = OpBitCount %int %int_20
               OpReturn
               OpFunctionEnd)",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:i32 = spirv.bit_count<i32> 20i
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, BitCount_Vector_UnsignedToUnsigned) {
    EXPECT_IR(R"(
               OpCapability Shader
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
       %void = OpTypeVoid
       %uint = OpTypeInt 32 0
    %v2_uint = OpTypeVector %uint 2
    %uint_10 = OpConstant %uint 10
    %uint_20 = OpConstant %uint 20
%v2_uint_10_20 = OpConstantComposite %v2_uint %uint_10 %uint_20
    %ep_type = OpTypeFunction %void
       %main = OpFunction %void None %ep_type
      %entry = OpLabel
          %1 = OpBitCount %v2_uint %v2_uint_10_20
               OpReturn
               OpFunctionEnd)",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:vec2<u32> = spirv.bit_count<u32> vec2<u32>(10u, 20u)
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, BitCount_Vector_UnsignedToSigned) {
    EXPECT_IR(R"(
               OpCapability Shader
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
       %void = OpTypeVoid
        %int = OpTypeInt 32 1
       %uint = OpTypeInt 32 0
     %v2_int = OpTypeVector %int 2
    %v2_uint = OpTypeVector %uint 2
    %uint_10 = OpConstant %uint 10
    %uint_20 = OpConstant %uint 20
%v2_uint_10_20 = OpConstantComposite %v2_uint %uint_10 %uint_20
    %ep_type = OpTypeFunction %void
       %main = OpFunction %void None %ep_type
      %entry = OpLabel
          %1 = OpBitCount %v2_int %v2_uint_10_20
               OpReturn
               OpFunctionEnd)",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:vec2<i32> = spirv.bit_count<i32> vec2<u32>(10u, 20u)
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, BitCount_Vector_SignedToUnsigned) {
    EXPECT_IR(R"(
               OpCapability Shader
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
       %void = OpTypeVoid
        %int = OpTypeInt 32 1
       %uint = OpTypeInt 32 0
     %v2_int = OpTypeVector %int 2
    %v2_uint = OpTypeVector %uint 2
     %int_10 = OpConstant %int 10
     %int_20 = OpConstant %int 20
%v2_int_10_20 = OpConstantComposite %v2_int %int_10 %int_20
    %ep_type = OpTypeFunction %void
       %main = OpFunction %void None %ep_type
      %entry = OpLabel
          %1 = OpBitCount %v2_uint %v2_int_10_20
               OpReturn
               OpFunctionEnd)",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:vec2<u32> = spirv.bit_count<u32> vec2<i32>(10i, 20i)
    ret
  }
}
)");
}

TEST_F(SpirvParserTest, BitCount_Vector_SignedToSigned) {
    EXPECT_IR(R"(
               OpCapability Shader
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main"
               OpExecutionMode %main LocalSize 1 1 1
       %void = OpTypeVoid
        %int = OpTypeInt 32 1
     %v2_int = OpTypeVector %int 2
     %int_10 = OpConstant %int 10
     %int_20 = OpConstant %int 20
%v2_int_10_20 = OpConstantComposite %v2_int %int_10 %int_20
    %ep_type = OpTypeFunction %void
       %main = OpFunction %void None %ep_type
      %entry = OpLabel
          %1 = OpBitCount %v2_int %v2_int_10_20
               OpReturn
               OpFunctionEnd)",
              R"(
%main = @compute @workgroup_size(1u, 1u, 1u) func():void {
  $B1: {
    %2:vec2<i32> = spirv.bit_count<i32> vec2<i32>(10i, 20i)
    ret
  }
}
)");
}

}  // namespace
}  // namespace tint::spirv::reader
