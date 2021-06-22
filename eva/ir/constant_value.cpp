// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#include "eva/ir/constant_value.h"
#include "eva/util/hash_combine.h"

namespace eva {

void ConstantValue::validateSlots(std::size_t slots) const {
  if (slots < size) {
    throw std::runtime_error("Slots must be at least size of constant");
  }
  if (slots % size != 0) {
    throw std::runtime_error("Size must exactly divide slots");
  }
}

DenseConstantValue::DenseConstantValue(std::size_t size, std::vector<double> values)
    : ConstantValue(size), values(values) {
  if (size % values.size() != 0) {
    throw std::runtime_error(
        "DenseConstantValue size must exactly divide size");
  }
}

const std::vector<double> &DenseConstantValue::expand(std::vector<double> &scratch,
                                  std::size_t slots) const {
  validateSlots(slots);
  if (values.size() == slots) {
    return values;
  } else {
    scratch.clear();
    for (int r = slots / values.size(); r > 0; --r) {
      scratch.insert(scratch.end(), values.begin(), values.end());
    }
    return scratch;
  }
}

void DenseConstantValue::expandTo(std::vector<double> &result, std::size_t slots) const {
  validateSlots(slots);
  result.clear();
  result.reserve(slots);
  for (int r = slots / values.size(); r > 0; --r) {
    result.insert(result.end(), values.begin(), values.end());
  }
}

bool DenseConstantValue::isZero() const {
  for (double value : values) {
    if (value != 0) return false;
  }
  return true;
}

void DenseConstantValue::serialize(msg::ConstantValue &msg) const {
  msg.set_size(size);
  auto valuesMsg = msg.mutable_values();
  valuesMsg->Reserve(values.size());
  for (const auto &value : values) {
    valuesMsg->Add(value);
  }
}

bool DenseConstantValue::operator==(const ConstantValue& other) const {
  return other.operator==(*this);
}

bool DenseConstantValue::operator==(const DenseConstantValue& other) const {
  return size == other.size && values == other.values;
}

bool DenseConstantValue::operator==(const SparseConstantValue& other) const {
  if (size != other.size) return false;
  for (const auto& entry : other.values) {
    if (values[entry.first % values.size()] != entry.second) return false;
  }
  return true;
}

size_t DenseConstantValue::hash() const {
  size_t hash = 0;
  size_t zero_count = 0;
  for (size_t i = 0; i < size; ++i) {
    if (values[i] == 0) {
      zero_count += 1;
    } else {
      if (zero_count > 0) {
        hash_combine(hash, zero_count);
        zero_count = 0;
      }
      hash_combine(hash, values[i]);
    }
  }
  if (zero_count > 0) {
    hash_combine(hash, zero_count);
    zero_count = 0;
  }
  return hash;
}

SparseConstantValue::SparseConstantValue(std::size_t size,
                    std::vector<std::pair<std::uint32_t, double>> values)
    : ConstantValue(size) {
  // Only include non-zero entries. isZero depends on this.
  std::copy_if(values.begin(), values.end(), std::back_inserter(this->values),
    [](const auto& entry) {
      return entry.second != 0;
  });
  if (this->values.size() > 0) {
    // Some member functions depend on the entries being sorted
    std::sort(std::begin(this->values),
              std::end(this->values),
              [](const std::pair<int, int>& p1, const std::pair<int, int>& p2) { return p1.first < p2.first; });
    std::uint32_t prev = this->values[0].first;
    for (size_t i = 1; i < this->values.size(); ++i) {
      auto next = this->values[i].first;
      if (next == prev) throw std::runtime_error("SparseConstantValue must not have duplicate indices");
      prev = next;
    }
  }
}

const std::vector<double> &SparseConstantValue::expand(std::vector<double> &scratch,
                                  std::size_t slots) const {
  validateSlots(slots);
  scratch.clear();
  scratch.resize(slots);
  for (auto &entry : values) {
    for (int i = 0; i < slots; i += values.size()) {
      scratch.at(entry.first + i) = entry.second;
    }
  }
  return scratch;
}

void SparseConstantValue::expandTo(std::vector<double> &result, std::size_t slots) const {
  validateSlots(slots);
  result.clear();
  result.resize(slots);
  for (auto &entry : values) {
    for (int i = 0; i < slots; i += values.size()) {
      result.at(entry.first + i) = entry.second;
    }
  }
}

bool SparseConstantValue::isZero() const {
  return values.size() == 0;
}

void SparseConstantValue::serialize(msg::ConstantValue &msg) const {
  msg.set_size(size);
  for (const auto &pair : values) {
    msg.add_sparse_indices(pair.first);
    msg.add_values(pair.second);
  }
}

bool SparseConstantValue::operator==(const ConstantValue& other) const {
  return other.operator==(*this);
}

bool SparseConstantValue::operator==(const DenseConstantValue& other) const {
  return other.operator==(*this);
}

bool SparseConstantValue::operator==(const SparseConstantValue& other) const {
  if (size != other.size || values.size() != other.values.size()) return false;
  for (size_t i = 0; i < values.size(); ++i) {
    if (values[i] != other.values[i]) return false;
  }
  return true;
}

size_t SparseConstantValue::hash() const {
  size_t hash = 0;
  std::uint32_t prev = 0;
  for (const auto &pair : values) {
    if (pair.first - prev > 0) {
      hash_combine(hash, pair.first - prev);
    }
    hash_combine(hash, pair.second);
    prev = pair.first;
  }
  if (size - prev > 0) {
    hash_combine(hash, size - prev);
  }
  return hash;
}

} // namespace eva
