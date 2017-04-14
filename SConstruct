import sys
import excons
import excons.tools.maya as maya
import excons.tools.houdini as houdini
import excons.tools.boost as boost
import excons.tools.tbb as tbb
import excons.tools.ilmbase as ilmbase
import excons.tools.openexr as openexr
import excons.tools.python as python
import excons.tools.gl as gl

env = excons.MakeBaseEnv()

if sys.platform == "win32":
  # Require mscver 14.0 at least
  mscver = float(excons.GetArgument("mscver", "14.0"))
  if mscver < 14.0:
    print("vc14.0 at least required on windows for C++11 proper support.")
    sys.exit(1)
  excons.SetArgument("mscver", str(mscver))
else:
  excons.SetArgument("use-c++11", 1)

abi3 = (excons.GetArgument("openvdb-abi3", 0, int) != 0)

excons.Call("c-blosc", imp=["RequireBlosc"])
excons.Call("glfw", imp=["RequireGLFW"])

cppflags = ""

defs = ([] if not abi3 else ["OPENVDB_3_ABI_COMPATIBLE"])
if sys.platform == "win32":
   defs.append("NOMINMAX")
   lib_defs = defs + ["HALF_EXPORTS"]
   # 4146: unary minus operator applied to unsigned type
   # 4800: forcing value to bool 'true' or 'false'
   cppflags += " /wd4800 /wd4146"
else:
   lib_defs = defs
lib_defs.extend(["OPENVDB_PRIVATE", "OPENVDB_USE_BLOSC"])

boost_libs = ["iostreams", "system"]

lib_srcs = excons.glob("openvdb/*.cc") + \
           excons.glob("openvdb/io/*.cc") + \
           excons.glob("openvdb/math/*.cc") + \
           excons.glob("openvdb/points/*.cc") + \
           excons.glob("openvdb/util/*.cc")

lib_requires = [ilmbase.Require(halfonly=True),
                boost.Require(libs=boost_libs),
                RequireBlosc(static=True),
                tbb.Require]

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
    "alias": "openvdb-lib",
    "version": "4.0.1",
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
    "alias": "openvdb-lib",
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
               ilmbase.Require(halfonly=True),
               RequireBlosc(static=True),
               tbb.Require]
  },
  {
    "name": "OpenVDBMaya",
    "type": "dynamicmodule",
    "desc": "OpenVDB Maya plugins",
    "alias": "openvdb-maya",
    "symvis": "default",
    "rpaths": "../../../lib",
    "bldprefix": maya.Version(),
    "prefix": "maya/%s/plug-ins" % maya.Version(nice=True),
    "defs": lib_defs + ["OPENVDB_STATICLIB", "GL_GLEXT_PROTOTYPES=1"],
    "cppflags": cppflags,
    "incdirs": [".", "openvdb"],
    "srcs": excons.glob("openvdb_maya/maya/*.cc"),
    "deps": ["blosc_s"],
    "staticlibs": ["openvdb_s"],
    "custom": [maya.Require,
               boost.Require(libs=boost_libs),
               ilmbase.Require(halfonly=True),
               RequireBlosc(static=True),
               tbb.Require,
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
    "custom": [openexr.Require(ilmbase=False, zlib=False),
               ilmbase.Require(ilmthread=True, iexmath=True, python=False),
               boost.Require(libs=boost_libs),
               RequireBlosc(static=True),
               tbb.Require]
  },
  {
    "name": "vdb_view",
    "type": "program",
    "desc": "OpenVDB command line tool",
    "alias": "openvdb-tools",
    "symvis": "default",
    "incdirs": [".", "openvdb"],
    "defs": defs + ["OPENVDB_STATICLIB", "OPENVDB_USE_GLFW_3", "GL_GLEXT_PROTOTYPES=1"],
    "cppflags": cppflags,
    "srcs": excons.glob("openvdb/cmd/openvdb_view/*.cc") +
            excons.glob("openvdb/viewer/*.cc"),
    "staticlibs": ["openvdb_s"],
    "custom": [RequireGLFW(static=True), boost.Require(libs=["thread"])] + lib_requires,
    "install": {"include/openvdb_viewer": excons.glob("openvdb/viewer/*.h")}
  }
]

build_opts = """OPENVDB OPTIONS
   openvdb-abi3=0|1 : Compile with OpenVDB 3 ABI compatibility"""
   
excons.AddHelpOptions(openvdb=build_opts)
excons.AddHelpTargets(eco="Ecosystem distribution")
excons.AddHelpOptions(eco="ECO OPTIONS\n  eco-dir=<path> : Ecosystem distribution install directory")

targets = excons.DeclareTargets(env, projs)

env.Depends(targets["openvdb"], InstallHeaders)
env.Depends(targets["openvdb_s"], InstallHeaders)

if "eco" in COMMAND_LINE_TARGETS:
  plat = excons.EcosystemPlatform()
  dist_env, ver_dir = excons.EcosystemDist(env, "openvdb.env",
                                           {"bin": "/%s/bin" % plat,
                                            "lib": "/%s/lib" % plat,
                                            "python": "/%s/lib/python/%s" % (plat, python.Version()),
                                            "maya": "/%s/maya/%s/plug-ins" % (plat, maya.Version(nice=True))})
  eco_incbase = "%s/%s/include/openvdb" % (ver_dir, plat)
  dist_env.Install(eco_incbase, excons.glob("openvdb/*.h"))
  dist_env.Install(eco_incbase + "/io", excons.glob("openvdb/io/*.h"))
  dist_env.Install(eco_incbase + "/math", excons.glob("openvdb/math/*.h"))
  dist_env.Install(eco_incbase + "/points", excons.glob("openvdb/points/*.h"))
  dist_env.Install(eco_incbase + "/util", excons.glob("openvdb/util/*.h"))
  dist_env.Install("%s/%s/maya/scripts" % (ver_dir, plat), excons.glob("openvdb_maya/maya/*.mel"))
