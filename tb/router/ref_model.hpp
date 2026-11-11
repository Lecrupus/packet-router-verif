// ref_model.hpp -- RefModel (predictor). Pure C++, no simulator calls (R-REF-01), so the
// SV UVM stretch goal can call it through DPI-C. Models the header check, src tagging and
// drop counting; not timing (R-REF-02).
#pragma once
#include <cstdint>

#include "base/object.hpp"
#include "router/packet.hpp"

struct Prediction {
    bool drop = false;
    Packet pkt;  // expected output packet when !drop
};

class RefModel : public Object {
public:
    RefModel() : Object("ref_model") {}
    Prediction predict(int in_port, const Packet& in) const;
    // Header-only form used by predict() and the DPI-C entry point
    static uint32_t tag_header(int in_port, uint32_t hdr, bool& drop);
};

// DPI-C entry point for the SV UVM port: returns the expected output header and sets
// *drop to 1 when the router must drop the packet.
extern "C" uint32_t ref_model_predict_header(int in_port, uint32_t hdr, int* drop);
