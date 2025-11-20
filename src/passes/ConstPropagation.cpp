#include "ConstPropagation.hpp"

#include "Instruction.hpp"
#include "logging.hpp"

ConstantInt *ConstFolder::compute(Instruction::OpID op, ConstantInt *value1, ConstantInt *value2) {
    int c_value1 = value1->get_value();
    int c_value2 = value2->get_value();

    switch (op) {
    case Instruction::add:
        return ConstantInt::get(c_value1 + c_value2, module_);
        break;
    case Instruction::sub:
        return ConstantInt::get(c_value1 - c_value2, module_);
        break;
    case Instruction::mul:
        return ConstantInt::get(c_value1 * c_value2, module_);
        break;
    case Instruction::sdiv:
        return ConstantInt::get(static_cast<int>(c_value1 / c_value2), module_);
        break;
    case Instruction::eq:
        return ConstantInt::get(c_value1 == c_value2, module_);
        break;
    case Instruction::ne:
        return ConstantInt::get(c_value1 != c_value2, module_);
        break;
    case Instruction::gt:
        return ConstantInt::get(c_value1 > c_value2, module_);
        break;
    case Instruction::ge:
        return ConstantInt::get(c_value1 >= c_value2, module_);
        break;
    case Instruction::lt:
        return ConstantInt::get(c_value1 < c_value2, module_);
        break;
    case Instruction::le:
        return ConstantInt::get(c_value1 <= c_value2, module_);
        break;
    default:
        return nullptr;
        break;
    }
}

ConstantFP *ConstFolder::compute(Instruction::OpID op, ConstantFP *value1, ConstantFP *value2) {
    float c_value1 = value1->get_value();
    float c_value2 = value2->get_value();
    switch (op) {
    case Instruction::fadd:
        return ConstantFP::get(c_value1 + c_value2, module_);
        break;
    case Instruction::fsub:
        return ConstantFP::get(c_value1 - c_value2, module_);
        break;
    case Instruction::fmul:
        return ConstantFP::get(c_value1 * c_value2, module_);
        break;
    case Instruction::fdiv:
        return ConstantFP::get(c_value1 / c_value2, module_);
        break;
    case Instruction::feq:
        return ConstantFP::get(c_value1 == c_value2, module_);
        break;
    case Instruction::fne:
        return ConstantFP::get(c_value1 != c_value2, module_);
        break;
    case Instruction::fgt:
        return ConstantFP::get(c_value1 > c_value2, module_);
        break;
    case Instruction::fge:
        return ConstantFP::get(c_value1 >= c_value2, module_);
        break;
    case Instruction::flt:
        return ConstantFP::get(c_value1 < c_value2, module_);
        break;
    case Instruction::fle:
        return ConstantFP::get(c_value1 <= c_value2, module_);
        break;
    default:
        return nullptr;
        break;
    }
}
ConstantFP *ConstFolder::compute(Instruction::OpID op, ConstantInt *value1) {
    int c_value1 = value1->get_value();

    switch (op) {
    case Instruction::sitofp:
        return ConstantFP::get((float) c_value1, module_);
        break;

    default:
        return nullptr;
        break;
    }
}

ConstantInt *ConstFolder::compute(Instruction::OpID op, ConstantFP *value1) {
    float c_value1 = value1->get_value();
    switch (op) {
    case Instruction::fptosi:
        return ConstantInt::get(static_cast<int>(c_value1), module_);
        break;

    default:
        return nullptr;
        break;
    }
}

ConstantFP *cast_constantfp(Value *value) {
    auto constant_fp_ptr = dynamic_cast<ConstantFP *>(value);
    if (constant_fp_ptr) {
        return constant_fp_ptr;
    }
    return nullptr;
}
ConstantInt *cast_constantint(Value *value) {
    auto constant_int_ptr = dynamic_cast<ConstantInt *>(value);
    if (constant_int_ptr) {
        return constant_int_ptr;
    }
    return nullptr;
}

void ConstPropagation::run() {
    for (auto &func : m_->get_functions()) {

        for (auto &bb : func.get_basic_blocks()) {
            wait_delete.clear();

            for (auto &instr : bb.get_instructions()) {
                // clear glbalvar_def map

                if (instr.is_add() || instr.is_sub() || instr.is_mul() || instr.is_div()) {
                    auto value1 = cast_constantint(instr.get_operand(0));
                    auto value2 = cast_constantint(instr.get_operand(1));
                    if (value1 && value2) {
                        auto fold_const = folder->compute(instr.get_instr_type(), value1, value2);

                        instr.replace_all_use_with(fold_const);
                        wait_delete.push_back(&instr);
                    }
                }
                // fold other types: int cmp, float binary ops, and casts
                else if (instr.is_cmp()) {
                    auto v1 = cast_constantint(instr.get_operand(0));
                    auto v2 = cast_constantint(instr.get_operand(1));
                    if (v1 && v2) {
                        auto fold_const = folder->compute(instr.get_instr_type(), v1, v2);
                        instr.replace_all_use_with(fold_const);
                        wait_delete.push_back(&instr);
                    }
                } else if (instr.is_fcmp()) {
                    auto v1 = cast_constantfp(instr.get_operand(0));
                    auto v2 = cast_constantfp(instr.get_operand(1));
                    if (v1 && v2) {
                        auto fold_const = folder->compute(instr.get_instr_type(), v1, v2);
                        instr.replace_all_use_with(fold_const);
                        wait_delete.push_back(&instr);
                    }
                } else if (instr.is_fadd() || instr.is_fsub() || instr.is_fmul() || instr.is_fdiv()) {
                    auto v1 = cast_constantfp(instr.get_operand(0));
                    auto v2 = cast_constantfp(instr.get_operand(1));
                    if (v1 && v2) {
                        auto fold_const = folder->compute(instr.get_instr_type(), v1, v2);
                        instr.replace_all_use_with(fold_const);
                        wait_delete.push_back(&instr);
                    }
                } else if (instr.is_fp2si()) {
                    auto v = cast_constantfp(instr.get_operand(0));
                    if (v) {
                        auto fold_const = folder->compute(instr.get_instr_type(), v);
                        instr.replace_all_use_with(fold_const);
                        wait_delete.push_back(&instr);
                    }
                } else if (instr.is_si2fp()) {
                    auto v = cast_constantint(instr.get_operand(0));
                    if (v) {
                        auto fold_const = folder->compute(instr.get_instr_type(), v);
                        instr.replace_all_use_with(fold_const);
                        wait_delete.push_back(&instr);
                    }
                }
            }
            globalvar_def.clear();
            for (auto instr : wait_delete) {
                bb.erase_instr(instr);
            }
        }
    }

    for (auto &func : m_->get_functions()) {
        for (auto &bb : func.get_basic_blocks()) {
            builder->set_insert_point(&bb);
            // check if conditional branch's condition is constant
            if (!bb.get_instructions().empty()) {
                Instruction *term = nullptr;
                if (bb.is_terminated())
                    term = bb.get_terminator();
                if (term && term->is_br()) {
                    BranchInst *br = static_cast<BranchInst *>(term);
                    if (br->is_cond_br()) {
                        Value *cond = br->get_condition();
                        bool taken = false;
                        bool is_const = false;
                        if (auto ci = cast_constantint(cond)) {
                            is_const = true;
                            taken = (ci->get_value() != 0);
                        } else if (auto cf = cast_constantfp(cond)) {
                            is_const = true;
                            taken = (cf->get_value() != 0.0f);
                        }
                        if (is_const) {
                            // determine chosen successor
                            BasicBlock *chosen = static_cast<BasicBlock *>(br->get_operand(taken ? 1 : 2));
                            // record old successors
                            std::vector<BasicBlock *> old_succs(br->get_num_operand() >= 3 ? std::vector<BasicBlock *>{static_cast<BasicBlock *>(br->get_operand(1)), static_cast<BasicBlock *>(br->get_operand(2))} : std::vector<BasicBlock *>{static_cast<BasicBlock *>(br->get_operand(0))});
                            // create unconditional branch to chosen
                            builder->create_br(chosen);
                            // remove old branch instruction
                            bb.erase_instr(br);
                            // update CFG: remove this bb from old successors' pre lists
                            for (auto s : old_succs) {
                                if (s == chosen) continue;
                                s->remove_pre_basic_block(&bb);
                                bb.remove_succ_basic_block(s);
                                // if successor now has no predecessors and is not entry, mark for deletion
                                if (s->get_pre_basic_blocks().empty() && !is_entry(s)) {
                                    delete_bb.push_back(s);
                                }
                            }
                            // ensure chosen has this bb as predecessor and is in succ list
                            chosen->add_pre_basic_block(&bb);
                            bb.add_succ_basic_block(chosen);
                        }
                    }
                }
            }
        }
        for (auto bb : delete_bb) {
            clear_blocks_recs(bb);
        }
        delete_bb.clear();
    }
}

bool ConstPropagation::is_entry(BasicBlock *bb) {
    if (bb == nullptr) return false;
    auto func = bb->get_parent();
    if (func == nullptr) return false;
    return bb == func->get_entry_block();
}

void ConstPropagation::clear_blocks_recs(BasicBlock *start_bb) {
    auto func = start_bb->get_parent();
    if (func == nullptr) {
        LOG(ERROR) << "basic block-" << start_bb->get_name() << " has no parent function";
    } else {
        auto prev_bb = start_bb->get_pre_basic_blocks();
        // start_bb has no previous bb and is not the entry of parent function
        if (prev_bb.size() == 0 && !is_entry(start_bb)) {
            func->remove(start_bb);
            auto succ_bb = start_bb->get_succ_basic_blocks();
            for (auto each_succ_bb : succ_bb) {
                std::vector<Instruction*> del_inst;
                for (auto &instr1 : each_succ_bb->get_instructions()) {
                    auto instr = &instr1;
                    if (instr->is_phi()) {
                        LOG(DEBUG) << "Find a PHI instruction in the sucess node of "
                                      "useless branch";
                        for (int i = 1; i < instr->get_num_operand(); i += 2) {
                            if (instr->get_operand(i) == start_bb &&
                                start_bb->get_pre_basic_blocks().size() <= 0) {
                                LOG(DEBUG) << "remove unuseful phi branch in the index of " << i - 1
                                           << " and " << i;

                                instr->remove_operand(i - 1);
                                instr->remove_operand(i - 1);
                            }
                        }
                        int operands_num_phi = instr->get_num_operand();
                        if (operands_num_phi == 2) {
                            auto value = instr->get_operand(0);
                            instr->replace_all_use_with(cast_constantint(value));
                            del_inst.push_back(instr);
                        }
                    }
                }
                for(auto instr : del_inst) each_succ_bb->erase_instr(instr);
                clear_blocks_recs(each_succ_bb);
            }
        }
    }
}
