// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#pragma once

#include "eva/serialization/eva.pb.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace eva {

class DenseConstantValue;
class SparseConstantValue;

class ConstantValue {
public:
  ConstantValue(std::size_t size) : size(size) {}
  virtual ~ConstantValue() {}
  virtual const std::vector<double> &expand(std::vector<double> &scratch,
                                            std::size_t slots) const = 0;
  virtual void expandTo(std::vector<double> &result,
                        std::size_t slots) const = 0;
  virtual bool isZero() const = 0;
  virtual void serialize(msg::ConstantValue &msg) const = 0;

  virtual bool operator==(const ConstantValue& other) const = 0;
  virtual bool operator==(const DenseConstantValue& other) const = 0;
  virtual bool operator==(const SparseConstantValue& other) const = 0;

  virtual size_t hash() const = 0;

protected:
  std::size_t size;

  void validateSlots(std::size_t slots) const;
};

class DenseConstantValue : public ConstantValue {
public:
  DenseConstantValue(std::size_t size, std::vector<double> values);

  const std::vector<double> &expand(std::vector<double> &scratch,
                                    std::size_t slots) const override;
  void expandTo(std::vector<double> &result, std::size_t slots) const override;
  bool isZero() const override;
  void serialize(msg::ConstantValue &msg) const override;
  bool operator==(const ConstantValue& other) const override;
  bool operator==(const DenseConstantValue& other) const override;
  bool operator==(const SparseConstantValue& other) const override;
  size_t hash() const override;

private:
  std::vector<double> values;
};

class SparseConstantValue : public ConstantValue {
public:
  SparseConstantValue(std::size_t size,
                      std::vector<std::pair<std::uint32_t, double>> values);

  const std::vector<double> &expand(std::vector<double> &scratch,
                                    std::size_t slots) const override;
  void expandTo(std::vector<double> &result, std::size_t slots) const override;
  bool isZero() const override;
  void serialize(msg::ConstantValue &msg) const override;
  bool operator==(const ConstantValue& other) const override;
  bool operator==(const DenseConstantValue& other) const override;
  bool operator==(const SparseConstantValue& other) const override;
  size_t hash() const override;

  friend DenseConstantValue;

private:
  std::vector<std::pair<std::uint32_t, double>> values;
};

std::unique_ptr<msg::ConstantValue> serialize(const ConstantValue &obj);
std::shared_ptr<ConstantValue> deserialize(const msg::ConstantValue &msg);

} // namespace eva

namespace std {

template <> struct hash<eva::ConstantValue>
{
  size_t operator()(const eva::ConstantValue & x) const
  {
    return x.hash();
  }
};

} // namespace std
