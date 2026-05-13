import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import cover

from esphome.const import CONF_ID, CONF_NAME, CONF_CHANNEL, CONF_OPEN_DURATION, CONF_CLOSE_DURATION
from .. import elero_ns, elero, CONF_ELERO_ID

DEPENDENCIES = ["elero"]
CODEOWNERS = ["@andyboeh"]

CONF_BLIND_ADDRESS = "blind_address"
CONF_REMOTE_ADDRESS = "remote_address"
CONF_PAYLOAD_1 = "payload_1"
CONF_PAYLOAD_2 = "payload_2"
CONF_CONTROL_PAYLOAD_1 = "control_payload_1"
CONF_CONTROL_PAYLOAD_2 = "control_payload_2"
CONF_PCKINF_1 = "pck_inf1"
CONF_PCKINF_2 = "pck_inf2"
CONF_CONTROL_PCKINF_1 = "control_pck_inf1"
CONF_CONTROL_PCKINF_2 = "control_pck_inf2"
CONF_CONTROL_DOWN_PCKINF_2 = "control_down_pck_inf2"
CONF_HOP = "hop"
CONF_CONTROL_HOP = "control_hop"
CONF_CONTROL_BACKWARD_ADDRESS = "control_backward_address"
CONF_CONTROL_FORWARD_ADDRESS = "control_forward_address"
CONF_CONTROL_SHORT_DST = "control_short_dst"
CONF_LEARN_REMOTE_ADDRESS = "learn_remote_address"
CONF_COMMAND_UP = "command_up"
CONF_COMMAND_DOWN = "command_down"
CONF_COMMAND_STOP = "command_stop"
CONF_COMMAND_CHECK = "command_check"
CONF_COMMAND_TILT = "command_tilt"
CONF_COMMAND_CHECK_LEN = "command_check_len"
CONF_COMMAND_CONTROL_LEN = "command_control_len"
CONF_POLL_INTERVAL = "poll_interval"
CONF_SUPPORTS_TILT = "supports_tilt"

EleroCover = elero_ns.class_("EleroCover", cover.Cover, cg.Component)

def poll_interval(value):
    if value == "never":
        return 4294967295  # uint32_t max
    return cv.positive_time_period_milliseconds(value)

CONFIG_SCHEMA = cover.cover_schema(EleroCover).extend(
    {
        cv.GenerateID(CONF_ELERO_ID): cv.use_id(elero),
        cv.Required(CONF_BLIND_ADDRESS): cv.hex_int_range(min=0x0, max=0xffffff),
        cv.Required(CONF_CHANNEL): cv.int_range(min=0, max=255),
        cv.Required(CONF_REMOTE_ADDRESS): cv.hex_int_range(min=0x0, max=0xffffff),
        cv.Optional(CONF_POLL_INTERVAL, default="5min"): poll_interval,
        cv.Optional(CONF_OPEN_DURATION, default="0s"): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_CLOSE_DURATION, default="0s"): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_PAYLOAD_1, default=0x00): cv.hex_int_range(min=0x0, max=0xff),
        cv.Optional(CONF_PAYLOAD_2, default=0x04): cv.hex_int_range(min=0x0, max=0xff),
        cv.Optional(CONF_CONTROL_PAYLOAD_1): cv.hex_int_range(min=0x0, max=0xff),
        cv.Optional(CONF_CONTROL_PAYLOAD_2): cv.hex_int_range(min=0x0, max=0xff),
        cv.Optional(CONF_PCKINF_1, default=0x6a): cv.hex_int_range(min=0x0, max=0xff),
        cv.Optional(CONF_PCKINF_2, default=0x00): cv.hex_int_range(min=0x0, max=0xff),
        cv.Optional(CONF_CONTROL_PCKINF_1): cv.hex_int_range(min=0x0, max=0xff),
        cv.Optional(CONF_CONTROL_PCKINF_2): cv.hex_int_range(min=0x0, max=0xff),
        cv.Optional(CONF_CONTROL_DOWN_PCKINF_2): cv.hex_int_range(min=0x0, max=0xff),
        cv.Optional(CONF_HOP, default=0x0a): cv.hex_int_range(min=0x0, max=0xff),
        cv.Optional(CONF_CONTROL_HOP): cv.hex_int_range(min=0x0, max=0xff),
        cv.Optional(CONF_CONTROL_BACKWARD_ADDRESS): cv.hex_int_range(min=0x0, max=0xffffff),
        cv.Optional(CONF_CONTROL_FORWARD_ADDRESS): cv.hex_int_range(min=0x0, max=0xffffff),
        cv.Optional(CONF_CONTROL_SHORT_DST): cv.hex_int_range(min=0x0, max=0xff),
        cv.Optional(CONF_LEARN_REMOTE_ADDRESS): cv.hex_int_range(min=0x0, max=0xffffff),
        cv.Optional(CONF_COMMAND_UP, default=0x20): cv.hex_int_range(min=0x0, max=0xff),
        cv.Optional(CONF_COMMAND_DOWN, default=0x40): cv.hex_int_range(min=0x0, max=0xff),
        cv.Optional(CONF_COMMAND_STOP, default=0x10): cv.hex_int_range(min=0x0, max=0xff),
        cv.Optional(CONF_COMMAND_CHECK, default=0x00): cv.hex_int_range(min=0x0, max=0xff),
        cv.Optional(CONF_COMMAND_TILT, default=0x24): cv.hex_int_range(min=0x0, max=0xff),
        cv.Optional(CONF_COMMAND_CHECK_LEN, default=29): cv.one_of(27, 29, int=True),
        cv.Optional(CONF_COMMAND_CONTROL_LEN, default=29): cv.one_of(27, 29, int=True),
        cv.Optional(CONF_SUPPORTS_TILT, default=False): cv.boolean,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = await cover.new_cover(config)
    await cg.register_component(var, config)

    parent = await cg.get_variable(config[CONF_ELERO_ID])
    cg.add(var.set_elero_parent(parent))
    cg.add(var.set_blind_address(config[CONF_BLIND_ADDRESS]))
    cg.add(var.set_channel(config[CONF_CHANNEL]))
    cg.add(var.set_remote_address(config[CONF_REMOTE_ADDRESS]))
    cg.add(var.set_open_duration(config[CONF_OPEN_DURATION]))
    cg.add(var.set_close_duration(config[CONF_CLOSE_DURATION]))
    cg.add(var.set_payload_1(config[CONF_PAYLOAD_1]))
    cg.add(var.set_payload_2(config[CONF_PAYLOAD_2]))
    cg.add(var.set_control_payload_1(config.get(CONF_CONTROL_PAYLOAD_1, config[CONF_PAYLOAD_1])))
    cg.add(var.set_control_payload_2(config.get(CONF_CONTROL_PAYLOAD_2, config[CONF_PAYLOAD_2])))
    cg.add(var.set_pckinf_1(config[CONF_PCKINF_1]))
    cg.add(var.set_pckinf_2(config[CONF_PCKINF_2]))
    cg.add(var.set_control_pckinf_1(config.get(CONF_CONTROL_PCKINF_1, config[CONF_PCKINF_1])))
    cg.add(var.set_control_pckinf_2(config.get(CONF_CONTROL_PCKINF_2, config[CONF_PCKINF_2])))
    cg.add(var.set_control_down_pckinf_2(config.get(CONF_CONTROL_DOWN_PCKINF_2, config.get(CONF_CONTROL_PCKINF_2, config[CONF_PCKINF_2]))))
    cg.add(var.set_hop(config[CONF_HOP]))
    cg.add(var.set_control_hop(config.get(CONF_CONTROL_HOP, config[CONF_HOP])))
    cg.add(var.set_control_backward_address(config.get(CONF_CONTROL_BACKWARD_ADDRESS, config[CONF_BLIND_ADDRESS])))
    cg.add(var.set_control_forward_address(config.get(CONF_CONTROL_FORWARD_ADDRESS, config[CONF_REMOTE_ADDRESS])))
    cg.add(var.set_control_short_dst(config.get(CONF_CONTROL_SHORT_DST, config[CONF_CHANNEL])))
    cg.add(var.set_learn_remote_address(config.get(CONF_LEARN_REMOTE_ADDRESS, config[CONF_REMOTE_ADDRESS])))
    cg.add(var.set_command_up(config[CONF_COMMAND_UP]))
    cg.add(var.set_command_down(config[CONF_COMMAND_DOWN]))
    cg.add(var.set_command_check(config[CONF_COMMAND_CHECK]))
    cg.add(var.set_command_stop(config[CONF_COMMAND_STOP]))
    cg.add(var.set_command_tilt(config[CONF_COMMAND_TILT]))
    cg.add(var.set_command_check_len(config[CONF_COMMAND_CHECK_LEN]))
    cg.add(var.set_command_control_len(config[CONF_COMMAND_CONTROL_LEN]))
    cg.add(var.set_poll_interval(config[CONF_POLL_INTERVAL]))
    cg.add(var.set_supports_tilt(config[CONF_SUPPORTS_TILT]))
