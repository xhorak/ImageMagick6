/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                            PPPP    GGGG  X   X                              %
%                            P   P  G       X X                               %
%                            PPPP   G  GG    X                                %
%                            P      G   G   X X                               %
%                            P       GGG   X   X                              %
%                                                                             %
%                                                                             %
%                           PGX JPEG 2000 Format                              %
%                                                                             %
%                              Software Design                                %
%                                   Cristy                                    %
%                                 July 1992                                   %
%                                                                             %
%                                                                             %
%  Copyright 1999-2019 ImageMagick Studio LLC, a non-profit organization      %
%  dedicated to making software imaging solutions freely available.           %
%                                                                             %
%  You may not use this file except in compliance with the License.  You may  %
%  obtain a copy of the License at                                            %
%                                                                             %
%    https://imagemagick.org/script/license.php                               %
%                                                                             %
%  Unless required by applicable law or agreed to in writing, software        %
%  distributed under the License is distributed on an "AS IS" BASIS,          %
%  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   %
%  See the License for the specific language governing permissions and        %
%  limitations under the License.                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%
*/

/*
  Include declarations.
*/
#include "magick/studio.h"
#include "magick/attribute.h"
#include "magick/blob.h"
#include "magick/blob-private.h"
#include "magick/cache.h"
#include "magick/color-private.h"
#include "magick/colormap.h"
#include "magick/colorspace.h"
#include "magick/colorspace-private.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/image.h"
#include "magick/image-private.h"
#include "magick/list.h"
#include "magick/magick.h"
#include "magick/memory_.h"
#include "magick/monitor.h"
#include "magick/monitor-private.h"
#include "magick/quantum-private.h"
#include "magick/static.h"
#include "magick/string_.h"
#include "magick/module.h"

/*
  Forward declarations.
*/
static MagickBooleanType
  WritePGXImage(const ImageInfo *,Image *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s P G X                                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsPGXreturns True if the image format type, identified by the magick
%  string, is PGX.
%
%  The format of the IsPGX method is:
%
%      unsigned int IsPGX(const unsigned char *magick,const size_t length)
%
%  A description of each parameter follows:
%
%    o magick: compare image format pattern against these bytes.
%
%    o length: Specifies the length of the magick string.
%
*/
static unsigned int IsPGX(const unsigned char *magick,const size_t length)
{
  if (length < 5)
    return(MagickFalse);
  if ((memcmp(magick,"PG ML",5) == 0) || (memcmp(magick,"PG LM",5) == 0))
    return(MagickTrue);
  return(MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d P G X I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadPGXImage() reads an image of raw bits in LSB order and returns it.
%  It allocates the memory necessary for the new Image structure and returns
%  a pointer to the new image.
%
%  The format of the ReadPGXImage method is:
%
%      Image *ReadPGXImage(const ImageInfo *image_info,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static Image *ReadPGXImage(const ImageInfo *image_info,ExceptionInfo *exception)
{
  char
    buffer[MaxTextExtent],
    endian[MaxTextExtent],
    sans[MaxTextExtent],
    sign[MaxTextExtent];

  Image
    *image;

  int
    height,
    precision,
    width;

  QuantumInfo
    *quantum_info;

  MagickBooleanType
    status;

  size_t
    length;

  ssize_t
    count,
    y;

  unsigned char
    *pixels;

  /*
    Open image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  image=AcquireImage(image_info);
  status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
  if (status == MagickFalse)
    {
      image=DestroyImageList(image);
      return((Image *) NULL);
    }
  if (ReadBlobString(image,buffer) == (char *) NULL)
    ThrowReaderException(CorruptImageError,"ImproperImageHeader");
  count=(ssize_t) sscanf(buffer,"PG%[ \t]%2s%[ \t+-]%d%[ \t]%d%[ \t]%d",sans,
    endian,sign,&precision,sans,&width,sans,&height);
  if (count != 8)
    ThrowReaderException(CorruptImageError,"ImproperImageHeader");
  image->depth=(size_t) precision;
  if (LocaleCompare(endian,"ML") == 0)
    image->endian=MSBEndian;
  image->columns=(size_t) width;
  image->rows=(size_t) height;
  if ((image->columns == 0) || (image->rows == 0))
    ThrowReaderException(CorruptImageError,"ImproperImageHeader");
  if (image_info->ping != MagickFalse)
    {
      (void) CloseBlob(image);
      return(GetFirstImageInList(image));
    }
  status=SetImageExtent(image,image->columns,image->rows);
  if (status == MagickFalse)
    return(DestroyImageList(image));
  /*
    Convert PGX image.
  */
  (void) SetImageColorspace(image,GRAYColorspace);
  quantum_info=AcquireQuantumInfo(image_info,image);
  if (quantum_info == (QuantumInfo *) NULL)
    ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
  length=GetQuantumExtent(image,quantum_info,GrayQuantum);
  pixels=GetQuantumPixels(quantum_info);
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    const void
      *stream;

    register PixelPacket
      *magick_restrict q;

    q=QueueAuthenticPixels(image,0,y,image->columns,1,exception);
    if (q == (PixelPacket *) NULL)
      break;
    stream=ReadBlobStream(image,length,pixels,&count);
    if (count != (ssize_t) length)
      break;
    (void) ImportQuantumPixels(image,(CacheView *) NULL,quantum_info,
      GrayQuantum,(unsigned char *) stream,exception);
    if (SyncAuthenticPixels(image,exception) == MagickFalse)
      break;
    if (SetImageProgress(image,LoadImageTag,(MagickOffsetType) y,image->rows) == MagickFalse)
      break;
  }
  SetQuantumImageType(image,GrayQuantum);
  quantum_info=DestroyQuantumInfo(quantum_info);
  if (EOFBlob(image) != MagickFalse)
    ThrowFileException(exception,CorruptImageError,"UnexpectedEndOfFile",
      image->filename);
  (void) CloseBlob(image);
  return(GetFirstImageInList(image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r P G X I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterPGXImage() adds attributes for the PGX image format to
%  the list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterPGXImage method is:
%
%      size_t RegisterPGXImage(void)
%
*/
ModuleExport size_t RegisterPGXImage(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("PGX");
  entry->decoder=(DecodeImageHandler *) ReadPGXImage;
  entry->encoder=(EncodeImageHandler *) WritePGXImage;
  entry->magick=(IsImageFormatHandler *) IsPGX;
  entry->adjoin=MagickFalse;
  entry->description=ConstantString("JPEG 2000 uncompressed format");
  entry->magick_module=ConstantString("PGX");
  (void) RegisterMagickInfo(entry);
  return(MagickImageCoderSignature);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r P G X I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterPGXImage() removes format registrations made by the
%  PGX module from the list of supported formats.
%
%  The format of the UnregisterPGXImage method is:
%
%      UnregisterPGXImage(void)
%
*/
ModuleExport void UnregisterPGXImage(void)
{
  (void) UnregisterMagickInfo("PGX");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e P G X I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WritePGXImage() writes an image of raw bits in LSB order to a file.
%
%  The format of the WritePGXImage method is:
%
%      MagickBooleanType WritePGXImage(const ImageInfo *image_info,
%        Image *image)
%
%  A description of each parameter follows.
%
%    o image_info: the image info.
%
%    o image:  The image.
%
*/
static MagickBooleanType WritePGXImage(const ImageInfo *image_info,Image *image)
{
  char
    buffer[MaxTextExtent];

  ExceptionInfo
    *exception;

  MagickBooleanType
    status;

  QuantumInfo
    *quantum_info;

  register const PixelPacket
    *p;

  size_t
    length;

  ssize_t
    count,
    y;

  unsigned char
    *pixels;

  /*
    Open output image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  exception=(&image->exception);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickCoreSignature);
  status=OpenBlob(image_info,image,WriteBinaryBlobMode,exception);
  if (status == MagickFalse)
    return(status);
  (void) FormatLocaleString(buffer,MaxTextExtent,"PG ML + %g %g %g\n",
    (double) image->depth,(double) image->columns,(double) image->rows);
  (void) WriteBlob(image,strlen(buffer),(unsigned char *) buffer);
  (void) TransformImageColorspace(image,sRGBColorspace);
  quantum_info=AcquireQuantumInfo(image_info,image);
  pixels=(unsigned char *) GetQuantumPixels(quantum_info);
  for (y=0; y < (ssize_t) image->rows; y++)
  {
    p=GetVirtualPixels(image,0,y,image->columns,1,exception);
    if (p == (const PixelPacket *) NULL)
      break;
    length=ExportQuantumPixels(image,(CacheView *) NULL,quantum_info,
      GrayQuantum,pixels,exception);
    count=WriteBlob(image,length,pixels);
    if (count != (ssize_t) length)
      break;
    count=WriteBlob(image,(size_t) (-(ssize_t) length) & 0x01,pixels);
    status=SetImageProgress(image,SaveImageTag,(MagickOffsetType) y,
      image->rows);
    if (status == MagickFalse)
      break;
  }
  quantum_info=DestroyQuantumInfo(quantum_info);
  if (y < (ssize_t) image->rows)
    ThrowWriterException(CorruptImageError,"UnableToWriteImageData");
  (void) CloseBlob(image);
  return(status);
}
