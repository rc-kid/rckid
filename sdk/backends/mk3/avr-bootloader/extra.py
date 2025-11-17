# from https://docs.platformio.org/en/latest/scripting/examples/extra_linker_flags.html

Import("env")

#
# Dump build environment (for debug)
# print(env.Dump())
#

env.Append(
  LINKFLAGS=[
      "-nostartfiles",
  ]
)