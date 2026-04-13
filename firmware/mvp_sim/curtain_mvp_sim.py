from __future__ import annotations

from dataclasses import dataclass


@dataclass
class Ctx:
    state: str = "Idle"
    move_ticks: int = 0
    timeout_ticks: int = 20
    fault_code: int = 0


def transition(ctx: Ctx, evt: str) -> None:
    s = ctx.state

    if s == "Fault":
        if evt == "reset":
            ctx.state = "Idle"
            ctx.fault_code = 0
            ctx.move_ticks = 0
            print("Fault -> Idle (reset)")
        else:
            print("Fault locked, ignore event")
        return

    if evt == "fault":
        ctx.state = "Fault"
        ctx.fault_code = 0x03
        print("Any -> Fault (forced)")
        return

    if evt == "tick":
        if s in ("Opening", "Closing"):
            ctx.move_ticks += 1
            if ctx.move_ticks > ctx.timeout_ticks:
                ctx.state = "Fault"
                ctx.fault_code = 0x02
                print("Timeout -> Fault (0x02)")
            else:
                print(f"{s}: tick={ctx.move_ticks}")
        else:
            print(f"{s}: idle tick")
        return

    if s == "Idle":
        if evt == "open":
            ctx.state = "Opening"
            ctx.move_ticks = 0
            print("Idle -> Opening")
        elif evt == "close":
            ctx.state = "Closing"
            ctx.move_ticks = 0
            print("Idle -> Closing")
        elif evt == "stop":
            print("Idle: already stopped")
        elif evt in ("left_limit", "right_limit"):
            print("Idle: limit ignored")
        else:
            print("Unknown event")
        return

    if s == "Opening":
        if evt == "right_limit":
            ctx.state = "Idle"
            ctx.move_ticks = 0
            print("Opening -> Idle (arrived right)")
        elif evt == "stop":
            ctx.state = "Idle"
            ctx.move_ticks = 0
            print("Opening -> Idle (manual stop)")
        elif evt == "close":
            ctx.state = "Closing"
            ctx.move_ticks = 0
            print("Opening -> Closing (switch target)")
        elif evt == "left_limit":
            ctx.state = "Fault"
            ctx.fault_code = 0x01
            print("Wrong limit -> Fault (0x01)")
        elif evt == "open":
            print("Opening: already opening")
        else:
            print("Unknown event")
        return

    if s == "Closing":
        if evt == "left_limit":
            ctx.state = "Idle"
            ctx.move_ticks = 0
            print("Closing -> Idle (arrived left)")
        elif evt == "stop":
            ctx.state = "Idle"
            ctx.move_ticks = 0
            print("Closing -> Idle (manual stop)")
        elif evt == "open":
            ctx.state = "Opening"
            ctx.move_ticks = 0
            print("Closing -> Opening (switch target)")
        elif evt == "right_limit":
            ctx.state = "Fault"
            ctx.fault_code = 0x01
            print("Wrong limit -> Fault (0x01)")
        elif evt == "close":
            print("Closing: already closing")
        else:
            print("Unknown event")
        return


def print_help() -> None:
    print("events: open close stop left_limit right_limit tick fault reset status quit")


def main() -> None:
    ctx = Ctx()
    print("Curtain MVP state machine sim started")
    print_help()
    while True:
        evt = input(">> ").strip().lower()
        if evt == "quit":
            break
        if evt == "help":
            print_help()
            continue
        if evt == "status":
            print(f"state={ctx.state}, tick={ctx.move_ticks}, fault=0x{ctx.fault_code:02X}")
            continue
        transition(ctx, evt)


if __name__ == "__main__":
    main()

