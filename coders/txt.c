/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                            TTTTT  X   X  TTTTT                              %
%                              T     X X     T                                %
%                              T      X      T                                %
%                              T     X X     T                                %
%                              T    X   X    T                                %
%                                                                             %
%                                                                             %
%                      Render Text Onto A Canvas Image.                       %
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
#include "magick/annotate.h"
#include "magick/attribute.h"
#include "magick/blob.h"
#include "magick/blob-private.h"
#include "magick/cache.h"
#include "magick/color.h"
#include "magick/color-private.h"
#include "magick/colorspace.h"
#include "magick/constitute.h"
#include "magick/draw.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/geometry.h"
#include "magick/image.h"
#include "magick/image-private.h"
#include "magick/list.h"
#include "magick/magick.h"
#include "magick/memory_.h"
#include "magick/monitor.h"
#include "magick/monitor-private.h"
#include "magick/option.h"
#include "magick/pixel-accessor.h"
#include "magick/pixel-private.h"
#include "magick/quantum-private.h"
#include "magick/static.h"
#include "magick/statistic.h"
#include "magick/string_.h"
#include "magick/module.h"

/*
  Forward declarations.
*/
static MagickBooleanType
  WriteTXTImage(const ImageInfo *,Image *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s T X T                                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsTXT() returns MagickTrue if the image format type, identified by the magick
%  string, is TXT.
%
%  The format of the IsTXT method is:
%
%      MagickBooleanType IsTXT(const unsigned char *magick,const size_t length)
%
%  A description of each parameter follows:
%
%    o magick: compare image format pattern against these bytes.
%
%    o length: Specifies the length of the magick string.
%
*/
static MagickBooleanType IsTXT(const unsigned char *magick,const size_t length)
{
#define MagickID  "# ImageMagick pixel enumeration:"

  char
    colorspace[MaxTextExtent];

  ssize_t
    count;

  unsigned long
    columns,
    depth,
    rows;

  if (length < 40)
    return(MagickFalse);
  if (LocaleNCompare((const char *) magick,MagickID,strlen(MagickID)) != 0)
    return(MagickFalse);
  count=(ssize_t) sscanf((const char *) magick+32,"%lu,%lu,%lu,%32s",&columns,
    &rows,&depth,colorspace);
  if (count != 4)
    return(MagickFalse);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d T E X T I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadTEXTImage() reads a text file and returns it as an image.  It
%  allocates the memory necessary for the new Image structure and returns a
%  pointer to the new image.
%
%  The format of the ReadTEXTImage method is:
%
%      Image *ReadTEXTImage(const ImageInfo *image_info,Image *image,
%        char *text,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o image: the image.
%
%    o text: the text storage buffer.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static Image *ReadTEXTImage(const ImageInfo *image_info,
  ExceptionInfo *exception)
{
  char
    filename[MaxTextExtent],
    geometry[MaxTextExtent],
    *p,
    text[MaxTextExtent];

  DrawInfo
    *draw_info;

  Image
    *image,
    *texture;

  MagickBooleanType
    status;

  PointInfo
    delta;

  RectangleInfo
    page;

  ssize_t
    offset;

  TypeMetric
    metrics;

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
  (void) memset(text,0,sizeof(text));
  (void) ReadBlobString(image,text);
  /*
    Set the page geometry.
  */
  delta.x=DefaultResolution;
  delta.y=DefaultResolution;
  if ((image->x_resolution == 0.0) || (image->y_resolution == 0.0))
    {
      GeometryInfo
        geometry_info;

      MagickStatusType
        flags;

      flags=ParseGeometry(PSDensityGeometry,&geometry_info);
      image->x_resolution=geometry_info.rho;
      image->y_resolution=geometry_info.sigma;
      if ((flags & SigmaValue) == 0)
        image->y_resolution=image->x_resolution;
    }
  page.width=612;
  page.height=792;
  page.x=43;
  page.y=43;
  if (image_info->page != (char *) NULL)
    (void) ParseAbsoluteGeometry(image_info->page,&page);
  /*
    Initialize Image structure.
  */
  image->columns=(size_t) floor((((double) page.width*image->x_resolution)/
    delta.x)+0.5);
  image->rows=(size_t) floor((((double) page.height*image->y_resolution)/
    delta.y)+0.5);
  status=SetImageExtent(image,image->columns,image->rows);
  if (status != MagickFalse)
    status=ResetImagePixels(image,&image->exception);
  if (status == MagickFalse)
    {
      InheritException(exception,&image->exception);
      return(DestroyImageList(image));
    }
  image->page.x=0;
  image->page.y=0;
  texture=(Image *) NULL;
  if (image_info->texture != (char *) NULL)
    {
      ImageInfo
        *read_info;

      read_info=CloneImageInfo(image_info);
      SetImageInfoBlob(read_info,(void *) NULL,0);
      (void) CopyMagickString(read_info->filename,image_info->texture,
        MaxTextExtent);
      texture=ReadImage(read_info,exception);
      read_info=DestroyImageInfo(read_info);
    }
  /*
    Annotate the text image.
  */
  (void) SetImageBackgroundColor(image);
  draw_info=CloneDrawInfo(image_info,(DrawInfo *) NULL);
  (void) CloneString(&draw_info->text,image_info->filename);
  (void) FormatLocaleString(geometry,MaxTextExtent,"%gx%g%+g%+g",(double)
    image->columns,(double) image->rows,(double) page.x,(double) page.y);
  (void) CloneString(&draw_info->geometry,geometry);
  status=GetTypeMetrics(image,draw_info,&metrics);
  if (status == MagickFalse)
    {
      draw_info=DestroyDrawInfo(draw_info);
      ThrowReaderException(TypeError,"UnableToGetTypeMetrics");
    }
  page.y=(ssize_t) ceil((double) page.y+metrics.ascent-0.5);
  (void) FormatLocaleString(geometry,MaxTextExtent,"%gx%g%+g%+g",(double)
    image->columns,(double) image->rows,(double) page.x,(double) page.y);
  (void) CloneString(&draw_info->geometry,geometry);
  (void) CopyMagickString(filename,image_info->filename,MaxTextExtent);
  if (*draw_info->text != '\0')
    *draw_info->text='\0';
  p=text;
  for (offset=2*page.y; p != (char *) NULL; )
  {
    /*
      Annotate image with text.
    */
    (void) ConcatenateString(&draw_info->text,text);
    (void) ConcatenateString(&draw_info->text,"\n");
    offset+=(ssize_t) (metrics.ascent-metrics.descent);
    if (image->previous == (Image *) NULL)
      {
        status=SetImageProgress(image,LoadImageTag,(MagickOffsetType) offset,
          image->rows);
        if (status == MagickFalse)
          break;
      }
    p=ReadBlobString(image,text);
    if ((offset < (ssize_t) image->rows) && (p != (char *) NULL))
      continue;
    if (texture != (Image *) NULL)
      {
        MagickProgressMonitor
          progress_monitor;

        progress_monitor=SetImageProgressMonitor(image,
          (MagickProgressMonitor) NULL,image->client_data);
        (void) TextureImage(image,texture);
        (void) SetImageProgressMonitor(image,progress_monitor,
          image->client_data);
      }
    (void) AnnotateImage(image,draw_info);
    if (p == (char *) NULL)
      break;
    /*
      Page is full-- allocate next image structure.
    */
    *draw_info->text='\0';
    offset=2*page.y;
    AcquireNextImage(image_info,image);
    if (GetNextImageInList(image) == (Image *) NULL)
      {
        status=MagickFalse;
        break;
      }
    image->next->columns=image->columns;
    image->next->rows=image->rows;
    image=SyncNextImageInList(image);
    (void) CopyMagickString(image->filename,filename,MaxTextExtent);
    (void) SetImageBackgroundColor(image);
    status=SetImageProgress(image,LoadImagesTag,TellBlob(image),
      GetBlobSize(image));
    if (status == MagickFalse)
      break;
  }
  if (texture != (Image *) NULL)
    {
      MagickProgressMonitor
        progress_monitor;

      progress_monitor=SetImageProgressMonitor(image,
        (MagickProgressMonitor) NULL,image->client_data);
      (void) TextureImage(image,texture);
      (void) SetImageProgressMonitor(image,progress_monitor,image->client_data);
    }
  (void) AnnotateImage(image,draw_info);
  if (texture != (Image *) NULL)
    texture=DestroyImage(texture);
  draw_info=DestroyDrawInfo(draw_info);
  (void) CloseBlob(image);
  if (status == MagickFalse)
    return(DestroyImageList(image));
  return(GetFirstImageInList(image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d T X T I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadTXTImage() reads a text file and returns it as an image.  It allocates
%  the memory necessary for the new Image structure and returns a pointer to
%  the new image.
%
%  The format of the ReadTXTImage method is:
%
%      Image *ReadTXTImage(const ImageInfo *image_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: the image info.
%
%    o exception: return any errors or warnings in this structure.
%
*/
static Image *ReadTXTImage(const ImageInfo *image_info,ExceptionInfo *exception)
{
  char
    colorspace[MaxTextExtent],
    text[MaxTextExtent];

  double
    max_value,
    x_offset,
    y_offset;

  Image
    *image;

  IndexPacket
    *indexes;

  MagickBooleanType
    status;

  MagickPixelPacket
    pixel;

  QuantumAny
    range;

  register ssize_t
    i,
    x;

  register PixelPacket
    *q;

  ssize_t
    count,
    type,
    y;

  unsigned long
    depth,
    height,
    width;

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
  (void) memset(text,0,sizeof(text));
  (void) ReadBlobString(image,text);
  if (LocaleNCompare((char *) text,MagickID,strlen(MagickID)) != 0)
    ThrowReaderException(CorruptImageError,"ImproperImageHeader");
  x_offset=(-1.0);
  y_offset=(-1.0);
  do
  {
    width=0;
    height=0;
    max_value=0.0;
    *colorspace='\0';
    count=(ssize_t) sscanf(text+32,"%lu,%lu,%lf,%32s",&width,&height,&max_value,
      colorspace);
    if ((count != 4) || (width == 0) || (height == 0) || (max_value == 0.0))
      ThrowReaderException(CorruptImageError,"ImproperImageHeader");
    image->columns=width;
    image->rows=height;
    if ((max_value == 0.0) || (max_value > 18446744073709551615.0))
      ThrowReaderException(CorruptImageError,"ImproperImageHeader");
    for (depth=1; (GetQuantumRange(depth)+1.0) < max_value; depth++) ;
    image->depth=depth;
    status=SetImageExtent(image,image->columns,image->rows);
    if (status != MagickFalse)
      status=ResetImagePixels(image,&image->exception);
    if (status == MagickFalse)
      {
        InheritException(exception,&image->exception);
        return(DestroyImageList(image));
      }
    LocaleLower(colorspace);
    i=(ssize_t) strlen(colorspace)-1;
    image->matte=MagickFalse;
    if ((i > 0) && (colorspace[i] == 'a'))
      {
        colorspace[i]='\0';
        image->matte=MagickTrue;
      }
    type=ParseCommandOption(MagickColorspaceOptions,MagickFalse,colorspace);
    if (type < 0)
      ThrowReaderException(CorruptImageError,"ImproperImageHeader");
    image->colorspace=(ColorspaceType) type;
    (void) memset(&pixel,0,sizeof(pixel));
    (void) SetImageBackgroundColor(image);
    range=GetQuantumRange(image->depth);
    status=MagickTrue;
    for (y=0; y < (ssize_t) image->rows; y++)
    {
      double
        blue,
        green,
        index,
        opacity,
        red;

      if (status == MagickFalse)
        break;
      red=0.0;
      green=0.0;
      blue=0.0;
      index=0.0;
      opacity=0.0;
      for (x=0; x < (ssize_t) image->columns; x++)
      {
        if (ReadBlobString(image,text) == (char *) NULL)
          {
            status=MagickFalse;
            break;
          }
        switch (image->colorspace)
        {
          case LinearGRAYColorspace:
          case GRAYColorspace:
          {
            if (image->matte != MagickFalse)
              {
                (void) sscanf(text,"%lf,%lf: (%lf%*[%,]%lf%*[%,]",&x_offset,
                  &y_offset,&red,&opacity);
                green=red;
                blue=red;
                break;
              }
            (void) sscanf(text,"%lf,%lf: (%lf%*[%,]",&x_offset,&y_offset,&red);
            green=red;
            blue=red;
            break;
          }
          case CMYKColorspace:
          {
            if (image->matte != MagickFalse)
              {
                (void) sscanf(text,
                  "%lf,%lf: (%lf%*[%,]%lf%*[%,]%lf%*[%,]%lf%*[%,]%lf%*[%,]",
                  &x_offset,&y_offset,&red,&green,&blue,&index,&opacity);
                break;
              }
            (void) sscanf(text,
              "%lf,%lf: (%lf%*[%,]%lf%*[%,]%lf%*[%,]%lf%*[%,]",&x_offset,
              &y_offset,&red,&green,&blue,&index);
            break;
          }
          default:
          {
            if (image->matte != MagickFalse)
              {
                (void) sscanf(text,
                  "%lf,%lf: (%lf%*[%,]%lf%*[%,]%lf%*[%,]%lf%*[%,]",
                  &x_offset,&y_offset,&red,&green,&blue,&opacity);
                break;
              }
            (void) sscanf(text,"%lf,%lf: (%lf%*[%,]%lf%*[%,]%lf%*[%,]",
              &x_offset,&y_offset,&red,&green,&blue);
            break;
          }
        }
        if (strchr(text,'%') != (char *) NULL)
          {
            red*=0.01*range;
            green*=0.01*range;
            blue*=0.01*range;
            index*=0.01*range;
            opacity*=0.01*range;
          }
        if (image->colorspace == LabColorspace)
          {
            green+=(range+1)/2.0;
            blue+=(range+1)/2.0;
          }
        pixel.red=(MagickRealType) ScaleAnyToQuantum((QuantumAny)
          MagickMax(red+0.5,0.0),range);
        pixel.green=(MagickRealType) ScaleAnyToQuantum((QuantumAny)
          MagickMax(green+0.5,0.0),range);
        pixel.blue=(MagickRealType) ScaleAnyToQuantum((QuantumAny)
          MagickMax(blue+0.5,0.0),range);
        pixel.index=(MagickRealType) ScaleAnyToQuantum((QuantumAny)
          MagickMax(index+0.5,0.0),range);
        pixel.opacity=(MagickRealType) ScaleAnyToQuantum((QuantumAny)
          MagickMax(opacity+0.5,0.0),range);
        q=GetAuthenticPixels(image,(ssize_t) x_offset,(ssize_t) y_offset,1,1,
          exception);
        if (q == (PixelPacket *) NULL)
          continue;
        SetPixelRed(q,pixel.red);
        SetPixelGreen(q,pixel.green);
        SetPixelBlue(q,pixel.blue);
        if (image->colorspace == CMYKColorspace)
          {
            indexes=GetAuthenticIndexQueue(image);
            SetPixelIndex(indexes,pixel.index);
          }
        if (image->matte != MagickFalse)
          SetPixelAlpha(q,pixel.opacity);
        if (SyncAuthenticPixels(image,exception) == MagickFalse)
          {
            status=MagickFalse;
            break;
          }
      }
    }
    if (status == MagickFalse)
      break;
    *text='\0';
    (void) ReadBlobString(image,text);
    if (LocaleNCompare((char *) text,MagickID,strlen(MagickID)) == 0)
      {
        /*
          Allocate next image structure.
        */
        AcquireNextImage(image_info,image);
        if (GetNextImageInList(image) == (Image *) NULL)
          {
            status=MagickFalse;
            break;
          }
        image=SyncNextImageInList(image);
        status=SetImageProgress(image,LoadImagesTag,TellBlob(image),
          GetBlobSize(image));
        if (status == MagickFalse)
          break;
      }
  } while (LocaleNCompare((char *) text,MagickID,strlen(MagickID)) == 0);
  (void) CloseBlob(image);
  if (status == MagickFalse)
    return(DestroyImageList(image));
  return(GetFirstImageInList(image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r T X T I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterTXTImage() adds attributes for the TXT image format to the
%  list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterTXTImage method is:
%
%      size_t RegisterTXTImage(void)
%
*/
ModuleExport size_t RegisterTXTImage(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("SPARSE-COLOR");
  entry->encoder=(EncodeImageHandler *) WriteTXTImage;
  entry->raw=MagickTrue;
  entry->endian_support=MagickTrue;
  entry->description=ConstantString("Sparse Color");
  entry->magick_module=ConstantString("TXT");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("TEXT");
  entry->decoder=(DecodeImageHandler *) ReadTEXTImage;
  entry->encoder=(EncodeImageHandler *) WriteTXTImage;
  entry->raw=MagickTrue;
  entry->endian_support=MagickTrue;
  entry->format_type=ImplicitFormatType;
  entry->description=ConstantString("Text");
  entry->magick_module=ConstantString("TXT");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("TXT");
  entry->decoder=(DecodeImageHandler *) ReadTXTImage;
  entry->encoder=(EncodeImageHandler *) WriteTXTImage;
  entry->description=ConstantString("Text");
  entry->magick=(IsImageFormatHandler *) IsTXT;
  entry->magick_module=ConstantString("TXT");
  (void) RegisterMagickInfo(entry);
  return(MagickImageCoderSignature);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r T X T I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterTXTImage() removes format registrations made by the
%  TXT module from the list of supported format.
%
%  The format of the UnregisterTXTImage method is:
%
%      UnregisterTXTImage(void)
%
*/
ModuleExport void UnregisterTXTImage(void)
{
  (void) UnregisterMagickInfo("TEXT");
  (void) UnregisterMagickInfo("TXT");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e T X T I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteTXTImage writes the pixel values as text numbers.
%
%  The format of the WriteTXTImage method is:
%
%      MagickBooleanType WriteTXTImage(const ImageInfo *image_info,Image *image)
%
%  A description of each parameter follows.
%
%    o image_info: the image info.
%
%    o image:  The image.
%
*/
static MagickBooleanType WriteTXTImage(const ImageInfo *image_info,Image *image)
{
  char
    buffer[MaxTextExtent],
    colorspace[MaxTextExtent],
    tuple[MaxTextExtent];

  MagickBooleanType
    status;

  MagickOffsetType
    scene;

  MagickPixelPacket
    pixel;

  register const IndexPacket
    *indexes;

  register const PixelPacket
    *p;

  register ssize_t
    x;

  size_t
    imageListLength;

  ssize_t
    y;

  /*
    Open output image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickCoreSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickCoreSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  status=OpenBlob(image_info,image,WriteBlobMode,&image->exception);
  if (status == MagickFalse)
    return(status);
  scene=0;
  imageListLength=GetImageListLength(image);
  do
  {
    ComplianceType
      compliance;

    const char
      *value;

    (void) CopyMagickString(colorspace,CommandOptionToMnemonic(
      MagickColorspaceOptions,(ssize_t) image->colorspace),MaxTextExtent);
    LocaleLower(colorspace);
    image->depth=GetImageQuantumDepth(image,MagickTrue);
    if (image->matte != MagickFalse)
      (void) ConcatenateMagickString(colorspace,"a",MaxTextExtent);
    compliance=NoCompliance;
    value=GetImageOption(image_info,"txt:compliance");
    if (value != (char *) NULL)
      compliance=(ComplianceType) ParseCommandOption(MagickComplianceOptions,
        MagickFalse,value);
    if (LocaleCompare(image_info->magick,"SPARSE-COLOR") != 0)
      {
        size_t
          depth;

        depth=compliance == SVGCompliance ? image->depth :
          MAGICKCORE_QUANTUM_DEPTH;
        (void) FormatLocaleString(buffer,MaxTextExtent,
          "# ImageMagick pixel enumeration: %.20g,%.20g,%.20g,%s\n",(double)
          image->columns,(double) image->rows,(double) GetQuantumRange(depth),
          colorspace);
        (void) WriteBlobString(image,buffer);
      }
    GetMagickPixelPacket(image,&pixel);
    for (y=0; y < (ssize_t) image->rows; y++)
    {
      p=GetVirtualPixels(image,0,y,image->columns,1,&image->exception);
      if (p == (const PixelPacket *) NULL)
        break;
      indexes=GetVirtualIndexQueue(image);
      for (x=0; x < (ssize_t) image->columns; x++)
      {
        SetMagickPixelPacket(image,p,indexes+x,&pixel);
        if (pixel.colorspace == LabColorspace)
          {
            pixel.green-=(QuantumRange+1)/2.0;
            pixel.blue-=(QuantumRange+1)/2.0;
          }
        if (LocaleCompare(image_info->magick,"SPARSE-COLOR") == 0)
          {
            /*
              Sparse-color format.
            */
            if ((image->matte == MagickFalse) ||
                (GetPixelOpacity(p) == (Quantum) OpaqueOpacity))
              {
                GetColorTuple(&pixel,MagickFalse,tuple);
                (void) FormatLocaleString(buffer,MaxTextExtent,"%.20g,%.20g,",
                  (double) x,(double) y);
                (void) WriteBlobString(image,buffer);
                (void) WriteBlobString(image,tuple);
                (void) WriteBlobString(image," ");
              }
            p++;
            continue;
          }
        (void) FormatLocaleString(buffer,MaxTextExtent,"%.20g,%.20g: ",(double)
          x,(double) y);
        (void) WriteBlobString(image,buffer);
        (void) CopyMagickString(tuple,"(",MaxTextExtent);
        ConcatenateColorComponent(&pixel,RedChannel,compliance,tuple);
        (void) ConcatenateMagickString(tuple,",",MaxTextExtent);
        ConcatenateColorComponent(&pixel,GreenChannel,compliance,tuple);
        (void) ConcatenateMagickString(tuple,",",MaxTextExtent);
        ConcatenateColorComponent(&pixel,BlueChannel,compliance,tuple);
        if (pixel.colorspace == CMYKColorspace)
          {
            (void) ConcatenateMagickString(tuple,",",MaxTextExtent);
            ConcatenateColorComponent(&pixel,IndexChannel,compliance,tuple);
          }
        if (pixel.matte != MagickFalse)
          {
            (void) ConcatenateMagickString(tuple,",",MaxTextExtent);
            ConcatenateColorComponent(&pixel,AlphaChannel,compliance,tuple);
          }
        (void) ConcatenateMagickString(tuple,")",MaxTextExtent);
        (void) WriteBlobString(image,tuple);
        (void) WriteBlobString(image,"  ");
        GetColorTuple(&pixel,MagickTrue,tuple);
        (void) FormatLocaleString(buffer,MaxTextExtent,"%s",tuple);
        (void) WriteBlobString(image,buffer);
        (void) WriteBlobString(image,"  ");
        (void) QueryMagickColorname(image,&pixel,SVGCompliance,tuple,
          &image->exception);
        (void) WriteBlobString(image,tuple);
        (void) WriteBlobString(image,"\n");
        p++;
      }
      status=SetImageProgress(image,SaveImageTag,(MagickOffsetType) y,
        image->rows);
      if (status == MagickFalse)
        break;
    }
    if (GetNextImageInList(image) == (Image *) NULL)
      break;
    image=SyncNextImageInList(image);
    status=SetImageProgress(image,SaveImagesTag,scene++,imageListLength);
    if (status == MagickFalse)
      break;
  } while (image_info->adjoin != MagickFalse);
  (void) CloseBlob(image);
  return(MagickTrue);
}
