Import("env")


def silence_external_library(env, node):
    return env.Object(node, CCFLAGS=env["CCFLAGS"] + ["-w"])


env.AddBuildMiddleware(silence_external_library, "*/SSLClient/src/SSLClientParameters.cpp")
