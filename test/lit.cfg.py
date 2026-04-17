# TODO: remove this file?
import os
import lit

config.name = "C' tests"
config.test_format = lit.formats.ShTest()

config.test_source_root = os.path.dirname(__file__)
config.test_exec_root = os.path.join(config.my_build_root, "test")
config.suffixes = [".cpr"]

config.substitutions.extend(
    [
        ("%cpc", os.path.join(config.my_build_root, "src/cprime/compiler/driver/cpc")),
        ("%check_output", os.path.join(config.test_source_root, "check_output.py")),
        ("%check_exitcode", os.path.join(config.test_source_root, "check_exitcode.py")),
        # ("%select", os.path.join(config.test_source_root, "select.py")),
    ]
)
