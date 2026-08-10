//=========================================================================
//	Module: PhysXRepXHelpers.h
//	Copyright (C) 2011.
//=========================================================================

#pragma once

//////////////////////////////////////////////////////////////////////////

#include "foundation/PxIO.h"
#include "foundation/PxAllocatorCallback.h"
#include <Platform/File.h>

//////////////////////////////////////////////////////////////////////////

namespace physx
{
	namespace repx
	{
		class RepXCollection;
	}
}
//////////////////////////////////////////////////////////////////////////

physx::repx::RepXCollection* loadCollection(const char* inPath, physx::PxAllocatorCallback& inCallback);

//////////////////////////////////////////////////////////////////////////

class PhysxUserFileReadStream : public physx::PxInputStream
{
public:
	explicit PhysxUserFileReadStream(const char* filename)
		: fpr(
			engine::platform::Path(filename),
			engine::platform::FileAccess::Read,
			engine::platform::FileCreation::OpenExisting)
	{
	}

	~PhysxUserFileReadStream() override = default;

	physx::PxU32 read(void* buffer, physx::PxU32 size) override
	{
		const engine::platform::FileIoResult result = fpr.Read(buffer, size);
		return static_cast<physx::PxU32>(result.bytesTransferred);
	}

	engine::platform::File fpr;
};

//////////////////////////////////////////////////////////////////////////

class PhysxUserMemoryReadStream: public physx::PxInputStream
{
public:
	PhysxUserMemoryReadStream(physx::PxU8* data, physx::PxU32 length);

	physx::PxU32	read(void* dest, physx::PxU32 count);
	physx::PxU32	getLength() const;
	void			seek(physx::PxU32 pos);
	physx::PxU32	tell() const;

private:
	physx::PxU32		mSize;
	const physx::PxU8*	mData;
	physx::PxU32		mPos;
};

//////////////////////////////////////////////////////////////////////////

class PhysxUserFileWriteStream: public physx::PxOutputStream
{
public:
	explicit PhysxUserFileWriteStream(const char* fileName)
		: fpw(
			engine::platform::Path(fileName),
			engine::platform::FileAccess::Write,
			engine::platform::FileCreation::CreateAlways)
	{
	}

	~PhysxUserFileWriteStream() override = default;

	physx::PxU32 write(const void* src, physx::PxU32 count) override
	{
		const engine::platform::FileIoResult result = fpw.Write(src, count);
		return static_cast<physx::PxU32>(result.bytesTransferred);
	}
	engine::platform::File fpw;
};

//////////////////////////////////////////////////////////////////////////

class PhysxUserMemoryWriteStream: public physx::PxOutputStream
{
public:
				 PhysxUserMemoryWriteStream();
	virtual		~PhysxUserMemoryWriteStream();

	physx::PxU32	write(const void* src, physx::PxU32 count);

	physx::PxU32	getSize()	const	{ return mSize; }
	physx::PxU8*	getData()	const	{ return mData; }
private:
	physx::PxU8*	mData;
	physx::PxU32	mSize;
	physx::PxU32	mCapacity;
};
