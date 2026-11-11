// object.hpp -- Object (uvm_object) and SeqItem (uvm_sequence_item).
#pragma once
#include <cstdint>
#include <memory>
#include <string>

class Object {
public:
    explicit Object(std::string name = "") : name_(std::move(name)) {}
    virtual ~Object() = default;

    const std::string& name() const { return name_; }
    void set_name(std::string n) { name_ = std::move(n); }

    virtual std::string to_string() const { return name_; }
    // Deep copy. Data classes override; structural objects may leave the default.
    virtual std::unique_ptr<Object> clone() const { return nullptr; }

private:
    std::string name_;
};

class SeqItem : public Object {
public:
    using Object::Object;
    uint64_t item_id = 0;  // set by the sequencer as items are issued
};
