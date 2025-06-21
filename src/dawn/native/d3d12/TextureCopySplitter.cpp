// Copyright 2017 The Dawn & Tint Authors
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

#include "dawn/native/d3d12/TextureCopySplitter.h"

#include "dawn/common/Assert.h"
#include "dawn/native/Format.h"
#include "dawn/native/d3d12/d3d12_platform.h"

namespace dawn::native::d3d12 {

namespace {
// TODO(424536624): Return a BlockOrigin3D
TexelOrigin3D ComputeTexelOffsets(const TypedTexelBlockInfo& blockInfo,
                                  uint32_t offset,
                                  BlockCount blocksPerRow) {
    DAWN_ASSERT(blocksPerRow != BlockCount{0});
    BlockCount offsetInBlocks = blockInfo.BytesToBlocks(offset);
    BlockCount blockOffsetX = offsetInBlocks % blocksPerRow;
    BlockCount blockOffsetY = offsetInBlocks / blocksPerRow;
    return {blockInfo.ToTexelWidth(blockOffsetX), blockInfo.ToTexelHeight(blockOffsetY),
            TexelCount{0}};
}

uint64_t OffsetToFirstCopiedTexel(const TypedTexelBlockInfo& blockInfo,
                                  BlockCount blocksPerRow,
                                  uint64_t alignedOffset,
                                  TexelOrigin3D bufferOffset) {
    DAWN_ASSERT(bufferOffset.z == TexelCount{0});

    uint64_t offset =
        alignedOffset + blockInfo.ToBytes(blockInfo.ToBlockWidth(bufferOffset.x) +
                                          blocksPerRow * blockInfo.ToBlockHeight(bufferOffset.y));
    return offset;
}

uint64_t AlignDownForDataPlacement(uint64_t offset) {
    return offset & ~static_cast<uint64_t>(D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT - 1);
}

}  // namespace

TextureCopySubresource::CopyInfo* TextureCopySubresource::AddCopy() {
    DAWN_ASSERT(this->count < kMaxTextureCopyRegions);
    return &this->copies[this->count++];
}

TextureCopySubresource Compute2DTextureCopySubresource(Origin3D origin_in,
                                                       Extent3D copySize_in,
                                                       const TexelBlockInfo& blockInfo_in,
                                                       uint64_t offset,
                                                       uint32_t bytesPerRow_in) {
    // TODO(424536624): Rework this function to compute in blocks, then convert to texels
    auto origin = TexelOrigin3D(origin_in);
    auto copySize = TexelExtent3D(copySize_in);
    auto blockInfo = TypedTexelBlockInfo(blockInfo_in);
    auto blocksPerRow = blockInfo.BytesToBlocks(bytesPerRow_in);

    TextureCopySubresource copy;

    // The copies must be 512-aligned. To do this, we calculate the first 512-aligned address
    // preceding our data.
    uint64_t alignedOffset = AlignDownForDataPlacement(offset);

    // If the provided offset to the data was already 512-aligned, we can simply copy the data
    // without further translation.
    if (offset == alignedOffset) {
        TextureCopySubresource::CopyInfo* copyInfo = copy.AddCopy();
        copyInfo->bufferOffset = TexelOrigin3D{{0, 0, 0}};
        copyInfo->textureOffset = origin;
        copyInfo->copySize = copySize;
        copyInfo->alignedOffset = alignedOffset;
        copyInfo->bufferSize = copySize;
        return copy;
    }

    DAWN_ASSERT(alignedOffset < offset);
    DAWN_ASSERT(offset - alignedOffset < D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);

    // We must reinterpret our aligned offset into X and Y offsets with respect to the row
    // pitch.
    //
    // You can visualize the data in the buffer like this:
    // |-----------------------++++++++++++++++++++++++++++++++|
    // ^ 512-aligned address   ^ Aligned offset               ^ End of copy data
    //
    // Now when you consider the row pitch, you can visualize the data like this:
    // |~~~~~~~~~~~~~~~~|
    // |~~~~~+++++++++++|
    // |++++++++++++++++|
    // |+++++~~~~~~~~~~~|
    // |<---row pitch-->|
    //
    // The X and Y offsets calculated in ComputeTexelOffsets can be visualized like this:
    // |YYYYYYYYYYYYYYYY|
    // |XXXXXX++++++++++|
    // |++++++++++++++++|
    // |++++++~~~~~~~~~~|
    // |<---row pitch-->|
    TexelOrigin3D texelOffset =
        ComputeTexelOffsets(blockInfo, static_cast<uint32_t>(offset - alignedOffset), blocksPerRow);

    DAWN_ASSERT(texelOffset.y <= blockInfo.height);
    DAWN_ASSERT(texelOffset.z == TexelCount{0});

    BlockCount copyBlocksPerRowPitch = blockInfo.ToBlockWidth(copySize.width);
    BlockCount blockOffsetInRowPitch = blockInfo.ToBlockWidth(texelOffset.x);
    if (copyBlocksPerRowPitch + blockOffsetInRowPitch <= blocksPerRow) {
        // The region's rows fit inside the bytes per row. In this case, extend the width of the
        // PlacedFootprint and copy the buffer with an offset location
        //  |<------------- bytes per row ------------->|
        //
        //  |-------------------------------------------|
        //  |                                           |
        //  |                 +++++++++++++++++~~~~~~~~~|
        //  |~~~~~~~~~~~~~~~~~+++++++++++++++++~~~~~~~~~|
        //  |~~~~~~~~~~~~~~~~~+++++++++++++++++~~~~~~~~~|
        //  |~~~~~~~~~~~~~~~~~+++++++++++++++++~~~~~~~~~|
        //  |~~~~~~~~~~~~~~~~~+++++++++++++++++         |
        //  |-------------------------------------------|

        // Copy 0:
        //  |----------------------------------|
        //  |                                  |
        //  |                 +++++++++++++++++|
        //  |~~~~~~~~~~~~~~~~~+++++++++++++++++|
        //  |~~~~~~~~~~~~~~~~~+++++++++++++++++|
        //  |~~~~~~~~~~~~~~~~~+++++++++++++++++|
        //  |~~~~~~~~~~~~~~~~~+++++++++++++++++|
        //  |----------------------------------|

        TextureCopySubresource::CopyInfo* copyInfo = copy.AddCopy();
        copyInfo->bufferOffset = texelOffset;
        copyInfo->textureOffset = origin;
        copyInfo->copySize = copySize;
        copyInfo->alignedOffset = alignedOffset;
        copyInfo->bufferSize = {copySize.width + texelOffset.x, copySize.height + texelOffset.y,
                                copySize.depthOrArrayLayers};
        return copy;
    }

    // The region's rows straddle the bytes per row. Split the copy into two copies
    //  |<------------- bytes per row ------------->|
    //
    //  |-------------------------------------------|
    //  |                                           |
    //  |                                   ++++++++|
    //  |+++++++++~~~~~~~~~~~~~~~~~~~~~~~~~~++++++++|
    //  |+++++++++~~~~~~~~~~~~~~~~~~~~~~~~~~++++++++|
    //  |+++++++++~~~~~~~~~~~~~~~~~~~~~~~~~~++++++++|
    //  |+++++++++~~~~~~~~~~~~~~~~~~~~~~~~~~++++++++|
    //  |+++++++++                                  |
    //  |-------------------------------------------|

    //  Copy 0:
    //  |-------------------------------------------|
    //  |                                           |
    //  |                                   ++++++++|
    //  |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~++++++++|
    //  |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~++++++++|
    //  |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~++++++++|
    //  |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~++++++++|
    //  |-------------------------------------------|

    //  Copy 1:
    //  |---------|
    //  |         |
    //  |         |
    //  |+++++++++|
    //  |+++++++++|
    //  |+++++++++|
    //  |+++++++++|
    //  |+++++++++|
    //  |---------|

    // Copy 0
    DAWN_ASSERT(blocksPerRow > blockOffsetInRowPitch);
    const TexelCount texelsPerRow = blockInfo.ToTexelWidth(blocksPerRow);
    const TexelExtent3D copySize0 = {texelsPerRow - texelOffset.x, copySize.height,
                                     copySize.depthOrArrayLayers};
    TextureCopySubresource::CopyInfo* copyInfo0 = copy.AddCopy();
    copyInfo0->bufferOffset = texelOffset;
    copyInfo0->textureOffset = origin;
    copyInfo0->copySize = copySize0;
    copyInfo0->alignedOffset = alignedOffset;
    copyInfo0->bufferSize = {texelsPerRow, copySize.height + texelOffset.y,
                             copySize.depthOrArrayLayers};

    // Copy 1
    const uint64_t offsetForCopy1 =
        offset + blockInfo.ToBytes(blockInfo.ToBlockWidth(copySize0.width));
    const uint64_t alignedOffsetForCopy1 = AlignDownForDataPlacement(offsetForCopy1);
    const TexelOrigin3D texelOffsetForCopy1 = ComputeTexelOffsets(
        blockInfo, static_cast<uint32_t>(offsetForCopy1 - alignedOffsetForCopy1), blocksPerRow);

    DAWN_ASSERT(texelOffsetForCopy1.y <= blockInfo.height);
    DAWN_ASSERT(texelOffsetForCopy1.z == TexelCount{0});

    const TexelOrigin3D textureOffset1 = {origin.x + copySize0.width, origin.y, origin.z};

    DAWN_ASSERT(copySize.width > copySize0.width);
    const TexelExtent3D copySize1 = {copySize.width - copySize0.width, copySize.height,
                                     copySize.depthOrArrayLayers};

    const TexelOrigin3D bufferOffset1 = texelOffsetForCopy1;
    const TexelExtent3D bufferSize1 = {copySize1.width + texelOffsetForCopy1.x,
                                       copySize.height + texelOffsetForCopy1.y,
                                       copySize.depthOrArrayLayers};

    TextureCopySubresource::CopyInfo* copyInfo1 = copy.AddCopy();
    copyInfo1->bufferOffset = bufferOffset1;
    copyInfo1->textureOffset = textureOffset1;
    copyInfo1->copySize = copySize1;
    copyInfo1->alignedOffset = alignedOffsetForCopy1;
    copyInfo1->bufferSize = bufferSize1;
    return copy;
}

TextureCopySplits Compute2DTextureCopySplits(Origin3D origin,
                                             Extent3D copySize,
                                             const TexelBlockInfo& blockInfo,
                                             uint64_t offset,
                                             uint32_t bytesPerRow,
                                             uint32_t rowsPerImage) {
    TextureCopySplits copies;

    const uint64_t bytesPerLayer = bytesPerRow * rowsPerImage;

    // The function Compute2DTextureCopySubresource() decides how to split the copy based on:
    // - the alignment of the buffer offset with D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT (512)
    // - the alignment of the buffer offset with D3D12_TEXTURE_DATA_PITCH_ALIGNMENT (256)
    // Each layer of a 2D array might need to be split, but because of the WebGPU
    // constraint that "bytesPerRow" must be a multiple of 256, all odd (resp. all even) layers
    // will be at an offset multiple of 512 of each other, which means they will all result in
    // the same 2D split. Thus we can just compute the copy splits for the first and second
    // layers, and reuse them for the remaining layers by adding the related offset of each
    // layer. Moreover, if "rowsPerImage" is even, both the first and second copy layers can
    // share the same copy split, so in this situation we just need to compute copy split once
    // and reuse it for all the layers.
    Extent3D copyOneLayerSize = copySize;
    Origin3D copyFirstLayerOrigin = origin;
    copyOneLayerSize.depthOrArrayLayers = 1;
    copyFirstLayerOrigin.z = 0;

    copies.copySubresources[0] = Compute2DTextureCopySubresource(
        copyFirstLayerOrigin, copyOneLayerSize, blockInfo, offset, bytesPerRow);

    // When the copy only refers one texture 2D array layer,
    // copies.copySubresources[1] will never be used so we can safely early return here.
    if (copySize.depthOrArrayLayers == 1) {
        return copies;
    }

    if (bytesPerLayer % D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT == 0) {
        copies.copySubresources[1] = copies.copySubresources[0];
        uint64_t alignedOffset0 =
            copies.copySubresources[1].copies[0].alignedOffset + bytesPerLayer;
        uint64_t alignedOffset1 =
            copies.copySubresources[1].copies[1].alignedOffset + bytesPerLayer;
        copies.copySubresources[1].copies[0].alignedOffset = alignedOffset0;
        copies.copySubresources[1].copies[1].alignedOffset = alignedOffset1;
    } else {
        const uint64_t bufferOffsetNextLayer = offset + bytesPerLayer;
        copies.copySubresources[1] = Compute2DTextureCopySubresource(
            copyFirstLayerOrigin, copyOneLayerSize, blockInfo, bufferOffsetNextLayer, bytesPerRow);
    }

    return copies;
}

void Recompute3DTextureCopyRegionWithEmptyFirstRowAndEvenCopyHeight(
    Origin3D origin_in,
    Extent3D copySize_in,
    const TexelBlockInfo& blockInfo_in,
    uint32_t bytesPerRow_in,
    uint32_t rowsPerImage_in,
    TextureCopySubresource& copy,
    uint32_t i) {
    // TODO(424536624): Rework this function to compute in blocks, then convert to texels
    auto origin = TexelOrigin3D(origin_in);
    auto copySize = TexelExtent3D(copySize_in);
    auto blockInfo = TypedTexelBlockInfo(blockInfo_in);
    auto rowsPerImage = BlockCount(rowsPerImage_in);
    auto blocksPerRow = blockInfo.BytesToBlocks(bytesPerRow_in);

    // Let's assign data and show why copy region generated by ComputeTextureCopySubresource
    // is incorrect if there is an empty row at the beginning of the copy block.
    // Assuming that bytesPerRow is 256 and we are doing a B2T copy, and copy size is {width: 2,
    // height: 4, depthOrArrayLayers: 3}. Then the data layout in buffer is demonstrated
    // as below:
    //
    //               |<----- bytes per row ------>|
    //
    //               |----------------------------|
    //  row (N - 1)  |                            |
    //  row N        |                 ++~~~~~~~~~|
    //  row (N + 1)  |~~~~~~~~~~~~~~~~~++~~~~~~~~~|
    //  row (N + 2)  |~~~~~~~~~~~~~~~~~++~~~~~~~~~|
    //  row (N + 3)  |~~~~~~~~~~~~~~~~~++~~~~~~~~~|
    //  row (N + 4)  |~~~~~~~~~~~~~~~~~++~~~~~~~~~|
    //  row (N + 5)  |~~~~~~~~~~~~~~~~~++~~~~~~~~~|
    //  row (N + 6)  |~~~~~~~~~~~~~~~~~++~~~~~~~~~|
    //  row (N + 7)  |~~~~~~~~~~~~~~~~~++~~~~~~~~~|
    //  row (N + 8)  |~~~~~~~~~~~~~~~~~++~~~~~~~~~|
    //  row (N + 9)  |~~~~~~~~~~~~~~~~~++~~~~~~~~~|
    //  row (N + 10) |~~~~~~~~~~~~~~~~~++~~~~~~~~~|
    //  row (N + 11) |~~~~~~~~~~~~~~~~~++         |
    //               |----------------------------|

    // The copy we mean to do is the following:
    //
    //   - image 0: row N to row (N + 3),
    //   - image 1: row (N + 4) to row (N + 7),
    //   - image 2: row (N + 8) to row (N + 11).
    //
    // Note that alignedOffset is at the beginning of row (N - 1), while buffer offset makes
    // the copy start at row N. Row (N - 1) is the empty row between alignedOffset and offset.
    //
    // The 2D copy region of image 0 we received from Compute2DTextureCopySubresource() is
    // the following:
    //
    //              |-------------------|
    //  row (N - 1) |                   |
    //  row N       |                 ++|
    //  row (N + 1) |~~~~~~~~~~~~~~~~~++|
    //  row (N + 2) |~~~~~~~~~~~~~~~~~++|
    //  row (N + 3) |~~~~~~~~~~~~~~~~~++|
    //              |-------------------|
    //
    // However, if we simply expand the copy region of image 0 to all depth ranges of a 3D
    // texture, we will copy 5 rows every time, and every first row of each slice will be
    // skipped. As a result, the copied data will be:
    //
    //   - image 0: row N to row (N + 3), which is correct. Row (N - 1) is skipped.
    //   - image 1: row (N + 5) to row (N + 8) because row (N + 4) is skipped. It is incorrect.
    //
    // Likewise, all other image followed will be incorrect because we wrongly keep skipping
    // one row for each depth slice.
    //
    // Solution: split the copy region to two copies: copy 3 (rowsPerImage - 1) rows and
    // expand to all depth slices in the first copy. 3 rows + one skipped rows = 4 rows, which
    // equals to rowsPerImage. Then copy the last row in the second copy. However, the copy
    // block of the last row of the last image may out-of-bound (see the details below), so
    // we need an extra copy for the very last row.

    // Copy 0: copy 3 rows, not 4 rows.
    //                _____________________
    //               /                    /|
    //              /                    / |
    //              |-------------------|  |
    //  row (N - 1) |                   |  |
    //  row N       |                 ++|  |
    //  row (N + 1) |~~~~~~~~~~~~~~~~~++| /
    //  row (N + 2) |~~~~~~~~~~~~~~~~~++|/
    //              |-------------------|

    // Copy 1: move down two rows and copy the last row on image 0, and expand to
    // copySize.depthOrArrayLayers - 1 depth slices. Note that if we expand it to all depth
    // slices, the last copy block will be row (N + 9) to row (N + 12). Row (N + 11) might
    // be the last row of the entire buffer. Then row (N + 12) will be out-of-bound.
    //                _____________________
    //               /                    /|
    //              /                    / |
    //              |-------------------|  |
    //  row (N + 1) |                   |  |
    //  row (N + 2) |                   |  |
    //  row (N + 3) |                 ++| /
    //  row (N + 4) |~~~~~~~~~~~~~~~~~~~|/
    //              |-------------------|
    //
    //  copy 2: copy the last row of the last image.
    //              |-------------------|
    //  row (N + 11)|                 ++|
    //              |-------------------|

    // Copy 0: copy copySize0.height - 1 rows
    TextureCopySubresource::CopyInfo& copy0 = copy.copies[i];
    copy0.copySize.height = copySize.height - blockInfo.height;
    copy0.bufferSize.height = blockInfo.ToTexelHeight(rowsPerImage);  // rowsPerImageInTexels

    // Copy 1: move down 2 rows and copy the last row on image 0, and expand to all depth slices
    // but the last one.
    TextureCopySubresource::CopyInfo* copy1 = copy.AddCopy();
    *copy1 = copy0;
    copy1->alignedOffset = copy1->alignedOffset + 2 * blockInfo.ToBytes(blocksPerRow);
    copy1->textureOffset.y += copySize.height - blockInfo.height;
    // Offset two rows from the copy height for bufferOffset1 (See the figure above):
    //   - one for the row we advanced in the buffer: row (N + 4).
    //   - one for the last row we want to copy: row (N + 3) itself.
    copy1->bufferOffset.y = copySize.height - TexelCount{2} * blockInfo.height;
    copy1->copySize.height = blockInfo.height;
    copy1->copySize.depthOrArrayLayers--;
    copy1->bufferSize.depthOrArrayLayers--;

    // Copy 2: copy the last row of the last image.
    uint64_t offsetForCopy0 =
        OffsetToFirstCopiedTexel(blockInfo, blocksPerRow, copy0.alignedOffset, copy0.bufferOffset);
    uint64_t offsetForLastRowOfLastImage =
        offsetForCopy0 +
        blockInfo.ToBytes(
            blocksPerRow *
            (blockInfo.ToBlockHeight(copy0.copySize.height) +
             rowsPerImage * (blockInfo.ToBlockDepth(copySize.depthOrArrayLayers) - BlockCount{1})));

    uint64_t alignedOffsetForLastRowOfLastImage =
        AlignDownForDataPlacement(offsetForLastRowOfLastImage);
    TexelOrigin3D texelOffsetForLastRowOfLastImage = ComputeTexelOffsets(
        blockInfo,
        static_cast<uint32_t>(offsetForLastRowOfLastImage - alignedOffsetForLastRowOfLastImage),
        blocksPerRow);

    TextureCopySubresource::CopyInfo* copy2 = copy.AddCopy();
    copy2->alignedOffset = alignedOffsetForLastRowOfLastImage;
    copy2->textureOffset = copy1->textureOffset;
    copy2->textureOffset.z = origin.z + copySize.depthOrArrayLayers - TexelCount{1};
    copy2->copySize = copy1->copySize;
    copy2->copySize.depthOrArrayLayers = TexelCount{1};
    copy2->bufferOffset = texelOffsetForLastRowOfLastImage;
    copy2->bufferSize.width = copy1->bufferSize.width;
    DAWN_ASSERT(copy2->copySize.height == blockInfo.height);
    copy2->bufferSize.height = copy2->bufferOffset.y + copy2->copySize.height;
    copy2->bufferSize.depthOrArrayLayers = TexelCount{1};
}

void Recompute3DTextureCopyRegionWithEmptyFirstRowAndOddCopyHeight(
    Extent3D copySize_in,
    const TexelBlockInfo& blockInfo_in,
    uint32_t bytesPerRow_in,
    TextureCopySubresource& copy,
    uint32_t i) {
    // TODO(424536624): Rework this function to compute in blocks, then convert to texels
    auto copySize = TexelExtent3D(copySize_in);
    auto blockInfo = TypedTexelBlockInfo(blockInfo_in);
    auto blocksPerRow = blockInfo.BytesToBlocks(bytesPerRow_in);

    // Read the comments of Recompute3DTextureCopyRegionWithEmptyFirstRowAndEvenCopyHeight() for
    // the reason why it is incorrect if we simply extend the copy region to all depth slices
    // when there is an empty first row at the copy region.
    //
    // If the copy height is odd, we can use two copies to make it correct:
    //   - copy 0: only copy the first depth slice. Keep other arguments the same.
    //   - copy 1: copy all rest depth slices because it will start without an empty row if
    //     copy height is odd. Odd height + one (empty row) is even. An even row number times
    //     bytesPerRow (256) will be aligned to D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT (512)

    // Copy 0: copy the first depth slice (image 0)
    TextureCopySubresource::CopyInfo& copy0 = copy.copies[i];
    copy0.copySize.depthOrArrayLayers = TexelCount{1};
    const TexelCount kBufferDepth0 = TexelCount{1};
    copy0.bufferSize.depthOrArrayLayers = kBufferDepth0;

    // Copy 1: copy the rest depth slices in one shot
    TextureCopySubresource::CopyInfo* copy1 = copy.AddCopy();
    *copy1 = copy0;
    DAWN_ASSERT(blockInfo.ToBlockHeight(copySize.height) % BlockCount{2} == BlockCount{1});
    copy1->alignedOffset += blockInfo.ToBytes(
        (blockInfo.ToBlockHeight(copySize.height) + BlockCount{1}) * blocksPerRow);

    DAWN_ASSERT(copy1->alignedOffset % D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT == 0);
    // textureOffset1.z should add one because the first slice has already been copied in copy0.
    copy1->textureOffset.z++;
    // bufferOffset1.y should be 0 because we skipped the first depth slice and there is no empty
    // row in this copy region.
    copy1->bufferOffset.y = TexelCount{0};
    copy1->copySize.height = copySize.height;
    copy1->copySize.depthOrArrayLayers = copySize.depthOrArrayLayers - TexelCount{1};
    copy1->bufferSize.height = copySize.height;
    copy1->bufferSize.depthOrArrayLayers = copySize.depthOrArrayLayers - TexelCount{1};
}

TextureCopySubresource Compute3DTextureCopySplits(Origin3D origin_in,
                                                  Extent3D copySize_in,
                                                  const TexelBlockInfo& blockInfo_in,
                                                  uint64_t offset,
                                                  uint32_t bytesPerRow,
                                                  uint32_t rowsPerImage_in) {
    // TODO(424536624): Rework this function to compute in blocks, then convert to texels
    auto origin = TexelOrigin3D(origin_in);
    auto copySize = TexelExtent3D(copySize_in);
    auto blockInfo = TypedTexelBlockInfo(blockInfo_in);
    auto rowsPerImage = BlockCount(rowsPerImage_in);

    // To compute the copy region(s) for 3D textures, we call Compute2DTextureCopySubresource
    // and get copy region(s) for the first slice of the copy, then extend to all depth slices
    // and become a 3D copy. However, this doesn't work as easily as that due to some corner
    // cases.
    //
    // For example, if bufferHeight is greater than rowsPerImage in each generated copy
    // region and we simply extend the 2D copy region to all copied depth slices, copied data
    // will be incorrectly offset for each depth slice except the first one.
    //
    // For these special cases, we need to recompute the copy regions for 3D textures by
    // splitting the incorrect copy region to a couple more copy regions.

    // Call Compute2DTextureCopySubresource and get copy regions. This function has already
    // forwarded "copySize.depthOrArrayLayers" to all depth slices.
    TextureCopySubresource copySubresource =
        Compute2DTextureCopySubresource(origin.ToOrigin3D(), copySize.ToExtent3D(),
                                        blockInfo.ToTexelBlockInfo(), offset, bytesPerRow);

    DAWN_ASSERT(copySubresource.count <= 2);
    // If copySize.depthOrArrayLayers is 1, we can return copySubresource. Because we don't need to
    // extend the copy region(s) to other depth slice(s).
    if (copySize.depthOrArrayLayers == TexelCount{1}) {
        return copySubresource;
    }

    TexelCount rowsPerImageInTexels = blockInfo.ToTexelHeight(rowsPerImage);
    // The copy region(s) generated by Compute2DTextureCopySubresource might be incorrect.
    // However, we may append a couple more copy regions in the for loop below. We don't need
    // to revise these new added copy regions.
    uint32_t originalCopyCount = copySubresource.count;
    for (uint32_t i = 0; i < originalCopyCount; ++i) {
        // There can be one empty row at most in a copy region.
        TexelCount bufferHeight = copySubresource.copies[i].bufferSize.height;
        DAWN_ASSERT(bufferHeight <= rowsPerImageInTexels + blockInfo.height);

        if (bufferHeight == rowsPerImageInTexels) {
            // If the copy region's bufferHeight equals to rowsPerImageInTexels, we can use this
            // copy region without any modification.
            continue;
        }

        if (bufferHeight < rowsPerImageInTexels) {
            // If we are copying multiple depth slices, we should skip rowsPerImageInTexels rows for
            // each slice even though we only copy partial rows in each slice sometimes.
            copySubresource.copies[i].bufferSize.height = rowsPerImageInTexels;
        } else {
            // bufferHeight > rowsPerImageInTexels. There is an empty row in this copy region due to
            // alignment adjustment.

            // bytesPerRow is definitely 256, and it is definitely a full copy on height.
            // Otherwise, bufferHeight won't be greater than rowsPerImageInTexels and there won't be
            // an empty row at the beginning of this copy region.
            DAWN_ASSERT(bytesPerRow == D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
            DAWN_ASSERT(copySize.height == rowsPerImageInTexels);

            const BlockCount copyHeightInBlocks = blockInfo.ToBlockHeight(copySize.height);
            if (static_cast<uint32_t>(copyHeightInBlocks) % 2 == 0) {
                // If copyHeightInBlocks is even and there is an empty row at the beginning of the
                // first slice of the copy region, the offset of all depth slices will never be
                // aligned to D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT (512) and there is always
                // an empty row at each depth slice. We need a totally different approach to
                // split the copy region.
                Recompute3DTextureCopyRegionWithEmptyFirstRowAndEvenCopyHeight(
                    origin.ToOrigin3D(), copySize.ToExtent3D(), blockInfo.ToTexelBlockInfo(),
                    bytesPerRow, static_cast<uint32_t>(rowsPerImage), copySubresource, i);
            } else {
                // If copyHeightInBlocks is odd and there is an empty row at the beginning of the
                // first slice of the copy region, we can split the copy region into two copies:
                // copy0 to copy the first slice, copy1 to copy the rest slices because the
                // offset of slice 1 is aligned to D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT (512)
                // without an empty row. This is an easier case relative to cases with even copy
                // height.
                Recompute3DTextureCopyRegionWithEmptyFirstRowAndOddCopyHeight(
                    copySize.ToExtent3D(), blockInfo.ToTexelBlockInfo(), bytesPerRow,
                    copySubresource, i);
            }
        }
    }

    return copySubresource;
}

TextureCopySubresource Compute2DTextureCopySubresourceWithRelaxedRowPitchAndOffset(
    Origin3D origin_in,
    Extent3D copySize_in,
    const TexelBlockInfo& blockInfo_in,
    uint64_t offset,
    uint32_t /*bytesPerRow*/) {
    // TODO(424536624): Rework this function to compute in blocks, then convert to texels
    auto origin = TexelOrigin3D(origin_in);
    auto copySize = TexelExtent3D(copySize_in);

    TextureCopySubresource copy;
    auto* copyInfo = copy.AddCopy();

    // You can visualize the data in the buffer (bufferLocation) like this:
    // * copy data is visualized as '+'.
    //
    //                bufferOffset(0, 0, 0)
    //                        ^
    //                        |
    // |<-------Offset------->|<-----------RowPitch----------->|----------|
    // |----------------------|++++++++++++++++++++++~~~~~~~~~~|    |     |
    //                        |++++++++++++++++++++++~~~~~~~~~~|CopyHeight|
    //                        |++++++++++++++++++++++|         |    |     |
    //                        |<-----CopyWidth------>|         |----------|
    //
    copyInfo->textureOffset = {origin.x, origin.y, TexelCount{0}};
    copyInfo->bufferOffset = TexelOrigin3D{{0, 0, 0}};
    copyInfo->copySize = {copySize.width, copySize.height, TexelCount{1}};
    copyInfo->alignedOffset = offset;
    copyInfo->bufferSize = {copySize.width, copySize.height, TexelCount{1}};
    return copy;
}

TextureCopySubresource Compute3DTextureCopySubresourceWithRelaxedRowPitchAndOffset(
    Origin3D origin_in,
    Extent3D copySize_in,
    const TexelBlockInfo& blockInfo_in,
    uint64_t offset,
    uint32_t bytesPerRow_in,
    uint32_t rowsPerImage_in) {
    // TODO(424536624): Rework this function to compute in blocks, then convert to texels
    auto origin = TexelOrigin3D(origin_in);
    auto copySize = TexelExtent3D(copySize_in);
    auto blockInfo = TypedTexelBlockInfo(blockInfo_in);
    auto rowsPerImage = BlockCount(rowsPerImage_in);
    auto blocksPerRow = blockInfo.BytesToBlocks(bytesPerRow_in);

    TextureCopySubresource copy;

    Origin3D bufferOffset = {0, 0, 0};

    // You can visualize the data in the buffer (bufferLocation) like the inline comments.
    // * copy data is visualized as '+'.
    const TexelCount depthInCopy1 = copySize.depthOrArrayLayers - TexelCount{1};
    if (depthInCopy1 > TexelCount{0}) {
        // `bufferLocation` in the 1st copy (first `depthInCopy1` images, optional):
        //
        //                bufferOffset(0, 0, 0)
        //                        ^
        //                        |
        // |<-------Offset1------>|<-----------RowPitch----------->|----------|------------|
        // |----------------------|++++++++++++++++++++++~~~~~~~~~~|    |     |     |      |
        //                        |++++++++++++++++++++++~~~~~~~~~~|CopyHeight|     |      |
        //                        |++++++++++++++++++++++~~~~~~~~~~|    |     |RowsPerImage|
        //                        |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|----------|     |      |
        //                        |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|          |     |      |
        // |---End of 1st image-->|--------------------------------|----------|------------|
        //                        |++++++++++++++++++++++~~~~~~~~~~|          |     |      |
        //                        |++++++++++++++++++++++~~~~~~~~~~|          |     |      |
        //                        |++++++++++++++++++++++~~~~~~~~~~|          |RowsPerImage|
        //                        |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|          |     |      |
        //                        |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|          |     |      |
        // |---End of 2nd image-->|--------------------------------|----------|------------|
        //                        |<-----CopyWidth------>|
        //
        auto* copyInfo1 = copy.AddCopy();
        copyInfo1->bufferOffset = bufferOffset;
        copyInfo1->textureOffset = origin;
        copyInfo1->copySize = {copySize.width, copySize.height, depthInCopy1};
        copyInfo1->alignedOffset = offset;
        copyInfo1->bufferSize = {copySize.width, blockInfo.ToTexelHeight(rowsPerImage),
                                 depthInCopy1};
    }

    {
        // We have to use the 2nd copy because there may not be enough memory to hold
        // (RowPitch * RowsPerImage) data for the last image in the buffer.
        //
        // `bufferLocation` in the 2nd copy (the last image):
        //
        //                bufferOffset (0, 0, 0)
        //                Begin of the last image
        //                        ^
        //                        |
        // |<-------Offset2------>|<-----------RowPitch----------->|----------|
        // |----------------------|++++++++++++++++++++++~~~~~~~~~~|    |     |
        //                        |++++++++++++++++++++++~~~~~~~~~~|CopyHeight|
        //                        |++++++++++++++++++++++|         |    |     |
        //                        |----------------------|---------|----------|
        //                        |<-----CopyWidth------>|
        //                                               ^
        //                                     End of all buffer data
        //
        DAWN_ASSERT(copySize.depthOrArrayLayers >= TexelCount{1});
        constexpr TexelCount depthInCopy2{1};
        const TexelCount rowsPerImageInTexels2 = copySize.height;

        auto* copyInfo2 = copy.AddCopy();
        copyInfo2->bufferOffset = bufferOffset;
        copyInfo2->textureOffset = {origin.x, origin.y, origin.z + depthInCopy1};
        copyInfo2->copySize = {copySize.width, copySize.height, depthInCopy2};
        copyInfo2->alignedOffset = offset + blockInfo.ToBytes(blocksPerRow * rowsPerImage *
                                                              blockInfo.ToBlockDepth(depthInCopy1));
        copyInfo2->bufferSize = {copySize.width, rowsPerImageInTexels2, depthInCopy2};
    }

    return copy;
}

}  // namespace dawn::native::d3d12
