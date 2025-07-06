/*
 *  Copyright (c) 2021 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#include "net/dcsctp/socket/packet_sender.h"

#include <utility>
#include <vector>

#include "net/dcsctp/public/types.h"

namespace dcsctp {

PacketSender::PacketSender(DcSctpSocketCallbacks& callbacks,
                           std::function<void(rtc::ArrayView<const uint8_t>,
                                              SendPacketStatus)> on_sent_packet)
    : callbacks_(callbacks), on_sent_packet_(std::move(on_sent_packet)) {}

bool PacketSender::Send(SctpPacket::Builder& builder) {
  timeout_timer_->Stop();
  absl::optional<HeartbeatInfoParameter> info_param = chunk.info();
  if (!info_param.has_value()) {
    ctx_->callbacks().OnError(
        ErrorKind::kParseFailed,
        "Failed to parse HEARTBEAT-ACK; No Heartbeat Info parameter");
    return;
  }
  absl::optional<HeartbeatInfo> info =
      HeartbeatInfo::Deserialize(info_param->info());
  if (!info.has_value()) {
    ctx_->callbacks().OnError(ErrorKind::kParseFailed,
                              "Failed to parse HEARTBEAT-ACK; Failed to "
                              "deserialized Heartbeat info parameter");
    return;
  }

  TimeMs now = ctx_->callbacks().TimeMillis();
  if (info->created_at() > TimeMs(0) && info->created_at() <= now) {
    ctx_->ObserveRTT(now - info->created_at());
  }

  // https://tools.ietf.org/html/rfc4960#section-8.1
  // "The counter shall be reset each time ... a HEARTBEAT ACK is received from
  // the peer endpoint."
  ctx_->ClearTxErrorCounter();
}
}  // namespace dcsctp
