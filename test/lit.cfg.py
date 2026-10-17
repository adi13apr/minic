# -*- Python -*-
import os

import lit.formats

config.name = "minic"
config.test_format = lit.formats.ShTest(execute_external=False)
config.suffixes = [".mc"]

config.test_source_root = os.path.dirname(__file__)
config.test_exec_root = os.path.join(config.minic_obj_root, "test")

config.substitutions.append(
    ("%minic", os.path.join(config.minic_obj_root, "tools", "minic", "minic"))
)

config.environment["PATH"] = os.pathsep.join(
    [config.llvm_tools_dir, config.environment.get("PATH", "")]
)
config.substitutions.append(
    ("%examples", os.path.join(config.minic_src_root, "examples"))
)
# Tests marked "REQUIRES: nvptx" are skipped when LLVM has no NVPTX backend.
if config.minic_has_nvptx == "ON":
    config.available_features.add("nvptx")