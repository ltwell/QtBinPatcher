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

#ifndef __QTBINPATCHER2_BACKUP__
#define __QTBINPATCHER2_BACKUP__

//------------------------------------------------------------------------------

#include "CommonTypes.hpp"

//------------------------------------------------------------------------------

class TBackup
{
    private :
        struct TFileMapping {
            std::string fileName;
            std::string bakFileName;

            TFileMapping(std::string _fileName, std::string _bakFileName);
        };
        typedef std::list<TFileMapping> TFilesMapping;

        static const char* const bakFileSuffix;

        TFilesMapping m_FilesMapping;
        bool          m_SkipBackup;

        static std::string backupFileName(const std::string& fileName);

    public :
        enum TBackupMethod {
            bmCopy,
            bmRename
        };

        TBackup();
        ~TBackup();

        bool backupFile(const std::string& fileName, const TBackupMethod method = bmCopy);
        bool backupFiles(const TStringList& files, const TBackupMethod method = bmCopy);
        bool remove();
        bool restore();
        void save();
        void setSkipBackup(bool skipBackup);

        inline bool skipBackup() const { return m_SkipBackup; }
};

//------------------------------------------------------------------------------

#endif // __QTBINPATCHER2_BACKUP__
