/*******************************************************************************

         Yuri V. Krugloff. 2013-2014. http://www.tver-soft.org

    This is free and unencumbered software released into the public domain.

    Anyone is free to copy, modify, publish, use, compile, sell, or
    distribute this software, either in source code form or as a compiled
    binary, for any purpose, commercial or non-commercial, and by any
    means.

    In jurisdictions that recognize copyright laws, the author or authors
    of this software dedicate any and all copyright interest in the
    software to the public domain. We make this dedication for the benefit
    of the public at large and to the detriment of our heirs and
    successors. We intend this dedication to be an overt act of
    relinquishment in perpetuity of all present and future rights to this
    software under copyright law.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
    OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
    ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
    OTHER DEALINGS IN THE SOFTWARE.

    For more information, please refer to <http://unlicense.org/>

*******************************************************************************/

#include "QMake.hpp"

#include <string.h>
#include <assert.h>

#include "Functions.hpp"
#include "Logger.hpp"
#include "Backup.hpp"

//------------------------------------------------------------------------------

using namespace std;
using namespace Functions;

//------------------------------------------------------------------------------

const string TQMake::m_QMakeName(
          #ifdef OS_WINDOWS
              "qmake.exe"
          #else
              "qmake"
          #endif
      );
const string TQMake::m_BinDirName("bin");

//------------------------------------------------------------------------------

bool TQMake::find(const string& qtDir)
{
    m_QMakePath = (qtDir.empty() ? currentDir() : qtDir) + separator();
    if (!isFileExists(m_QMakePath + m_QMakeName)) {
        m_QMakePath += m_BinDirName + separator();
        if (!isFileExists(m_QMakePath + m_QMakeName))
            m_QMakePath.clear();
    }

    if (m_QMakePath.empty())
        m_ErrorString += "Can't find qmake.\n";
    else
        LOG_V("Path to \"%s\": \"%s\".\n", m_QMakeName.c_str(), m_QMakePath.c_str());

    return !m_QMakePath.empty();
}

//------------------------------------------------------------------------------

bool TQMake::query()
{
    m_QMakeOutput.clear();
    if (!m_QMakePath.empty())
    {
        string QMakeStart = "\"\"" + m_QMakePath + m_QMakeName + "\" -query\"";
        LOG_V("qmake command line: %s.\n", QMakeStart.c_str());

        m_QMakeOutput = getProgramOutput(QMakeStart);
        LOG_V("\n"
              ">>>>>>>>>> BEGIN QMAKE OUTPUT >>>>>>>>>>\n"
              "%s"
              "<<<<<<<<<<  END QMAKE OUTPUT  <<<<<<<<<<\n",
              m_QMakeOutput.c_str());
    }
    return !m_QMakeOutput.empty();
}

//------------------------------------------------------------------------------

bool TQMake::parseValues()
{
    const char* const Delimiters = "\r\n";
    char* s = strtok(const_cast<char*>(m_QMakeOutput.c_str()), Delimiters);
    while (s != NULL) {
        const string str(s);
        const string::size_type i = str.find(':');
        if (i != string::npos) {
            m_QMakeValues[str.substr(0, i)] = str.substr(i + 1);
        }
        else {
            m_ErrorString += "Error parsing qmake output string:\n"
                             "\"" + str + "\"\n";
            return false;
        }
        s = strtok(NULL, Delimiters);
    }

    TStringMap::const_iterator Iter = m_QMakeValues.find("QT_VERSION");
    if (Iter != m_QMakeValues.end()) {
        const string& Version = Iter->second;
        if (!Version.empty())
            m_QtVersion = Version[0];
    }

    LOG_V("\nParsed qmake variables:\n");
    for (Iter = m_QMakeValues.begin(); Iter != m_QMakeValues.end(); ++Iter)
        LOG_V("  %s = \"%s\"\n", Iter->first.c_str(), Iter->second.c_str());

    return true;
}

//------------------------------------------------------------------------------

bool TQMake::addSuffix(const TStringMap::const_iterator& Iter, const string& prefix)
{
    assert(hasOnlyNormalSeparators(prefix));

    if (!Iter->second.empty())
    {
        string value = normalizeSeparators(Iter->second);
        if (startsWith(value, prefix)) {
            value = trimSeparators(value.substr(prefix.length()));
            if (!value.empty())
                m_Suffixes[Iter->first] = normalizeSeparators(value);
        }
        else {
            m_ErrorString += "QMake variable \"" + Iter->first +
                             "\" with value \"" + Iter->second +
                             "\" don't have prefix \"" + prefix + "\".\n";
            return false;
        }
    }
    return true;
}

//------------------------------------------------------------------------------

bool TQMake::parseSuffixes()
{
    string prefix = normalizeSeparators(qtInstallPrefix());
    string hostPrefix = normalizeSeparators(qtHostPrefix());
    for (TStringMap::iterator Iter = m_QMakeValues.begin(); Iter != m_QMakeValues.end(); ++Iter)
    {
        if (startsWith(Iter->first, "QT_INSTALL_")) {
            if (!addSuffix(Iter, prefix))
                return false;
        }
        else if (startsWith(Iter->first, "QT_HOST_")) {
            if (!addSuffix(Iter, hostPrefix))
                return false;
        }
    }

    LOG_V("\nParsed Qt subdirs:\n");
    for (TStringMap::iterator Iter = m_Suffixes.begin(); Iter != m_Suffixes.end(); ++Iter)
        LOG_V("  %s = \"%s\"\n", Iter->first.c_str(), Iter->second.c_str());

    return true;
}

//------------------------------------------------------------------------------

bool TQMake::parse()
{
    m_QMakeValues.clear();
    m_Suffixes.clear();
    m_QtVersion = '\0';

    if (m_QMakeOutput.empty())
        return false;

    return parseValues() && parseSuffixes();
}

//------------------------------------------------------------------------------

bool TQMake::getQtPath()
{
    const string& binSuffix = suffix("QT_INSTALL_BINS");

    assert(hasOnlyNormalSeparators(binSuffix));

    int levels = 1;
    if (!binSuffix.empty()) {
        ++levels;
        for (string::size_type i = 0; i < binSuffix.length(); ++i)
            if (binSuffix[i] == separator())
                ++levels;
    }

    m_QtPath = m_QMakePath;
    for (int i = 0; i < levels; ++i) {
        // One level up.
        string::size_type pos = m_QtPath.find_last_of(separator());
        if (pos != string::npos) {
            m_QtPath.resize(pos);
        }
        else {
            m_ErrorString += "Can't determine path to Qt directory.\n";
            return false;
        }
    }
    return true;
}

//------------------------------------------------------------------------------

TQMake::TQMake(const string& qtDir)
    : m_QtVersion('\0')
{
    if (find(qtDir)) {
        TBackup Backup;
        Backup.backupFile(m_QMakePath + "qt.conf", TBackup::bmRename);
        query() && parse() && getQtPath();
    }
}

//------------------------------------------------------------------------------

string TQMake::value(const string& variable) const
{
    TStringMap::const_iterator Iter = m_QMakeValues.find(variable);
    if (Iter != m_QMakeValues.end())
        return Iter->second;
    return string();
}

//------------------------------------------------------------------------------

string TQMake::suffix(const string& variable) const
{
    TStringMap::const_iterator Iter = m_Suffixes.find(variable);
    if (Iter != m_Suffixes.end())
        return Iter->second;
    return string();
}

//------------------------------------------------------------------------------
