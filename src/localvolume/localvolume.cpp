#include "localvolume.h"
#include <rsys/common.h>
#include <rsys/byteswap.h>
#include <FileMgr.h>
#include <MemoryMgr.h>
#include <rsys/file.h>
#include <rsys/hfs.h>
#include <map>
#include <iostream>
#include "item.h"

using namespace Executor;


ItemPtr DirectoryHandler::handleDirEntry(const DirectoryItem& parent, const fs::directory_entry& e)
{
    if(fs::is_directory(e.path()))
    {
        return volume.lookupDirectory(parent, e.path());
    }
    return nullptr;
}

ItemPtr ExtensionHandler::handleDirEntry(const DirectoryItem& parent, const fs::directory_entry& e)
{
    if(fs::is_regular_file(e.path()))
    {
        return std::make_shared<PlainFileItem>(parent, e.path());
    }
    return nullptr;
}


Item::Item(LocalVolume& vol, fs::path p)
    : volume_(vol), path_(std::move(p))
{
    name_ = toMacRomanFilename(path_.filename());

    parID_ = 1;
}

Item::Item(const DirectoryItem& parent, fs::path p)
    : volume_(parent.volume_), path_(std::move(p))
{
    name_ = toMacRomanFilename(path_.filename());

    parID_ = parent.dirID();
}

std::unique_ptr<OpenFile> PlainFileItem::open()
{
    return std::make_unique<PlainDataFork>(path_);
}
std::unique_ptr<OpenFile> PlainFileItem::openRF()
{
    return std::make_unique<EmptyFork>();
}
void PlainFileItem::deleteFile()
{
    fs::remove(path());
}


DirectoryItem::DirectoryItem(LocalVolume& vol, fs::path p)
    : Item(vol, std::move(p)), dirID_(2)
{
    name_ = vol.getVolumeName();
}

DirectoryItem::DirectoryItem(const DirectoryItem& parent, fs::path p, long dirID)
    : Item(parent, std::move(p)), dirID_(dirID)
{
}

void DirectoryItem::flushCache()
{
    cache_valid_ = false;
    contents_.clear();
    contents_by_name_.clear();
}
void DirectoryItem::updateCache()
{
    auto now = std::chrono::steady_clock::now();
    if(cache_valid_)
    {
        using namespace std::chrono_literals;
        if(now > cache_timestamp_ + 1s)
        {
            std::cout << "flushed.\n";
            flushCache();
        }
    }
    if(cache_valid_)
        return;
    
    for(const auto& e : fs::directory_iterator(path_))
    {
        bool hidden = false;
        for(auto& handler : volume_.handlers)
        {
            if(handler->isHidden(e))
            {
                hidden = true;
                break;
            }
        }
        if(hidden)
            continue;

        for(auto& handler : volume_.handlers)
        {
            if(ItemPtr item = handler->handleDirEntry(*this, e))
            {
                mac_string nameUpr = item->name();
                ROMlib_UprString(nameUpr.data(), false, nameUpr.size());
                auto [it, inserted] = contents_by_name_.emplace(nameUpr, item);
                if(inserted)
                {
                    contents_.push_back(item);
                }
                else
                {
                    std::cout << "duplicate name mapping: " << e.path() << std::endl; 
                }
                break;
            }
        }
    }

    cache_timestamp_ = now;
    cache_valid_ = true;
}

ItemPtr DirectoryItem::resolve(mac_string_view name)
{
    updateCache();
    mac_string nameUpr { name };
    ROMlib_UprString(nameUpr.data(), false, nameUpr.size());
    auto it = contents_by_name_.find(nameUpr);
    if(it != contents_by_name_.end())
        return it->second;
    throw OSErrorException(fnfErr);
}

ItemPtr DirectoryItem::resolve(int index)
{
    updateCache();
    if(index >= 1 && index <= contents_.size())
        return contents_[index-1];
    throw OSErrorException(fnfErr);
}

LocalVolume::LocalVolume(VCB& vcb, fs::path root)
    : Volume(vcb), root(root)
{
    pathToId[root] = 2;
    directories_[2] = std::make_shared<DirectoryItem>(*this, root);

    handlers.push_back(std::make_unique<DirectoryHandler>(*this));
    handlers.push_back(std::make_unique<AppleDoubleHandler>(*this));
    handlers.push_back(std::make_unique<BasiliskHandler>(*this));
    handlers.push_back(std::make_unique<ExtensionHandler>(*this));
}

std::shared_ptr<DirectoryItem> LocalVolume::lookupDirectory(const DirectoryItem& parent, const fs::path& path)
{
    auto [it, inserted] = pathToId.emplace(path, nextDirectory);
    if(inserted)
    {
        DirID id = nextDirectory++;
        auto dir = std::make_shared<DirectoryItem>(parent, path, id);
        directories_.emplace(id, dir);
        return dir;
    }
    else
        return directories_.at(it->second);
}

std::shared_ptr<DirectoryItem> LocalVolume::resolve(short vRef, long dirID)
{
    if(dirID)
    {
        auto it = directories_.find(dirID);
        if(it == directories_.end())
            throw OSErrorException(fnfErr);
        else
            return it->second;
    }
    else if(vRef == 0)
    {
        return resolve(CW(vcb.vcbVRefNum), CL(DefDirID));
    }
    else if(ISWDNUM(vRef))
    {
        auto wdp = WDNUMTOWDP(vRef);
        return resolve(CW(vcb.vcbVRefNum), CL(wdp->dirid));
    }
    else
    {
        // "poor man's search path": not implemented
        return resolve(CW(vcb.vcbVRefNum), 2);
    }
}
ItemPtr LocalVolume::resolve(mac_string_view name, short vRef, long dirID)
{
    if(name.empty())
        return resolve(vRef, dirID);

    size_t colon = name.find(':');

    if(colon != mac_string_view::npos)
    {
        if(colon == 0)
        {
            std::shared_ptr<DirectoryItem> dir = resolve(vRef, dirID);
            return resolveRelative(dir, name);
        }
        else
        {
            return resolveRelative(directories_[2], mac_string_view(name.begin() + colon, name.end()));
        }
    }
    else
    {
        std::shared_ptr<DirectoryItem> dir = resolve(vRef, dirID);
        return dir->resolve(name);
    }
}

ItemPtr LocalVolume::resolveRelative(const std::shared_ptr<DirectoryItem>& base, mac_string_view name)
{
    auto p = name.begin();
    //for(++p; p != name.end() && *p == ':'; ++p)
    //    base = 
    // TODO: relative paths

    if(p == name.end())
        return base;

    ++p;

    if(p == name.end())
        return base;

    auto colon = std::find(p, name.end(), ':');

    ItemPtr item = base->resolve(mac_string_view(p, colon));

    if(colon == name.end())
        return item;
    else if(auto dir = std::dynamic_pointer_cast<DirectoryItem>(item))
    {
        return resolveRelative(dir, mac_string_view(colon, name.end()));
    }
    else
        throw OSErrorException(fnfErr);
}

ItemPtr LocalVolume::resolve(short vRef, long dirID, short index)
{
    std::shared_ptr<DirectoryItem> dir = resolve(vRef, dirID);
    return dir->resolve(index);
}

ItemPtr LocalVolume::resolve(mac_string_view name, short vRef, long dirID, short index)
{
    if(index > 0)
        return resolve(vRef, dirID, index);
    else if(index == 0 && !name.empty())
        return resolve(name, vRef, dirID);
    else
        return resolve(vRef, dirID);
}

mac_string LocalVolume::getVolumeName() const
{
    return mac_string(mac_string_view(vcb.vcbVN));
}


void LocalVolume::PBHRename(HParmBlkPtr pb)
{
    throw OSErrorException(paramErr);
}
void LocalVolume::PBDirCreate(HParmBlkPtr pb)
{
    throw OSErrorException(paramErr);
}

struct LocalVolume::FCBExtension
{
    filecontrolblock *fcb;
    short refNum;
    std::unique_ptr<OpenFile> access;

    FCBExtension() = default;
    FCBExtension(filecontrolblock *fcb, short refNum)
        : fcb(fcb), refNum(refNum)
    {}
};

LocalVolume::FCBExtension& LocalVolume::getFCBX(short refNum)
{
    if(refNum < 0 || refNum >= fcbExtensions.size())
        throw OSErrorException(paramErr);
    return fcbExtensions[refNum];
}

LocalVolume::FCBExtension& LocalVolume::openFCBX()
{
    filecontrolblock* fcb = ROMlib_getfreefcbp();
    short refNum = (char *)fcb - (char *)MR(LM(FCBSPtr));

    memset(fcb, 0, sizeof(*fcb));
    fcb->fcbVPtr = RM(&vcb);

    if(fcbExtensions.size() < refNum + 1)
        fcbExtensions.resize(refNum+1);
    fcbExtensions[refNum] = FCBExtension(fcb, refNum);

    return fcbExtensions[refNum];
}


void LocalVolume::PBHOpen(HParmBlkPtr pb)
{
    ItemPtr item = resolve(MR(pb->ioParam.ioNamePtr), CW(pb->ioParam.ioVRefNum), CL(pb->fileParam.ioDirID));
    if(FileItem *fileItem = dynamic_cast<FileItem*>(item.get()))
    {
        FCBExtension& fcbx = openFCBX();
        fcbx.access = fileItem->open();
        fcbx.fcb->fcbFlNum = CL(42);
        pb->ioParam.ioRefNum = CW(fcbx.refNum);
    }
    else
        throw OSErrorException(paramErr);    // TODO: what's the correct error?
}
void LocalVolume::PBHOpenRF(HParmBlkPtr pb)
{
    ItemPtr item = resolve(MR(pb->ioParam.ioNamePtr), CW(pb->ioParam.ioVRefNum), CL(pb->fileParam.ioDirID));
    if(FileItem *fileItem = dynamic_cast<FileItem*>(item.get()))
    {
        FCBExtension& fcbx = openFCBX();
        fcbx.access = fileItem->openRF();
        fcbx.fcb->fcbFlNum = CL(42);
        pb->ioParam.ioRefNum = CW(fcbx.refNum);
    }
    else
        throw OSErrorException(paramErr);    // TODO: what's the correct error?
}

void LocalVolume::PBOpenDF(ParmBlkPtr pb)
{
    ItemPtr item = resolve(MR(pb->ioParam.ioNamePtr), CW(pb->ioParam.ioVRefNum), 0);
    if(FileItem *fileItem = dynamic_cast<FileItem*>(item.get()))
    {
        FCBExtension& fcbx = openFCBX();
        fcbx.access = fileItem->open();
        fcbx.fcb->fcbFlNum = CL(42);
        pb->ioParam.ioRefNum = CW(fcbx.refNum);
    }
    else
        throw OSErrorException(paramErr);    // TODO: what's the correct error?
}
void LocalVolume::PBOpenRF(ParmBlkPtr pb)
{
    ItemPtr item = resolve(MR(pb->ioParam.ioNamePtr), CW(pb->ioParam.ioVRefNum), 0);
    if(FileItem *fileItem = dynamic_cast<FileItem*>(item.get()))
    {
        FCBExtension& fcbx = openFCBX();
        fcbx.access = fileItem->openRF();
        fcbx.fcb->fcbFlNum = CL(42);
        pb->ioParam.ioRefNum = CW(fcbx.refNum);
    }
    else
        throw OSErrorException(paramErr);    // TODO: what's the correct error?
}


void LocalVolume::PBHGetFInfo(HParmBlkPtr pb)
{
    StringPtr inputName = MR(pb->fileParam.ioNamePtr);
    if(CW(pb->fileParam.ioFDirIndex) > 0)
        inputName = nullptr;
    
        // negative index means what?
    ItemPtr item = resolve(PascalStringView(inputName),
        CW(pb->fileParam.ioVRefNum), CL(pb->fileParam.ioDirID), CW(pb->fileParam.ioFDirIndex));

    std::cout << "HGetFInfo: " << item->path() << std::endl;
    if(StringPtr outputName = MR(pb->fileParam.ioNamePtr))
    {
        const mac_string name = item->name();
        size_t n = std::min(name.size(), (size_t)255);
        memcpy(outputName+1, name.data(), n);
        outputName[0] = n;
    }
    
    if(FileItem *fileItem = dynamic_cast<FileItem*>(item.get()))
    {
        pb->fileParam.ioFlAttrib = 0;
        pb->fileParam.ioFlFndrInfo = fileItem->getFInfo();
    }
    else
        throw OSErrorException(paramErr);
}
void LocalVolume::PBGetFInfo(ParmBlkPtr pb)
{
    StringPtr inputName = MR(pb->fileParam.ioNamePtr);
    if(CW(pb->fileParam.ioFDirIndex) > 0)
        inputName = nullptr;
    
        // negative index means what?
    ItemPtr item = resolve(inputName,
        CW(pb->fileParam.ioVRefNum), 0, CW(pb->fileParam.ioFDirIndex));

    std::cout << "GetFInfo: " << item->path() << std::endl;
    if(StringPtr outputName = MR(pb->fileParam.ioNamePtr))
    {
        const mac_string name = item->name();
        size_t n = std::min(name.size(), (size_t)255);
        memcpy(outputName+1, name.data(), n);
        outputName[0] = n;
    }
    
    if(FileItem *fileItem = dynamic_cast<FileItem*>(item.get()))
    {
        pb->fileParam.ioFlAttrib = 0;   // TODO
        pb->fileParam.ioFlFndrInfo = fileItem->getFInfo();
        pb->fileParam.ioFlCrDat = 0;    // TODO
        pb->fileParam.ioFlMdDat = 0;    // TODO
    }
    else
        throw OSErrorException(paramErr);
}

void LocalVolume::setFInfoCommon(Item& item, ParmBlkPtr pb)
{
    if(FileItem *fitem = dynamic_cast<FileItem*>(&item))
    {
        fitem->setFInfo(pb->fileParam.ioFlFndrInfo);
    }
    else
        throw OSErrorException(paramErr);   // TODO: item is a directory
    
    // pb->fileParam.ioFlCrDat      TODO
    // pb->fileParam.ioFlMdDat      TODO
}
void LocalVolume::PBSetFInfo(ParmBlkPtr pb)
{
    setFInfoCommon(*resolve(MR(pb->fileParam.ioNamePtr), CW(pb->fileParam.ioVRefNum), 0), pb);
}
void LocalVolume::PBHSetFInfo(HParmBlkPtr pb)
{
    setFInfoCommon(*resolve(MR(pb->fileParam.ioNamePtr), CW(pb->fileParam.ioVRefNum), CL(pb->fileParam.ioDirID)),
        reinterpret_cast<ParmBlkPtr>(pb));
}

void LocalVolume::PBGetCatInfo(CInfoPBPtr pb)
{
    StringPtr inputName = MR(pb->hFileInfo.ioNamePtr);
    if(CW(pb->hFileInfo.ioFDirIndex) != 0)
        inputName = nullptr;
    ItemPtr item = resolve(PascalStringView(inputName),
        CW(pb->hFileInfo.ioVRefNum), CL(pb->hFileInfo.ioDirID), CW(pb->hFileInfo.ioFDirIndex));

    std::cout << "GetCatInfo: " << item->path() << std::endl;
    if(StringPtr outputName = MR(pb->hFileInfo.ioNamePtr))
    {
        if(!inputName)
        {
            const mac_string name = item->name();
            size_t n = std::min(name.size(), (size_t)255);
            memcpy(outputName+1, name.data(), n);
            outputName[0] = n;
        }
    }
    
    if(DirectoryItem *dirItem = dynamic_cast<DirectoryItem*>(item.get()))
    {
        pb->dirInfo.ioFlAttrib = ATTRIB_ISADIR;
        pb->dirInfo.ioDrDirID = CL(dirItem->dirID());

        pb->dirInfo.ioDrParID = CL(dirItem->parID());

        pb->dirInfo.ioDrNmFls = CW(dirItem->countItems());

        // ioACUser
        // ioDrUserWds
        // ioDrCrDat
        // ioDrMdDat
        // ioDrBkDat
        // ioDrFndrInfo
    }
    else if(FileItem *fileItem = dynamic_cast<FileItem*>(item.get()))
    {
        pb->hFileInfo.ioFlAttrib = 0;
        pb->hFileInfo.ioFlFndrInfo = fileItem->getFInfo();

        pb->hFileInfo.ioVRefNum = vcb.vcbVRefNum;
        pb->hFileInfo.ioFlParID = CL(fileItem->parID());

        // ioFlStBlk
        // ioFlLgLen
        // ioFlPyLen
        // ioFlRStBlk
        // ioFlRPyLen

        // ioFlCrDat
        // ioFlMdDat
        // ioFlBkDat
        // ioFlXFndrInfo
        // ioFlClpSiz
    }
}
void LocalVolume::PBSetCatInfo(CInfoPBPtr pb)
{
    throw OSErrorException(paramErr);
}
void LocalVolume::PBCatMove(CMovePBPtr pb)
{
    throw OSErrorException(paramErr);
}

void LocalVolume::createCommon(DirectoryItem& parent, mac_string_view name)
{
    try
    {
        parent.resolve(name);
    }
    catch(OSErrorException& e)
    {
        if(e.code == fnfErr)
            ;
        else if(e.code == noErr)
            throw OSErrorException(dupFNErr);
        else
            throw;
    }

    // TODO: this is the responsible of the BasiliskHandler class
    fs::create_directory(parent.path() / ".rsrc");
    fs::create_directory(parent.path() / ".finf");
    fs::path fn = toUnicodeFilename(name);

    fs::ofstream(parent.path() / fn);
    fs::ofstream(parent.path() / ".rsrc" / fn);
    FInfo info = {0};
    fs::ofstream(parent.path() / ".finf" / fn).write((char*)&info, sizeof(info));
    parent.flushCache();
}
void LocalVolume::PBCreate(ParmBlkPtr pb)
{
    createCommon(*resolve(CW(pb->fileParam.ioVRefNum), 0), MR(pb->ioParam.ioNamePtr));
}
void LocalVolume::PBHCreate(HParmBlkPtr pb)
{
    createCommon(*resolve(CW(pb->fileParam.ioVRefNum), CL(pb->fileParam.ioDirID)), MR(pb->ioParam.ioNamePtr));
}


void LocalVolume::PBDelete(ParmBlkPtr pb)
{
    ItemPtr item = resolve(MR(pb->ioParam.ioNamePtr), CW(pb->ioParam.ioVRefNum), 0);
    if(FileItem *fileItem = dynamic_cast<FileItem*>(item.get()))
    {
        fileItem->deleteFile();
        resolve(CW(pb->ioParam.ioVRefNum), 0)->flushCache();    // TODO: fileItem should know about parent
    }
    else
        throw OSErrorException(paramErr);    // TODO: what's the correct error?
}
void LocalVolume::PBHDelete(HParmBlkPtr pb)
{
    ItemPtr item = resolve(MR(pb->ioParam.ioNamePtr), CW(pb->ioParam.ioVRefNum), CL(pb->fileParam.ioDirID));
    if(FileItem *fileItem = dynamic_cast<FileItem*>(item.get()))
    {
        fileItem->deleteFile();
        resolve(CW(pb->ioParam.ioVRefNum), CL(pb->fileParam.ioDirID))->flushCache();   // TODO: fileItem should know about parent
    }
    else
        throw OSErrorException(paramErr);    // TODO: what's the correct error?
}


void LocalVolume::PBOpenWD(WDPBPtr pb)
{
    ItemPtr item = resolve(MR(pb->ioNamePtr), CW(pb->ioVRefNum), CL(pb->ioWDDirID));
    std::cout << "PBOpenWD: " << item->path() << std::endl;

    long dirID;
    if(DirectoryItem *dirItem = dynamic_cast<DirectoryItem*>(item.get()))
        dirID = dirItem->dirID();
    else
        dirID = item->parID();

    OSErr err =  ROMlib_mkwd(pb, &vcb, dirID, Cx(pb->ioWDProcID));
    if(err)
        throw OSErrorException(err);
}
void LocalVolume::PBSetFLock(ParmBlkPtr pb)
{
    throw OSErrorException(paramErr);
}
void LocalVolume::PBHSetFLock(HParmBlkPtr pb)
{
    throw OSErrorException(paramErr);
}
void LocalVolume::PBRstFLock(ParmBlkPtr pb)
{
    throw OSErrorException(paramErr);
}
void LocalVolume::PBHRstFLock(HParmBlkPtr pb)
{
    throw OSErrorException(paramErr);
}
void LocalVolume::PBSetFVers(ParmBlkPtr pb)
{
    throw OSErrorException(paramErr);
}
void LocalVolume::PBRename(ParmBlkPtr pb)
{
    throw OSErrorException(paramErr);
}
void LocalVolume::PBSetVInfo(HParmBlkPtr pb)
{
    throw OSErrorException(paramErr);
}
void LocalVolume::PBFlushVol(ParmBlkPtr pb)
{
    throw OSErrorException(paramErr);
}
void LocalVolume::PBUnmountVol(ParmBlkPtr pb)
{
    throw OSErrorException(paramErr);
}
void LocalVolume::PBEject(ParmBlkPtr pb)
{
    throw OSErrorException(paramErr);
}
void LocalVolume::PBOffLine(ParmBlkPtr pb)
{
    throw OSErrorException(paramErr);
}
void LocalVolume::PBRead(ParmBlkPtr pb)
{
    PBSetFPos(pb);
    auto& fcbx = getFCBX(CW(pb->ioParam.ioRefNum));
    size_t n = fcbx.access->read(CL(fcbx.fcb->fcbCrPs), MR(pb->ioParam.ioBuffer), CL(pb->ioParam.ioReqCount));
    pb->ioParam.ioActCount = CL(n);
    fcbx.fcb->fcbCrPs = CL( CL(fcbx.fcb->fcbCrPs) + n );
}
void LocalVolume::PBWrite(ParmBlkPtr pb)
{
    PBSetFPos(pb);
    auto& fcbx = getFCBX(CW(pb->ioParam.ioRefNum));
    size_t n = fcbx.access->write(CL(fcbx.fcb->fcbCrPs), MR(pb->ioParam.ioBuffer), CL(pb->ioParam.ioReqCount));
    pb->ioParam.ioActCount = CL(n);
    fcbx.fcb->fcbCrPs = CL( CL(fcbx.fcb->fcbCrPs) + n );
}
void LocalVolume::PBClose(ParmBlkPtr pb)
{
    auto& fcbx = getFCBX(CW(pb->ioParam.ioRefNum));
    fcbx.fcb->fcbFlNum = 0;
    fcbx = FCBExtension();
}

void LocalVolume::PBAllocate(ParmBlkPtr pb)
{
    throw OSErrorException(paramErr);
}
void LocalVolume::PBAllocContig(ParmBlkPtr pb)
{
    throw OSErrorException(paramErr);
}
void LocalVolume::PBLockRange(ParmBlkPtr pb)
{
    throw OSErrorException(paramErr);
}
void LocalVolume::PBUnlockRange(ParmBlkPtr pb)
{
    throw OSErrorException(paramErr);
}
void LocalVolume::PBGetFPos(ParmBlkPtr pb)
{
    auto& fcbx = getFCBX(CW(pb->ioParam.ioRefNum));
    pb->ioParam.ioReqCount = CL(0);
    pb->ioParam.ioActCount = CL(0);
    pb->ioParam.ioPosMode = CW(0);
    pb->ioParam.ioPosOffset = fcbx.fcb->fcbCrPs;
}

void LocalVolume::PBSetFPos(ParmBlkPtr pb)
{
    auto& fcbx = getFCBX(CW(pb->ioParam.ioRefNum));
    ssize_t eof = (ssize_t) fcbx.access->getEOF();
    ssize_t newPos = CL(fcbx.fcb->fcbCrPs);
    
    switch(CW(pb->ioParam.ioPosMode))
    {
        case fsAtMark:
            break;
        case fsFromStart:
            newPos = CL(pb->ioParam.ioPosOffset);
            break;
        case fsFromLEOF:
            newPos = eof + CL(pb->ioParam.ioPosOffset);
            break;
        case fsFromMark:
            newPos = CL(fcbx.fcb->fcbCrPs) + CL(pb->ioParam.ioPosOffset);
            break;
    }

    OSErr err = noErr;
    if(newPos > eof)
    {
        newPos = eof;
        err = eofErr;
    }
    
    if(newPos < 0)
    {
        newPos = CL(fcbx.fcb->fcbCrPs);     // MacOS 9 behavior.
                                            // System 6 actually sets fcbCrPs to a negative value.
        err = posErr;
    }

    pb->ioParam.ioPosOffset = fcbx.fcb->fcbCrPs = CL(newPos);

    if(err)
        throw OSErrorException(err);
}
void LocalVolume::PBGetEOF(ParmBlkPtr pb)
{
    auto& fcbx = getFCBX(CW(pb->ioParam.ioRefNum));
    size_t n = fcbx.access->getEOF();
    pb->ioParam.ioMisc = CL(n);
}

void LocalVolume::PBSetEOF(ParmBlkPtr pb)
{
    auto& fcbx = getFCBX(CW(pb->ioParam.ioRefNum));
    fcbx.access->setEOF(CL(pb->ioParam.ioMisc));
}

void LocalVolume::PBFlushFile(ParmBlkPtr pb)
{
    throw OSErrorException(paramErr);
}

void Executor::initLocalVol()
{
    VCBExtra *vp;
    GUEST<THz> savezone;

    savezone = LM(TheZone);
    LM(TheZone) = LM(SysZone);
    vp = (VCBExtra *)NewPtr(sizeof(VCBExtra));
    LM(TheZone) = savezone;

    if(!vp)
        return;
    memset(vp, 0, sizeof(VCBExtra));
    vp->vcb.vcbDrvNum = CWC(42);//pb->ioParam.ioVRefNum;

    --ROMlib_nextvrn;
    vp->vcb.vcbVRefNum = CW(ROMlib_nextvrn);

    
    strcpy((char *)vp->vcb.vcbVN + 1, "vol");
    vp->vcb.vcbVN[0] = strlen((char *)vp->vcb.vcbVN+1);

    vp->vcb.vcbSigWord = CWC(0x4244); /* IMIV-188 */
    vp->vcb.vcbFreeBks = CWC(20480); /* arbitrary */
    vp->vcb.vcbCrDate = 0; /* I'm lying here */
    vp->vcb.vcbVolBkUp = 0;
    vp->vcb.vcbAtrb = CWC(VNONEJECTABLEBIT);
    vp->vcb.vcbNmFls = CWC(100);
    vp->vcb.vcbNmAlBlks = CWC(300);
    vp->vcb.vcbAlBlkSiz = CLC(512);
    vp->vcb.vcbClpSiz = CLC(1);
    vp->vcb.vcbAlBlSt = CWC(10);
    vp->vcb.vcbNxtCNID = CLC(1000);
    if(!LM(DefVCBPtr))
    {
        LM(DefVCBPtr) = RM((VCB *)vp);
        LM(DefVRefNum) = vp->vcb.vcbVRefNum;
        DefDirID = CLC(2); /* top level */
    }
    Enqueue((QElemPtr)vp, &LM(VCBQHdr));

    vp->volume = new LocalVolume(vp->vcb, "/");
}
