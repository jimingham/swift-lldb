//===-- DWARFDebugAranges.cpp -----------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "DWARFDebugAranges.h"

#include <assert.h>
#include <stdio.h>

#include <algorithm>

#include "lldb/Utility/Stream.h"
#include "lldb/Utility/Timer.h"

#include "DWARFUnit.h"
#include "DWARFDebugInfo.h"
#include "SymbolFileDWARF.h"

using namespace lldb;
using namespace lldb_private;

//----------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------
DWARFDebugAranges::DWARFDebugAranges() : m_aranges() {}

//----------------------------------------------------------------------
// CountArangeDescriptors
//----------------------------------------------------------------------
class CountArangeDescriptors {
public:
  CountArangeDescriptors(uint32_t &count_ref) : count(count_ref) {
    //      printf("constructor CountArangeDescriptors()\n");
  }
  void operator()(const DWARFDebugArangeSet &set) {
    count += set.NumDescriptors();
  }
  uint32_t &count;
};

//----------------------------------------------------------------------
// Extract
//----------------------------------------------------------------------
llvm::Error
DWARFDebugAranges::extract(const DWARFDataExtractor &debug_aranges_data) {
  assert(debug_aranges_data.ValidOffset(0));

  lldb::offset_t offset = 0;

  DWARFDebugArangeSet set;
  Range range;
  while (debug_aranges_data.ValidOffset(offset)) {
    llvm::Error error = set.extract(debug_aranges_data, &offset);
    if (!error)
      return error;

    const uint32_t num_descriptors = set.NumDescriptors();
    if (num_descriptors > 0) {
      const dw_offset_t cu_offset = set.GetCompileUnitDIEOffset();

      for (uint32_t i = 0; i < num_descriptors; ++i) {
        const DWARFDebugArangeSet::Descriptor &descriptor =
            set.GetDescriptorRef(i);
        m_aranges.Append(RangeToDIE::Entry(descriptor.address,
                                           descriptor.length, cu_offset));
      }
    }
    set.Clear();
  }
  return llvm::ErrorSuccess();
}

//----------------------------------------------------------------------
// Generate
//----------------------------------------------------------------------
bool DWARFDebugAranges::Generate(SymbolFileDWARF *dwarf2Data) {
  Clear();
  DWARFDebugInfo *debug_info = dwarf2Data->DebugInfo();
  if (debug_info) {
    uint32_t cu_idx = 0;
    const uint32_t num_compile_units = dwarf2Data->GetNumCompileUnits();
    for (cu_idx = 0; cu_idx < num_compile_units; ++cu_idx) {
      DWARFUnit *cu = debug_info->GetCompileUnitAtIndex(cu_idx);
      if (cu)
        cu->BuildAddressRangeTable(dwarf2Data, this);
    }
  }
  return !IsEmpty();
}

void DWARFDebugAranges::Dump(Log *log) const {
  if (log == NULL)
    return;

  const size_t num_entries = m_aranges.GetSize();
  for (size_t i = 0; i < num_entries; ++i) {
    const RangeToDIE::Entry *entry = m_aranges.GetEntryAtIndex(i);
    if (entry)
      log->Printf("0x%8.8x: [0x%" PRIx64 " - 0x%" PRIx64 ")", entry->data,
                  entry->GetRangeBase(), entry->GetRangeEnd());
  }
}

void DWARFDebugAranges::AppendRange(dw_offset_t offset, dw_addr_t low_pc,
                                    dw_addr_t high_pc) {
  if (high_pc > low_pc)
    m_aranges.Append(RangeToDIE::Entry(low_pc, high_pc - low_pc, offset));
}

void DWARFDebugAranges::Sort(bool minimize) {
  static Timer::Category func_cat(LLVM_PRETTY_FUNCTION);
  Timer scoped_timer(func_cat, "%s this = %p", LLVM_PRETTY_FUNCTION,
                     static_cast<void *>(this));

  m_aranges.Sort();
  m_aranges.CombineConsecutiveEntriesWithEqualData();
}

//----------------------------------------------------------------------
// FindAddress
//----------------------------------------------------------------------
dw_offset_t DWARFDebugAranges::FindAddress(dw_addr_t address) const {
  const RangeToDIE::Entry *entry = m_aranges.FindEntryThatContains(address);
  if (entry)
    return entry->data;
  return DW_INVALID_OFFSET;
}
