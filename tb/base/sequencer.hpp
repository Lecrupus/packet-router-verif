// sequencer.hpp -- Sequence<T> (uvm_sequence) and Sequencer<T> (uvm_sequencer).
// Pull model: the driver asks the sequencer for the next item, the sequencer asks the
// sequence at the front of its queue, and moves on when that sequence is finished.
#pragma once
#include <deque>
#include <memory>

#include "base/component.hpp"
#include "base/object.hpp"
#include "base/rng.hpp"

template <class T>
class Sequence : public Object {
public:
    using Object::Object;
    // Next item, or nullptr when the sequence has nothing more to send.
    virtual std::unique_ptr<T> next(Rng& rng) = 0;
};

template <class T>
class Sequencer : public Component {
public:
    Sequencer(const std::string& name, Component* parent) : Component(name, parent) {}

    void set_rng(Rng* rng) { rng_ = rng; }
    void start(std::unique_ptr<Sequence<T>> seq) { seqs_.push_back(std::move(seq)); }
    void stop() {
        stopped_ = true;
        seqs_.clear();
    }

    // Driver side (uvm_sequencer::get_next_item)
    std::unique_ptr<T> get_next_item() {
        while (!stopped_ && !seqs_.empty()) {
            auto item = seqs_.front()->next(*rng_);
            if (item) {
                item->item_id = issued_++;
                return item;
            }
            seqs_.pop_front();
        }
        return nullptr;
    }

    bool finished() const { return stopped_ || seqs_.empty(); }
    uint64_t issued() const { return issued_; }

private:
    Rng* rng_ = nullptr;
    std::deque<std::unique_ptr<Sequence<T>>> seqs_;
    bool stopped_ = false;
    uint64_t issued_ = 0;
};
