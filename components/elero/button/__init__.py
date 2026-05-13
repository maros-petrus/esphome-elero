import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import button
from esphome.const import CONF_ID

from .. import elero_ns
from ..cover import EleroCover

AUTO_LOAD = ["button"]
DEPENDENCIES = ["elero"]

CONF_COVER_ID = "cover_id"
CONF_LEARN_STEP = "learn_step"

EleroLearnButton = elero_ns.class_("EleroLearnButton", button.Button, cg.Component)
EleroLearnStep = elero_ns.enum("EleroLearnStep")

LEARN_STEPS = {
    "start": EleroLearnStep.ELERO_LEARN_STEP_START,
    "confirm_up": EleroLearnStep.ELERO_LEARN_STEP_CONFIRM_UP,
    "confirm_down": EleroLearnStep.ELERO_LEARN_STEP_CONFIRM_DOWN,
    "finalize": EleroLearnStep.ELERO_LEARN_STEP_FINALIZE,
}

CONFIG_SCHEMA = button.button_schema(EleroLearnButton).extend(
    {
        cv.GenerateID(): cv.declare_id(EleroLearnButton),
        cv.Required(CONF_COVER_ID): cv.use_id(EleroCover),
        cv.Required(CONF_LEARN_STEP): cv.enum(LEARN_STEPS, lower=True),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = await button.new_button(config)
    await cg.register_component(var, config)
    parent = await cg.get_variable(config[CONF_COVER_ID])
    cg.add(var.set_parent(parent))
    cg.add(var.set_learn_step(config[CONF_LEARN_STEP]))
