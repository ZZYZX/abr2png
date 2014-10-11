#include "PngWrite.h"
#include "png.h"

bool WritePNG(int m_width, int m_height, unsigned char* m_pData, int m_bytesPerPixel, colorType_e m_colorType, int m_clevel, const char* in_sName)
{
  int colorType, bitDepthPerChannel;

  FILE *pFile = NULL;	
  pFile = fopen(in_sName,"wb");

  if(!pFile)
    return false;

  png_structp pPng = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!pPng)
  {
    fclose(pFile);
    return false;   
  }

  png_infop pPngInfo = png_create_info_struct(pPng);
  if (!pPngInfo) 
  {
    fclose(pFile);
    png_destroy_write_struct(&pPng, NULL);
    return false;   
  }

  // it's a goto (in case png lib hits the bucket)
  if (setjmp(png_jmpbuf(pPng))) 
  {
    fclose(pFile);
    png_destroy_write_struct(&pPng, &pPngInfo);
    return false;
  }

  png_init_io(pPng, pFile);

  // compression level 0(none)-9(best compression) 
  png_set_compression_level(pPng, m_clevel);

  if (m_colorType == COLOR_GRAY)
  {
    bitDepthPerChannel = m_bytesPerPixel*8;
    colorType = PNG_COLOR_TYPE_GRAY;
  }
  else if (m_colorType == COLOR_RGB)
  {
    bitDepthPerChannel = m_bytesPerPixel/3*8;
    colorType = PNG_COLOR_TYPE_RGB;
  }
  else if (m_colorType == COLOR_RGBA)
  {
    bitDepthPerChannel = m_bytesPerPixel/4*8;
    colorType = PNG_COLOR_TYPE_RGB_ALPHA;
  }
  else 
  {
    png_destroy_write_struct(&pPng, &pPngInfo);
    return false;
  }

  png_set_IHDR(pPng, pPngInfo, (ULONG)m_width, (ULONG)m_height,
               bitDepthPerChannel, colorType, PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

  png_write_info(pPng, pPngInfo);

  if(bitDepthPerChannel == 16) //reverse endian order (PNG is Big Endien)
    png_set_swap(pPng);

  int bytesPerRow = m_width*m_bytesPerPixel;
  BYTE* pImgData = m_pData;

  //write non-interlaced buffer  
  for(int row=0; row < m_height; ++row) {
    png_write_row(pPng, pImgData);
    pImgData += bytesPerRow;
  }

  png_write_end(pPng, NULL);

  png_destroy_write_struct(&pPng, &pPngInfo);

  fclose(pFile);

  return true;
}
