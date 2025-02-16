// Copyright (c) 2021 CINN Authors. All Rights Reserved.
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

#include <unordered_set>

#include "paddle/cinn/common/ir_util.h"
#include "paddle/cinn/ir/ir_printer.h"
#include "paddle/cinn/ir/ir_visitor.h"
#include "paddle/cinn/ir/utils/ir_compare.h"
#include "paddle/cinn/utils/string.h"

namespace cinn {
namespace ir {

bool operator==(Expr a, Expr b) {
  if (a.get() == b.get()) return true;
  return ir_utils::IRCompare(a, b);
}

template <typename T>
static bool CompareExpressions(const ir::IndexExpr& a, const ir::IndexExpr& b) {
  auto aPart = common::GetFlatternExprs<T>(a);
  auto bPart = common::GetFlatternExprs<T>(b);

  std::sort(aPart.begin(), aPart.end(), common::ComparePriority);
  std::sort(bPart.begin(), bPart.end(), common::ComparePriority);

  if (aPart.size() != bPart.size()) return false;

  size_t i = 0;
  while (i < aPart.size()) {
    if (!common::ComparePriority(aPart[i], bPart[i])) return false;
    std::vector<std::pair<ir::IndexExpr, int>> aGroup, bGroup;

    do {
      aGroup.emplace_back(aPart[i], 0);
      bGroup.emplace_back(bPart[i], 0);
      ++i;
    } while (i < aPart.size() &&
             common::ComparePriority(aPart[i - 1], aPart[i]) == 1 &&
             common::ComparePriority(bPart[i - 1], bPart[i]) == 1);

    // compare expressions with same priority.
    for (size_t k = 0; k < aGroup.size(); ++k) {
      for (auto& b : bGroup) {
        if (b.second == 0 && aGroup[k].first == b.first) {
          b.second = 1;
          aGroup[k].second = 1;
          break;
        }
      }
      if (aGroup[k].second == 0) return false;
    }
  }

  return true;
}

bool operator!=(Expr a, Expr b) { return !(a == b); }

bool operator==(IndexExpr a, IndexExpr b) {
  if (a.get() == b.get()) return true;
  if (a.node_type() != b.node_type()) return false;
  std::vector<ir::IndexExpr> aPart;
  std::vector<ir::IndexExpr> bPart;
  switch (a.node_type()) {
    case ir::IrNodeTy::IterMark:
      [[fallthrough]];
    case ir::IrNodeTy::IterSplit:
      [[fallthrough]];
    case ir::IrNodeTy::IterSum: {
      return ir_utils::IRCompare(a, b);
    }
    case ir::IrNodeTy::IntImm: {
      return a.as_int64() == b.as_int64();
    }
    case ir::IrNodeTy::_Var_: {
      return a.as_var()->name == b.as_var()->name;
    }
    case ir::IrNodeTy::Div:
    case ir::IrNodeTy::Mod: {
      return a->operand(0).as_index() == b->operand(0).as_index() &&
             a->operand(1).as_index() == b->operand(1).as_index();
    }
    case ir::IrNodeTy::Add:
      return CompareExpressions<ir::Add>(a.as_index(), b.as_index());
    case ir::IrNodeTy::Mul:
      return CompareExpressions<ir::Mul>(a.as_index(), b.as_index());
  }
}

bool operator!=(IndexExpr a, IndexExpr b) { return !(a == b); }

}  // namespace ir
}  // namespace cinn
