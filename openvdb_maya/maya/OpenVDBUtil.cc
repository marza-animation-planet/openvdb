///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2012-2017 DreamWorks Animation LLC
//
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
//
// Redistributions of source code must retain the above copyright
// and license notice and the following restrictions and disclaimer.
//
// *     Neither the name of DreamWorks Animation nor the names of
// its contributors may be used to endorse or promote products derived
// from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// IN NO EVENT SHALL THE COPYRIGHT HOLDERS' AND CONTRIBUTORS' AGGREGATE
// LIABILITY FOR ALL CLAIMS REGARDLESS OF THEIR BASIS EXCEED US$250.00.
//
///////////////////////////////////////////////////////////////////////////

/// @file OpenVDBUtil.cc
///
/// @author FX R&D OpenVDB team

#include "OpenVDBUtil.h"

#include <openvdb/math/Math.h>

#include <maya/MGlobal.h>

#include <iomanip> // std::setw, std::setfill, std::left
#include <sstream> // std::stringstream
#include <string> // std::string, std::getline
#include <vector>

namespace openvdb_maya {


////////////////////////////////////////


const OpenVDBData*
getInputVDB(const MObject& vdb, MDataBlock& data)
{
    MStatus status;
    MDataHandle inputVdbHandle = data.inputValue(vdb, &status);

    if (status != MS::kSuccess) {
        MGlobal::displayError("Invalid VDB input");
        return NULL;
    }

    MFnPluginData inputPluginData(inputVdbHandle.data());
    const MPxData * inputPxData = inputPluginData.data();

    if (!inputPxData) {
        MGlobal::displayError("Invalid VDB input");
        return NULL;
    }

    return dynamic_cast<const OpenVDBData*>(inputPxData);
}

void getGrids(std::vector<openvdb::GridBase::ConstPtr>& grids,
    const OpenVDBData& vdb, const std::string& names)
{
    grids.clear();

    if (names.empty() || names == "*") {
        for (size_t n = 0, N = vdb.numberOfGrids(); n < N; ++n) {
            grids.push_back(vdb.gridPtr(n));
        }
    } else {
        for (size_t n = 0, N = vdb.numberOfGrids(); n < N; ++n) {
            if (vdb.grid(n).getName() == names) grids.push_back(vdb.gridPtr(n));
        }
    }
}


std::string getGridNames(const OpenVDBData& vdb)
{
    std::vector<std::string> names;
    for (size_t n = 0, N = vdb.numberOfGrids(); n < N; ++n) {
        names.push_back(vdb.grid(n).getName());
    }

    return boost::algorithm::join(names, " ");
}


bool containsGrid(const std::vector<std::string>& selectionList,
    const std::string& gridName, size_t gridIndex)
{
    for (size_t n = 0, N = selectionList.size(); n < N; ++n) {

        const std::string& word = selectionList[n];

        try {

            return boost::lexical_cast<size_t>(word) == gridIndex;

        } catch (const boost::bad_lexical_cast&) {

            bool match = true;
            for (size_t i = 0, I = std::min(word.length(), gridName.length()); i < I; ++i) {

                if (word[i] == '*') {
                    return true;
                } else if (word[i] != gridName[i]) {
                    match = false;
                    break;
                }
            }

            if (match && (word.length() == gridName.length())) return true;
        }
    }

    return selectionList.empty();
}


bool
getSelectedGrids(GridCPtrVec& grids, const std::string& selection,
    const OpenVDBData& inputVdb, OpenVDBData& outputVdb)
{
    grids.clear();

    std::vector<std::string> selectionList;
    boost::split(selectionList, selection, boost::is_any_of(" "));

    for (size_t n = 0, N = inputVdb.numberOfGrids(); n < N; ++n) {

        GridCRef grid = inputVdb.grid(n);

        if (containsGrid(selectionList, grid.getName(), n)) {
            grids.push_back(inputVdb.gridPtr(n));
        } else {
            outputVdb.insert(grid);
        }
    }

    return !grids.empty();
}


bool
getSelectedGrids(GridCPtrVec& grids, const std::string& selection,
    const OpenVDBData& inputVdb)
{
    grids.clear();

    std::vector<std::string> selectionList;
    boost::split(selectionList, selection, boost::is_any_of(" "));

    for (size_t n = 0, N = inputVdb.numberOfGrids(); n < N; ++n) {

        GridCRef grid = inputVdb.grid(n);

        if (containsGrid(selectionList, grid.getName(), n)) {
            grids.push_back(inputVdb.gridPtr(n));
        }
    }

    return !grids.empty();
}


////////////////////////////////////////


void
printGridInfo(std::ostream& os, const OpenVDBData& vdb)
{
    os << "\nOutput " << vdb.numberOfGrids() << " VDB(s)\n";
    openvdb::GridPtrVec::const_iterator it;

    size_t memUsage = 0, idx = 0;
    for (size_t n = 0, N = vdb.numberOfGrids(); n < N; ++n) {

        const openvdb::GridBase& grid = vdb.grid(n);

        memUsage += grid.memUsage();
        openvdb::Coord dim = grid.evalActiveVoxelDim();

        os << "[" << idx++ << "]";

        if (!grid.getName().empty()) os << " '" << grid.getName() << "'";

        os << " voxel size: " << grid.voxelSize()[0] << ", type: "
            << grid.valueType() << ", dim: "
            << dim[0] << "x" << dim[1] << "x" << dim[2] <<"\n";
    }

    openvdb::util::printBytes(os, memUsage, "\nApproximate Memory Usage:");
}


void
updateNodeInfo(std::stringstream& stream, MDataBlock& data, MObject& strAttr)
{
    MString str = stream.str().c_str();
    MDataHandle strHandle = data.outputValue(strAttr);
    strHandle.set(str);
    data.setClean(strAttr);
}


void
insertFrameNumber(std::string& str, const MTime& time, int numberingScheme)
{
    size_t pos = str.find_first_of("#");
    if (pos != std::string::npos) {

        size_t length = str.find_last_of("#") + 1 - pos;

        // Current frame value
        const double frame = time.as(MTime::uiUnit());

        // Frames per second
        const MTime dummy(1.0, MTime::kSeconds);
        const double fps = dummy.as(MTime::uiUnit());

        // Ticks per frame
        const double tpf = 6000.0 / fps;
        const int tpfDigits = int(std::log10(int(tpf)) + 1);

        const int wholeFrame = int(frame);
        std::stringstream ss;
        ss << std::setw(int(length)) << std::setfill('0');

        if (numberingScheme == 1) { // Fractional frame values
            ss << wholeFrame;

            std::stringstream stream;
            stream << frame;
            std::string tmpStr = stream.str();;
            tmpStr = tmpStr.substr(tmpStr.find('.'));
            if (!tmpStr.empty()) ss << tmpStr;

        } else if (numberingScheme == 2) { // Global ticks
            int ticks = int(openvdb::math::Round(frame * tpf));
            ss << ticks;
        } else { // Frame.SubTick
            ss << wholeFrame;
            const int frameTick = static_cast<int>(
                openvdb::math::Round(frame - double(wholeFrame)) * tpf);
            if (frameTick > 0) {
                ss << "." << std::setw(tpfDigits) << std::setfill('0') << frameTick;
            }
        }

        str.replace(pos, length, ss.str());
    }
}


////////////////////////////////////////


BufferObject::BufferObject():
    mVertexBuffer(0),
    mNormalBuffer(0),
    mIndexBuffer(0),
    mColorBuffer(0),
    mPrimType(GL_POINTS),
    mPrimNum(0),
    mError("")
{
}

BufferObject::~BufferObject() { clear(); }

bool BufferObject::isValid() const {
    if (mPrimNum == 0 || mIndexBuffer == 0 || mVertexBuffer == 0 || !glIsBuffer(mIndexBuffer) || !glIsBuffer(mVertexBuffer)) {
        return false;
    } else {
        return true;
    }
}

void
BufferObject::render() const
{
    if (!isValid()) {
        // "request to render empty or uninitialized buffer"
        OPENVDB_LOG_DEBUG_RUNTIME(mError.c_str());
        return;
    }

    const bool usesColorBuffer = glIsBuffer(mColorBuffer);
    const bool usesNormalBuffer = glIsBuffer(mNormalBuffer);

    glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, 0);

    if (usesColorBuffer) {
        glBindBuffer(GL_ARRAY_BUFFER, mColorBuffer);
        glEnableClientState(GL_COLOR_ARRAY);
        glColorPointer(3, GL_FLOAT, 0, 0);
    }

    if (usesNormalBuffer) {
        glEnableClientState(GL_NORMAL_ARRAY);
        glBindBuffer(GL_ARRAY_BUFFER, mNormalBuffer);
        glNormalPointer(GL_FLOAT, 0, 0);
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);
    glDrawElements(mPrimType, mPrimNum, GL_UNSIGNED_INT, 0);

    // disable client-side capabilities
    if (usesColorBuffer) {
        glDisableClientState(GL_COLOR_ARRAY);
    }
    if (usesNormalBuffer) {
        glDisableClientState(GL_NORMAL_ARRAY);
    }

    // release vbo's
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void
BufferObject::genIndexBuffer(const std::vector<GLuint>& v, GLenum primType)
{
    // clear old buffer
    if (mIndexBuffer != 0 && glIsBuffer(mIndexBuffer) == GL_TRUE) {
        glDeleteBuffers(1, &mIndexBuffer);
    }

    // gen new buffer
    glGenBuffers(1, &mIndexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);
    if (glIsBuffer(mIndexBuffer) == GL_FALSE) {
        if (mError.length() > 0) {
            mError += "\n";
        }
        mError += "Unable to create index buffer";
        mIndexBuffer = 0;
        return;
    }

    // upload data
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * v.size(), &v[0], GL_STATIC_DRAW); // upload data
    if (GL_NO_ERROR != glGetError()) {
        if (mError.length() > 0) {
            mError += "\n";
        }
        mError += "Unable to upload index buffer data";
        glDeleteBuffers(1, &mIndexBuffer);
        mIndexBuffer = 0;
        return;
    }

    // release buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    mPrimNum = static_cast<GLsizei>(v.size());
    mPrimType = primType;
}

void
BufferObject::genVertexBuffer(const std::vector<GLfloat>& v)
{
    if (mVertexBuffer != 0 && glIsBuffer(mVertexBuffer) == GL_TRUE) {
        glDeleteBuffers(1, &mVertexBuffer);
    }

    glGenBuffers(1, &mVertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
    if (glIsBuffer(mVertexBuffer) == GL_FALSE) {
        if (mError.length() > 0) {
            mError += "\n";
        }
        mError += "Unable to create vertex buffer";
        mVertexBuffer = 0;
        return;
    }

    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * v.size(), &v[0], GL_STATIC_DRAW);
    if (GL_NO_ERROR != glGetError()) {
        if (mError.length() > 0) {
            mError += "\n";
        }
        mError += "Unable to upload vertex buffer data";
        glDeleteBuffers(1, &mVertexBuffer);
        mVertexBuffer = 0;
        return;
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void
BufferObject::genNormalBuffer(const std::vector<GLfloat>& v)
{
    if (mNormalBuffer != 0 && glIsBuffer(mNormalBuffer) == GL_TRUE) {
        glDeleteBuffers(1, &mNormalBuffer);
    }

    glGenBuffers(1, &mNormalBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, mNormalBuffer);
    if (glIsBuffer(mNormalBuffer) == GL_FALSE) {
        if (mError.length() > 0) {
            mError += "\n";
        }
        mError += "Unable to create normal buffer";
        mNormalBuffer = 0;
        return;
    }

    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * v.size(), &v[0], GL_STATIC_DRAW);
    if (GL_NO_ERROR != glGetError()) {
        if (mError.length() > 0) {
            mError += "\n";
        }
        mError += "Unable to upload normal buffer data";
        glDeleteBuffers(1, &mNormalBuffer);
        mNormalBuffer = 0;
        return;
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void
BufferObject::genColorBuffer(const std::vector<GLfloat>& v)
{
    if (mColorBuffer != 0 && glIsBuffer(mColorBuffer) == GL_TRUE) {
        glDeleteBuffers(1, &mColorBuffer);
    }

    glGenBuffers(1, &mColorBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, mColorBuffer);
    if (glIsBuffer(mColorBuffer) == GL_FALSE) {
        if (mError.length() > 0) {
            mError += "\n";
        }
        mError += "Unable to create color buffer";
        mColorBuffer = 0;
        return;
    }

    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * v.size(), &v[0], GL_STATIC_DRAW);
    if (GL_NO_ERROR != glGetError()) {
        if (mError.length() > 0) {
            mError += "\n";
        }
        mError += "Unable to upload color buffer data";
        glDeleteBuffers(1, &mColorBuffer);
        mColorBuffer = 0;
        return;
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void
BufferObject::clear()
{
    if (mIndexBuffer != 0) {
        if (glIsBuffer(mIndexBuffer) == GL_TRUE) {
            glDeleteBuffers(1, &mIndexBuffer);
        }
        mIndexBuffer = 0;
    }
    if (mVertexBuffer != 0) {
        if (glIsBuffer(mVertexBuffer) == GL_TRUE) {
            glDeleteBuffers(1, &mVertexBuffer);
        }
        mVertexBuffer = 0;
    }
    if (mColorBuffer != 0) {
        if (glIsBuffer(mColorBuffer) == GL_TRUE) {
            glDeleteBuffers(1, &mColorBuffer);
        }
        mColorBuffer = 0;
    }
    if (mNormalBuffer != 0) {
        if (glIsBuffer(mNormalBuffer) == GL_TRUE) {
            glDeleteBuffers(1, &mNormalBuffer);
        }
        mNormalBuffer = 0;
    }

    mPrimType = GL_POINTS;
    mPrimNum = 0;
    mError = "";
}

////////////////////////////////////////

ShaderProgram::ShaderProgram():
    mProgram(0),
    mVertShader(0),
    mFragShader(0),
    mError("")
{
}

ShaderProgram::~ShaderProgram() { clear(); }

bool ShaderProgram::isValid() const {
    if (mProgram == 0 || !glIsProgram(mProgram)) {
        return false;
    } else {
        return true;
    }
}

void
ShaderProgram::setVertShader(const std::string& s)
{
    if (mVertShader == 0 || !glIsShader(mVertShader)) {
        mVertShader = glCreateShader(GL_VERTEX_SHADER);
    }

    if (glIsShader(mVertShader) == GL_FALSE) {
        if (mError.length() > 0) {
            mError += "\n";
        }
        mError += "Unable to create vertex shader.";
        mVertShader = 0;
        return;
    }

    GLint length = static_cast<GLint>(s.length());
    const char *str = s.c_str();

    glShaderSource(mVertShader, 1, &str, &length);

    glCompileShader(mVertShader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(mVertShader, GL_COMPILE_STATUS, &compiled);

    if (compiled == GL_FALSE) {
        if (mError.length() > 0) {
            mError += "\n";
        }
        mError += "Unable to compile vertex shader.";

        // Get info log
        GLint logLength = 0;
        glGetShaderiv(mVertShader, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 0) {
            char *log = new char[logLength];
            glGetShaderInfoLog(mVertShader, logLength, NULL, log);
            mError += "\n";
            mError += log;
            delete[] log;
        }

        glDeleteShader(mVertShader);
        mVertShader = 0;
    }
}

void
ShaderProgram::setFragShader(const std::string& s)
{
    if (mFragShader == 0 || !glIsShader(mFragShader)) {
        mFragShader = glCreateShader(GL_FRAGMENT_SHADER);
    }

    if (glIsShader(mFragShader) == GL_FALSE) {
        if (mError.length() > 0) {
            mError += "\n";
        }
        mError += "Unable to create fragment shader.";
        mFragShader = 0;
        return;
    }

    GLint length = static_cast<GLint>(s.length());
    const char *str = s.c_str();

    glShaderSource(mFragShader, 1, &str, &length);

    glCompileShader(mFragShader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(mFragShader, GL_COMPILE_STATUS, &compiled);

    if (compiled == GL_FALSE) {
        if (mError.length() > 0) {
            mError += "\n";
        }
        mError += "Unable to compile fragment shader.";

        // Get info log
        GLint logLength = 0;
        glGetShaderiv(mFragShader, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 0) {
            char *log = new char[logLength];
            glGetShaderInfoLog(mFragShader, logLength, NULL, log);
            mError += "\n";
            mError += log;
            delete[] log;
        }

        glDeleteShader(mFragShader);
        mFragShader = 0;
    }
}

void
ShaderProgram::build()
{
    if (mProgram == 0 || !glIsProgram(mProgram)) {
        mProgram = glCreateProgram();
    } else {
        // Detach currently assigned vertex and fragment programs
        GLsizei numShaders = 0;
        GLuint shaders[2];
        glGetAttachedShaders(mProgram, 2, &numShaders, shaders);
        for (GLsizei n = 0; n < numShaders; ++n) {
            glDetachShader(mProgram, shaders[n]);
        }
    }

    if (glIsProgram(mProgram) == GL_FALSE) {
        if (mError.length() > 0) {
            mError += "\n";
        }
        mError += "Unable to create shader program.";
        mProgram = 0;
        return;
    }

    if (glIsShader(mVertShader) == GL_TRUE) {
        glAttachShader(mProgram, mVertShader);
        if (GL_NO_ERROR != glGetError()) {
            if (mError.length() > 0) {
                mError += "\n";
            }
            mError += "Unable to attach vertex shader.";
            glDeleteProgram(mProgram);
            mProgram = 0;
            return;
        }
    }

    if (glIsShader(mFragShader) == GL_TRUE) {
        glAttachShader(mProgram, mFragShader);
        if (GL_NO_ERROR != glGetError()) {
            if (mError.length() > 0) {
                mError += "\n";
            }
            mError += "Unable to attach fragment shader.";
            if (glIsShader(mVertShader)) {
                glDetachShader(mProgram, mVertShader);
            }
            glDeleteProgram(mProgram);
            mProgram = 0;
            return;
        }
    }

    glLinkProgram(mProgram);

    GLint linked = GL_FALSE;
    glGetProgramiv(mProgram, GL_LINK_STATUS, &linked);

    if (linked == GL_FALSE) {
        if (mError.length() > 0) {
            mError += "\n";
        }
        mError += "Unable to link shader program.";
        
        // Get info log
        GLint logLength = 0;
        glGetProgramiv(mProgram, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 0) {
            char *log = new char[logLength];
            glGetProgramInfoLog(mProgram, logLength, NULL, log);
            mError += "\n";
            mError += log;
            delete[] log;
        }

        if (glIsShader(mVertShader) == GL_TRUE) {
            glDetachShader(mProgram, mVertShader);
        }
        if (glIsShader(mFragShader) == GL_TRUE) {
            glDetachShader(mProgram, mFragShader);
        }
        glDeleteProgram(mProgram);
        mProgram = 0;
    }
}

void
ShaderProgram::build(const std::vector<GLchar*>& attributes)
{
    if (mProgram == 0 || !glIsProgram(mProgram)) {
        mProgram = glCreateProgram();
    } else {
        // Detach currently assigned vertex and fragment programs
        GLsizei numShaders = 0;
        GLuint shaders[2];
        glGetAttachedShaders(mProgram, 2, &numShaders, shaders);
        for (GLsizei n = 0; n < numShaders; ++n) {
            glDetachShader(mProgram, shaders[n]);
        }
    }

    if (glIsProgram(mProgram) == GL_FALSE) {
        if (mError.length() > 0) {
            mError += "\n";
        }
        mError += "Unable to create shader program.";
        mProgram = 0;
        return;
    }


    for (GLuint n = 0, N = static_cast<GLuint>(attributes.size()); n < N; ++n) {
        glBindAttribLocation(mProgram, n, attributes[n]);
        if (GL_NO_ERROR != glGetError()) {
            if (mError.length() > 0) {
                mError += "\n";
            }
            mError += "Unable to bind shader program attribute '";
            mError += attributes[n];
            mError += "'.";
            glDeleteProgram(mProgram);
            mProgram = 0;
            return;
        }
    }

    if (glIsShader(mVertShader) == GL_TRUE) {
        glAttachShader(mProgram, mVertShader);
        if (GL_NO_ERROR != glGetError()) {
            if (mError.length() > 0) {
                mError += "\n";
            }
            mError += "Unable to attach vertex shader.";
            glDeleteProgram(mProgram);
            mProgram = 0;
            return;
        }
    }

    if (glIsShader(mFragShader) == GL_TRUE) {
        glAttachShader(mProgram, mFragShader);
        if (GL_NO_ERROR != glGetError()) {
            if (mError.length() > 0) {
                mError += "\n";
            }
            mError += "Unable to attach fragment shader.";
            if (glIsShader(mVertShader)) {
                glDetachShader(mProgram, mVertShader);
            }
            glDeleteProgram(mProgram);
            mProgram = 0;
            return;
        }
    }

    glLinkProgram(mProgram);

    GLint linked = GL_FALSE;
    glGetProgramiv(mProgram, GL_LINK_STATUS, &linked);

    if (linked == GL_FALSE) {
        if (mError.length() > 0) {
            mError += "\n";
        }
        mError += "Unable to link shader program.";

        // Get info log
        GLint logLength = 0;
        glGetProgramiv(mProgram, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 0) {
            char *log = new char[logLength];
            glGetProgramInfoLog(mProgram, logLength, NULL, log);
            mError += "\n";
            mError += log;
            delete[] log;
        }

        if (glIsShader(mVertShader) == GL_TRUE) {
            glDetachShader(mProgram, mVertShader);
        }
        if (glIsShader(mFragShader) == GL_TRUE) {
            glDetachShader(mProgram, mFragShader);
        }
        glDeleteProgram(mProgram);
        mProgram = 0;
    }
}

void
ShaderProgram::startShading() const
{
    if (isValid()) {
        glUseProgram(mProgram);
    }
}

void
ShaderProgram::stopShading() const
{
    if (isValid()) {
        glUseProgram(0);
    }
}

void
ShaderProgram::clear()
{
    if (mProgram != 0) {
        if (glIsProgram(mProgram) == GL_TRUE) {
            GLsizei numShaders = 0;
            GLuint shaders[2];
            glGetAttachedShaders(mProgram, 2, &numShaders, shaders);
            for (GLsizei n = 0; n < numShaders; ++n) {
                glDetachShader(mProgram, shaders[n]);
            }
            glDeleteProgram(mProgram);
        }
        mProgram = 0;
    }

    if (mVertShader != 0) {
        if (glIsShader(mVertShader) == GL_TRUE) {
            glDeleteShader(mVertShader);
        }
        mVertShader = 0;
    }

    if (mFragShader != 0) {
        if (glIsShader(mFragShader) == GL_TRUE) {
            glDeleteShader(mFragShader);
        }
        mFragShader = 0;
    }

    mError = "";
}


////////////////////////////////////////

WireBoxBuilder::WireBoxBuilder(
    const openvdb::math::Transform& xform,
    std::vector<GLuint>& indices,
    std::vector<GLfloat>& points,
    std::vector<GLfloat>& colors)
    : mXForm(&xform)
    , mIndices(&indices)
    , mPoints(&points)
    , mColors(&colors)
{
}

void WireBoxBuilder::add(GLuint boxIndex, const openvdb::CoordBBox& bbox, const openvdb::Vec3s& color)
{
    GLuint ptnCount = boxIndex * 8;

    // Generate corner points

    GLuint ptnOffset = ptnCount * 3;
    GLuint colorOffset = ptnOffset;

    // Nodes are rendered as cell-centered
    const openvdb::Vec3d min(bbox.min().x()-0.5, bbox.min().y()-0.5, bbox.min().z()-0.5);
    const openvdb::Vec3d max(bbox.max().x()+0.5, bbox.max().y()+0.5, bbox.max().z()+0.5);

    // corner 1
    openvdb::Vec3d ptn = mXForm->indexToWorld(min);
    (*mPoints)[ptnOffset++] = static_cast<GLfloat>(ptn[0]);
    (*mPoints)[ptnOffset++] = static_cast<GLfloat>(ptn[1]);
    (*mPoints)[ptnOffset++] = static_cast<GLfloat>(ptn[2]);

    // corner 2
    ptn.x() = min.x();
    ptn.y() = min.y();
    ptn.z() = max.z();
    ptn = mXForm->indexToWorld(ptn);
    (*mPoints)[ptnOffset++] = static_cast<GLfloat>(ptn[0]);
    (*mPoints)[ptnOffset++] = static_cast<GLfloat>(ptn[1]);
    (*mPoints)[ptnOffset++] = static_cast<GLfloat>(ptn[2]);

    // corner 3
    ptn.x() = max.x();
    ptn.y() = min.y();
    ptn.z() = max.z();
    ptn = mXForm->indexToWorld(ptn);
    (*mPoints)[ptnOffset++] = static_cast<GLfloat>(ptn[0]);
    (*mPoints)[ptnOffset++] = static_cast<GLfloat>(ptn[1]);
    (*mPoints)[ptnOffset++] = static_cast<GLfloat>(ptn[2]);

    // corner 4
    ptn.x() = max.x();
    ptn.y() = min.y();
    ptn.z() = min.z();
    ptn = mXForm->indexToWorld(ptn);
    (*mPoints)[ptnOffset++] = static_cast<GLfloat>(ptn[0]);
    (*mPoints)[ptnOffset++] = static_cast<GLfloat>(ptn[1]);
    (*mPoints)[ptnOffset++] = static_cast<GLfloat>(ptn[2]);

    // corner 5
    ptn.x() = min.x();
    ptn.y() = max.y();
    ptn.z() = min.z();
    ptn = mXForm->indexToWorld(ptn);
    (*mPoints)[ptnOffset++] = static_cast<GLfloat>(ptn[0]);
    (*mPoints)[ptnOffset++] = static_cast<GLfloat>(ptn[1]);
    (*mPoints)[ptnOffset++] = static_cast<GLfloat>(ptn[2]);

    // corner 6
    ptn.x() = min.x();
    ptn.y() = max.y();
    ptn.z() = max.z();
    ptn = mXForm->indexToWorld(ptn);
    (*mPoints)[ptnOffset++] = static_cast<GLfloat>(ptn[0]);
    (*mPoints)[ptnOffset++] = static_cast<GLfloat>(ptn[1]);
    (*mPoints)[ptnOffset++] = static_cast<GLfloat>(ptn[2]);

    // corner 7
    ptn = mXForm->indexToWorld(max);
    (*mPoints)[ptnOffset++] = static_cast<GLfloat>(ptn[0]);
    (*mPoints)[ptnOffset++] = static_cast<GLfloat>(ptn[1]);
    (*mPoints)[ptnOffset++] = static_cast<GLfloat>(ptn[2]);

    // corner 8
    ptn.x() = max.x();
    ptn.y() = max.y();
    ptn.z() = min.z();
    ptn = mXForm->indexToWorld(ptn);
    (*mPoints)[ptnOffset++] = static_cast<GLfloat>(ptn[0]);
    (*mPoints)[ptnOffset++] = static_cast<GLfloat>(ptn[1]);
    (*mPoints)[ptnOffset] = static_cast<GLfloat>(ptn[2]);

    for (int n = 0; n < 8; ++n) {
        (*mColors)[colorOffset++] = color[0];
        (*mColors)[colorOffset++] = color[1];
        (*mColors)[colorOffset++] = color[2];
    }

    // Generate edges

    GLuint edgeOffset = boxIndex * 24;

    // edge 1
    (*mIndices)[edgeOffset++] = ptnCount;
    (*mIndices)[edgeOffset++] = ptnCount + 1;
    // edge 2
    (*mIndices)[edgeOffset++] = ptnCount + 1;
    (*mIndices)[edgeOffset++] = ptnCount + 2;
    // edge 3
    (*mIndices)[edgeOffset++] = ptnCount + 2;
    (*mIndices)[edgeOffset++] = ptnCount + 3;
    // edge 4
    (*mIndices)[edgeOffset++] = ptnCount + 3;
    (*mIndices)[edgeOffset++] = ptnCount;
    // edge 5
    (*mIndices)[edgeOffset++] = ptnCount + 4;
    (*mIndices)[edgeOffset++] = ptnCount + 5;
    // edge 6
    (*mIndices)[edgeOffset++] = ptnCount + 5;
    (*mIndices)[edgeOffset++] = ptnCount + 6;
    // edge 7
    (*mIndices)[edgeOffset++] = ptnCount + 6;
    (*mIndices)[edgeOffset++] = ptnCount + 7;
    // edge 8
    (*mIndices)[edgeOffset++] = ptnCount + 7;
    (*mIndices)[edgeOffset++] = ptnCount + 4;
    // edge 9
    (*mIndices)[edgeOffset++] = ptnCount;
    (*mIndices)[edgeOffset++] = ptnCount + 4;
    // edge 10
    (*mIndices)[edgeOffset++] = ptnCount + 1;
    (*mIndices)[edgeOffset++] = ptnCount + 5;
    // edge 11
    (*mIndices)[edgeOffset++] = ptnCount + 2;
    (*mIndices)[edgeOffset++] = ptnCount + 6;
    // edge 12
    (*mIndices)[edgeOffset++] = ptnCount + 3;
    (*mIndices)[edgeOffset]   = ptnCount + 7;
}


} // namespace util


// Copyright (c) 2012-2017 DreamWorks Animation LLC
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
