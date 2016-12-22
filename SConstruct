import sys
import glob
import excons
import excons.tools.maya as maya
import excons.tools.houdini as houdini
import excons.tools.boost as boost
import excons.tools.tbb as tbb
import excons.tools.zlib as zlib
import excons.tools.ilmbase as ilmbase
import excons.tools.openexr as openexr
import excons.tools.python as python
import excons.tools.gl as gl

env = excons.MakeBaseEnv()

excons.SetArgument("use-c++11", 1)

# Force Blosc static build
old_static = ARGUMENTS.get("static", "0")
ARGUMENTS["static"] = "1"
SConscript("c-blosc/SConstruct")
ARGUMENTS["static"] = old_static
Import("RequireBlosc")


defs = []
if sys.platform == "win32":
   defs.append("NOMINMAX")
   lib_defs = defs + ["HALF_EXPORTS"]
else:
   lib_defs = defs
lib_defs.extend(["OPENVDB_PRIVATE", "OPENVDB_USE_BLOSC"])

boost_libs = ["iostreams", "system"] #, "thread"]

lib_requires = [ilmbase.Require(halfonly=True),
                boost.Require(libs=boost_libs),
                RequireBlosc,
                tbb.Require,
                zlib.Require]

install_files = {"include/openvdb": glob.glob("openvdb/*.h"),
                 "include/openvdb/io": glob.glob("openvdb/io/*.h"),
                 "include/openvdb/math": glob.glob("openvdb/math/*.h"),
                 "include/openvdb/points": glob.glob("openvdb/points/*.h"),
                 "include/openvdb/util": glob.glob("openvdb/util/*.h")}

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
      "custom": lib_requires,
      "install": install_files
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
      #"install": install_files
   },
   {
      "name": "pyopenvdb",
      "type": "dynamicmodule",
      "alias": "python",
      "rpaths": ["../.."],
      "ext": python.ModuleExtension(),
      "prefix": "%s/%s" % (python.ModulePrefix(), python.Version()),
      "incdirs": [".", "openvdb"],
      "defs": defs + ["OPENVDB_STATICLIB"], #["OPENVDB_DLL"],
      "srcs": glob.glob("openvdb/python/*.cc"),
      "libs": ["openvdb_s"], #["openvdb"],
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
   }
   # houdini
   # vdb_view
   # vdb_render
   # vdb_print
]

targets = excons.DeclareTargets(env, projs)

Default(["lib"])
