from command_parser import parse_voice_command


def assert_target(text: str, expected: int) -> None:
    cmd = parse_voice_command(text)
    assert cmd is not None
    assert cmd.kind == "target"
    assert cmd.target_pos == expected


def assert_stop(text: str) -> None:
    cmd = parse_voice_command(text)
    assert cmd is not None
    assert cmd.kind == "stop"


def run() -> None:
    assert_target("打开窗帘", 100)
    assert_target("关闭窗帘", 0)
    assert_stop("停止窗帘")
    assert_target("窗帘到50", 50)
    assert_target("窗帘到75%", 75)
    assert_target("窗帘到五十", 50)
    assert_target("窗帘开到一百", 100)
    assert_target("窗帘一半", 50)
    assert parse_voice_command("天气怎么样") is None
    print("command parser tests passed")


if __name__ == "__main__":
    run()

