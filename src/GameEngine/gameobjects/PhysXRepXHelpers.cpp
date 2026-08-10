//=========================================================================
//	Module: PhysXRepXHelpers.hpp
//	Copyright (C) 2011.
//=========================================================================

#include "PhysXRepXHelpers.h"

#include "foundation/PxAssert.h"
#include "foundation/PxMath.h"

#include <cstring>
#include <limits>
#include <optional>

using physx::PxAllocatorCallback;
using physx::PxMin;
using physx::PxU32;
using physx::PxU8;

#if !(defined(WO_SERVER) && defined(_WIN64))
#include "RepX\RepX.h"
#include "RepX\RepXUtility.h"
#include "extensions\PxStringTableExt.h"
//#include "PhysX\PxFoundation\internal\PxIOStream\public\PxFileBuf.h"
#endif

//////////////////////////////////////////////////////////////////////////

#if defined(WO_SERVER) && defined(_WIN64)
physx::repx::RepXCollection* loadCollection(const char* inPath, PxAllocatorCallback& inCallback)
{
	(void)inPath;
	(void)inCallback;
	return NULL;
}
#else
class MyPhysXFileBuf_ReadOnly : public PxInputData
{
private:
	engine::platform::File f;
	PxU32 length;
public:
	MyPhysXFileBuf_ReadOnly(const char* fname)
	: f(
		engine::platform::Path(fname),
		engine::platform::FileAccess::Read,
		engine::platform::FileCreation::OpenExisting)
	, length(0)
	{
		PX_ASSERT(f);

		const std::optional<std::uint64_t> fileSize = f.GetSize();
		if(fileSize && *fileSize <= std::numeric_limits<PxU32>::max())
			length = static_cast<PxU32>(*fileSize);
		else
			PX_ASSERT(false);
	}

	virtual ~MyPhysXFileBuf_ReadOnly(void) {}

	virtual PxU32	getLength(void) const { return length; }
	virtual void	seek(PxU32 loc)
	{
		(void)f.Seek(loc, engine::platform::FileSeekOrigin::Begin);
	}
	virtual PxU32	read(void *mem, PxU32 len)
	{
		const engine::platform::FileIoResult result = f.Read(mem, len);
		return static_cast<PxU32>(result.bytesTransferred);
	}
	virtual PxU32	tell(void) const
	{
		const std::optional<std::uint64_t> position = f.GetPosition();
		return
			position && *position <= std::numeric_limits<PxU32>::max()
				? static_cast<PxU32>(*position)
				: 0;
	}
};

//////////////////////////////////////////////////////////////////////////

physx::repx::RepXCollection* loadCollection(const char* inPath, PxAllocatorCallback& inCallback)
{
	physx::repx::RepXExtension* theExtensions[64];
	PxU32 numExtensions = buildExtensionList( theExtensions, 64, inCallback );

	MyPhysXFileBuf_ReadOnly fileBuf(inPath);
	physx::repx::RepXCollection* retval = physx::repx::RepXCollection::create( fileBuf, theExtensions, numExtensions, inCallback );
	if ( retval )
		retval = &physx::repx::RepXUpgrader::upgradeCollection( *retval );
	return retval;
}
#endif

//////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------
//	PhysxUserMemoryReadStream
//-------------------------------------------------------------------------

PhysxUserMemoryWriteStream::PhysxUserMemoryWriteStream()
: mData(0)
, mSize(0)
, mCapacity(0)
{

}

//////////////////////////////////////////////////////////////////////////

PhysxUserMemoryWriteStream::~PhysxUserMemoryWriteStream()
{
	delete[] mData;
}

//////////////////////////////////////////////////////////////////////////

PxU32 PhysxUserMemoryWriteStream::write(const void* src, PxU32 size)
{
	PxU32 expectedSize = mSize + size;
	if(expectedSize > mCapacity)
	{
		mCapacity = expectedSize + 4096;

		PxU8* newData = new PxU8[mCapacity];
		PX_ASSERT(newData!=NULL);

		if(newData)
		{
			memcpy(newData, mData, mSize);
			delete[] mData;
		}
		mData = newData;
	}
	memcpy(mData+mSize, src, size);
	mSize += size;
	return size;
}

//-------------------------------------------------------------------------
//	PhysxUserFileWriteStream
//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
//	PhysxUserMemoryWriteStream
//-------------------------------------------------------------------------

PhysxUserMemoryReadStream::PhysxUserMemoryReadStream(PxU8* data, PxU32 length)
: mSize(length)
, mData(data)
, mPos(0)
{
}

//////////////////////////////////////////////////////////////////////////

PxU32 PhysxUserMemoryReadStream::read(void* dest, PxU32 count)
{
	PxU32 length = PxMin<PxU32>(count, mSize - mPos);
	memcpy(dest, mData + mPos, length);
	mPos += length;
	return length;
}

//////////////////////////////////////////////////////////////////////////

PxU32 PhysxUserMemoryReadStream::getLength() const
{
	return mSize;
}

//////////////////////////////////////////////////////////////////////////

void PhysxUserMemoryReadStream::seek(PxU32 offset)
{
	mPos = PxMin<PxU32>(mSize, offset);
}

//////////////////////////////////////////////////////////////////////////

PxU32 PhysxUserMemoryReadStream::tell() const
{
	return mPos;
}
