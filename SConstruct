import sys
import glob
import excons
import excons.tools.maya as maya
import excons.tools.houdini as houdini
import excons.tools.boost as boost
import excons.tools.tbb as tbb
import excons.tools.ilmbase as ilmbase
import excons.tools.openexr as openexr
import excons.tools.openexr as openexr
import excons.tools.python as python
import excons.tools.gl as gl

env = excons.MakeBaseEnv()

excons.SetArgument("use-c++11", 1)

abi3 = (excons.GetArgument("abi3", 0, int) != 0)

excons.Call("c-blosc", {"static": 1})
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

lib_requires = [ilmbase.Require(halfonly=True),
                boost.Require(libs=boost_libs),
                RequireBlosc,
                tbb.Require]

include_basedir = "%s/include/openvdb" % excons.OutputBaseDirectory()
InstallHeaders  = env.Install(include_basedir, glob.glob("openvdb/*.h"))
InstallHeaders += env.Install(include_basedir + "/io", glob.glob("openvdb/io/*.h"))
InstallHeaders += env.Install(include_basedir + "/math", glob.glob("openvdb/math/*.h"))
InstallHeaders += env.Install(include_basedir + "/points", glob.glob("openvdb/points/*.h"))
InstallHeaders += env.Install(include_basedir + "/util", glob.glob("openvdb/util/*.h"))

projs = [
   {  "name": "openvdb",
      "type": "sharedlib",
      "alias": "lib",
      "version": "4.0.1",
      "install_name": "libopenvdb.4.dylib",
      "soname": "libopenvdb.so.4",
      "incdirs": [".", "openvdb"],
      "defs": lib_defs + ["OPENVDB_DLL"],
      "srcs": glob.glob("openvdb/*.cc") +
              glob.glob("openvdb/io/*.cc") +
              glob.glob("openvdb/math/*.cc") +
              glob.glob("openvdb/points/*.cc") +
              glob.glob("openvdb/util/*.cc"),
      "deps": ["blosc"],
      "custom": lib_requires
   },
   {  "name": "openvdb_s",
      "type": "staticlib",
      "alias": "lib",
      "incdirs": [".", "openvdb"],
      "defs": lib_defs + ["OPENVDB_STATICLIB"],
      "srcs": glob.glob("openvdb/*.cc") +
              glob.glob("openvdb/io/*.cc") +
              glob.glob("openvdb/math/*.cc") +
              glob.glob("openvdb/points/*.cc") +
              glob.glob("openvdb/util/*.cc"),
      "deps": ["blosc"],
      "custom": lib_requires
   },
   {
      "name": "pyopenvdb",
      "type": "dynamicmodule",
      "alias": "python",
      "rpaths": ["../.."],
      "ext": python.ModuleExtension(),
      "prefix": "%s/%s" % (python.ModulePrefix(), python.Version()),
      "incdirs": [".", "openvdb"],
      "defs": defs + ["OPENVDB_STATICLIB"],
      "srcs": glob.glob("openvdb/python/*.cc"),
      "libs": ["openvdb_s"],
      "custom": [python.SoftRequire,
                 boost.Require(libs=boost_libs + ["python"]),
                 ilmbase.Require(halfonly=True),
                 RequireBlosc,
                 tbb.Require]
   },
   {
      "name": "OpenVDBMaya",
      "type": "dynamicmodule",
      "alias": "maya",
      "rpaths": "../../../lib",
      "bldprefix": maya.Version(),
      "prefix": "maya/%s/plug-ins" % maya.Version(nice=True),
      "defs": defs + ["OPENVDB_STATICLIB", "GL_GLEXT_PROTOTYPES=1"],
      "incdirs": [".", "openvdb"],
      "srcs": glob.glob("openvdb_maya/maya/*.cc"),
      "libs": ["openvdb_s"],
      "custom": [maya.Require,
                 boost.Require(libs=boost_libs),
                 ilmbase.Require(halfonly=True),
                 RequireBlosc,
                 tbb.Require,
                 gl.Require],
      "install": {"maya/scripts": glob.glob("openvdb_maya/maya/*.mel"),
                  "include/openvdb_maya": glob.glob("openvdb_maya/maya/*.h")}
   },
   {
      "name": "vdb_print",
      "type": "program",
      "alias": "bin",
      "incdirs": [".", "openvdb"],
      "defs": defs + ["OPENVDB_STATICLIB"],
      "srcs": glob.glob("openvdb/cmd/openvdb_print/*.cc"),
      "libs": ["openvdb_s"],
      "custom": lib_requires
   },
   {
      "name": "vdb_render",
      "type": "program",
      "alias": "bin",
      "incdirs": [".", "openvdb"],
      "defs": defs + ["OPENVDB_STATICLIB"],
      "srcs": glob.glob("openvdb/cmd/openvdb_render/*.cc"),
      "libs": ["openvdb_s"],
      "custom": [openexr.Require(ilmbase=False, zlib=False),
                 ilmbase.Require(ilmthread=True, iexmath=False, python=False),
                 boost.Require(libs=boost_libs),
                 RequireBlosc,
                 tbb.Require]
   },
   {
      "name": "vdb_view",
      "type": "program",
      "alias": "bin",
      "incdirs": [".", "openvdb"],
      "defs": defs + ["OPENVDB_STATICLIB", "OPENVDB_USE_GLFW_3", "GL_GLEXT_PROTOTYPES=1"],
      "srcs": glob.glob("openvdb/cmd/openvdb_view/*.cc") +
              glob.glob("openvdb/viewer/*.cc"),
      "libs": ["openvdb_s"],
      "custom": [RequireGLFW(static=True), boost.Require(libs=["thread"])] + lib_requires,
      "install": {"include/openvdb_viewer": glob.glob("openvdb/viewer/*.h")}
   }
]

targets = excons.DeclareTargets(env, projs)

env.Depends("lib", InstallHeaders)

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


Default(["lib"])
