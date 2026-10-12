/*
 * MIT License
 *
 * Copyright (c) 2020-2025  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/Zip.h"

#include <libnetq/Limits.h>
#include <libnetq/String.h>

bool ZipLocalFileHeader_parse(struct ZipLocalFileHeader* header, const uint8_t* start, const uint8_t* end, const uint8_t** pend)
{
  uint32_t signature;

  if (end && (end - start) < ZipLocalFileHeaderSize)
    return false;

  NQMemcpy(&signature, start, sizeof(signature));
  if (signature != ZipLocalFileHeaderSig)
    return false;

  header->signature = signature;
  start += sizeof(header->signature);

  NQMemcpy(&header->version, start, sizeof(header->version));
  start += sizeof(header->version);

  NQMemcpy(&header->flags, start, sizeof(header->flags));
  start += sizeof(header->flags);

  NQMemcpy(&header->compression, start, sizeof(header->compression));
  start += sizeof(header->compression);

  NQMemcpy(&header->lastTime, start, sizeof(header->lastTime));
  start += sizeof(header->lastTime);

  NQMemcpy(&header->lastDate, start, sizeof(header->lastDate));
  start += sizeof(header->lastDate);

  NQMemcpy(&header->crc32, start, sizeof(header->crc32));
  start += sizeof(header->crc32);

  NQMemcpy(&header->compressedSize, start, sizeof(header->compressedSize));
  start += sizeof(header->compressedSize);

  NQMemcpy(&header->uncompressedSize, start, sizeof(header->uncompressedSize));
  start += sizeof(header->uncompressedSize);

  NQMemcpy(&header->nameSize, start, sizeof(header->nameSize));
  start += sizeof(header->nameSize);

  NQMemcpy(&header->extraFieldSize, start, sizeof(header->extraFieldSize));
  start += sizeof(header->extraFieldSize);

  if (pend != NULL)
    *pend = start;

  return true;
}

bool ZipDataDescriptor_parse(struct ZipDataDescriptor* desc, const uint8_t* start, const uint8_t* end, const uint8_t** pend)
{
  uint32_t signature;

  if (end && (end - start) < ZipDataDescriptorSize)
    return false;

  NQMemcpy(&signature, start, sizeof(signature));
  if (signature != ZipDataDescriptorSig)
    return false;

  desc->signature = signature;
  start += sizeof(desc->signature);

  NQMemcpy(&desc->crc32, start, sizeof(desc->crc32));
  start += sizeof(desc->crc32);

  NQMemcpy(&desc->compressedSize, start, sizeof(desc->compressedSize));
  start += sizeof(desc->compressedSize);

  NQMemcpy(&desc->uncompressedSize, start, sizeof(desc->uncompressedSize));
  start += sizeof(desc->uncompressedSize);

  if (pend != NULL)
    *pend = start;

  return true;
}

bool ZipDataDescriptor64_parse(const uint8_t* data, size_t size, struct ZipDataDescriptor64* result)
{
  //if (size != ZIP_DATA_DESC64_SIZE)
  //  return false;

  NQMemcpy(&result->crc32, data, sizeof(result->crc32));
  data += sizeof(result->crc32);

  NQMemcpy(&result->compressedSize, data, sizeof(result->compressedSize));
  data += sizeof(result->compressedSize);

  NQMemcpy(&result->uncompressedSize, data, sizeof(result->uncompressedSize));
  data += sizeof(result->uncompressedSize);

  return true;
}

void ZipExtraFields_process(const uint8_t* data, size_t size, struct ZipExtraFields* result)
{
  NQMemset(result, 0, sizeof(*result));

  const uint8_t* start = data;
  const uint8_t* end = data + size;
  while ((start + 4) <= end) {
    uint16_t field;
    NQMemcpy(&field, start, sizeof(field));
    start += sizeof(field);

    uint16_t size;
    NQMemcpy(&size, start, sizeof(size));
    start += sizeof(size);

    if ((start + size) > end)
      break;

    switch (field) {
    case ZIP_ZIP64_FIELD:
      break;

    case ZIP_TIMESTAMP_FIELD:
      if (size == 13) {
        result->timestamp.flags = start[0];
        if (result->timestamp.flags & ZIP_MODIFY_TIME_BIT)
          NQMemcpy(&result->timestamp.modification, &start[1], sizeof(result->timestamp.modification));
        if (result->timestamp.flags & ZIP_ACCESS_TIME_BIT)
          NQMemcpy(&result->timestamp.access, &start[5], sizeof(result->timestamp.access));
        if (result->timestamp.flags & ZIP_CREATE_TIME_BIT)
          NQMemcpy(&result->timestamp.creation, &start[9], sizeof(result->timestamp.creation));
      }
      break;

    case ZIP_UNIXSTAT_FIELD:
      if (size == 11 && start[0] == 1 && start[1] == 4 && start[6] == 4) {
        NQMemcpy(&result->unixstat.uid, &start[2], sizeof(result->unixstat.uid));
        NQMemcpy(&result->unixstat.gid, &start[7], sizeof(result->unixstat.gid));
      }
      break;
    }

    start += size;
  }
}

const uint8_t* ZipEndCentralDir_find(struct ZipEndCentralDir* self, const uint8_t* data, size_t size)
{
  if (size < ZipEndCentralDirSize)
    return NULL;

  const uint8_t* end = data + size;
  const uint8_t* p = end - ZipEndCentralDirSize;
  for (; data <= p; p--) {
    if (p[0] != ((ZipEndCentralDirSig >> 0) % 256))
      continue;
    if (p[1] != ((ZipEndCentralDirSig >> 8) % 256))
      continue;
    if (p[2] != ((ZipEndCentralDirSig >> 16) % 256))
      continue;
    if (p[3] != ((ZipEndCentralDirSig >> 24) % 256))
      continue;

    if ((end - p) > (ZipEndCentralDirSize + NQ_UINT16_MAX))
      return NULL;

    const uint8_t* header = p;
    NQMemcpy(&self->signature, header, sizeof(self->signature));
    header += sizeof(self->signature);
    NQMemcpy(&self->totalCurDisk, header, sizeof(self->totalCurDisk));
    header += sizeof(self->totalCurDisk);
    NQMemcpy(&self->totalDisks, header, sizeof(self->totalDisks));
    header += sizeof(self->totalDisks);
    NQMemcpy(&self->totalCurEntries, header, sizeof(self->totalCurEntries));
    header += sizeof(self->totalCurEntries);
    NQMemcpy(&self->totalEntries, header, sizeof(self->totalEntries));
    header += sizeof(self->totalEntries);
    NQMemcpy(&self->centralDirSize, header, sizeof(self->centralDirSize));
    header += sizeof(self->centralDirSize);
    NQMemcpy(&self->centralDirOffset, header, sizeof(self->centralDirOffset));
    header += sizeof(self->centralDirOffset);
    NQMemcpy(&self->commentLength, header, sizeof(self->commentLength));
    header += sizeof(self->commentLength);

    if ((end - header) != self->commentLength)
      continue;

    return p;
  }

  return NULL;
}

bool ZipCentralDirHeader_parse(struct ZipCentralDirHeader* header, const uint8_t* start, const uint8_t* end, const uint8_t** pend)
{
  if (end && (end - start) < ZipCentralDirHeaderSize)
    return false;

  uint32_t signature;
  NQMemcpy(&signature, start, sizeof(signature));
  if (signature != ZipCentralDirHeaderSig)
    return false;

  header->signature = signature;
  start += sizeof(header->signature);

  NQMemcpy(&header->targetVersion, start, sizeof(header->targetVersion));
  start += sizeof(header->targetVersion);

  NQMemcpy(&header->minVersion, start, sizeof(header->minVersion));
  start += sizeof(header->minVersion);

  NQMemcpy(&header->flags, start, sizeof(header->flags));
  start += sizeof(header->flags);

  NQMemcpy(&header->compression, start, sizeof(header->compression));
  start += sizeof(header->compression);

  NQMemcpy(&header->lastTime, start, sizeof(header->lastTime));
  start += sizeof(header->lastTime);

  NQMemcpy(&header->lastDate, start, sizeof(header->lastDate));
  start += sizeof(header->lastDate);

  NQMemcpy(&header->crc32, start, sizeof(header->crc32));
  start += sizeof(header->crc32);

  NQMemcpy(&header->compressedSize, start, sizeof(header->compressedSize));
  start += sizeof(header->compressedSize);

  NQMemcpy(&header->uncompressedSize, start, sizeof(header->uncompressedSize));
  start += sizeof(header->uncompressedSize);

  NQMemcpy(&header->nameSize, start, sizeof(header->nameSize));
  start += sizeof(header->nameSize);

  NQMemcpy(&header->extraFieldSize, start, sizeof(header->extraFieldSize));
  start += sizeof(header->extraFieldSize);

  NQMemcpy(&header->commentSize, start, sizeof(header->commentSize));
  start += sizeof(header->commentSize);

  NQMemcpy(&header->diskNumberStart, start, sizeof(header->diskNumberStart));
  start += sizeof(header->diskNumberStart);

  NQMemcpy(&header->internalAttributes, start, sizeof(header->internalAttributes));
  start += sizeof(header->internalAttributes);

  NQMemcpy(&header->externalAttributes, start, sizeof(header->externalAttributes));
  start += sizeof(header->externalAttributes);

  NQMemcpy(&header->offset, start, sizeof(header->offset));
  start += sizeof(header->offset);

  if (pend)
    *pend = start;

  return true;
}
