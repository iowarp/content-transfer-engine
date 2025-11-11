@CLAUDE.md Build python bindings for core using nanobind. Use context7 MCP to get documentation on nanobind. We have already added it as a submodule to this repository under external/nanobind. For now, only build python bindings for PollTelemetryLog of the client code. Make sure to add nanobind to the cmakes. Ensure that everything compiles after the changes.

Place the bindings under wrapper/python. Make sure to also implement bindings for the CTE initialization code (WRP_CTE_CLIENT_INIT).  Replace the existing python bindings and cmake for the new code.

Make sure to build a unit test and add to cmake for the python bindings. Just make sure it compiles

we need to test PollTelemetryLog in the python bindings. We should also add the chimaera  runtime initialization functions. The unit test should start the chimaera runtime and then initialize the  cte. And then execute all subsequent tests.