Import("env")
#env.Append(LINKFLAGS=["--specs=nano.specs"])
env.Append(LINKFLAGS=["--specs=nosys.specs", "--specs=nano.specs"])
