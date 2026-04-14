from command_parser import parse_voice_command


def assert_target(text: str, expected: int) -> None:
    cmd = parse_voice_command(text)
    assert cmd is not None, f"expected target command, got None for: {text}"
    assert cmd.kind == "target", f"expected target kind, got: {cmd.kind}"
    assert cmd.target_pos == expected, f"expected {expected}, got {cmd.target_pos} for: {text}"


def assert_stop(text: str) -> None:
    cmd = parse_voice_command(text)
    assert cmd is not None, f"expected stop command, got None for: {text}"
    assert cmd.kind == "stop", f"expected stop kind, got: {cmd.kind}"


def run() -> None:
    assert_target("打开窗帘", 100)
    assert_target("关闭窗帘", 0)
    assert_stop("停止窗帘")

    assert_target("窗帘到50", 50)
    assert_target("窗帘到75%", 75)
    assert_target("窗帘到五十", 50)
    assert_target("窗帘开到一百", 100)
    assert_target("窗帘调到二十五", 25)
    assert_target("窗帘百分之三十", 30)
    assert_target("窗帘一半", 50)
    assert_target("窗帘全开", 100)
    assert_target("窗帘全关", 0)

    # Stop has higher priority than movement words.
    assert_stop("停止并打开窗帘")

    assert parse_voice_command("今天天气怎么样") is None
    print("command parser tests passed")


if __name__ == "__main__":
    run()
