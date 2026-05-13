#include "EleroLearnButton.h"

namespace esphome {
namespace elero {

void EleroLearnButton::press_action() {
  if (this->parent_ == nullptr) {
    return;
  }
  this->parent_->trigger_learn_step(this->step_);
}

}  // namespace elero
}  // namespace esphome
