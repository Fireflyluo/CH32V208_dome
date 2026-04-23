const builtin = @import("builtin");
const std = @import("std");

const tmosTaskID = u8;
const tmosEvents = u16;
const tmosTimer = u32;

extern fn TMOS_SystemProcess() void;
extern fn TMOS_ProcessEventRegister(cb: *const fn (tmosTaskID, tmosEvents) callconv(.c) tmosEvents) tmosTaskID;
extern fn tmos_set_event(task_id: tmosTaskID, event: tmosEvents) u8;
extern fn tmos_start_reload_task(task_id: tmosTaskID, event: tmosEvents, time: tmosTimer) u8;

extern fn board_led_toggle() void;
extern fn board_led_write(on: c_int) void;

const EVT_INIT: tmosEvents = 1 << 0;
const EVT_TICK: tmosEvents = 1 << 1;

const system_time_micros: u32 = 625;
fn msToSystemTime(ms: u32) u32 {
    return (ms * 1000) / system_time_micros;
}

var g_task_id: tmosTaskID = 0xff;

fn appProcessEvent(task_id: tmosTaskID, events: tmosEvents) callconv(.c) tmosEvents {
    _ = task_id;

    var left = events;
    if ((events & EVT_INIT) != 0) {
        _ = tmos_start_reload_task(g_task_id, EVT_TICK, msToSystemTime(500));
        left ^= EVT_INIT;
    }
    if ((events & EVT_TICK) != 0) {
        board_led_toggle();
        left ^= EVT_TICK;
    }
    return left;
}

pub export fn zig_app_main() noreturn {
    board_led_write(0);

    g_task_id = TMOS_ProcessEventRegister(appProcessEvent);
    _ = tmos_set_event(g_task_id, EVT_INIT);

    while (true) {
        TMOS_SystemProcess();
    }
}

pub fn panic(msg: []const u8, trace: ?*std.builtin.StackTrace, ret_addr: ?usize) noreturn {
    _ = msg;
    _ = trace;
    _ = ret_addr;
    while (true) {
        asm volatile ("wfi");
    }
}
