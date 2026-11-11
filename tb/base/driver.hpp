// driver.hpp -- Driver<T> (uvm_driver). Drivers only set DUT inputs; they never check
// results (R-TB-04). The scheduler calls drive() before eval and post_eval() after it.
#pragma once
#include "base/component.hpp"
#include "base/sequencer.hpp"

// Anything the scheduler calls in the "drive" slot of a cycle (drivers and reactive
// drivers such as ReadyResponder).
class Drivable {
public:
    virtual ~Drivable() = default;
    virtual void drive() = 0;       // set inputs for this cycle
    virtual void post_eval() {}     // inputs settled; note whether the word was accepted
    virtual bool busy() const { return false; }  // still has work in flight
};

template <class T>
class Driver : public Component, public Drivable {
public:
    using Component::Component;
    void set_sequencer(Sequencer<T>* s) { seqr_ = s; }

protected:
    Sequencer<T>* seqr_ = nullptr;  // non-owning
};
