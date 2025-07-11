// Copyright (c) 2018 Google LLC.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <string>

#include "gmock/gmock.h"
#include "test/unit_spirv.h"
#include "test/val/val_fixtures.h"

namespace spvtools {
namespace val {
namespace {

using ::testing::HasSubstr;

using ValidateInterfacesTest = spvtest::ValidateBase<bool>;

TEST_F(ValidateInterfacesTest, EntryPointMissingInput) {
  std::string text = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %1 "func"
OpExecutionMode %1 OriginUpperLeft
%2 = OpTypeVoid
%3 = OpTypeInt 32 0
%4 = OpTypePointer Input %3
%5 = OpVariable %4 Input
%6 = OpTypeFunction %2
%1 = OpFunction %2 None %6
%7 = OpLabel
%8 = OpLoad %3 %5
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text);
  ASSERT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "Interface variable id <5> is used by entry point 'func' id <1>, "
          "but is not listed as an interface"));
}

TEST_F(ValidateInterfacesTest, EntryPointMissingOutput) {
  std::string text = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %1 "func"
OpExecutionMode %1 OriginUpperLeft
%2 = OpTypeVoid
%3 = OpTypeInt 32 0
%4 = OpTypePointer Output %3
%5 = OpVariable %4 Output
%6 = OpTypeFunction %2
%1 = OpFunction %2 None %6
%7 = OpLabel
%8 = OpLoad %3 %5
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text);
  ASSERT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "Interface variable id <5> is used by entry point 'func' id <1>, "
          "but is not listed as an interface"));
}

TEST_F(ValidateInterfacesTest, InterfaceMissingUseInSubfunction) {
  std::string text = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %1 "func"
OpExecutionMode %1 OriginUpperLeft
%2 = OpTypeVoid
%3 = OpTypeInt 32 0
%4 = OpTypePointer Input %3
%5 = OpVariable %4 Input
%6 = OpTypeFunction %2
%1 = OpFunction %2 None %6
%7 = OpLabel
%8 = OpFunctionCall %2 %9
OpReturn
OpFunctionEnd
%9 = OpFunction %2 None %6
%10 = OpLabel
%11 = OpLoad %3 %5
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text);
  ASSERT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "Interface variable id <5> is used by entry point 'func' id <1>, "
          "but is not listed as an interface"));
}

TEST_F(ValidateInterfacesTest, TwoEntryPointsOneFunction) {
  std::string text = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %1 "func" %2
OpEntryPoint Fragment %1 "func2"
OpExecutionMode %1 OriginUpperLeft
%3 = OpTypeVoid
%4 = OpTypeInt 32 0
%5 = OpTypePointer Input %4
%2 = OpVariable %5 Input
%6 = OpTypeFunction %3
%1 = OpFunction %3 None %6
%7 = OpLabel
%8 = OpLoad %4 %2
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text);
  ASSERT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions());
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "Interface variable id <2> is used by entry point 'func2' id <1>, "
          "but is not listed as an interface"));
}

TEST_F(ValidateInterfacesTest, MissingInterfaceThroughInitializer) {
  const std::string text = R"(
OpCapability Shader
OpCapability VariablePointers
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %1 "func"
OpExecutionMode %1 OriginUpperLeft
%2 = OpTypeVoid
%3 = OpTypeInt 32 0
%4 = OpTypePointer Input %3
%5 = OpTypePointer Function %4
%6 = OpVariable %4 Input
%7 = OpTypeFunction %2
%1 = OpFunction %2 None %7
%8 = OpLabel
%9 = OpVariable %5 Function %6
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_UNIVERSAL_1_3);
  ASSERT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "Interface variable id <6> is used by entry point 'func' id <1>, "
          "but is not listed as an interface"));
}

TEST_F(ValidateInterfacesTest, NonUniqueInterfacesSPV1p3) {
  const std::string text = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main" %var %var
OpExecutionMode %main LocalSize 1 1 1
%void = OpTypeVoid
%uint = OpTypeInt 32 0
%uint3 = OpTypeVector %uint 3
%struct = OpTypeStruct %uint3
%ptr_struct = OpTypePointer Input %struct
%var = OpVariable %ptr_struct Input
%func_ty = OpTypeFunction %void
%main = OpFunction %void None %func_ty
%1 = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_UNIVERSAL_1_3);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
}

TEST_F(ValidateInterfacesTest, NonUniqueInterfacesSPV1p4) {
  const std::string text = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main" %var %var
OpExecutionMode %main LocalSize 1 1 1
OpName %main "main"
OpName %var "var"
%void = OpTypeVoid
%uint = OpTypeInt 32 0
%uint3 = OpTypeVector %uint 3
%struct = OpTypeStruct %uint3
%ptr_struct = OpTypePointer Input %struct
%var = OpVariable %ptr_struct Input
%func_ty = OpTypeFunction %void
%main = OpFunction %void None %func_ty
%1 = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_UNIVERSAL_1_4);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_UNIVERSAL_1_4));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Non-unique OpEntryPoint interface '2[%var]' is disallowed"));
}

TEST_F(ValidateInterfacesTest, MissingGlobalVarSPV1p3) {
  const std::string text = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
OpExecutionMode %main LocalSize 1 1 1
%void = OpTypeVoid
%uint = OpTypeInt 32 0
%uint3 = OpTypeVector %uint 3
%struct = OpTypeStruct %uint3
%ptr_struct = OpTypePointer StorageBuffer %struct
%var = OpVariable %ptr_struct StorageBuffer
%func_ty = OpTypeFunction %void
%main = OpFunction %void None %func_ty
%1 = OpLabel
%ld = OpLoad %struct %var
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_UNIVERSAL_1_3);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
}

TEST_F(ValidateInterfacesTest, MissingGlobalVarSPV1p4) {
  const std::string text = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
OpExecutionMode %main LocalSize 1 1 1
OpName %var "var"
%void = OpTypeVoid
%uint = OpTypeInt 32 0
%uint3 = OpTypeVector %uint 3
%struct = OpTypeStruct %uint3
%ptr_struct = OpTypePointer StorageBuffer %struct
%var = OpVariable %ptr_struct StorageBuffer
%func_ty = OpTypeFunction %void
%main = OpFunction %void None %func_ty
%1 = OpLabel
%ld = OpLoad %struct %var
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_UNIVERSAL_1_4);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_UNIVERSAL_1_4));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Interface variable id <2> is used by entry point "
                        "'main' id <1>, but is not listed as an interface"));
}

TEST_F(ValidateInterfacesTest, FunctionInterfaceVarSPV1p3) {
  const std::string text = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main" %var
OpExecutionMode %main LocalSize 1 1 1
OpName %var "var"
%void = OpTypeVoid
%uint = OpTypeInt 32 0
%uint3 = OpTypeVector %uint 3
%struct = OpTypeStruct %uint3
%ptr_struct = OpTypePointer Function %struct
%func_ty = OpTypeFunction %void
%main = OpFunction %void None %func_ty
%1 = OpLabel
%var = OpVariable %ptr_struct Function
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_UNIVERSAL_1_3);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_UNIVERSAL_1_3));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("OpEntryPoint interfaces must be OpVariables with "
                        "Storage Class of Input(1) or Output(3). Found Storage "
                        "Class 7 for Entry Point id 1."));
}

TEST_F(ValidateInterfacesTest, FunctionInterfaceVarSPV1p4) {
  const std::string text = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main" %var
OpExecutionMode %main LocalSize 1 1 1
OpName %var "var"
%void = OpTypeVoid
%uint = OpTypeInt 32 0
%uint3 = OpTypeVector %uint 3
%struct = OpTypeStruct %uint3
%ptr_struct = OpTypePointer Function %struct
%func_ty = OpTypeFunction %void
%main = OpFunction %void None %func_ty
%1 = OpLabel
%var = OpVariable %ptr_struct Function
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_UNIVERSAL_1_4);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_UNIVERSAL_1_4));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("OpEntryPoint interfaces should only list global variables"));
}

TEST_F(ValidateInterfacesTest, ModuleSPV1p3ValidateSPV1p4_NotAllUsedGlobals) {
  const std::string text = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
OpExecutionMode %main LocalSize 1 1 1
OpName %var "var"
%void = OpTypeVoid
%uint = OpTypeInt 32 0
%uint3 = OpTypeVector %uint 3
%struct = OpTypeStruct %uint3
%ptr_struct = OpTypePointer StorageBuffer %struct
%var = OpVariable %ptr_struct StorageBuffer
%func_ty = OpTypeFunction %void
%main = OpFunction %void None %func_ty
%1 = OpLabel
%ld = OpLoad %struct %var
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_UNIVERSAL_1_3);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_UNIVERSAL_1_4));
}

TEST_F(ValidateInterfacesTest, ModuleSPV1p3ValidateSPV1p4_DuplicateInterface) {
  const std::string text = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main" %gid %gid
OpExecutionMode %main LocalSize 1 1 1
OpDecorate %gid BuiltIn GlobalInvocationId
%void = OpTypeVoid
%int = OpTypeInt 32 0
%int3 = OpTypeVector %int 3
%ptr_input_int3 = OpTypePointer Input %int3
%gid = OpVariable %ptr_input_int3 Input
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_UNIVERSAL_1_3);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_UNIVERSAL_1_4));
}

TEST_F(ValidateInterfacesTest, SPV14MultipleEntryPointsSameFunction) {
  const std::string text = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main1" %gid
OpEntryPoint GLCompute %main "main2" %gid
OpExecutionMode %main LocalSize 1 1 1
OpDecorate %gid BuiltIn GlobalInvocationId
%void = OpTypeVoid
%int = OpTypeInt 32 0
%int3 = OpTypeVector %int 3
%ptr_input_int3 = OpTypePointer Input %int3
%gid = OpVariable %ptr_input_int3 Input
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_UNIVERSAL_1_4);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_UNIVERSAL_1_4));
}

TEST_F(ValidateInterfacesTest, VulkanLocationsDoubleAssignmentVariable) {
  const std::string text = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %var
OpExecutionMode %main OriginUpperLeft
OpDecorate %var Location 0
OpDecorate %var Location 1
%void = OpTypeVoid
%void_fn = OpTypeFunction %void
%float = OpTypeFloat 32
%ptr_input_float = OpTypePointer Input %float
%var = OpVariable %ptr_input_float Input
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_VULKAN_1_0);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_0));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("decorated with Location multiple times is not allowed"));
}

TEST_F(ValidateInterfacesTest, VulkanLocationsVariableAndMemberAssigned) {
  const std::string text = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %var
OpExecutionMode %main OriginUpperLeft
OpDecorate %var Location 0
OpDecorate %struct Block
OpMemberDecorate %struct 0 Location 0
%void = OpTypeVoid
%void_fn = OpTypeFunction %void
%float = OpTypeFloat 32
%struct = OpTypeStruct %float
%ptr_input_struct = OpTypePointer Input %struct
%var = OpVariable %ptr_input_struct Input
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_VULKAN_1_0);
  EXPECT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions(SPV_ENV_VULKAN_1_0));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-Location-04918"));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Members cannot be assigned a location"));
}

TEST_F(ValidateInterfacesTest, VulkanLocationsMemberAndSubMemberAssigned) {
  const std::string text = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %var
OpExecutionMode %main OriginUpperLeft
OpDecorate %outer Block
OpMemberDecorate %outer 0 Location 0
OpMemberDecorate %struct 0 Location 0
%void = OpTypeVoid
%void_fn = OpTypeFunction %void
%float = OpTypeFloat 32
%struct = OpTypeStruct %float
%outer = OpTypeStruct %struct
%ptr_input_outer = OpTypePointer Input %outer
%var = OpVariable %ptr_input_outer Input
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_VULKAN_1_0);
  EXPECT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions(SPV_ENV_VULKAN_1_0));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-Location-04918"));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Members cannot be assigned a location"));
}

TEST_F(ValidateInterfacesTest, VulkanLocationsDoubleAssignmentStructMember) {
  const std::string text = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %var
OpExecutionMode %main OriginUpperLeft
OpDecorate %struct Block
OpMemberDecorate %struct 1 Location 0
OpMemberDecorate %struct 1 Location 1
%void = OpTypeVoid
%void_fn = OpTypeFunction %void
%float = OpTypeFloat 32
%struct = OpTypeStruct %float %float
%ptr_input_struct = OpTypePointer Input %struct
%var = OpVariable %ptr_input_struct Input
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_VULKAN_1_0);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_0));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("decorated with Location multiple times is not allowed"));
}

TEST_F(ValidateInterfacesTest, VulkanLocationsMissingAssignmentStructMember) {
  const std::string text = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %var
OpExecutionMode %main OriginUpperLeft
OpDecorate %struct Block
OpMemberDecorate %struct 1 Location 1
%void = OpTypeVoid
%void_fn = OpTypeFunction %void
%float = OpTypeFloat 32
%struct = OpTypeStruct %float %float
%ptr_input_struct = OpTypePointer Input %struct
%var = OpVariable %ptr_input_struct Input
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_VULKAN_1_0);
  EXPECT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions(SPV_ENV_VULKAN_1_0));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Member index 0 is missing a location assignment"));
}

TEST_F(ValidateInterfacesTest, VulkanLocationsMissingAssignmentNonBlockStruct) {
  const std::string text = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %var
OpExecutionMode %main OriginUpperLeft
%void = OpTypeVoid
%void_fn = OpTypeFunction %void
%float = OpTypeFloat 32
%struct = OpTypeStruct %float %float
%ptr_input_struct = OpTypePointer Input %struct
%var = OpVariable %ptr_input_struct Input
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_VULKAN_1_0);
  EXPECT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions(SPV_ENV_VULKAN_1_0));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Variable must be decorated with a location"));
}

TEST_F(ValidateInterfacesTest, VulkanLocationsVariableConflictInput) {
  const std::string text = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %var1 %var2
OpExecutionMode %main OriginUpperLeft
OpDecorate %var1 Location 0
OpDecorate %var2 Location 0
%void = OpTypeVoid
%void_fn = OpTypeFunction %void
%float = OpTypeFloat 32
%struct = OpTypeStruct %float %float
%ptr_input_struct = OpTypePointer Input %struct
%var1 = OpVariable %ptr_input_struct Input
%var2 = OpVariable %ptr_input_struct Input
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_VULKAN_1_0);
  EXPECT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions(SPV_ENV_VULKAN_1_0));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-OpEntryPoint-08721"));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Entry-point has conflicting input location assignment "
                        "at location 0"));
}

TEST_F(ValidateInterfacesTest, VulkanLocationsVariableConflictOutput) {
  const std::string text = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %var1 %var2
OpExecutionMode %main OriginUpperLeft
OpDecorate %var1 Location 1
OpDecorate %var2 Location 1
%void = OpTypeVoid
%void_fn = OpTypeFunction %void
%float = OpTypeFloat 32
%struct = OpTypeStruct %float %float
%ptr_output_struct = OpTypePointer Output %struct
%var1 = OpVariable %ptr_output_struct Output
%var2 = OpVariable %ptr_output_struct Output
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_VULKAN_1_0);
  EXPECT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions(SPV_ENV_VULKAN_1_0));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-OpEntryPoint-08722"));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Entry-point has conflicting output location assignment "
                "at location 1"));
}

TEST_F(ValidateInterfacesTest, VulkanPatchAndNonPatchOverlap) {
  const std::string text = R"(
               OpCapability Tessellation
               OpMemoryModel Logical GLSL450
               OpEntryPoint TessellationControl %main "main" %a %b
               OpExecutionMode %main OutputVertices 4
               OpDecorate %a Location 0
               OpDecorate %b Patch
               OpDecorate %b Location 0
       %void = OpTypeVoid
          %3 = OpTypeFunction %void
      %float = OpTypeFloat 32
       %uint = OpTypeInt 32 0
     %uint_4 = OpConstant %uint 4
%_arr_float_uint_4 = OpTypeArray %float %uint_4
%_ptr_Output__arr_float_uint_4 = OpTypePointer Output %_arr_float_uint_4
          %a = OpVariable %_ptr_Output__arr_float_uint_4 Output
%_ptr_Output_float = OpTypePointer Output %float
          %b = OpVariable %_ptr_Output_float Output
       %main = OpFunction %void None %3
          %5 = OpLabel
               OpReturn
               OpFunctionEnd
    )";

  CompileSuccessfully(text, SPV_ENV_VULKAN_1_2);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_VULKAN_1_2));
}

TEST_F(ValidateInterfacesTest, VulkanPatchOverlap) {
  const std::string text = R"(
               OpCapability Tessellation
               OpMemoryModel Logical GLSL450
               OpEntryPoint TessellationControl %main "main" %a %b %c
               OpExecutionMode %main OutputVertices 4
               OpDecorate %a Location 0
               OpDecorate %b Patch
               OpDecorate %b Location 6
               OpDecorate %c Patch
               OpDecorate %c Location 6
       %void = OpTypeVoid
          %3 = OpTypeFunction %void
      %float = OpTypeFloat 32
       %uint = OpTypeInt 32 0
     %uint_4 = OpConstant %uint 4
%_arr_float_uint_4 = OpTypeArray %float %uint_4
%_ptr_Output__arr_float_uint_4 = OpTypePointer Output %_arr_float_uint_4
          %a = OpVariable %_ptr_Output__arr_float_uint_4 Output
%_ptr_Output_float = OpTypePointer Output %float
          %b = OpVariable %_ptr_Output_float Output
          %c = OpVariable %_ptr_Output_float Output
       %main = OpFunction %void None %3
          %5 = OpLabel
               OpReturn
               OpFunctionEnd
    )";

  CompileSuccessfully(text, SPV_ENV_VULKAN_1_2);
  EXPECT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions(SPV_ENV_VULKAN_1_2));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-OpEntryPoint-08722"));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Entry-point has conflicting output location "
                        "assignment at location 6, component 0"));
}

TEST_F(ValidateInterfacesTest,
       VulkanLocationsSameLocationInputAndOutputNoConflict) {
  const std::string text = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %var1 %var2
OpExecutionMode %main OriginUpperLeft
OpDecorate %var1 Location 1
OpDecorate %var2 Location 1
%void = OpTypeVoid
%void_fn = OpTypeFunction %void
%float = OpTypeFloat 32
%struct = OpTypeStruct %float %float
%ptr_input_struct = OpTypePointer Input %struct
%ptr_output_struct = OpTypePointer Output %struct
%var1 = OpVariable %ptr_input_struct Input
%var2 = OpVariable %ptr_output_struct Output
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_VULKAN_1_0);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_VULKAN_1_0));
}

TEST_F(ValidateInterfacesTest, VulkanLocationsVariableInGap) {
  const std::string text = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %var1 %var2
OpExecutionMode %main OriginUpperLeft
OpDecorate %struct Block
OpMemberDecorate %struct 0 Location 0
OpMemberDecorate %struct 1 Location 2
OpDecorate %var2 Location 1
%void = OpTypeVoid
%void_fn = OpTypeFunction %void
%float = OpTypeFloat 32
%struct = OpTypeStruct %float %float
%ptr_input_struct = OpTypePointer Input %struct
%ptr_input_float = OpTypePointer Input %float
%var1 = OpVariable %ptr_input_struct Input
%var2 = OpVariable %ptr_input_float Input
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_VULKAN_1_0);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_VULKAN_1_0));
}

TEST_F(ValidateInterfacesTest, VulkanLocationsLargeFloatVectorConflict) {
  const std::string text = R"(
OpCapability Shader
OpCapability Float64
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %var1 %var2
OpExecutionMode %main OriginUpperLeft
OpDecorate %var1 Location 0
OpDecorate %var2 Location 1
%void = OpTypeVoid
%void_fn = OpTypeFunction %void
%float = OpTypeFloat 32
%double = OpTypeFloat 64
%vector = OpTypeVector %double 3
%ptr_input_float = OpTypePointer Input %float
%ptr_input_vector = OpTypePointer Input %vector
%var1 = OpVariable %ptr_input_vector Input
%var2 = OpVariable %ptr_input_float Input
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_VULKAN_1_0);
  EXPECT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions(SPV_ENV_VULKAN_1_0));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-OpEntryPoint-08721"));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Entry-point has conflicting input location assignment "
                        "at location 1"));
}

TEST_F(ValidateInterfacesTest, VulkanLocationsLargeIntVectorConflict) {
  const std::string text = R"(
OpCapability Shader
OpCapability Int64
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %var1 %var2
OpExecutionMode %main OriginUpperLeft
OpDecorate %var1 Location 0
OpDecorate %var1 Flat
OpDecorate %var2 Location 1
OpDecorate %var2 Flat
%void = OpTypeVoid
%void_fn = OpTypeFunction %void
%float = OpTypeFloat 32
%long = OpTypeInt 64 0
%vector = OpTypeVector %long 4
%ptr_input_float = OpTypePointer Input %float
%ptr_input_vector = OpTypePointer Input %vector
%var1 = OpVariable %ptr_input_vector Input
%var2 = OpVariable %ptr_input_float Input
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_VULKAN_1_0);
  EXPECT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions(SPV_ENV_VULKAN_1_0));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-OpEntryPoint-08721"));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Entry-point has conflicting input location assignment "
                        "at location 1"));
}

TEST_F(ValidateInterfacesTest, VulkanLocationsMatrix2x2Conflict) {
  const std::string text = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %var1 %var2
OpExecutionMode %main OriginUpperLeft
OpDecorate %var1 Location 0
OpDecorate %var2 Location 1
%void = OpTypeVoid
%void_fn = OpTypeFunction %void
%float = OpTypeFloat 32
%vector = OpTypeVector %float 2
%matrix = OpTypeMatrix %vector 2
%ptr_input_float = OpTypePointer Input %float
%ptr_input_matrix = OpTypePointer Input %matrix
%var1 = OpVariable %ptr_input_matrix Input
%var2 = OpVariable %ptr_input_float Input
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_VULKAN_1_0);
  EXPECT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions(SPV_ENV_VULKAN_1_0));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-OpEntryPoint-08721"));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Entry-point has conflicting input location assignment "
                        "at location 1"));
}

TEST_F(ValidateInterfacesTest, VulkanLocationsMatrix3x3Conflict) {
  const std::string text = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %var1 %var2
OpExecutionMode %main OriginUpperLeft
OpDecorate %var1 Location 0
OpDecorate %var2 Location 2
%void = OpTypeVoid
%void_fn = OpTypeFunction %void
%float = OpTypeFloat 32
%vector = OpTypeVector %float 3
%matrix = OpTypeMatrix %vector 3
%ptr_input_float = OpTypePointer Input %float
%ptr_input_matrix = OpTypePointer Input %matrix
%var1 = OpVariable %ptr_input_matrix Input
%var2 = OpVariable %ptr_input_float Input
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_VULKAN_1_0);
  EXPECT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions(SPV_ENV_VULKAN_1_0));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-OpEntryPoint-08721"));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Entry-point has conflicting input location assignment "
                        "at location 2"));
}

TEST_F(ValidateInterfacesTest, VulkanLocationsMatrix4x4Conflict) {
  const std::string text = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %var1 %var2
OpExecutionMode %main OriginUpperLeft
OpDecorate %var1 Location 0
OpDecorate %var2 Location 3
%void = OpTypeVoid
%void_fn = OpTypeFunction %void
%float = OpTypeFloat 32
%vector = OpTypeVector %float 4
%matrix = OpTypeMatrix %vector 4
%ptr_input_float = OpTypePointer Input %float
%ptr_input_matrix = OpTypePointer Input %matrix
%var1 = OpVariable %ptr_input_matrix Input
%var2 = OpVariable %ptr_input_float Input
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_VULKAN_1_0);
  EXPECT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions(SPV_ENV_VULKAN_1_0));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-OpEntryPoint-08721"));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Entry-point has conflicting input location assignment "
                        "at location 3"));
}

TEST_F(ValidateInterfacesTest, VulkanLocationsLargeMatrix2x2Conflict) {
  const std::string text = R"(
OpCapability Shader
OpCapability Float64
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %var1 %var2
OpExecutionMode %main OriginUpperLeft
OpDecorate %var1 Location 0
OpDecorate %var2 Location 1
%void = OpTypeVoid
%void_fn = OpTypeFunction %void
%float = OpTypeFloat 32
%double = OpTypeFloat 64
%vector = OpTypeVector %double 2
%matrix = OpTypeMatrix %vector 2
%ptr_input_float = OpTypePointer Input %float
%ptr_input_matrix = OpTypePointer Input %matrix
%var1 = OpVariable %ptr_input_matrix Input
%var2 = OpVariable %ptr_input_float Input
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_VULKAN_1_0);
  EXPECT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions(SPV_ENV_VULKAN_1_0));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-OpEntryPoint-08721"));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Entry-point has conflicting input location assignment "
                        "at location 1"));
}

TEST_F(ValidateInterfacesTest, VulkanLocationsLargeMatrix3x3Conflict) {
  const std::string text = R"(
OpCapability Shader
OpCapability Float64
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %var1 %var2
OpExecutionMode %main OriginUpperLeft
OpDecorate %var1 Location 0
OpDecorate %var2 Location 5
%void = OpTypeVoid
%void_fn = OpTypeFunction %void
%float = OpTypeFloat 32
%double = OpTypeFloat 64
%vector = OpTypeVector %double 3
%matrix = OpTypeMatrix %vector 3
%ptr_input_float = OpTypePointer Input %float
%ptr_input_matrix = OpTypePointer Input %matrix
%var1 = OpVariable %ptr_input_matrix Input
%var2 = OpVariable %ptr_input_float Input
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_VULKAN_1_0);
  EXPECT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions(SPV_ENV_VULKAN_1_0));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-OpEntryPoint-08721"));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Entry-point has conflicting input location assignment "
                        "at location 5"));
}

TEST_F(ValidateInterfacesTest, VulkanLocationsLargeMatrix4x4Conflict) {
  const std::string text = R"(
OpCapability Shader
OpCapability Float64
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %var1 %var2
OpExecutionMode %main OriginUpperLeft
OpDecorate %var1 Location 0
OpDecorate %var2 Location 7
%void = OpTypeVoid
%void_fn = OpTypeFunction %void
%float = OpTypeFloat 32
%double = OpTypeFloat 64
%vector = OpTypeVector %double 4
%matrix = OpTypeMatrix %vector 4
%ptr_input_float = OpTypePointer Input %float
%ptr_input_matrix = OpTypePointer Input %matrix
%var1 = OpVariable %ptr_input_matrix Input
%var2 = OpVariable %ptr_input_float Input
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_VULKAN_1_0);
  EXPECT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions(SPV_ENV_VULKAN_1_0));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-OpEntryPoint-08721"));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Entry-point has conflicting input location assignment "
                        "at location 7"));
}

TEST_F(ValidateInterfacesTest, VulkanLocationsArray2Conflict) {
  const std::string text = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %var1 %var2
OpExecutionMode %main OriginUpperLeft
OpDecorate %var1 Location 0
OpDecorate %var2 Location 1
%void = OpTypeVoid
%void_fn = OpTypeFunction %void
%float = OpTypeFloat 32
%int = OpTypeInt 32 0
%int_2 = OpConstant %int 2
%array = OpTypeArray %int %int_2
%struct = OpTypeStruct %array
%ptr_input_float = OpTypePointer Input %float
%ptr_input_struct = OpTypePointer Input %struct
%var1 = OpVariable %ptr_input_struct Input
%var2 = OpVariable %ptr_input_float Input
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_VULKAN_1_0);
  EXPECT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions(SPV_ENV_VULKAN_1_0));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-OpEntryPoint-08721"));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Entry-point has conflicting input location assignment "
                        "at location 1"));
}

TEST_F(ValidateInterfacesTest, VulkanLocationsArray4Conflict) {
  const std::string text = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %var1 %var2
OpExecutionMode %main OriginUpperLeft
OpDecorate %var1 Location 0
OpDecorate %var2 Location 3
%void = OpTypeVoid
%void_fn = OpTypeFunction %void
%float = OpTypeFloat 32
%int = OpTypeInt 32 0
%int_4 = OpConstant %int 4
%array = OpTypeArray %int %int_4
%struct = OpTypeStruct %array
%ptr_input_float = OpTypePointer Input %float
%ptr_input_struct = OpTypePointer Input %struct
%var1 = OpVariable %ptr_input_struct Input
%var2 = OpVariable %ptr_input_float Input
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_VULKAN_1_0);
  EXPECT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions(SPV_ENV_VULKAN_1_0));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-OpEntryPoint-08721"));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Entry-point has conflicting input location assignment "
                        "at location 3"));
}

TEST_F(ValidateInterfacesTest, VulkanLocationsMatrix4x4Array4Conflict) {
  const std::string text = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %var1 %var2
OpExecutionMode %main OriginUpperLeft
OpDecorate %var1 Location 0
OpDecorate %var2 Location 15
%void = OpTypeVoid
%void_fn = OpTypeFunction %void
%float = OpTypeFloat 32
%int = OpTypeInt 32 0
%int_4 = OpConstant %int 4
%vector = OpTypeVector %float 4
%matrix = OpTypeMatrix %vector 4
%array = OpTypeArray %matrix %int_4
%struct = OpTypeStruct %array
%ptr_input_float = OpTypePointer Input %float
%ptr_input_struct = OpTypePointer Input %struct
%var1 = OpVariable %ptr_input_struct Input
%var2 = OpVariable %ptr_input_float Input
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_VULKAN_1_0);
  EXPECT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions(SPV_ENV_VULKAN_1_0));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-OpEntryPoint-08721"));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Entry-point has conflicting input location assignment "
                        "at location 15"));
}

TEST_F(ValidateInterfacesTest, VulkanLocationsComponentDisambiguates) {
  const std::string text = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %var1
OpExecutionMode %main OriginUpperLeft
OpDecorate %struct Block
OpMemberDecorate %struct 0 Location 0
OpMemberDecorate %struct 0 Component 0
OpMemberDecorate %struct 1 Location 0
OpMemberDecorate %struct 1 Component 1
%void = OpTypeVoid
%void_fn = OpTypeFunction %void
%float = OpTypeFloat 32
%struct = OpTypeStruct %float %float
%ptr_input_struct = OpTypePointer Input %struct
%var1 = OpVariable %ptr_input_struct Input
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_VULKAN_1_0);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_VULKAN_1_0));
}

TEST_F(ValidateInterfacesTest, VulkanLocationsComponentIn64BitVec3) {
  const std::string text = R"(
OpCapability Shader
OpCapability Float64
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %var
OpExecutionMode %main OriginUpperLeft
OpDecorate %struct Block
OpMemberDecorate %struct 0 Location 0
OpMemberDecorate %struct 1 Location 1
OpMemberDecorate %struct 1 Component 1
%void = OpTypeVoid
%void_fn = OpTypeFunction %void
%float = OpTypeFloat 32
%double = OpTypeFloat 64
%double3 = OpTypeVector %double 3
%struct = OpTypeStruct %double3 %float
%ptr_input_struct = OpTypePointer Input %struct
%var = OpVariable %ptr_input_struct Input
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_VULKAN_1_0);
  EXPECT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions(SPV_ENV_VULKAN_1_0));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-OpEntryPoint-08721"));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Entry-point has conflicting input location assignment "
                        "at location 1, component 1"));
}

TEST_F(ValidateInterfacesTest, VulkanLocationsComponentAfter64BitVec3) {
  const std::string text = R"(
OpCapability Shader
OpCapability Float64
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %var
OpExecutionMode %main OriginUpperLeft
OpDecorate %struct Block
OpMemberDecorate %struct 0 Location 0
OpMemberDecorate %struct 1 Location 1
OpMemberDecorate %struct 1 Component 2
%void = OpTypeVoid
%void_fn = OpTypeFunction %void
%float = OpTypeFloat 32
%double = OpTypeFloat 64
%double3 = OpTypeVector %double 3
%struct = OpTypeStruct %double3 %float
%ptr_input_struct = OpTypePointer Input %struct
%var = OpVariable %ptr_input_struct Input
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_VULKAN_1_0);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_VULKAN_1_0));
}

TEST_F(ValidateInterfacesTest, VulkanLocationsConflictingComponentVariable) {
  const std::string text = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %var
OpExecutionMode %main OriginUpperLeft
OpDecorate %var Location 0
OpDecorate %var Component 0
OpDecorate %var Component 1
%void = OpTypeVoid
%void_fn = OpTypeFunction %void
%float = OpTypeFloat 32
%ptr_input_float = OpTypePointer Input %float
%var = OpVariable %ptr_input_float Input
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_VULKAN_1_0);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_0));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("decorated with Component multiple times is not allowed"));
}

TEST_F(ValidateInterfacesTest,
       VulkanLocationsConflictingComponentStructMember) {
  const std::string text = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %var
OpExecutionMode %main OriginUpperLeft
OpDecorate %struct Block
OpMemberDecorate %struct 0 Location 0
OpMemberDecorate %struct 0 Component 0
OpMemberDecorate %struct 0 Component 1
%void = OpTypeVoid
%void_fn = OpTypeFunction %void
%float = OpTypeFloat 32
%struct = OpTypeStruct %float
%ptr_input_struct = OpTypePointer Input %struct
%var = OpVariable %ptr_input_struct Input
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_VULKAN_1_0);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_0));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("decorated with Component multiple times is not allowed"));
}

TEST_F(ValidateInterfacesTest, VulkanLocationsVariableConflictOutputIndex1) {
  const std::string text = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %var1 %var2
OpExecutionMode %main OriginUpperLeft
OpDecorate %var1 Location 1
OpDecorate %var1 Index 1
OpDecorate %var2 Location 1
OpDecorate %var2 Index 1
%void = OpTypeVoid
%void_fn = OpTypeFunction %void
%float = OpTypeFloat 32
%struct = OpTypeStruct %float %float
%ptr_output_struct = OpTypePointer Output %struct
%var1 = OpVariable %ptr_output_struct Output
%var2 = OpVariable %ptr_output_struct Output
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_VULKAN_1_0);
  EXPECT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions(SPV_ENV_VULKAN_1_0));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-OpEntryPoint-08722"));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Entry-point has conflicting output location assignment "
                "at location 1"));
}

TEST_F(ValidateInterfacesTest,
       VulkanLocationsVariableNoConflictDifferentIndex) {
  const std::string text = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %var1 %var2
OpExecutionMode %main OriginUpperLeft
OpDecorate %var1 Location 1
OpDecorate %var1 Index 0
OpDecorate %var2 Location 1
OpDecorate %var2 Index 1
%void = OpTypeVoid
%void_fn = OpTypeFunction %void
%float = OpTypeFloat 32
%struct = OpTypeStruct %float %float
%ptr_output_struct = OpTypePointer Output %struct
%var1 = OpVariable %ptr_output_struct Output
%var2 = OpVariable %ptr_output_struct Output
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_VULKAN_1_0);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_VULKAN_1_0));
}

TEST_F(ValidateInterfacesTest, VulkanLocationsIndexGLCompute) {
  const std::string text = R"(
OpCapability Shader
OpCapability Geometry
OpMemoryModel Logical GLSL450
OpEntryPoint Geometry %main "main" %var1
OpExecutionMode %main Triangles
OpExecutionMode %main OutputPoints
OpDecorate %var1 Location 1
OpDecorate %var1 Index 1
%void = OpTypeVoid
%void_fn = OpTypeFunction %void
%float = OpTypeFloat 32
%struct = OpTypeStruct %float %float
%ptr_output_struct = OpTypePointer Output %struct
%var1 = OpVariable %ptr_output_struct Output
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_VULKAN_1_0);
  EXPECT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions(SPV_ENV_VULKAN_1_0));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("Index can only be applied to Fragment output variables"));
}

TEST_F(ValidateInterfacesTest, VulkanLocationsIndexInput) {
  const std::string text = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %var1
OpExecutionMode %main OriginUpperLeft
OpDecorate %var1 Location 1
OpDecorate %var1 Index 1
%void = OpTypeVoid
%void_fn = OpTypeFunction %void
%float = OpTypeFloat 32
%struct = OpTypeStruct %float %float
%ptr_input_struct = OpTypePointer Input %struct
%var1 = OpVariable %ptr_input_struct Input
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_VULKAN_1_0);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_0));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("must be in the Output storage class"));
}

TEST_F(ValidateInterfacesTest, VulkanLocationsArrayWithComponent) {
  const std::string text = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %4 "main" %11 %18 %28 %36 %40
OpExecutionMode %4 OriginUpperLeft
OpDecorate %11 Location 0
OpDecorate %18 Component 0
OpDecorate %18 Location 0
OpDecorate %28 Component 1
OpDecorate %28 Location 0
OpDecorate %36 Location 1
OpDecorate %40 Component 0
OpDecorate %40 Location 1
%void = OpTypeVoid
%3 = OpTypeFunction %void
%float = OpTypeFloat 32
%v4float = OpTypeVector %float 4
%_ptr_Input_v4float = OpTypePointer Input %v4float
%11 = OpVariable %_ptr_Input_v4float Input
%_ptr_Output_float = OpTypePointer Output %float
%18 = OpVariable %_ptr_Output_float Output
%uint = OpTypeInt 32 0
%v3float = OpTypeVector %float 3
%uint_2 = OpConstant %uint 2
%_arr_v3float_uint_2 = OpTypeArray %v3float %uint_2
%_ptr_Output__arr_v3float_uint_2 = OpTypePointer Output %_arr_v3float_uint_2
%28 = OpVariable %_ptr_Output__arr_v3float_uint_2 Output
%_ptr_Output_v3float = OpTypePointer Output %v3float
%36 = OpVariable %_ptr_Input_v4float Input
%40 = OpVariable %_ptr_Output_float Output
%4 = OpFunction %void None %3
%5 = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_VULKAN_1_0);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_VULKAN_1_0));
}

TEST_F(ValidateInterfacesTest, VulkanLocationsArrayWithComponentBad) {
  const std::string text = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %4 "main" %11 %18 %28 %36 %40
OpExecutionMode %4 OriginUpperLeft
OpDecorate %11 Location 0
OpDecorate %18 Component 0
OpDecorate %18 Location 0
OpDecorate %28 Component 1
OpDecorate %28 Location 0
OpDecorate %36 Location 1
OpDecorate %40 Component 1
OpDecorate %40 Location 1
%void = OpTypeVoid
%3 = OpTypeFunction %void
%float = OpTypeFloat 32
%v4float = OpTypeVector %float 4
%_ptr_Input_v4float = OpTypePointer Input %v4float
%11 = OpVariable %_ptr_Input_v4float Input
%_ptr_Output_float = OpTypePointer Output %float
%18 = OpVariable %_ptr_Output_float Output
%uint = OpTypeInt 32 0
%v3float = OpTypeVector %float 3
%uint_2 = OpConstant %uint 2
%_arr_v3float_uint_2 = OpTypeArray %v3float %uint_2
%_ptr_Output__arr_v3float_uint_2 = OpTypePointer Output %_arr_v3float_uint_2
%28 = OpVariable %_ptr_Output__arr_v3float_uint_2 Output
%_ptr_Output_v3float = OpTypePointer Output %v3float
%36 = OpVariable %_ptr_Input_v4float Input
%40 = OpVariable %_ptr_Output_float Output
%4 = OpFunction %void None %3
%5 = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_VULKAN_1_0);
  EXPECT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions(SPV_ENV_VULKAN_1_0));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-OpEntryPoint-08722"));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Entry-point has conflicting output location "
                        "assignment at location 1, component 1"));
}

TEST_F(ValidateInterfacesTest, VulkanLocationsLargeLocation) {
  const std::string text = R"(
               OpCapability Shader
               OpMemoryModel Logical GLSL450
               OpEntryPoint Fragment %4 "????????" %17
               OpExecutionMode %4 OriginUpperLeft
               OpDecorate %17 Location 4227868160
       %void = OpTypeVoid
          %3 = OpTypeFunction %void
      %float = OpTypeFloat 32
    %v3float = OpTypeVector %float 3
%_ptr_Input_v3float = OpTypePointer Input %v3float
         %17 = OpVariable %_ptr_Input_v3float Input
          %4 = OpFunction %void None %3
          %5 = OpLabel
               OpUnreachable
               OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_VULKAN_1_0);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_VULKAN_1_0));
}

TEST_F(ValidateInterfacesTest, VulkanLocationMeshShader) {
  const std::string text = R"(
OpCapability Shader
OpCapability MeshShadingNV
OpExtension "SPV_NV_mesh_shader"
OpMemoryModel Logical GLSL450
OpEntryPoint MeshNV %foo "foo" %in
OpExecutionMode %foo LocalSize 1 1 1
OpDecorate %block Block
OpMemberDecorate %block 0 PerTaskNV
OpMemberDecorate %block 0 Offset 0
%void = OpTypeVoid
%int = OpTypeInt 32 0
%int_32 = OpConstant %int 32
%array = OpTypeArray %int %int_32
%block = OpTypeStruct %array
%ptr_input_block = OpTypePointer Input %block
%in = OpVariable %ptr_input_block Input
%void_fn = OpTypeFunction %void
%foo = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_VULKAN_1_2);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_VULKAN_1_2));
}

TEST_F(ValidateInterfacesTest, VulkanLocationArrayWithComponent1) {
  const std::string text = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %in
OpExecutionMode %main OriginUpperLeft
OpDecorate %struct Block
OpMemberDecorate %struct 0 Location 0
OpMemberDecorate %struct 0 Component 0
OpMemberDecorate %struct 1 Location 0
OpMemberDecorate %struct 1 Component 1
%void = OpTypeVoid
%void_fn = OpTypeFunction %void
%float = OpTypeFloat 32
%int = OpTypeInt 32 0
%int_2 = OpConstant %int 2
%float_arr = OpTypeArray %float %int_2
%struct = OpTypeStruct %float_arr %float_arr
%ptr = OpTypePointer Input %struct
%in = OpVariable %ptr Input
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_VULKAN_1_0);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_VULKAN_1_0));
}

TEST_F(ValidateInterfacesTest, VulkanLocationArrayWithComponent2) {
  const std::string text = R"(
OpCapability Shader
OpCapability Float64
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %in
OpExecutionMode %main OriginUpperLeft
OpDecorate %struct Block
OpMemberDecorate %struct 0 Location 0
OpMemberDecorate %struct 0 Component 0
OpMemberDecorate %struct 1 Location 0
OpMemberDecorate %struct 1 Component 2
%void = OpTypeVoid
%void_fn = OpTypeFunction %void
%float = OpTypeFloat 32
%double = OpTypeFloat 64
%int = OpTypeInt 32 0
%int_2 = OpConstant %int 2
%double_arr = OpTypeArray %double %int_2
%struct = OpTypeStruct %float %double_arr
%ptr = OpTypePointer Input %struct
%in = OpVariable %ptr Input
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_VULKAN_1_0);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_VULKAN_1_0));
}

TEST_F(ValidateInterfacesTest, DuplicateInterfaceVariableSuccess) {
  const std::string text = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %in %out %in
OpExecutionMode %main OriginUpperLeft
OpDecorate %in Location 0
OpDecorate %out Location 0
%void = OpTypeVoid
%float = OpTypeFloat 32
%in_ptr = OpTypePointer Input %float
%out_ptr = OpTypePointer Output %float
%in = OpVariable %in_ptr Input
%out = OpVariable %out_ptr Output
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_VULKAN_1_0);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_VULKAN_1_0));
}

TEST_F(ValidateInterfacesTest, StructWithBuiltinsMissingBlock_Bad) {
  // See https://github.com/KhronosGroup/SPIRV-Registry/issues/134
  //
  // When a shader input or output is a struct that does not have Block,
  // then it must have a Location.
  // But BuiltIns must not have locations.
  const std::string text = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %in
OpExecutionMode %main OriginUpperLeft
; %struct needs a Block decoration
OpMemberDecorate %struct 0 BuiltIn Position
%void = OpTypeVoid
%float = OpTypeFloat 32
%v4float = OpTypeVector %float 4
%struct = OpTypeStruct %v4float
%in_ptr = OpTypePointer Input %struct
%in = OpVariable %in_ptr Input
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_VULKAN_1_0);
  EXPECT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions(SPV_ENV_VULKAN_1_0));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-Location-04919"));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr(
          "Interface struct has no Block decoration but has BuiltIn members."));
}

TEST_F(ValidateInterfacesTest, InvalidLocationTypePointer) {
  const std::string text = R"(
               OpCapability Shader
               OpMemoryModel Logical Simple
               OpEntryPoint Vertex %1 "Aiqn0" %2 %3
               OpDecorate %2 Location 0
       %void = OpTypeVoid
          %5 = OpTypeFunction %void
      %float = OpTypeFloat 32
%_ptr_Private_void = OpTypePointer Private %void
       %uint = OpTypeInt 32 0
%uint_4278132784 = OpConstant %uint 4278132784
%_arr__ptr_Private_void_uint_4278132784 = OpTypeArray %_ptr_Private_void %uint_4278132784
%_ptr_Output__arr__ptr_Private_void_uint_4278132784 = OpTypePointer Output %_arr__ptr_Private_void_uint_4278132784
          %2 = OpVariable %_ptr_Output__arr__ptr_Private_void_uint_4278132784 Output
%_ptr_Output__ptr_Private_void = OpTypePointer Output %_ptr_Private_void
          %3 = OpVariable %_ptr_Output__arr__ptr_Private_void_uint_4278132784 Output
          %1 = OpFunction %void None %5
         %15 = OpLabel
               OpReturn
               OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_VULKAN_1_1);
  EXPECT_EQ(SPV_ERROR_INVALID_DATA, ValidateInstructions(SPV_ENV_VULKAN_1_1));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Invalid type to assign a location"));
}

TEST_F(ValidateInterfacesTest, PhysicalStorageBufferPointer) {
  const std::string text = R"(
OpCapability Shader
OpCapability PhysicalStorageBufferAddresses
OpMemoryModel PhysicalStorageBuffer64 GLSL450
OpEntryPoint Vertex %main "main" %var
OpDecorate %var Location 0
OpDecorate %var RestrictPointer
%void = OpTypeVoid
%uint = OpTypeInt 32 0
%psb_ptr = OpTypePointer PhysicalStorageBuffer %uint
%in_ptr = OpTypePointer Input %psb_ptr
%var = OpVariable %in_ptr Input
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";
  CompileSuccessfully(text, SPV_ENV_VULKAN_1_3);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_3));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-Input-09557"));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Input/Output interface variable id <2> contains a "
                        "PhysicalStorageBuffer pointer, which is not allowed"));
}

TEST_F(ValidateInterfacesTest, PhysicalStorageBufferPointerArray) {
  const std::string text = R"(
  OpCapability Shader
  OpCapability PhysicalStorageBufferAddresses
  OpMemoryModel PhysicalStorageBuffer64 GLSL450
  OpEntryPoint Vertex %main "main" %var
  OpDecorate %var Location 0
  OpDecorate %var RestrictPointer
  %void = OpTypeVoid
  %uint = OpTypeInt 32 0
  %uint_3 = OpConstant %uint 3
  %psb_ptr = OpTypePointer PhysicalStorageBuffer %uint
  %array = OpTypeArray %psb_ptr %uint_3
  %in_ptr = OpTypePointer Input %array
  %var = OpVariable %in_ptr Input
  %void_fn = OpTypeFunction %void
  %main = OpFunction %void None %void_fn
  %entry = OpLabel
  OpReturn
  OpFunctionEnd
  )";
  CompileSuccessfully(text, SPV_ENV_VULKAN_1_3);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_3));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-Input-09557"));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Input/Output interface variable id <2> contains a "
                        "PhysicalStorageBuffer pointer, which is not allowed"));
}
TEST_F(ValidateInterfacesTest, PhysicalStorageBufferPointerStruct) {
  const std::string text = R"(
  OpCapability Shader
  OpCapability PhysicalStorageBufferAddresses
  OpMemoryModel PhysicalStorageBuffer64 GLSL450
  OpEntryPoint Vertex %main "main" %var
  OpDecorate %var Location 0
  OpDecorate %var RestrictPointer
  %void = OpTypeVoid
  %int = OpTypeInt 32 1
  OpTypeForwardPointer %psb_ptr PhysicalStorageBuffer
  %struct_0 = OpTypeStruct %int %psb_ptr
  %struct_1 = OpTypeStruct %int %int
  %psb_ptr = OpTypePointer PhysicalStorageBuffer %struct_1
  %in_ptr = OpTypePointer Input %struct_0
  %var = OpVariable %in_ptr Input
  %void_fn = OpTypeFunction %void
  %main = OpFunction %void None %void_fn
  %entry = OpLabel
  OpReturn
  OpFunctionEnd
  )";
  CompileSuccessfully(text, SPV_ENV_VULKAN_1_3);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_3));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-Input-09557"));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Input/Output interface variable id <2> contains a "
                        "PhysicalStorageBuffer pointer, which is not allowed"));
}

TEST_F(ValidateInterfacesTest, PhysicalStorageBufferPointerArrayOfStruct) {
  const std::string text = R"(
  OpCapability Shader
  OpCapability PhysicalStorageBufferAddresses
  OpMemoryModel PhysicalStorageBuffer64 GLSL450
  OpEntryPoint Vertex %main "main" %var
  OpDecorate %var Location 0
  OpDecorate %var RestrictPointer
  %void = OpTypeVoid
  %int = OpTypeInt 32 1
  %uint = OpTypeInt 32 0
  %uint_3 = OpConstant %uint 3
  OpTypeForwardPointer %psb_ptr PhysicalStorageBuffer
  %array_1 = OpTypeArray %psb_ptr %uint_3
  %struct_0 = OpTypeStruct %int %array_1
   %struct_1 = OpTypeStruct %int %int
  %psb_ptr = OpTypePointer PhysicalStorageBuffer %struct_1
  %array_0 = OpTypeArray %struct_0 %uint_3
  %in_ptr = OpTypePointer Input %array_0
  %var = OpVariable %in_ptr Input
  %void_fn = OpTypeFunction %void
  %main = OpFunction %void None %void_fn
  %entry = OpLabel
  OpReturn
  OpFunctionEnd
  )";
  CompileSuccessfully(text, SPV_ENV_VULKAN_1_3);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_3));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-Input-09557"));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Input/Output interface variable id <2> contains a "
                        "PhysicalStorageBuffer pointer, which is not allowed"));
}

TEST_F(ValidateInterfacesTest, PhysicalStorageBufferPointerNestedStruct) {
  const std::string text = R"(
  OpCapability Shader
  OpCapability PhysicalStorageBufferAddresses
  OpMemoryModel PhysicalStorageBuffer64 GLSL450
  OpEntryPoint Vertex %main "main" %var
  OpDecorate %var Location 0
  OpDecorate %var RestrictPointer
  %void = OpTypeVoid
  %int = OpTypeInt 32 1
  OpTypeForwardPointer %psb_ptr PhysicalStorageBuffer
  %struct_0 = OpTypeStruct %int %psb_ptr
  %struct_1 = OpTypeStruct %int %int
  %psb_ptr = OpTypePointer PhysicalStorageBuffer %struct_1
  %struct_2 = OpTypeStruct %int %struct_0
  %in_ptr = OpTypePointer Input %struct_2
  %var = OpVariable %in_ptr Input
  %void_fn = OpTypeFunction %void
  %main = OpFunction %void None %void_fn
  %entry = OpLabel
  OpReturn
  OpFunctionEnd
  )";
  CompileSuccessfully(text, SPV_ENV_VULKAN_1_3);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_3));
  EXPECT_THAT(getDiagnosticString(),
              AnyVUID("VUID-StandaloneSpirv-Input-09557"));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Input/Output interface variable id <2> contains a "
                        "PhysicalStorageBuffer pointer, which is not allowed"));
}

TEST_F(ValidateInterfacesTest, UntypedVariableInputMissing) {
  const std::string text = R"(
OpCapability Kernel
OpCapability UntypedPointersKHR
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical OpenCL
OpEntryPoint Kernel %main "main"
OpExecutionMode %main LocalSize 1 1 1
OpName %var "var"
OpDecorate %var BuiltIn LocalInvocationId
%void = OpTypeVoid
%int = OpTypeInt 32 0
%int3 = OpTypeVector %int 3
%ptr = OpTypeUntypedPointerKHR Input
%var = OpUntypedVariableKHR %ptr Input %int3
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
%load = OpLoad %int3 %var
OpReturn
OpFunctionEnd
)";
  CompileSuccessfully(text);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions());
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Interface variable id <2> is used by entry point "
                        "'main' id <1>, but is not listed as an interface"));
}

TEST_F(ValidateInterfacesTest, UntypedVariableWorkgroupMissingSpv1p4) {
  const std::string text = R"(
OpCapability Shader
OpCapability UntypedPointersKHR
OpCapability WorkgroupMemoryExplicitLayoutKHR
OpExtension "SPV_KHR_workgroup_memory_explicit_layout"
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
OpEntryPoint GLCompute %main "main"
OpName %var "var"
%void = OpTypeVoid
%int = OpTypeInt 32 0
%ptr = OpTypeUntypedPointerKHR Workgroup
%var = OpUntypedVariableKHR %ptr Workgroup %int
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
%load = OpLoad %int %var
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_UNIVERSAL_1_4);
  EXPECT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_UNIVERSAL_1_4));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Interface variable id <2> is used by entry point "
                        "'main' id <1>, but is not listed as an interface"));
}

TEST_F(ValidateInterfacesTest, UntypedIdMatchesInputVulkan1p3) {
  const std::string text = R"(
OpCapability Shader
OpCapability UntypedPointersKHR
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %var
OpExecutionMode %main OriginUpperLeft
OpDecorate %var DescriptorSet 0
OpDecorate %var Binding 0
OpDecorate %1 Block
OpMemberDecorate %1 0 Offset 0
%void = OpTypeVoid
%float = OpTypeFloat 32
%1 = OpTypeStruct %float ; this id matches Input storage class
%ptr = OpTypeUntypedPointerKHR Uniform
%var = OpUntypedVariableKHR %ptr Uniform %1
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  SetAssembleOptions(SPV_TEXT_TO_BINARY_OPTION_PRESERVE_NUMERIC_IDS);
  CompileSuccessfully(text, SPV_ENV_VULKAN_1_3);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_VULKAN_1_3));
}

TEST_F(ValidateInterfacesTest, UntypedIdMatchesPushConstantVulkan1p3) {
  const std::string text = R"(
OpCapability Shader
OpCapability UntypedPointersKHR
OpExtension "SPV_KHR_untyped_pointers"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %var
OpExecutionMode %main OriginUpperLeft
OpDecorate %var DescriptorSet 0
OpDecorate %var Binding 0
OpDecorate %9 Block
OpMemberDecorate %9 0 Offset 0
%void = OpTypeVoid
%float = OpTypeFloat 32
%9 = OpTypeStruct %float ; this id matches PushConstant storage class
%ptr = OpTypeUntypedPointerKHR Uniform
%var = OpUntypedVariableKHR %ptr Uniform %9
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  SetAssembleOptions(SPV_TEXT_TO_BINARY_OPTION_PRESERVE_NUMERIC_IDS);
  CompileSuccessfully(text, SPV_ENV_VULKAN_1_3);
  EXPECT_EQ(SPV_SUCCESS, ValidateInstructions(SPV_ENV_VULKAN_1_3));
}

TEST_F(ValidateInterfacesTest,
       InvalidBfloat16VariableWithInputOutputStorageClass) {
  const std::string text = R"(
OpCapability Shader
OpCapability BFloat16TypeKHR
OpExtension "SPV_KHR_bfloat16"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %in %out
OpExecutionMode %main OriginUpperLeft
OpDecorate %in Location 0
OpDecorate %out Location 0
%void = OpTypeVoid
%bfloat16 = OpTypeFloat 16 BFloat16KHR
%in_ptr = OpTypePointer Input %bfloat16
%out_ptr = OpTypePointer Output %bfloat16
%in = OpVariable %in_ptr Input
%out = OpVariable %out_ptr Output
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_VULKAN_1_3);
  ASSERT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_3));
  EXPECT_THAT(getDiagnosticString(),
              HasSubstr("Bfloat16 OpVariable <id> '2[%2]' must not be declared "
                        "with a Storage Class of Input or Output.\n"));
}

TEST_F(ValidateInterfacesTest,
       InvalidFP8E4M3VariableWithInputOutputStorageClass) {
  const std::string text = R"(
OpCapability Shader
OpCapability Float8EXT
OpExtension "SPV_EXT_float8"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %in %out
OpExecutionMode %main OriginUpperLeft
OpDecorate %in Location 0
OpDecorate %out Location 0
%void = OpTypeVoid
%fp8e4m3 = OpTypeFloat 8 Float8E4M3EXT
%in_ptr = OpTypePointer Input %fp8e4m3
%out_ptr = OpTypePointer Output %fp8e4m3
%in = OpVariable %in_ptr Input
%out = OpVariable %out_ptr Output
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_VULKAN_1_3);
  ASSERT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_3));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("FP8 E4M3/E5M2 OpVariable <id> '2[%2]' must not be declared "
                "with a Storage Class of Input or Output.\n"));
}

TEST_F(ValidateInterfacesTest,
       InvalidFP8E5M2VariableWithInputOutputStorageClass) {
  const std::string text = R"(
OpCapability Shader
OpCapability Float8EXT
OpExtension "SPV_EXT_float8"
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %in %out
OpExecutionMode %main OriginUpperLeft
OpDecorate %in Location 0
OpDecorate %out Location 0
%void = OpTypeVoid
%fp8e5m2 = OpTypeFloat 8 Float8E5M2EXT
%in_ptr = OpTypePointer Input %fp8e5m2
%out_ptr = OpTypePointer Output %fp8e5m2
%in = OpVariable %in_ptr Input
%out = OpVariable %out_ptr Output
%void_fn = OpTypeFunction %void
%main = OpFunction %void None %void_fn
%entry = OpLabel
OpReturn
OpFunctionEnd
)";

  CompileSuccessfully(text, SPV_ENV_VULKAN_1_3);
  ASSERT_EQ(SPV_ERROR_INVALID_ID, ValidateInstructions(SPV_ENV_VULKAN_1_3));
  EXPECT_THAT(
      getDiagnosticString(),
      HasSubstr("FP8 E4M3/E5M2 OpVariable <id> '2[%2]' must not be declared "
                "with a Storage Class of Input or Output.\n"));
}

}  // namespace
}  // namespace val
}  // namespace spvtools
