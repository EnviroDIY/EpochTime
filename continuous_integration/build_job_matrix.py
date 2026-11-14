#!/usr/bin/env python
# %%
"""
Custom matrix builder for EpochTime step 3 - Build Matrix.

This script builds the EpochTime-specific job matrix by:
1. Reading the modem list from EpochTimeClient.h
2. Loading the extra PlatformIO configuration
3. Applying EpochTime-specific matrix exclusions and inclusions
4. Returning the final filtered matrix

This is designed to be called from the CI pipeline as a custom builder.
See: https://github.com/EnviroDIY/workflows
"""

import os
import re
import json

# # %%
# local testing paths below
# local testing paths above

# %%
# Import helper functions from the CI pipeline
from build_utils import (
    dict_product,
    remove_nested_duplicates,
    read_platformio_config,
    build_pio_mappings,
)


# %%
def build_custom_matrix(config: dict) -> list[dict]:
    """
    Build the EpochTime-specific job matrix.

    This is the main entry point called by the CI pipeline (3_build_matrix.py).
    """

    # %%
    # Extract config values
    workspace_path = config.get("workspace_path", os.getcwd())

    compiler_list = config.get("compiler_list", ["platformio", "arduino-cli"])
    examples_to_build = config.get("examples_to_build", [])
    build_envs = config.get("build_envs", [])
    build_fqbns = config.get("build_fqbns", [])
    inline_defines = config.get("inline_defines", [[]])
    compiler_flags = config.get("compiler_flags", [[]])

    if "platformio" in compiler_list:
        # Read the PlatformIO config and build mapping dictionaries for boards and
        # environments only when PlatformIO jobs were requested.
        print("Reading the PlatformIO config...")
        pio_ini_dir = os.path.dirname(config.get("pio_config_file"))  # type: ignore
        pio_config = read_platformio_config(pio_ini_dir)
        print("Building mapping dictionaries...")
        build_pio_mappings(pio_config)

    # %%
    print("=== EpochTime Custom Matrix Builder ===")

    # %%
    # expand the combination of boards, modems, and examples into a job matrix

    p_cart_join = list(
        dict_product(
            {
                "compiler": ["platformio"],
                "example": examples_to_build,
                "pio_env": build_envs,
                "inline_defines": inline_defines,
                "compiler_flags": compiler_flags,
            }
        )
    )

    a_cart_join = list(
        dict_product(
            {
                "compiler": ["arduino-cli"],
                "example": examples_to_build,
                "fqbn": build_fqbns,
                "inline_defines": inline_defines,
                "compiler_flags": compiler_flags,
            }
        )
    )

    cart_join = p_cart_join + a_cart_join
    cart_join_list = [json.dumps(e) for e in cart_join]
    print(f"Total possible combinations: {len(cart_join)}")

    # %%

    # a list of known failures to skip in the job matrix

    # don't use strftime on the full test of smaller boards
    matrix_exclusions = [
        {
            "compiler": ["arduino-cli"],
            # "example": [os.path.join("extras", "FormattingTests")],
            "example": examples_to_build,
            "fqbn": [
                "arduino:avr:leonardo",
                "arduino:avr:yun",
                "arduino:avr:uno",
                "adafruit:avr:feather328p",
                "adafruit:avr:feather32u4",
            ],
            "inline_defines": [[]],
            "compiler_flags": [["-D EPOCHTIME_ENABLE_STRFTIME=1"]],
        },
        {
            "compiler": ["platformio"],
            # "example": [os.path.join("extras", "FormattingTests")],
            "example": examples_to_build,
            "pio_env": [
                "leonardo",
                "yun",
                "uno",
                "feather328p",
                "feather32u4",
            ],
            "inline_defines": [[]],
            "compiler_flags": [["-D EPOCHTIME_ENABLE_STRFTIME=1"]],
        },
    ]

    # expand the matrix exclusions
    expanded_matrix_exclusions = []
    for exclusion in matrix_exclusions:
        exclusion_list = list(dict_product(exclusion))
        expanded_matrix_exclusions.extend(exclusion_list)

    # filter out the known failures from the job matrix
    expanded_matrix_exclusions_set = remove_nested_duplicates(
        expanded_matrix_exclusions
    )
    expanded_matrix_exclusions_list = [
        json.dumps(e) for e in expanded_matrix_exclusions_set
    ]
    expanded_matrix_exclusions_list.sort()
    print(f"Matrix exclusions: {len(expanded_matrix_exclusions_list)}")

    # %%
    # decide on the filtered matrix to use for the job matrix
    expanded_matrix_exclusions_lookup = set(expanded_matrix_exclusions_list)
    assembled_matrix = [
        json.loads(e)
        for e in cart_join_list
        if e not in expanded_matrix_exclusions_lookup
    ]

    final_matrix = remove_nested_duplicates(assembled_matrix)
    print(f"Final filtered matrix: {len(final_matrix)}")

    # %%
    return final_matrix


# %%
# cSpell:ignore fqbn fqbns rensas
