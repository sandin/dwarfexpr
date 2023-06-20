#include "dwarfexpr/dwarf_frames.h"

#include <cstdlib>  // std::abs

#include "dwarfexpr/dwarf_expression.h"
#include "dwarfexpr/dwarf_utils.h"

namespace dwarfexpr {

Dwarf_Addr DwarfFrames::GetCfa(const DwarfExpression::Context& context,
                               Dwarf_Addr pc) const {
  Dwarf_Addr cfa = MAX_DWARF_ADDR;
  DwarfFrames::FdeList fde_list;
  if (GetAllFde(&fde_list)) {
    auto fde_list_guard = make_scope_exit([&]() {
      dwarf_dealloc_fde_cie_list(dbg_, fde_list.cie_data,
                                 fde_list.cie_element_count, fde_list.fde_data,
                                 fde_list.fde_element_count);
    });

    Dwarf_Fde fde = nullptr;
    Dwarf_Addr low_pc = 0;
    Dwarf_Addr high_pc = 0;
    Dwarf_Error err = nullptr;
    if (dwarf_get_fde_at_pc(fde_list.fde_data, pc, &fde, &low_pc, &high_pc,
                            &err) == DW_DLV_OK) {
      Dwarf_Off fde_off = 0;
      Dwarf_Off cie_off = 0;
      if (dwarf_fde_section_offset(dbg_, fde, &fde_off, &cie_off, &err) ==
          DW_DLV_OK) {
        printf("fde_off: 0x%llx [0x%llx - 0x%llx], cie_off: 0x%llx\n", fde_off,
               low_pc, high_pc, cie_off);
      }
      for (Dwarf_Addr p = low_pc; p < high_pc; ++p) {
        // cfa
        printf("0x%llx: ", p);
        FdeInfo info = {};
        if (dwarf_get_fde_info_for_cfa_reg3_b(
                fde, p, &info.value_type, &info.offset_relevant, &info.reg,
                &info.offset, &info.block, &info.row_pc, &info.has_more_rows,
                &info.subsequent_pc, &err) == DW_DLV_OK) {
          cfa = GetReg(context, DW_FRAME_CFA_COL, p, info);
          if (cfa == MAX_DWARF_ADDR) {
            printf("Error: can not get the value for cfa\n");
            continue;
          }
          info.cfa = cfa;
          if (info.subsequent_pc > p) {
            p = info.subsequent_pc - 1;
          }
          Dwarf_Bool has_more_rows = info.has_more_rows;

          // other regs
          for (Dwarf_Half x = 0; x < 33; ++x) {  // TODO: arm64 only
            if (dwarf_get_fde_info_for_reg3_b(
                    fde, x, p, &info.value_type, &info.offset_relevant,
                    &info.reg, &info.offset, &info.block, &info.row_pc,
                    &info.has_more_rows, &info.subsequent_pc,
                    &err) == DW_DLV_OK) {
              if (info.row_pc != p) {
                // this pc has no new register value, the last one found still
                // applies hence this is a duplicate row.
                continue;
              }
              GetReg(context, x, p, info);
            }
          }

          if (!has_more_rows) {
            break;
          }
        }
        printf("\n");
      }
      printf("\n");
    }
  }

  return cfa;
}

static inline Dwarf_Signed dwarf_unsigned2signed(Dwarf_Unsigned v) {
  return *reinterpret_cast<Dwarf_Signed*>(&v);
}

static inline std::string regname(int reg) {  // TODO: arm64 only
  if (reg == DW_FRAME_CFA_COL) {
    return "CFA";
  } else if (reg == 29) {
    return "W29(FP)";
  } else if (reg == 30) {
    return "W30(LR)";
  } else if (reg == 31) {
    return "W31(SP)";
  }
  return std::string("W") + std::to_string(reg);
}

Dwarf_Addr DwarfFrames::GetReg(const DwarfExpression::Context& context,
                               Dwarf_Half i, Dwarf_Addr pc,
                               const DwarfFrames::FdeInfo& info) const {
  Dwarf_Addr result = MAX_DWARF_ADDR;
  if ((info.value_type == DW_EXPR_OFFSET ||
       info.value_type == DW_EXPR_VAL_OFFSET) &&
      (info.reg == DW_FRAME_UNDEFINED_VAL || info.reg == DW_FRAME_SAME_VAL)) {
    return result;
  }

  printf("%s=", regname(i).c_str());
  Dwarf_Signed offset = dwarf_unsigned2signed(info.offset);
  switch (info.value_type) {
    case DW_EXPR_OFFSET:
      // src/lib/libdwarf/dwarf_frame.c case DW_CFA_def_cfa:
      // cfa_reg.ru_is_offset = 1; CFA only support:
      // 1. DWARF_LOCATION_REGISTER
      // 2. DWARF_LOCATION_VAL_EXPRESSION
      if (info.offset_relevant != 0 && i != DW_FRAME_CFA_COL) {
        printf("Offset(N): %s%s%lld ", regname(info.reg).c_str(),
               offset >= 0 ? "+" : "-", std::abs(offset));
        Dwarf_Addr addr = info.cfa + offset;

        Dwarf_Addr val =
            DwarfExpression::readMemory(context.memory, addr, MAX_DWARF_ADDR);
        if (val == MAX_DWARF_ADDR) {
          printf("Error: can not read memory at addr 0x%llx\n", addr);
          break;
        }
        result = val;
        printf("addr=0x%llx, val=0x%llx\n", addr, result);
      } else {
        printf("register(R): %s%s%lld ", regname(info.reg).c_str(),
               offset >= 0 ? "+" : "-", std::abs(offset));
        uint64_t reg_val = context.registers(info.reg);
        result = reg_val + offset;
        printf("register=%d, reg_val=0x%llx, offset=%d, val=0x%llx\n", info.reg,
               reg_val, offset, result);
      }
      break;
    case DW_EXPR_VAL_OFFSET: {
      printf("val_offset(N): %s%s%lld ", regname(info.reg).c_str(),
             offset >= 0 ? "+" : "-", std::abs(offset));
      result = info.cfa + offset;
      break;
    }
    case DW_EXPR_EXPRESSION: {
      printf("expression(E): ");
      Dwarf_Addr addr = EvalExpr(context, i, pc, info);
      Dwarf_Addr val =
          DwarfExpression::readMemory(context.memory, addr, MAX_DWARF_ADDR);
      if (val == MAX_DWARF_ADDR) {
        printf("Error: can not read memory at addr 0x%llx\n", addr);
        break;
      }
      result = val;
      break;
    }
    case DW_EXPR_VAL_EXPRESSION: {
      printf("val_expression(E): ");
      result = EvalExpr(context, i, pc, info);
      break;
    }
    default:
      break;
  }
  return result;
}

Dwarf_Addr DwarfFrames::EvalExpr(const DwarfExpression::Context& context,
                                 Dwarf_Half i, Dwarf_Addr pc,
                                 const DwarfFrames::FdeInfo& info) const {
  Dwarf_Addr result = MAX_DWARF_ADDR;

  Dwarf_Loc_Head_c head = 0;
  Dwarf_Unsigned ulistlen = 0;
  Dwarf_Error err = nullptr;
  if (dwarf_loclist_from_expr_c(dbg_, info.block.bl_data, info.block.bl_len,
                                addr_size_, offset_size_, version_, &head,
                                &ulistlen, &err) == DW_DLV_OK) {
    auto guard = make_scope_exit([&]() { dwarf_dealloc_loc_head_c(head); });

    DwarfExpression expr;
    Dwarf_Addr lowAddr = 0;
    Dwarf_Addr highAddr = 0;
    if (DwarfExpression::loadExprFromLoclist(head, 0, &expr, &lowAddr,
                                             &highAddr)) {
      DwarfExpression::Result expr_result = expr.evaluate(context, pc);
      if (expr_result.valid()) {
        result = expr_result.value;
      }  // TODO: print error
    }
  }
  return result;
}

bool DwarfFrames::GetAllFde(DwarfFrames::FdeList* fde_list) const {
  *fde_list = {0};
  Dwarf_Error err = nullptr;

  // try .eh_frame
  if (dwarf_get_fde_list_eh(dbg_, &fde_list->cie_data,
                            &fde_list->cie_element_count, &fde_list->fde_data,
                            &fde_list->fde_element_count, &err) == DW_DLV_OK) {
    return fde_list;
  }

  // .debug_frame
  *fde_list = {0};
  if (dwarf_get_fde_list(dbg_, &fde_list->cie_data,
                         &fde_list->cie_element_count, &fde_list->fde_data,
                         &fde_list->fde_element_count, &err) == DW_DLV_OK) {
    return fde_list;
  }

  return fde_list;
}

};  // namespace dwarfexpr