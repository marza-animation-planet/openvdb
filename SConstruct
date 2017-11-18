import os
import sys
import excons
import excons.tools.maya as maya
import excons.tools.boost as boost
import excons.tools.python as python
import excons.tools.gl as gl
import excons.tools.threads as threads

ARGUMENTS["use-c++11"] = "1"
if sys.platform == "win32":
  # Require mscver 14.0 at least
  mscver = float(ARGUMENTS.get("mscver", "14.0"))
  if mscver < 14.0:
    print("vc14.0 at least required on windows for C++11 proper support.")
    sys.exit(1)
  ARGUMENTS["mscver"] = str(mscver)

env = excons.MakeBaseEnv()

abi3 = (excons.GetArgument("openvdb-abi3", 0, int) != 0)


overrides = {}

# Zlib (require by openexr and c-blosc)
def zlibName(static):
  return (("zlib" if static else "zdll") if sys.platform == "win32" else "z")

def zlibDefines(static):
  return ([] if static else ["ZLIB_DLL"])

rv = excons.ExternalLibRequire("zlib", libnameFunc=zlibName, definesFunc=zlibDefines)
if not rv["require"]:
  excons.PrintOnce("OpenVDB: Build zlib from sources ...")
  excons.Call("zlib", imp=["ZlibName", "ZlibPath", "RequireZlib"])
  def zlibRequire(env):
    RequireZlib(env, static=True)
  overrides["with-zlib"] = os.path.dirname(os.path.dirname(ZlibPath(True)))
  overrides["zlib-static"] = 1
  overrides["zlib-name"] = ZlibName(True)
else:
  zlibRequire = rv["require"]

# C-Blosc (always build from sources)
excons.Call("c-blosc", overrides=overrides, imp=["RequireBlosc"])
def bloscRequire(env):
  RequireBlosc(env, static=True)

# GLFW (always build from sources)
excons.Call("glfw", imp=["RequireGLFW"])
def glfwRequire(env):
  RequireGLFW(static=True)(env)

# OpenEXR (always build from sources)
excons.Call("openexr", overrides=overrides, imp=["RequireHalf", "RequireIlmImf"])
def halfRequire(env):
  RequireHalf(env, static=True)
def openexrRequire(env):
  RequireIlmImf(env, static=True)

# TBB (always build from sources)
excons.Call("tbb", overrides={"tbb-static": 1}, imp=["RequireTBB"])

# GLEW (always include sources)
glew_incdirs = ["ext/glew-2.0.0/include"]
glew_defs = ["GLEW_STATIC"]
glew_srcs = ["ext/glew-2.0.0/src/glew.c"]



cppflags = ""

defs = ["OPENVDB_OPENEXR_STATICLIB"] + ([] if not abi3 else ["OPENVDB_3_ABI_COMPATIBLE"])
if sys.platform == "win32":
  defs.append("NOMINMAX")
  # 4146: unary minus operator applied to unsigned type
  # 4800: forcing value to bool 'true' or 'false'
  cppflags += " /wd4800 /wd4146"
lib_defs = defs + ["OPENVDB_PRIVATE", "OPENVDB_USE_BLOSC"]

boost_libs = ["iostreams", "system"]

lib_srcs = excons.glob("openvdb/*.cc") + \
           excons.glob("openvdb/io/*.cc") + \
           excons.glob("openvdb/math/*.cc") + \
           excons.glob("openvdb/points/*.cc") + \
           excons.glob("openvdb/util/*.cc")

lib_requires = [boost.Require(libs=boost_libs),
                halfRequire,
                bloscRequire,
                zlibRequire, # openvdb uses zlib directly too
                RequireTBB]

include_basedir = "%s/include/openvdb" % excons.OutputBaseDirectory()
InstallHeaders  = env.Install(include_basedir, excons.glob("openvdb/*.h"))
InstallHeaders += env.Install(include_basedir + "/io", excons.glob("openvdb/io/*.h"))
InstallHeaders += env.Install(include_basedir + "/math", excons.glob("openvdb/math/*.h"))
InstallHeaders += env.Install(include_basedir + "/points", excons.glob("openvdb/points/*.h"))
InstallHeaders += env.Install(include_basedir + "/util", excons.glob("openvdb/util/*.h"))

projs = [
  {
    "name": "openvdb",
    "type": "sharedlib",
    "desc": "OpenVDB shared library",
    "alias": "openvdb-shared",
    "version": "4.0.2",
    "install_name": "libopenvdb.4.dylib",
    "soname": "libopenvdb.so.4",
    "incdirs": [".", "openvdb"],
    "defs": lib_defs + ["OPENVDB_DLL"],
    "cppflags": cppflags,
    "srcs": lib_srcs,
    "deps": ["blosc_s"],
    "custom": lib_requires
  },
  {
    "name": "openvdb_s",
    "type": "staticlib",
    "desc": "OpenVDB static library",
    "alias": "openvdb-static",
    "symvis": "default",
    "incdirs": [".", "openvdb"],
    "defs": lib_defs + ["OPENVDB_STATICLIB"],
    "cppflags": cppflags,
    "srcs": lib_srcs,
    "deps": ["blosc_s"],
    "custom": lib_requires
  },
  {
    "name": "pyopenvdb",
    "type": "dynamicmodule",
    "desc": "OpenVDB python module",
    "alias": "openvdb-python",
    "symvis": "default",
    "rpaths": ["../.."],
    "ext": python.ModuleExtension(),
    "prefix": "%s/%s" % (python.ModulePrefix(), python.Version()),
    "incdirs": [".", "openvdb"],
    "defs": defs + ["OPENVDB_STATICLIB"],
    "cppflags": cppflags,
    "srcs": excons.glob("openvdb/python/*.cc"),
    "staticlibs": ["openvdb_s"],
    "custom": [python.SoftRequire,
               boost.Require(libs=boost_libs + ["python"]),
               halfRequire,
               bloscRequire,
               RequireTBB]
  },
  {
    "name": "OpenVDBMaya",
    "type": "dynamicmodule",
    "desc": "OpenVDB Maya plugins",
    "alias": "openvdb-maya",
    "symvis": "default",
    "ext": maya.PluginExt(),
    "rpaths": "../../../lib",
    "bldprefix": maya.Version(),
    "prefix": "maya/%s/plug-ins" % maya.Version(nice=True),
    "defs": lib_defs + ["OPENVDB_STATICLIB"] + glew_defs,
    "cppflags": cppflags,
    "incdirs": [".", "openvdb"] + glew_incdirs,
    "srcs": excons.glob("openvdb_maya/maya/*.cc") + glew_srcs,
    "deps": ["blosc_s"],
    "staticlibs": ["openvdb_s"],
    "custom": [maya.Require,
               boost.Require(libs=boost_libs),
               halfRequire,
               bloscRequire,
               RequireTBB,
               gl.Require],
    "install": {"maya/scripts": excons.glob("openvdb_maya/maya/*.mel"),
                "include/openvdb_maya": ["openvdb_maya/maya/OpenVDBData.h",
                                         "openvdb_maya/maya/OpenVDBUtil.h"]}
  },
  {
    "name": "vdb_print",
    "type": "program",
    "desc": "OpenVDB command line tool",
    "alias": "openvdb-tools",
    "symvis": "default",
    "incdirs": [".", "openvdb"],
    "defs": defs + ["OPENVDB_STATICLIB"],
    "cppflags": cppflags,
    "srcs": excons.glob("openvdb/cmd/openvdb_print/*.cc"),
    "staticlibs": ["openvdb_s"],
    "custom": lib_requires
  },
  {
    "name": "vdb_render",
    "type": "program",
    "desc": "OpenVDB command line tool",
    "alias": "openvdb-tools",
    "symvis": "default",
    "incdirs": [".", "openvdb"],
    "defs": defs + ["OPENVDB_STATICLIB"],
    "cppflags": cppflags,
    "srcs": excons.glob("openvdb/cmd/openvdb_render/*.cc"),
    "staticlibs": ["openvdb_s"],
    "custom": [boost.Require(libs=boost_libs),
               openexrRequire,
               bloscRequire,
               RequireTBB]
  },
  {
    "name": "vdb_view",
    "type": "program",
    "desc": "OpenVDB command line tool",
    "alias": "openvdb-tools",
    "symvis": "default",
    "incdirs": [".", "openvdb"] + glew_incdirs,
    "defs": defs + ["OPENVDB_STATICLIB", "OPENVDB_USE_GLFW_3"] + glew_defs,
    "cppflags": cppflags + (" -Wno-deprecated-declarations" if sys.platform == "darwin" else ""),
    "srcs": excons.glob("openvdb/cmd/openvdb_view/*.cc") +
            excons.glob("openvdb/viewer/*.cc") +
            glew_srcs,
    "staticlibs": ["openvdb_s"],
    "custom": [glfwRequire, boost.Require(libs=["thread"])] + lib_requires,
    "install": {"include/openvdb_viewer": excons.glob("openvdb/viewer/*.h")}
  },
  {
    "name": "vdb_lod",
    "type": "program",
    "desc": "OpenVDB command line tool",
    "alias": "openvdb-tools",
    "symvis": "default",
    "incdirs": [".", "openvdb"],
    "defs": defs + ["OPENVDB_STATICLIB"],
    "cppflags": cppflags + (" -Wno-unused-variable" if sys.platform != "win32" else ""),
    "srcs": excons.glob("openvdb/cmd/openvdb_lod/*.cc"),
    "staticlibs": ["openvdb_s"],
    "custom": lib_requires
  }
]

build_opts = """OPENVDB OPTIONS
   openvdb-abi3=0|1   : Compile with OpenVDB 3 ABI compatibility"""
   
excons.AddHelpOptions(openvdb=build_opts)
excons.AddHelpTargets(eco="Ecosystem distribution")
excons.AddHelpOptions(eco="ECO OPTIONS\n  eco-dir=<path> : Ecosystem distribution install directory")

targets = excons.DeclareTargets(env, projs)

env.Depends(targets["openvdb"], InstallHeaders)
env.Depends(targets["openvdb_s"], InstallHeaders)

if "eco" in COMMAND_LINE_TARGETS:
  plat = excons.EcosystemPlatform()
  dist_env, ver_dir = excons.EcosystemDist(env, "openvdb.env",
                                           {"openvdb-tools": "/%s/bin" % plat,
                                            "openvdb-static": "/%s/lib" % plat,
                                            "openvdb-shared": "/%s/lib" % plat,
                                            "openvdb-python": "/%s/lib/python/%s" % (plat, python.Version()),
                                            "openvdb-maya": "/%s/maya/%s/plug-ins" % (plat, maya.Version(nice=True))})
  eco_incbase = "%s/%s/include/openvdb" % (ver_dir, plat)
  dist_env.Install(eco_incbase, excons.glob("openvdb/*.h"))
  dist_env.Install(eco_incbase + "/io", excons.glob("openvdb/io/*.h"))
  dist_env.Install(eco_incbase + "/math", excons.glob("openvdb/math/*.h"))
  dist_env.Install(eco_incbase + "/points", excons.glob("openvdb/points/*.h"))
  dist_env.Install(eco_incbase + "/util", excons.glob("openvdb/util/*.h"))
  dist_env.Install("%s/%s/maya/scripts" % (ver_dir, plat), excons.glob("openvdb_maya/maya/*.mel"))
