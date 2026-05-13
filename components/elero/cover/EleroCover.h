#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/core/preferences.h"
#include "esphome/components/cover/cover.h"
#include "esphome/components/elero/elero.h"
#include <queue>

namespace esphome {
namespace elero {

struct QueuedCommand {
  bool raw_mode;
  uint8_t packet_len;
  uint8_t counter;
  bool has_counter;
  uint32_t not_before;
  uint8_t payload[10];
  uint8_t raw_packet[CC1101_FIFO_LENGTH];
  uint8_t pck_inf_1;
  uint8_t pck_inf_2;
  uint8_t hop;
  uint32_t blind_addr;
  uint32_t remote_addr;
  uint32_t backward_addr;
  uint32_t forward_addr;
  uint8_t short_dst;
};

enum EleroLearnStep : uint8_t {
  ELERO_LEARN_STEP_START = 0,
  ELERO_LEARN_STEP_CONFIRM_UP = 1,
  ELERO_LEARN_STEP_CONFIRM_DOWN = 2,
  ELERO_LEARN_STEP_FINALIZE = 3,
};

class EleroCover : public cover::Cover, public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override;
  
  cover::CoverTraits get_traits() override;
  
  void set_elero_parent(Elero *parent) { this->parent_ = parent; }
  void set_blind_address(uint32_t address) { this->command_.blind_addr = address; }
  void set_channel(uint8_t channel) { this->command_.channel = channel; }
  void set_remote_address(uint32_t remote) { this->command_.remote_addr = remote; }
  void set_payload_1(uint8_t payload) { this->command_.payload[0] = payload; }
  void set_payload_2(uint8_t payload) { this->command_.payload[1] = payload; }
  void set_hop(uint8_t hop) { this->command_.hop = hop; }
  void set_pckinf_1(uint8_t pckinf) { this->command_.pck_inf[0] = pckinf; }
  void set_pckinf_2(uint8_t pckinf) { this->command_.pck_inf[1] = pckinf; }
  void set_control_payload_1(uint8_t payload) { this->control_payload_1_ = payload; }
  void set_control_payload_2(uint8_t payload) { this->control_payload_2_ = payload; }
  void set_control_hop(uint8_t hop) { this->control_hop_ = hop; }
  void set_control_pckinf_1(uint8_t pckinf) { this->control_pckinf_1_ = pckinf; }
  void set_control_pckinf_2(uint8_t pckinf) { this->control_pckinf_2_ = pckinf; }
  void set_control_down_pckinf_2(uint8_t pckinf) { this->control_down_pckinf_2_ = pckinf; }
  void set_control_backward_address(uint32_t address) { this->control_backward_address_ = address; }
  void set_control_forward_address(uint32_t address) { this->control_forward_address_ = address; }
  void set_control_short_dst(uint8_t dst) { this->control_short_dst_ = dst; }
  void set_command_up(uint8_t cmd) { this->command_up_ = cmd; }
  void set_command_down(uint8_t cmd) { this->command_down_ = cmd; }
  void set_command_stop(uint8_t cmd) { this->command_stop_ = cmd; }
  void set_command_check(uint8_t cmd) { this->command_check_ = cmd; }
  void set_command_tilt(uint8_t cmd) { this->command_tilt_ = cmd; }
  void set_command_check_len(uint8_t len) { this->command_check_len_ = len; }
  void set_command_control_len(uint8_t len) { this->command_control_len_ = len; }
  void set_poll_offset(uint32_t offset) { this->poll_offset_ = offset; }
  void set_close_duration(uint32_t dur) { this->close_duration_ = dur; }
  void set_open_duration(uint32_t dur) { this->open_duration_ = dur; }
  void set_poll_interval(uint32_t intvl) { this->poll_intvl_ = intvl; }
  void set_learn_remote_address(uint32_t remote) { this->learn_remote_address_ = remote; }
  uint32_t get_blind_address() { return this->command_.blind_addr; }
  uint32_t get_remote_address() { return this->command_.remote_addr; }
  void set_supports_tilt(bool tilt) { this->supports_tilt_ = tilt; }
  void set_rx_state(uint8_t state);
  void sync_counter_from_remote(uint8_t seen_counter);
  void capture_learn_remote_init(const uint8_t *data, uint8_t len);
  void handle_learn_blind_reply();
  void handle_commands(uint32_t now);
  void queue_check_command();
  void queue_control_command(uint8_t command);
  void trigger_learn_step(EleroLearnStep step);
  void recompute_position();
  void start_movement(cover::CoverOperation op);
  bool is_at_target();
  
 protected:
  void control(const cover::CoverCall &call) override;
  void increase_counter();
  uint8_t next_counter_(uint8_t counter);
  bool counter_is_ahead_(uint8_t candidate) const;
  void save_counter_();

  t_elero_command command_ = {
    .counter = 1,
  };
  Elero *parent_;
  uint32_t last_poll_{0};
  uint32_t last_command_{0};
  uint32_t poll_offset_{0};
  uint32_t movement_start_{0};
  uint32_t open_duration_{0};
  uint32_t close_duration_{0};
  uint32_t last_publish_{0};
  uint32_t last_recompute_time_{0};
  uint32_t poll_intvl_{0};
  float target_position_{0};
  bool supports_tilt_{false};
  uint8_t command_up_{0x20};
  uint8_t command_down_{0x40};
  uint8_t command_check_{0x00};
  uint8_t command_stop_{0x10};
  uint8_t command_tilt_{0x24};
  uint8_t control_payload_1_{0x00};
  uint8_t control_payload_2_{0x04};
  uint8_t control_hop_{0x0a};
  uint8_t control_pckinf_1_{0x6a};
  uint8_t control_pckinf_2_{0x00};
  uint8_t control_down_pckinf_2_{0x00};
  uint8_t control_short_dst_{0x00};
  uint32_t control_backward_address_{0x000000};
  uint32_t control_forward_address_{0x000000};
  uint32_t learn_remote_address_{0x000000};
  uint8_t command_check_len_{0x1d};
  uint8_t command_control_len_{0x1d};
  std::queue<QueuedCommand> commands_to_send_;
  uint8_t send_retries_{0};
  uint8_t send_packets_{0};
  cover::CoverOperation last_operation_{cover::COVER_OPERATION_OPENING};
  uint8_t learn_counter_{1};
  uint8_t last_learn_init_data_[8]{0};
  uint8_t last_learn_init_len_{0};
  bool has_last_learn_init_{false};
  bool learn_waiting_for_reply_{false};
  bool learn_followup_queued_{false};
  ESPPreferenceObject counter_pref_;
  bool counter_pref_loaded_{false};
};

} // namespace elero
} // namespace esphome
