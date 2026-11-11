#include "router/ref_model.hpp"

uint32_t RefModel::tag_header(int in_port, uint32_t hdr, bool& drop) {
    const uint32_t len = hdr & 0xFF;
    drop = !(len >= 1 && len <= static_cast<uint32_t>(MAX_LEN));        // R-DUT-02
    return (hdr & ~(3u << 10)) | ((static_cast<uint32_t>(in_port) & 3u) << 10);  // R-DUT-04
}

Prediction RefModel::predict(int in_port, const Packet& in) const {
    Prediction pr;
    pr.drop = false;
    const uint32_t out_hdr = tag_header(in_port, in.header(), pr.drop);
    if (!pr.drop) {
        pr.pkt = in;
        pr.pkt.set_header(out_hdr);
        pr.pkt.src = static_cast<uint8_t>(in_port);
    }
    return pr;
}

extern "C" uint32_t ref_model_predict_header(int in_port, uint32_t hdr, int* drop) {
    bool d = false;
    const uint32_t h = RefModel::tag_header(in_port, hdr, d);
    if (drop) *drop = d ? 1 : 0;
    return h;
}
