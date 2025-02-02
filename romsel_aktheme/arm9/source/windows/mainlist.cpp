/*
    mainlist.cpp
    Copyright (C) 2007 Acekard, www.acekard.com
    Copyright (C) 2007-2009 somebody
    Copyright (C) 2009 yellow wood goblin

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

//�

#include <nds/arm9/dldi.h>
#include <sys/dirent.h>
#define ATTRIB_HID 0x02
#include "mainlist.h"
//#include "files.h"
#include "windows/startmenu.h"
#include "systemfilenames.h"
#include "common/systemdetails.h"
#include "common/flashcard.h"
#include "common/dsimenusettings.h"
#include "ui/windowmanager.h"
#include "tool/timetool.h"
#include "tool/memtool.h"
#include "tool/dbgtool.h"
#include "common/inifile.h"
#include "unknown_banner_bin.h"
#include "nds_save_banner_bin.h"
#include "nand_banner_bin.h"
#include "microsd_banner_bin.h"
#include "gba_banner_bin.h"
#include "gbarom_banner_bin.h"
#include "sysmenu_banner_bin.h"
#include "settings_banner_bin.h"
#include "manual_banner_bin.h"
#include "folder_banner_bin.h"
#include "ds2plg_banner_bin.h"
#include "nesrom_banner_bin.h"
#include "gbcrom_banner_bin.h"
#include "gbrom_banner_bin.h"
#include "s8ds_banner_bin.h"
#include "snemulds_banner_bin.h"
#include "lolsnes_banner_bin.h"
#include "smdrom_banner_bin.h"
#include "ui/progresswnd.h"
#include "language.h"
#include "unicode.h"

using namespace akui;

MainList::MainList(s32 x, s32 y, u32 w, u32 h, Window *parent, const std::string &text)
    : ListView(x, y, w, h, parent, text, ms().ak_scrollSpeed), _showAllFiles(false)
{
    _viewMode = VM_LIST;
    _activeIconScale = 1;
    _activeIcon.hide();
    _activeIcon.update();
    animationManager().addAnimation(&_activeIcon);
    seq();
    dbg_printf("_activeIcon.init\n");
}

MainList::~MainList()
{
}

int MainList::init()
{
    CIniFile ini(SFN_UI_SETTINGS);
    _textColor = ini.GetInt("main list", "textColor", RGB15(7, 7, 7));
    _textColorHilight = ini.GetInt("main list", "textColorHilight", RGB15(31, 0, 31));
    _selectionBarColor1 = ini.GetInt("main list", "selectionBarColor1", RGB15(16, 20, 24));
    _selectionBarColor2 = ini.GetInt("main list", "selectionBarColor2", RGB15(20, 25, 0));
    _selectionBarOpacity = ini.GetInt("main list", "selectionBarOpacity", 100);
    _showSelectionBarBg = ini.GetInt("main list", "showSelectionBarBg", false);

    insertColumn(ICON_COLUMN, "icon", 0);
    insertColumn(SHOWNAME_COLUMN, "showName", 0);
    insertColumn(INTERNALNAME_COLUMN, "internalName", 0);
    insertColumn(REALNAME_COLUMN, "realName", 0); // hidden column for contain real filename
    insertColumn(SAVETYPE_COLUMN, "saveType", 0);
    insertColumn(FILESIZE_COLUMN, "fileSize", 0);

    setViewMode((MainList::VIEW_MODE)ms().ak_viewMode);

    _activeIcon.hide();

    return 1;
}

static bool itemSortComp(const ListView::itemVector &item1, const ListView::itemVector &item2)
{
    const std::string &fn1 = item1[MainList::REALNAME_COLUMN].text();
    const std::string &fn2 = item2[MainList::REALNAME_COLUMN].text();
    if ("../" == fn1)
        return true;
    if ("../" == fn2)
        return false;
    if ('/' == fn1[fn1.size() - 1] && '/' == fn2[fn2.size() - 1])
        return fn1 < fn2;
    if ('/' == fn1[fn1.size() - 1])
        return true;
    if ('/' == fn2[fn2.size() - 1])
        return false;

    return fn1 < fn2;
}

static bool extnameFilter(const std::vector<std::string> &extNames, std::string extName)
{
    if (0 == extNames.size())
        return true;

    for (size_t i = 0; i < extName.size(); ++i)
        extName[i] = tolower(extName[i]);

    for (size_t i = 0; i < extNames.size(); ++i)
    {
        if (extName == extNames[i])
        {
            return true;
        }
    }
    return false;
}

void MainList::addDirEntry(int pos, const std::string row1, const std::string row2, const std::string path, const std::string &bannerKey, const u8 *banner)
{
    std::vector<std::string> a_row;
    a_row.push_back(""); // make a space for icon
    DSRomInfo rominfo;

    a_row.push_back(row1);
    a_row.push_back(row2);
    a_row.push_back(path);

    if (!bannerKey.empty())
    {
        rominfo.setBanner(bannerKey, banner);
    }

    insertRow(pos, a_row);
    _romInfoList.push_back(rominfo);
}

static int listNum = 0;

bool MainList::enterDir(const std::string &dirName)
{
    dbg_printf("Enter Dir: %s\n", dirName.c_str());
    if (SPATH_ROOT == dirName) // select RPG or SD card
    {
        listNum = 0;
        removeAllRows();
        _romInfoList.clear();

        if (sdFound())
        {
            addDirEntry(listNum, LANG("mainlist", ((ms().consoleModel < 3) ? "SD Card" : "microSD Card")), "", (ms().showDirectories ? SD_ROOT : ms().romfolder[0]), "usd", microsd_banner_bin);
            listNum++;
        }
        if (flashcardFound())
        {
            addDirEntry(listNum, LANG("mainlist", ((sys().isRegularDS()) ? "microSD Card" : "SLOT-1 microSD Card")), "", (ms().showDirectories ? S1SD_ROOT : ms().romfolder[1]), "usd", microsd_banner_bin);
            listNum++;
        }
        addDirEntry(listNum, "GBARunner2", "", SPATH_GBARUNNER, "gbarunner", gbarom_banner_bin);
        listNum++;
        if (!sys().isRegularDS())
        {
            if (!flashcardFound() && sdFound())
            {
                addDirEntry(listNum, "SLOT-1 Card", "", SPATH_SLOT1, "slot1", nand_banner_bin);
                listNum++;
            }
            addDirEntry(listNum, "System Menu", "", SPATH_SYSMENU, "sysmenu", sysmenu_banner_bin);
            listNum++;
            //addDirEntry(5, "System Settings", "", SPATH_SYSTEMSETTINGS, "systemsettings", settings_banner_bin);
        }
        addDirEntry(listNum, "Settings", "", SPATH_TITLEANDSETTINGS, "titleandsettings", settings_banner_bin);
        listNum++;
        addDirEntry(listNum, "Manual", "", SPATH_MANUAL, "manual", manual_banner_bin);
        //listNum++;
        _currentDir = SPATH_ROOT;
        directoryChanged();
        return true;
    }

    if (!strncmp(dirName.c_str(), "^*::", 4))
    {
        dbg_printf("Special directory entered");
        _currentDir = dirName;
        directoryChanged();
        return true;
    }

    DIR *dir = NULL;

    dir = opendir(dirName.c_str());

    if (dir == NULL)
    {
        if (SD_ROOT == dirName || S1SD_ROOT == dirName)
        {
            std::string title = LANG("sd card error", "title");
            std::string sdError = LANG("sd card error", "text");
            messageBox(NULL, title, sdError, MB_OK);
        }
        dbg_printf("Unable to open directory<%s>.\n", dirName.c_str());
        return false;
    }

    if (strncmp(dirName.c_str(), S1SD_ROOT, 5) == 0)
    {
        ms().secondaryDevice = true;
    }
    else
    {
        ms().secondaryDevice = false;
    }

    removeAllRows();
    _romInfoList.clear();

    std::vector<std::string> extNames;

    if (_showAllFiles || ms().showNds)
    {
        extNames.push_back(".nds");
        extNames.push_back(".ids");
        extNames.push_back(".dsi");
        extNames.push_back(".argv");
    }
	if (_showAllFiles || memcmp(io_dldi_data->friendlyName, "DSTWO(Slot-1)", 0xD) == 0) {
		extNames.push_back(".plg");
	}
    if (_showAllFiles || ms().showRvid)
    {
		extNames.push_back(".rvid");
	}
    extNames.push_back(".gba");
    if (_showAllFiles || ms().showGb)
    {
        extNames.push_back(".gb");
        extNames.push_back(".gbc");
    }
    if (_showAllFiles || ms().showNes)
    {
        extNames.push_back(".nes");
        extNames.push_back(".fds");
    }
    if (_showAllFiles || ms().showSmsGg)
    {
        extNames.push_back(".sms");
        extNames.push_back(".gg");
    }
    if (_showAllFiles || ms().showMd)
    {
        extNames.push_back(".gen");
    }
    if (_showAllFiles || ms().showSnes)
    {
        extNames.push_back(".smc");
        extNames.push_back(".sfc");
    }

    if (_showAllFiles)
        extNames.clear();
    std::vector<std::string> savNames;
    savNames.push_back(".sav");

    struct stat st;
    dirent *direntry;
    std::string extName;
    char lfnBuf[strlen(dirName.c_str()) + 256 + 2] = {'\0'};
    // list dir

    cwl();
    if (dir)
    {
        _currentDir = dirName;
        
        while ((direntry = readdir(dir)) != NULL)
        {
            memset(lfnBuf, 0, sizeof(lfnBuf));
            snprintf(lfnBuf, sizeof(lfnBuf), "%s/%s", dirName.c_str(), direntry->d_name);
            stat(lfnBuf, &st);
            std::string lfn(direntry->d_name);

            // st.st_mode & S_IFDIR indicates a directory
            size_t lastDotPos = lfn.find_last_of('.');
            if (lfn.npos != lastDotPos)
                extName = lfn.substr(lastDotPos);
            else
                extName = "";

            dbg_printf("%s: %s %s\n", (st.st_mode & S_IFDIR ? " DIR" : "FILE"), lfnBuf, extName.c_str());
            bool showThis = (st.st_mode & S_IFDIR) ? ((strcmp(lfn.c_str(), ".") && strcmp(lfn.c_str(), "..") && strcmp(lfn.c_str(), "_nds") && strcmp(lfn.c_str(), "saves") && ms().showDirectories)) // directory filter
                                                   : extnameFilter(extNames, extName);                                                                                                                // extension name filter
            showThis = showThis && (_showAllFiles || (strncmp(".", direntry->d_name, 1)));                                                                                                           // Hide dotfiles

            if (showThis)
            {
                u32 row_count = getRowCount();
                size_t insertPos(row_count);

                std::string real_name = dirName + lfn;
                if (st.st_mode & S_IFDIR)
                {
                    real_name += "/";
                }

                addDirEntry(insertPos, lfn, "", real_name, "", NULL);
            }
        }
        closedir(dir);

        std::sort(_rows.begin(), _rows.end(), itemSortComp);

        for (size_t ii = 0; ii < _rows.size(); ++ii)
        {

            DSRomInfo &rominfo = _romInfoList[ii];
            std::string filename = _rows[ii][REALNAME_COLUMN].text();
            size_t lastDotPos = filename.find_last_of('.');
            if (filename.npos != lastDotPos)
                extName = filename.substr(lastDotPos);
            else
                extName = "";
            for (size_t jj = 0; jj < extName.size(); ++jj)
                extName[jj] = tolower(extName[jj]);

            if ('/' == filename[filename.size() - 1])
            {
                rominfo.setBanner("folder", folder_banner_bin);
            }
            else
            {
                bool allowExt = true, allowUnknown = false;
                if (".gba" == extName)
                {
                    rominfo.MayBeGbaRom(filename);
                }
                //else if (".launcharg" == extName || ".argv" == extName)
                else if (".argv" == extName)
                {
                    memcpy(&rominfo.banner(), unknown_banner_bin, sizeof(tNDSBanner));
                    rominfo.MayBeArgv(filename);
                    allowUnknown = true;
                }
                else if (".plg" == extName)
                {
                    rominfo.setBanner("plg", ds2plg_banner_bin);
                }
                else if (".gb" == extName)
                {
                    rominfo.setBanner("gb", gbrom_banner_bin);
                }
                else if (".gbc" == extName)
                {
                    rominfo.setBanner("gbc", gbcrom_banner_bin);
                }
                else if (".nes" == extName)
                {
                    rominfo.setBanner("nes", nesrom_banner_bin);
                }
                else if (".sms" == extName || ".gg" == extName)
                {
                    rominfo.setBanner("sms", s8ds_banner_bin);
                }
                else if (".gen" == extName)
                {
                    rominfo.setBanner("sms", smdrom_banner_bin);
                }
                else if (".smc" == extName || ".sfc" == extName)
                {
                    rominfo.setBanner("sms", snemulds_banner_bin);
                }
                else if (".nds" != extName && ".ids" != extName && ".dsi" != extName)
                {
                    memcpy(&rominfo.banner(), unknown_banner_bin, sizeof(tNDSBanner));
                    allowUnknown = true;
                }
                else
                {
                    rominfo.MayBeDSRom(filename);
                    allowExt = false;
                }
                rominfo.setExtIcon(_rows[ii][SHOWNAME_COLUMN].text());
                if (allowExt && extName.length() && !rominfo.isExtIcon())
                    rominfo.setExtIcon(extName.substr(1));
                if (allowUnknown && !rominfo.isExtIcon())
                    rominfo.setExtIcon("unknown");
            }
        }
    }

    directoryChanged();

    return true;
}

void MainList::onSelectChanged(u32 index)
{
    dbg_printf("%s\n", _rows[index][3].text().c_str());
}

void MainList::onSelectedRowClicked(u32 index)
{
    const INPUT &input = getInput();
    //dbg_printf("%d %d", input.touchPt.px, _position.x );
    if (input.touchPt.px > _position.x && input.touchPt.px < _position.x + 32)
        selectedRowHeadClicked(index);
}

void MainList::onScrolled(u32 index)
{
    _activeIconScale = 1;
    //updateActiveIcon( CONTENT );
}

void MainList::backParentDir()
{
    if ("..." == _currentDir)
        return;

    dbg_printf("CURDIR:\"%s\"\n", _currentDir.c_str());
    if ("" == _currentDir || SD_ROOT == _currentDir || S1SD_ROOT == _currentDir || !ms().showDirectories)
    {
        dbg_printf("Entering HOME\n");
        enterDir(SPATH_ROOT);
        return;
    }

    if (!strncmp(_currentDir.c_str(), "^*::", 4)) {
        return;
    }

    if (!ms().showDirectories)
    {
        // Allow going back into the root, but don't allow going
        // back in other directories.
        return;
    }

    size_t pos = _currentDir.rfind("/", _currentDir.size() - 2);
    std::string parentDir = _currentDir.substr(0, pos + 1);
    dbg_printf("%s->%s\n", _currentDir.c_str(), parentDir.c_str());

    std::string oldCurrentDir = _currentDir;

    if (enterDir(parentDir))
    { // select last entered director
        for (size_t i = 0; i < _rows.size(); ++i)
        {
            if (parentDir + _rows[i][SHOWNAME_COLUMN].text() == oldCurrentDir)
            {
                selectRow(i);
            }
        }
    }
}

std::string MainList::getSelectedFullPath()
{
    if (!_rows.size())
        return std::string("");
    return _rows[_selectedRowId][REALNAME_COLUMN].text();
}

std::string MainList::getSelectedShowName()
{
    if (!_rows.size())
        return std::string("");
    return _rows[_selectedRowId][SHOWNAME_COLUMN].text();
}

bool MainList::getRomInfo(u32 rowIndex, DSRomInfo &info) const
{
    if (rowIndex < _romInfoList.size())
    {
        info = _romInfoList[rowIndex];
        return true;
    }
    else
    {
        return false;
    }
}

void MainList::setRomInfo(u32 rowIndex, const DSRomInfo &info)
{
    if (!_romInfoList[rowIndex].isDSRom())
        return;

    if (rowIndex < _romInfoList.size())
    {
        _romInfoList[rowIndex] = info;
    }
}

void MainList::draw()
{
    updateInternalNames();
    ListView::draw();
    updateActiveIcon(POSITION);
    drawIcons();
}

void MainList::drawIcons()
{
    size_t total = _visibleRowCount;
    if (total > _rows.size() - _firstVisibleRowId)
        total = _rows.size() - _firstVisibleRowId;

    for (size_t i = 0; i < total; ++i)
    {
        if (_firstVisibleRowId + i == _selectedRowId)
        {
            if (_activeIcon.visible())
            {
                continue;
            }
        }
        s32 itemX = _position.x + 1;
        s32 itemY = _position.y + i * _rowHeight + ((_rowHeight - 32) >> 1) - 1;
        if (_romInfoList[_firstVisibleRowId + i].isBannerAnimated() && ms().animateDsiIcons)
        {
            int seqIdx = seq().allocate_sequence(
                _romInfoList[_firstVisibleRowId + i].saveInfo().gameCode,
                _romInfoList[_firstVisibleRowId + i].animatedIcon().sequence);

            int bmpIdx = seq()._dsiIconSequence[seqIdx]._bitmapIndex;
            int palIdx = seq()._dsiIconSequence[seqIdx]._paletteIndex;
            bool flipH = seq()._dsiIconSequence[seqIdx]._flipH;
            bool flipV = seq()._dsiIconSequence[seqIdx]._flipV;
            if (VM_LIST != _viewMode)
            {
                _romInfoList[_firstVisibleRowId + i].drawDSiAnimatedRomIcon(itemX, itemY, bmpIdx, palIdx, flipH, flipV, _engine);
            }
        }
        else
        {
            if (VM_LIST != _viewMode)
            {
                _romInfoList[_firstVisibleRowId + i].drawDSRomIcon(itemX, itemY, _engine);
            }
        }
    }
}

void MainList::setViewMode(VIEW_MODE mode)
{
    if (!_columns.size())
        return;
    _viewMode = mode;
    switch (_viewMode)
    {
    case VM_LIST:
        _columns[ICON_COLUMN].width = 0;
        _columns[SHOWNAME_COLUMN].width = 250;
        _columns[INTERNALNAME_COLUMN].width = 0;
        arangeColumnsSize();
        setRowHeight(15);
        break;
    case VM_ICON:
        _columns[ICON_COLUMN].width = 36;
        _columns[SHOWNAME_COLUMN].width = 214;
        _columns[INTERNALNAME_COLUMN].width = 0;
        arangeColumnsSize();
        setRowHeight(38);
        break;
    case VM_INTERNAL:
        _columns[ICON_COLUMN].width = 36;
        _columns[SHOWNAME_COLUMN].width = 0;
        _columns[INTERNALNAME_COLUMN].width = 214;
        arangeColumnsSize();
        setRowHeight(38);
        break;
    }
    scrollTo(_selectedRowId - _visibleRowCount + 1);
}

void MainList::updateActiveIcon(bool updateContent)
{
    const INPUT &temp = getInput();
    bool allowAnimation = true;
    animateIcons(allowAnimation);

    //do not show active icon when hold key to list files. Otherwise the icon will not show correctly.
    if (getInputIdleMs() > 1000 && VM_LIST != _viewMode && allowAnimation && _romInfoList.size() && 0 == temp.keysHeld && ms().ak_zoomIcons)
    {
        if (!_activeIcon.visible())
        {
            u8 backBuffer[32 * 32 * 2];
            zeroMemory(backBuffer, 32 * 32 * 2);

            // if (_romInfoList[_selectedRowId].isBannerAnimated()) {
            //       int seqIdx = seq().allocate_sequence(
            //         _romInfoList[_selectedRowId].saveInfo().gameCode,
            //         _romInfoList[_selectedRowId].animatedIcon().sequence);

            //     int bmpIdx = seq()._dsiIconSequence[seqIdx]._bitmapIndex;
            //     int palIdx = seq()._dsiIconSequence[seqIdx]._paletteIndex;

            //     _romInfoList[_selectedRowId].drawDSiAnimatedRomIconMem(backBuffer, bmpIdx, palIdx);
            // }

            _romInfoList[_selectedRowId].drawDSRomIconMem(backBuffer);
            memcpy(_activeIcon.buffer(), backBuffer, 32 * 32 * 2);
            _activeIcon.setBufferChanged();

            s32 itemX = _position.x;
            s32 itemY = _position.y + (_selectedRowId - _firstVisibleRowId) * _rowHeight + ((_rowHeight - 32) >> 1) - 1;
            _activeIcon.setPosition(itemX, itemY);
            _activeIcon.show();
            dbg_printf("sel %d ac ico x %d y %d\n", _selectedRowId, itemX, itemY);
            for (u8 i = 0; i < 8; ++i)
                dbg_printf("%02x", backBuffer[i]);
            dbg_printf("\n");
        }
    }
    else
    {
        if (_activeIcon.visible())
        {
            _activeIcon.hide();
            cwl();
        }
    }
}

std::string MainList::getCurrentDir()
{
    return _currentDir;
}

void MainList::updateInternalNames(void)
{
    if (_viewMode == VM_INTERNAL)
    {
        size_t total = _visibleRowCount;
        if (total > _rows.size() - _firstVisibleRowId)
            total = _rows.size() - _firstVisibleRowId;
        for (size_t ii = 0; ii < total; ++ii)
        {
            if (0 == _rows[_firstVisibleRowId + ii][INTERNALNAME_COLUMN].text().length())
            {
                if (_romInfoList[_firstVisibleRowId + ii].isDSRom())
                {
                    _rows[_firstVisibleRowId + ii][INTERNALNAME_COLUMN]
                        .setText(unicode_to_local_string(_romInfoList[_firstVisibleRowId + ii].banner().titles[ms().getGuiLanguage()], 128, NULL));
                }
                else
                {
                    _rows[_firstVisibleRowId + ii][INTERNALNAME_COLUMN]
                        .setText(_rows[_firstVisibleRowId + ii][SHOWNAME_COLUMN].text());
                }
            }
        }
    }
}

void MainList::SwitchShowAllFiles(void)
{
    _showAllFiles = !_showAllFiles;
    enterDir(getCurrentDir());
}
