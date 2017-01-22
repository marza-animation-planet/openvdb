import sys
import glob
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

abi3 = (excons.GetArgument("abi3", 0, int) != 0)

excons.Call("c-blosc")
Import("RequireBlosc")

excons.Call("glfw")
Import("RequireGLFW")

defs = ([] if not abi3 else ["OPENVDB_3_ABI_COMPATIBLE"])
if sys.platform == "win32":
   defs.append("NOMINMAX")
   lib_defs = defs + ["HALF_EXPORTS"]
else:
   lib_defs = defs
lib_defs.extend(["OPENVDB_PRIVATE", "OPENVDB_USE_BLOSC"])

boost_libs = ["iostreams", "system"]

lib_srcs = glob.glob("openvdb/*.cc") + \
           glob.glob("openvdb/io/*.cc") + \
           glob.glob("openvdb/math/*.cc") + \
           glob.glob("openvdb/points/*.cc") + \
           glob.glob("openvdb/util/*.cc")

lib_requires = [ilmbase.Require(halfonly=True),
                boost.Require(libs=boost_libs),
                RequireBlosc(static=True),
                tbb.Require]

include_basedir = "%s/include/openvdb" % excons.OutputBaseDirectory()
InstallHeaders  = env.Install(include_basedir, glob.glob("openvdb/*.h"))
InstallHeaders += env.Install(include_basedir + "/io", glob.glob("openvdb/io/*.h"))
InstallHeaders += env.Install(include_basedir + "/math", glob.glob("openvdb/math/*.h"))
InstallHeaders += env.Install(include_basedir + "/points", glob.glob("openvdb/points/*.h"))
InstallHeaders += env.Install(include_basedir + "/util", glob.glob("openvdb/util/*.h"))

projs = [
  {
    "name": "openvdb",
    "type": "sharedlib",
    "desc": "OpenVDB shared library",
    "alias": "lib",
    "version": "4.0.1",
    "install_name": "libopenvdb.4.dylib",
    "soname": "libopenvdb.so.4",
    "incdirs": [".", "openvdb"],
    "defs": lib_defs + ["OPENVDB_DLL"],
    "srcs": lib_srcs,
    "deps": ["blosc_s"],
    "custom": lib_requires
  },
  {
    "name": "openvdb_s",
    "type": "staticlib",
    "desc": "OpenVDB static library",
    "alias": "lib",
    "symvis": "default",
    "incdirs": [".", "openvdb"],
    "defs": lib_defs + ["OPENVDB_STATICLIB"],
    "srcs": lib_srcs,
    "deps": ["blosc_s"],
    "custom": lib_requires
  },
  {
    "name": "pyopenvdb",
    "type": "dynamicmodule",
    "desc": "OpenVDB python module",
    "alias": "python",
    "symvis": "default",
    "rpaths": ["../.."],
    "ext": python.ModuleExtension(),
    "prefix": "%s/%s" % (python.ModulePrefix(), python.Version()),
    "incdirs": [".", "openvdb"],
    "defs": defs + ["OPENVDB_STATICLIB"],
    "srcs": glob.glob("openvdb/python/*.cc"),
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
    "alias": "maya",
    "symvis": "default",
    "rpaths": "../../../lib",
    "bldprefix": maya.Version(),
    "prefix": "maya/%s/plug-ins" % maya.Version(nice=True),
    "defs": lib_defs + ["OPENVDB_STATICLIB", "GL_GLEXT_PROTOTYPES=1"],
    "incdirs": [".", "openvdb"],
    "srcs": glob.glob("openvdb_maya/maya/*.cc"),
    "deps": ["blosc_s"],
    "staticlibs": ["openvdb_s"],
    "custom": [maya.Require,
               boost.Require(libs=boost_libs),
               ilmbase.Require(halfonly=True),
               RequireBlosc(static=True),
               tbb.Require,
               gl.Require],
    "install": {"maya/scripts": glob.glob("openvdb_maya/maya/*.mel"),
                "include/openvdb_maya": ["openvdb_maya/maya/OpenVDBData.h",
                                         "openvdb_maya/maya/OpenVDBUtil.h"]}
  },
  {
    "name": "vdb_print",
    "type": "program",
    "desc": "OpenVDB command line tool",
    "alias": "bins",
    "symvis": "default",
    "incdirs": [".", "openvdb"],
    "defs": defs + ["OPENVDB_STATICLIB"],
    "srcs": glob.glob("openvdb/cmd/openvdb_print/*.cc"),
    "staticlibs": ["openvdb_s"],
    "custom": lib_requires
  },
  {
    "name": "vdb_render",
    "type": "program",
    "desc": "OpenVDB command line tool",
    "alias": "bins",
    "symvis": "default",
    "incdirs": [".", "openvdb"],
    "defs": defs + ["OPENVDB_STATICLIB"],
    "srcs": glob.glob("openvdb/cmd/openvdb_render/*.cc"),
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
    "alias": "bins",
    "symvis": "default",
    "incdirs": [".", "openvdb"],
    "defs": defs + ["OPENVDB_STATICLIB", "OPENVDB_USE_GLFW_3", "GL_GLEXT_PROTOTYPES=1"],
    "srcs": glob.glob("openvdb/cmd/openvdb_view/*.cc") +
            glob.glob("openvdb/viewer/*.cc"),
    "staticlibs": ["openvdb_s"],
    "custom": [RequireGLFW(static=True), boost.Require(libs=["thread"])] + lib_requires,
    "install": {"include/openvdb_viewer": glob.glob("openvdb/viewer/*.h")}
  }
]

build_opts = """OPENVDB OPTIONS
   abi3=0|1 : Compile with OpenVDB 3 ABI compatibility"""
   
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
  dist_env.Install(eco_incbase, glob.glob("openvdb/*.h"))
  dist_env.Install(eco_incbase + "/io", glob.glob("openvdb/io/*.h"))
  dist_env.Install(eco_incbase + "/math", glob.glob("openvdb/math/*.h"))
  dist_env.Install(eco_incbase + "/points", glob.glob("openvdb/points/*.h"))
  dist_env.Install(eco_incbase + "/util", glob.glob("openvdb/util/*.h"))
  dist_env.Install("%s/%s/maya/scripts" % (ver_dir, plat), glob.glob("openvdb_maya/maya/*.mel"))

# excons.AddHelpTargets(("openvdb_s", "Static library"),
#                       ("openvdb", "Shared library"),
#                       ("pyopenvdb", "Python binding"),
#                       ("OpenVDBMaya", "Maya plugins"),
#                       ("vdb_print", "Command line tool"),
#                       ("vdb_render", "Command line tool"),
#                       ("vdb_view", "Command line tool"))

# excons.SetHelp("""USAGE
#   scons [OPTIONS] TARGET*

# AVAILABLE TARGETS
#   openvdb     : Shared library
#   openvdb_s   : Static library
#   pyopenvdb   : Python module
#   OpenVDBMaya : Maya plugins
#   vdb_print   : Command line tool
#   vdb_render  : Command line tool
#   vdb_view    : Command line tool

#   lib         : Static and shared libraries
#   python      : Same as 'pyopenvdb'
#   bins        : All command line tools
#   maya        : Same as 'OpenVDBMaya'
#   eco         : Ecosystem distribution

# OPENVDB OPTIONS
#   abi3=0|1    : Compile with OpenVDB 3 ABI compatibility

# %s
# %s
# %s
# %s
# %s
# %s""" % (python.GetOptionsString(),
#          ilmbase.GetOptionsString(),
#          openexr.GetOptionsString(),
#          boost.GetOptionsString(),
#          tbb.GetOptionsString(),
#          excons.GetOptionsString()))
