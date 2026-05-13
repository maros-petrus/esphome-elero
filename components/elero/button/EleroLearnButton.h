#pragma once

#include "esphome/components/button/button.h"
#include "esphome/components/elero/cover/EleroCover.h"

namespace esphome {
namespace elero {

class EleroLearnButton : public button::Button, public Component {
 public:
  void set_parent(EleroCover *parent) { this->parent_ = parent; }
  void set_learn_step(EleroLearnStep step) { this->step_ = step; }

 protected:
  void press_action() override;

  EleroCover *parent_{nullptr};
  EleroLearnStep step_{ELERO_LEARN_STEP_START};
};

}  // namespace elero
}  // namespace esphome
