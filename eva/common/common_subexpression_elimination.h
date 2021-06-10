// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#pragma once

#include "eva/ir/program.h"
#include "eva/util/hash_combine.h"
#include "eva/util/logging.h"

namespace eva {

// Checks whether two Terms are semantically equivalent (i.e. represent the same subexpression).
// This assumes that operands are pointer-equal if they are semantically equal.
struct SemanticTermEquality {
    bool operator()(const Term::Ptr& lhs, const Term::Ptr& rhs) const {
        if (lhs->op != rhs-> op) return false;
        if (lhs->numOperands() != rhs->numOperands()) return false;
        for (size_t i = 0; i < lhs->numOperands(); ++i) {
            // This check is always safe but may unnecessarily return false if operand terms aren't unique.
            if (lhs->operandAt(i) != rhs->operandAt(i)) return false;
        }
        switch (lhs->op) {
        case Op::Undef:
            // Semantics for undef operations are unknown (maybe someone is abusing them with Attributes providing
            // context), so don't assume equality.
            return false;
        case Op::Input:
        case Op::Output:
            return lhs->index == rhs->index;
        case Op::Constant:
            return (*lhs->get<ConstantValueAttribute>()) == (*rhs->get<ConstantValueAttribute>());
        case Op::Negate:
        case Op::Add:
        case Op::Sub:
        case Op::Mul:
        case Op::Relinearize:
        case Op::ModSwitch:
            return true;
        case Op::RotateLeftConst:
        case Op::RotateRightConst:
            return lhs->get<RotationAttribute>() == rhs->get<RotationAttribute>();
        case Op::Rescale:
            return lhs->get<RescaleDivisorAttribute>() == rhs->get<RescaleDivisorAttribute>();
        case Op::Encode:
            return lhs->get<EncodeAtScaleAttribute>() == rhs->get<EncodeAtScaleAttribute>() &&
                lhs->get<EncodeAtLevelAttribute>() == rhs->get<EncodeAtLevelAttribute>();
        }
        throw std::runtime_error("Unhandled op " + getOpName(lhs->op));
    }
};

// Produces a hash code matching SemanticTermEquality, meaning that if two Terms are equal then they will have the same
// hash code.
struct SemanticTermHash {
    size_t operator()(const Term::Ptr& term) const {
        size_t hash = 0;
        hash_combine(hash, term->op);
        for (const auto& operand : term->getOperands()) {
            hash_combine(hash, operand);
        }
        switch (term->op) {
        case Op::Input:
        case Op::Output:
            hash_combine(hash, term->index);
            break;
        case Op::Constant:
            hash_combine(hash, *(term->get<ConstantValueAttribute>()));
            break;
        case Op::Undef:
        case Op::Negate:
        case Op::Add:
        case Op::Sub:
        case Op::Mul:
        case Op::Relinearize:
        case Op::ModSwitch:
            break;
        case Op::RotateLeftConst:
        case Op::RotateRightConst:
            hash_combine(hash, term->get<RotationAttribute>());
            break;
        case Op::Rescale:
            hash_combine(hash, term->get<RescaleDivisorAttribute>());
            break;
        case Op::Encode:
            hash_combine(hash, term->get<EncodeAtScaleAttribute>());
            hash_combine(hash, term->get<EncodeAtLevelAttribute>());
            break;
        default:
            throw std::runtime_error("Unhandled op " + getOpName(term->op));
        }
        return hash;
    }
};

// Eliminates common subexpressions by enforcing one representative for each semantic equivalence class of Terms.
class CommonSubexpressionEliminator {
  Program &program;
  std::unordered_set<Term::Ptr, SemanticTermHash, SemanticTermEquality> unique_terms;

public:
  CommonSubexpressionEliminator(Program &g) : program(g) {}

  void
  operator()(Term::Ptr &term) { // must only be used with forward pass traversal
    // Insert term if a semantically equivalent term hasn't already been seen
    auto pair = unique_terms.insert(term);
    if (!pair.second) {
        // If there was already an equivalent term, replace this term with that
        term->replaceAllUsesWith(*pair.first);
        log(Verbosity::Trace, "Eliminated term with index=%lu", term->index);
    }
  }
};

} // namespace eva
