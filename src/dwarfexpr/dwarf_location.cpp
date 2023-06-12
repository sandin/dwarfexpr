#include "dwarfexpr/dwarf_location.h"

#include "dwarfexpr/dwarf_utils.h"

namespace dwarfexpr {

/*  DW_FORM_data16 should not apply here. */
static bool is_simple_location_expr(int form) {
  if (form == DW_FORM_block1 || form == DW_FORM_block2 ||
      form == DW_FORM_block4 || form == DW_FORM_block ||
      form == DW_FORM_exprloc) {
    return true;
  }
  return false;
}
static bool is_location_form(int form) {
  if (form == DW_FORM_data4 || form == DW_FORM_data8 ||
      form == DW_FORM_sec_offset || form == DW_FORM_loclistx ||
      form == DW_FORM_rnglistx) {
    return true;
  }
  return false;
}

bool DwarfLocation::load() {
  Dwarf_Half form = 0;
  Dwarf_Error error = nullptr;
  if (dwarf_whatform(attr_, &form, &error) != DW_DLV_OK) {
    return false;
  }

  const char* form_name;
  if (dwarf_get_FORM_name(form, &form_name) != DW_DLV_OK) {
    return false;
  }
  printf("DW_AT_location: form_name=%s\n", form_name);

  if (is_simple_location_expr(form)) {
    printf("is_simple_location_expr\n");
    // Get expression location pointer.
    Dwarf_Unsigned retExprLen;     // Length of location expression.
    Dwarf_Ptr blockPtr = nullptr;  // Pointer to location expression.
    if (dwarf_formexprloc(attr_, &retExprLen, &blockPtr, &error) != DW_DLV_OK) {
      return false;
    }

    Dwarf_Unsigned cnt;  // number of list records
    Dwarf_Loc_Head_c loclist_head = nullptr;
    if (dwarf_loclist_from_expr_c(dbg_, blockPtr, retExprLen, addr_size_,
                                  offset_size_, version_, &loclist_head,
                                  &cnt,  // should be set to 1.
                                  &error) != DW_DLV_OK) {
      return false;
    }
    auto guard =
        make_scope_exit([&]() { dwarf_dealloc_loc_head_c(loclist_head); });

    if (cnt > 0) {
      loadLocDescEntry(loclist_head, 0);
    }
    return true;

  } else if (is_location_form(form)) {
    printf("is_location_form\n");
    Dwarf_Unsigned cnt;  // number of list records
    Dwarf_Loc_Head_c loclist_head = nullptr;

    if (dwarf_get_loclist_c(attr_, &loclist_head, &cnt, &error) != DW_DLV_OK) {
      return false;
    }
    auto guard =
        make_scope_exit([&]() { dwarf_dealloc_loc_head_c(loclist_head); });

    for (Dwarf_Unsigned i = 0; i < cnt; ++i) {
      loadLocDescEntry(loclist_head, i);
    }
    return true;
  } else {
    return false;  // unsupport form
  }

  return true;
}

bool DwarfLocation::loadLocDescEntry(Dwarf_Loc_Head_c loclist_head,
                                     Dwarf_Unsigned idx) {
  Dwarf_Error error = nullptr;
  Dwarf_Small loclist_lkind = 0;
  Dwarf_Small lle_value = 0;
  Dwarf_Unsigned rawval1 = 0;
  Dwarf_Unsigned rawval2 = 0;
  Dwarf_Bool debug_addr_unavailable = false;
  Dwarf_Addr lopc = 0;
  Dwarf_Addr hipc = 0;
  Dwarf_Unsigned loclist_expr_op_count = 0;
  Dwarf_Locdesc_c locdesc_entry = 0;  // list of location descriptions
  Dwarf_Unsigned expression_offset = 0;
  Dwarf_Unsigned locdesc_offset = 0;

  if (dwarf_get_locdesc_entry_d(loclist_head, idx, &lle_value, &rawval1,
                                &rawval2, &debug_addr_unavailable, &lopc, &hipc,
                                &loclist_expr_op_count, &locdesc_entry,
                                &loclist_lkind, &expression_offset,
                                &locdesc_offset, &error) != DW_DLV_OK) {
    return false;
  }

  Expression expr;
  expr.lowAddr = lopc;
  expr.highAddr = hipc;
  printf("[0x%llx - 0x%llx): ", expr.lowAddr, expr.highAddr);
  // List of atoms in one expression.
  Dwarf_Small op = 0;
  for (int j = 0; j < static_cast<int>(loclist_expr_op_count); j++) {
    Dwarf_Unsigned opd1 = 0;
    Dwarf_Unsigned opd2 = 0;
    Dwarf_Unsigned opd3 = 0;
    Dwarf_Unsigned offsetforbranch = 0;

    if (dwarf_get_location_op_value_c(locdesc_entry, j, &op, &opd1, &opd2,
                                      &opd3, &offsetforbranch,
                                      &error) == DW_DLV_OK) {
      Atom a;
      a.opcode = op;
      a.op1 = opd1;
      a.op2 = opd2;
      a.op3 = opd3;
      a.off = offsetforbranch;

      const char *name;
			dwarf_get_OP_name(a.opcode, &name);
      if (j > 0) {
        printf(" ");
      }
      printf("op=%s, op1=0x%llx, op2=0x%llx, op3=0x%llx, off=0x%llx", name, opd1, opd2, opd3, offsetforbranch);

      expr.atoms.push_back(a);
    }
  }
  printf("\n");

  exprs_.emplace_back(expr);
  return true;
}

};  // namespace dwarfexpr
