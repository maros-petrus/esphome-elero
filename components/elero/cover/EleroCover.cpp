#include "EleroCover.h"
#include "esphome/core/log.h"

namespace esphome {
namespace elero {

using namespace esphome::cover;

static const char *const TAG = "elero.cover";
static const uint8_t ELERO_LEARN_RAW_INIT_DATA[8] = {0x90, 0xA8, 0x07, 0x53, 0xB0, 0xA8, 0x0B, 0xAF};
static const uint8_t ELERO_LEARN_RAW_ACK_PAYLOAD[10] = {0x00, 0x00, 0x00, 0x00, 0x6c, 0xec, 0xa0, 0x20, 0xe4, 0x64};
static const uint8_t ELERO_LEARN_RAW_START_PAYLOAD[10] = {0x04, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80};
static const uint8_t ELERO_LEARN_RAW_CONFIRM_UP_PAYLOAD_A[10] = {0x00, 0x02, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0xc0};
static const uint8_t ELERO_LEARN_RAW_CONFIRM_UP_PAYLOAD_B[10] = {0x00, 0x02, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x40};
static const uint8_t ELERO_LEARN_RAW_CONFIRM_DOWN_PAYLOAD_A[10] = {0x00, 0x02, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0xc0};
static const uint8_t ELERO_LEARN_RAW_CONFIRM_DOWN_PAYLOAD_B[10] = {0x00, 0x02, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x40};
static const uint8_t ELERO_LEARN_RAW_FINALIZE_PAYLOAD[10] = {0x08, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80};

void EleroCover::dump_config() {
  LOG_COVER("", "Elero Cover", this);
}

void EleroCover::setup() {
  this->parent_->register_cover(this);
  this->counter_pref_ = global_preferences->make_preference<uint8_t>(0x454C0000U ^ this->command_.remote_addr);
  uint8_t restored_counter;
  if (this->counter_pref_.load(&restored_counter) && restored_counter != 0) {
    this->command_.counter = restored_counter;
    this->counter_pref_loaded_ = true;
    ESP_LOGD(TAG, "Restored counter %u for remote 0x%06x", restored_counter, this->command_.remote_addr);
  }
  auto restore = this->restore_state_();
  if (restore.has_value()) {
    restore->apply(this);
  } else {
    if((this->open_duration_ > 0) && (this->close_duration_ > 0))
      this->position = 0.5f;
  }
}

uint8_t EleroCover::next_counter_(uint8_t counter) {
  if (counter == 0xff)
    return 1;
  return counter + 1;
}

bool EleroCover::counter_is_ahead_(uint8_t candidate) const {
  if (candidate == this->command_.counter)
    return false;

  uint8_t distance = static_cast<uint8_t>(candidate - this->command_.counter);
  return distance < 128;
}

void EleroCover::save_counter_() {
  if (this->command_.counter == 0)
    return;
  this->counter_pref_.save(&this->command_.counter);
  this->counter_pref_loaded_ = true;
}

void EleroCover::sync_counter_from_remote(uint8_t seen_counter) {
  uint8_t candidate = this->next_counter_(seen_counter);
  if (!this->counter_pref_loaded_ || this->counter_is_ahead_(candidate)) {
    if (candidate != this->command_.counter) {
      ESP_LOGD(TAG, "Syncing counter for remote 0x%06x from %u to %u", this->command_.remote_addr, this->command_.counter, candidate);
      this->command_.counter = candidate;
      this->save_counter_();
    } else {
      this->counter_pref_loaded_ = true;
    }
  }
}

void EleroCover::loop() {
  uint32_t intvl = this->poll_intvl_;
  uint32_t now = millis();
  if(this->current_operation != COVER_OPERATION_IDLE) {
    if((now - ELERO_TIMEOUT_MOVEMENT) < this->movement_start_) // do not poll frequently for an extended period of time
      intvl = ELERO_POLL_INTERVAL_MOVING;
  }

  if((now > this->poll_offset_) && (now - this->poll_offset_ - this->last_poll_) > intvl) {
    this->queue_check_command();
    this->last_poll_ = now - this->poll_offset_;
  }

  this->handle_commands(now);

  if((this->current_operation != COVER_OPERATION_IDLE) && (this->open_duration_ > 0) && (this->close_duration_ > 0)) {
    this->recompute_position();
    if(this->is_at_target()) {
      this->queue_control_command(this->command_stop_);
      this->current_operation = COVER_OPERATION_IDLE;
      this->target_position_ = COVER_OPEN;
    }

    // Publish position every second
    if(now - this->last_publish_ > 1000) {
      this->publish_state(false);
      this->last_publish_ = now;
    }
  }
}

bool EleroCover::is_at_target() {
  // We return false as we don't want to send a stop command for completely open or
  // close - this is handled by the cover
  if((this->target_position_ == COVER_OPEN) || (this->target_position_ == COVER_CLOSED))
    return false;

  switch (this->current_operation) {
    case COVER_OPERATION_OPENING:
      return this->position >= this->target_position_;
    case COVER_OPERATION_CLOSING:
      return this->position <= this->target_position_;
    case COVER_OPERATION_IDLE:
    default:
      return true;
  }
}

void EleroCover::handle_commands(uint32_t now) {
  if((now - this->last_command_) > ELERO_DELAY_SEND_PACKETS) {
    if(this->commands_to_send_.size() > 0) {
      const auto queued = this->commands_to_send_.front();
      if (queued.not_before != 0 && static_cast<int32_t>(now - queued.not_before) < 0) {
        return;
      }
      bool success = false;
      if (queued.raw_mode) {
        success = this->parent_->send_raw_packet(queued.raw_packet, queued.packet_len);
      } else {
        auto tx = this->command_;
        if (queued.has_counter) {
          tx.counter = queued.counter;
        }
        tx.blind_addr = queued.blind_addr;
        tx.remote_addr = queued.remote_addr;
        tx.backward_addr = queued.backward_addr;
        tx.forward_addr = queued.forward_addr;
        tx.short_dst = queued.short_dst;
        tx.pck_inf[0] = queued.pck_inf_1;
        tx.pck_inf[1] = queued.pck_inf_2;
        tx.hop = queued.hop;
        for (int i = 0; i < 10; i++) {
          tx.payload[i] = queued.payload[i];
        }
        success = this->parent_->send_command(&tx, queued.packet_len);
      }
      if(success) {
        this->send_packets_++;
        this->send_retries_ = 0;
        if(this->send_packets_ >= ELERO_SEND_PACKETS) {
          this->commands_to_send_.pop();
          this->send_packets_ = 0;
          if (!queued.has_counter && !queued.raw_mode) {
            this->increase_counter();
          }
        }
      } else {
        ESP_LOGD(TAG, "Retry #%d for blind 0x%02x", this->send_retries_, this->command_.blind_addr);
        this->send_retries_++;
        if(this->send_retries_ > ELERO_SEND_RETRIES) {
          ESP_LOGE(TAG, "Hit maximum number of retries, giving up.");
          this->send_retries_ = 0;
          this->commands_to_send_.pop();
        }
      }
      this->last_command_ = now;
    }
  }
}

void EleroCover::queue_check_command() {
  QueuedCommand queued{};
  queued.raw_mode = false;
  queued.packet_len = this->command_check_len_;
  queued.has_counter = false;
  queued.not_before = 0;
  for (int i = 0; i < 10; i++) {
    queued.payload[i] = this->command_.payload[i];
  }
  queued.payload[4] = this->command_check_;
  queued.pck_inf_1 = this->command_.pck_inf[0];
  queued.pck_inf_2 = this->command_.pck_inf[1];
  queued.hop = this->command_.hop;
  queued.blind_addr = this->command_.blind_addr;
  queued.remote_addr = this->command_.remote_addr;
  queued.backward_addr = this->command_.remote_addr;
  queued.forward_addr = this->command_.remote_addr;
  queued.short_dst = this->command_.channel;
  this->commands_to_send_.push(queued);
}

void EleroCover::queue_control_command(uint8_t command) {
  uint8_t pck_inf_2 = this->control_pckinf_2_;
  if (command == this->command_down_) {
    pck_inf_2 = this->control_down_pckinf_2_;
  }

  QueuedCommand queued{};
  queued.raw_mode = false;
  queued.packet_len = this->command_control_len_;
  queued.has_counter = false;
  queued.not_before = 0;
  for (int i = 0; i < 10; i++) {
    queued.payload[i] = 0x00;
  }
  queued.payload[0] = this->control_payload_1_;
  queued.payload[1] = this->control_payload_2_;
  queued.payload[4] = command;
  queued.pck_inf_1 = this->control_pckinf_1_;
  queued.pck_inf_2 = pck_inf_2;
  queued.hop = this->control_hop_;
  queued.blind_addr = this->command_.blind_addr;
  queued.remote_addr = this->command_.remote_addr;
  queued.backward_addr = this->control_backward_address_;
  queued.forward_addr = this->control_forward_address_;
  queued.short_dst = this->control_short_dst_;
  this->commands_to_send_.push(queued);
}

void EleroCover::handle_learn_blind_reply() {
  if (!this->learn_waiting_for_reply_) {
    return;
  }
  const uint32_t learn_remote = this->learn_remote_address_ != 0 ? this->learn_remote_address_ : this->command_.remote_addr;
  if (learn_remote == 0) {
    return;
  }
  auto next_learn_counter = [&]() -> uint8_t {
    uint8_t counter = this->learn_counter_;
    this->learn_counter_ = this->next_counter_(this->learn_counter_);
    return counter;
  };
  auto queue_raw_learn_packet = [&](uint8_t length, uint8_t type, uint8_t type2, uint8_t hop,
                                   uint8_t counter, uint8_t dst0, uint8_t dst1, uint8_t dst2,
                                   const uint8_t *body, uint8_t body_len, uint32_t not_before = 0) {
    QueuedCommand queued{};
    queued.raw_mode = true;
    queued.has_counter = true;
    queued.counter = counter;
    queued.not_before = not_before;
    queued.packet_len = length + 1;
    uint8_t *p = queued.raw_packet;
    p[0] = length;
    p[1] = counter;
    p[2] = type;
    p[3] = type2;
    p[4] = hop;
    p[5] = 0x01;
    p[6] = this->command_.channel;
    p[7] = (learn_remote >> 16) & 0xFF;
    p[8] = (learn_remote >> 8) & 0xFF;
    p[9] = learn_remote & 0xFF;
    p[10] = (learn_remote >> 16) & 0xFF;
    p[11] = (learn_remote >> 8) & 0xFF;
    p[12] = learn_remote & 0xFF;
    p[13] = (learn_remote >> 16) & 0xFF;
    p[14] = (learn_remote >> 8) & 0xFF;
    p[15] = learn_remote & 0xFF;
    p[16] = 0x01;
    p[17] = (this->command_.blind_addr >> 16) & 0xFF;
    p[18] = (this->command_.blind_addr >> 8) & 0xFF;
    p[19] = this->command_.blind_addr & 0xFF;
    for (uint8_t i = 0; i < body_len; i++) {
      p[20 + i] = body[i];
    }
    this->commands_to_send_.push(queued);
  };

  ESP_LOGI(TAG, "Blind learn reply received, queueing follow-up learn packets");
  this->learn_waiting_for_reply_ = false;
  const uint32_t now = millis();
  queue_raw_learn_packet(0x1d, 0xf8, 0x10, 0x0a, next_learn_counter(), (this->command_.blind_addr >> 16) & 0xFF, (this->command_.blind_addr >> 8) & 0xFF, this->command_.blind_addr & 0xFF, ELERO_LEARN_RAW_ACK_PAYLOAD, 10, now + 400);
  queue_raw_learn_packet(0x1d, 0x78, 0x10, 0x00, next_learn_counter(), (this->command_.blind_addr >> 16) & 0xFF, (this->command_.blind_addr >> 8) & 0xFF, this->command_.blind_addr & 0xFF, ELERO_LEARN_RAW_START_PAYLOAD, 10, now + 900);
}

void EleroCover::trigger_learn_step(EleroLearnStep step) {
  const uint32_t learn_remote = this->learn_remote_address_ != 0 ? this->learn_remote_address_ : this->command_.remote_addr;
  if (learn_remote == 0) {
    ESP_LOGE(TAG, "No learn remote address configured");
    return;
  }

  auto queue_raw_learn_packet = [&](uint8_t length, uint8_t type, uint8_t type2, uint8_t hop,
                                   uint8_t counter, uint8_t dst0, uint8_t dst1, uint8_t dst2,
                                   const uint8_t *body, uint8_t body_len, uint32_t not_before = 0) {
    QueuedCommand queued{};
    queued.raw_mode = true;
    queued.has_counter = true;
    queued.counter = counter;
    queued.not_before = not_before;
    queued.packet_len = length + 1;
    uint8_t *p = queued.raw_packet;
    p[0] = length;
    p[1] = counter;
    p[2] = type;
    p[3] = type2;
    p[4] = hop;
    p[5] = 0x01;
    p[6] = this->command_.channel;
    p[7] = (learn_remote >> 16) & 0xFF;
    p[8] = (learn_remote >> 8) & 0xFF;
    p[9] = learn_remote & 0xFF;
    p[10] = (learn_remote >> 16) & 0xFF;
    p[11] = (learn_remote >> 8) & 0xFF;
    p[12] = learn_remote & 0xFF;
    p[13] = (learn_remote >> 16) & 0xFF;
    p[14] = (learn_remote >> 8) & 0xFF;
    p[15] = learn_remote & 0xFF;
    p[16] = 0x01;
    p[17] = dst0;
    p[18] = dst1;
    p[19] = dst2;
    for (uint8_t i = 0; i < body_len; i++) {
      p[20 + i] = body[i];
    }
    this->commands_to_send_.push(queued);
  };

  auto next_learn_counter = [&]() -> uint8_t {
    uint8_t counter = this->learn_counter_;
    this->learn_counter_ = this->next_counter_(this->learn_counter_);
    return counter;
  };

  switch (step) {
    case ELERO_LEARN_STEP_START:
      ESP_LOGI(TAG, "Queueing learn start for remote 0x%06x", learn_remote);
      queue_raw_learn_packet(0x1b, 0x70, 0x10, 0x00, next_learn_counter(), 0x21, 0x00, 0x02, ELERO_LEARN_RAW_INIT_DATA, 8);
      this->learn_waiting_for_reply_ = true;
      break;
    case ELERO_LEARN_STEP_CONFIRM_UP:
      ESP_LOGI(TAG, "Queueing learn confirm UP for remote 0x%06x", learn_remote);
      queue_raw_learn_packet(0x1d, 0x78, 0x10, 0x00, next_learn_counter(), (this->command_.blind_addr >> 16) & 0xFF, (this->command_.blind_addr >> 8) & 0xFF, this->command_.blind_addr & 0xFF, ELERO_LEARN_RAW_CONFIRM_UP_PAYLOAD_A, 10);
      queue_raw_learn_packet(0x1d, 0x78, 0x10, 0x00, next_learn_counter(), (this->command_.blind_addr >> 16) & 0xFF, (this->command_.blind_addr >> 8) & 0xFF, this->command_.blind_addr & 0xFF, ELERO_LEARN_RAW_CONFIRM_UP_PAYLOAD_B, 10);
      break;
    case ELERO_LEARN_STEP_CONFIRM_DOWN:
      ESP_LOGI(TAG, "Queueing learn confirm DOWN for remote 0x%06x", learn_remote);
      queue_raw_learn_packet(0x1d, 0x78, 0x10, 0x00, next_learn_counter(), (this->command_.blind_addr >> 16) & 0xFF, (this->command_.blind_addr >> 8) & 0xFF, this->command_.blind_addr & 0xFF, ELERO_LEARN_RAW_CONFIRM_DOWN_PAYLOAD_A, 10);
      queue_raw_learn_packet(0x1d, 0x78, 0x10, 0x00, next_learn_counter(), (this->command_.blind_addr >> 16) & 0xFF, (this->command_.blind_addr >> 8) & 0xFF, this->command_.blind_addr & 0xFF, ELERO_LEARN_RAW_CONFIRM_DOWN_PAYLOAD_B, 10);
      break;
    case ELERO_LEARN_STEP_FINALIZE:
      ESP_LOGI(TAG, "Queueing learn finalize for remote 0x%06x", learn_remote);
      queue_raw_learn_packet(0x1d, 0x78, 0x10, 0x00, next_learn_counter(), (this->command_.blind_addr >> 16) & 0xFF, (this->command_.blind_addr >> 8) & 0xFF, this->command_.blind_addr & 0xFF, ELERO_LEARN_RAW_FINALIZE_PAYLOAD, 10);
      break;
  }
}

float EleroCover::get_setup_priority() const { return setup_priority::DATA; }

cover::CoverTraits EleroCover::get_traits() {
  auto traits = cover::CoverTraits();
  traits.set_supports_stop(true);
  if((this->open_duration_ > 0) && (this->close_duration_ > 0))
    traits.set_supports_position(true);
  else
    traits.set_supports_position(false);
  traits.set_supports_toggle(true);
  traits.set_is_assumed_state(true);
  traits.set_supports_tilt(this->supports_tilt_);
  return traits;
}

void EleroCover::set_rx_state(uint8_t state) {
  ESP_LOGV(TAG, "Got state: 0x%02x for blind 0x%02x", state, this->command_.blind_addr);
  float pos = this->position;
  float current_tilt = this->tilt;
  CoverOperation op = this->current_operation;

  switch(state) {
  case ELERO_STATE_TOP:
    pos = COVER_OPEN;
    op = COVER_OPERATION_IDLE;
    current_tilt = 0.0;
    break;
  case ELERO_STATE_BOTTOM:
    pos = COVER_CLOSED;
    op = COVER_OPERATION_IDLE;
    current_tilt = 0.0;
    break;
  case ELERO_STATE_START_MOVING_UP:
  case ELERO_STATE_MOVING_UP:
    op = COVER_OPERATION_OPENING;
    current_tilt = 0.0;
    break;
  case ELERO_STATE_START_MOVING_DOWN:
  case ELERO_STATE_MOVING_DOWN:
    op = COVER_OPERATION_CLOSING;
    current_tilt = 0.0;
    break;
  case ELERO_STATE_TILT:
    op = COVER_OPERATION_IDLE;
    current_tilt = 1.0;
    break;
  case ELERO_STATE_STOPPED:
    op = COVER_OPERATION_IDLE;
    current_tilt = 0.0;
    break;
  default:
    op = COVER_OPERATION_IDLE;
    current_tilt = 0.0;
  }

  if((pos != this->position) || (op != this->current_operation) || (current_tilt != this->tilt)) {
    this->position = pos;
    this->tilt = current_tilt;
    this->current_operation = op;
    this->publish_state();
  }
}

void EleroCover::increase_counter() {
  this->command_.counter = this->next_counter_(this->command_.counter);
  this->save_counter_();
}

void EleroCover::control(const cover::CoverCall &call) {
  if (call.get_stop()) {
    this->start_movement(COVER_OPERATION_IDLE);
  }
  if (call.get_position().has_value()) {
    auto pos = *call.get_position();
    this->target_position_ = pos;
    if((pos > this->position) || (pos == COVER_OPEN)) {
      this->start_movement(COVER_OPERATION_OPENING);
    } else {
      this->start_movement(COVER_OPERATION_CLOSING);
    }
  }
  if (call.get_tilt().has_value()) {
    auto tilt = *call.get_tilt();
    if(tilt > 0) {
      this->queue_control_command(this->command_tilt_);
      this->tilt = 1.0;
    } else {
      this->tilt = 0.0;
    }
  }
  if (call.get_toggle().has_value()) {
    if(this->current_operation != COVER_OPERATION_IDLE) {
      this->start_movement(COVER_OPERATION_IDLE);
    } else {
      if(this->position == COVER_CLOSED || this->last_operation_ == COVER_OPERATION_CLOSING) {
        this->target_position_ = COVER_OPEN;
        this->start_movement(COVER_OPERATION_OPENING);
      } else {
        this->target_position_ = COVER_CLOSED;
        this->start_movement(COVER_OPERATION_CLOSING);
      }
    }
  }
}

// FIXME: Most of this should probably be moved to the
// handle_commands function to only publish a new state
// if at least the transmission was successful
void EleroCover::start_movement(CoverOperation dir) {
  switch(dir) {
    case COVER_OPERATION_OPENING:
      ESP_LOGV(TAG, "Sending OPEN command");
      this->queue_control_command(this->command_up_);
      // Reset tilt state on movement
      this->tilt = 0.0;
      this->last_operation_ = COVER_OPERATION_OPENING;
    break;
    case COVER_OPERATION_CLOSING:
      ESP_LOGV(TAG, "Sending CLOSE command");
      this->queue_control_command(this->command_down_);
      // Reset tilt state on movement
      this->tilt = 0.0;
      this->last_operation_ = COVER_OPERATION_CLOSING;
    break;
    case COVER_OPERATION_IDLE:
      this->queue_control_command(this->command_stop_);
    break;
  }

  if(dir == this->current_operation)
    return;

  this->current_operation = dir;
  this->movement_start_ = millis();
  this->last_recompute_time_ = millis();
  this->publish_state();
}

void EleroCover::recompute_position() {
  if(this->current_operation == COVER_OPERATION_IDLE)
    return;


  float dir;
  float action_dur;
  switch (this->current_operation) {
    case COVER_OPERATION_OPENING:
      dir = 1.0f;
      action_dur = this->open_duration_;
      break;
    case COVER_OPERATION_CLOSING:
      dir = -1.0f;
      action_dur = this->close_duration_;
      break;
    default:
      return;
  }

  const uint32_t now = millis();
  this->position += dir * (now - this->last_recompute_time_) / action_dur;
  this->position = clamp(this->position, 0.0f, 1.0f);

  this->last_recompute_time_ = now;

}

} // namespace elero
} // namespace esphome
