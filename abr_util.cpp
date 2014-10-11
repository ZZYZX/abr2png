#include "abr_util.h"
#include <iostream>

brush_list_t* brush_load_abr(const char *filename)
{
  GError      *error;
  FILE      *file;
  AbrHeader  abr_hdr;
  brush_list_t *brush_list=0;
  file = fopen (filename, "rb");
  abr_hdr.version = abr_read_short (file);
  abr_hdr.count   = abr_read_short (file); /* sub-version for ABR v6 */
  if (abr_supported (&abr_hdr))
  {
	  switch (abr_hdr.version)
	  {
	  case 1:
	  case 2:
		  brush_list = brush_load_abr_v12 (file, &abr_hdr, filename, &error);
		  break;

	  case 6:
		  brush_list = 
			  brush_load_abr_v6 (file, &abr_hdr, filename, &error);
		  break;
	  }
  }

  fclose (file);
return brush_list;
}

static char 
abr_read_short (FILE *file)
{
  char val;
  fread (&val, sizeof (val), 1, file);
  return ByteSwap16(val);
}
static int
abr_read_long (FILE *file)
{
  int val;

  fread (&val, sizeof (val), 1, file);

  return ByteSwap32(val);
}

static char
abr_read_char (FILE *file)
{
  return fgetc (file);
}

static bool
abr_supported (AbrHeader *abr_hdr)
{
  switch (abr_hdr->version)
    {
    case 1:
    case 2:
      return true;
    break;

    case 6:
      /* in this case, count contains format sub-version */
      if(abr_hdr->count == 1 || abr_hdr->count == 2)
        return true;
    break;

    default:
		  std::cerr<<"Brush version "<<abr_hdr->version<<" is not supported."<<std::endl;
    }

  return false;
}

//static GList * 
static brush_list_t * brush_load_abr_v6 (FILE *file,
                   AbrHeader    *abr_hdr,
                   const char  *filename,
                   GError      **error)
{

	brush_list_t* brush_list=0;
  int  sample_section_size;
  int  sample_section_end;
  int    i = 1;

  if (! abr_reach_8bim_section (file, "samp"))
    return brush_list;

  brush_list = new brush_list_t;
  sample_section_size = abr_read_long (file);
  sample_section_end  = sample_section_size + ftell (file);

  while (ftell (file) < sample_section_end)
    {
      GimpBrush *brush;
      GError    *my_error = NULL;

      brush = brush_load_abr_brush_v6 (file, abr_hdr, sample_section_end,
                                            i, filename, &my_error);

      //*  a NULL brush without an error means an unsupported brush
      //*  type was encountered, silently skip it and try the next one
      //*

      if (brush)
        {
          brush_list->push_back(brush);
        }
      else if (my_error)
        {
          //g_propagate_error (error, my_error);
			std::cerr<<my_error->message<<std::endl;
          break;
        }

      i++;
    }

  return brush_list;
  
}

static bool abr_reach_8bim_section (FILE *abr, const char *name)
{
  char  tag[4];
  char  tagname[5];
  int section_size;
  int   r;

  while (! feof (abr))
    {
      r = (int)fread (&tag, 1, 4, abr);
      if (r != 4)
        return false;

      if (strncmp (tag, "8BIM", 4))
        return false;

      r = (int)fread (&tagname, 1, 4, abr);
      if (r != 4)
        return false;

      tagname[4] = '\0';

      if (! strncmp (tagname, name, 4))
        return true;

      section_size = abr_read_long (abr);
      r = fseek (abr, section_size, SEEK_CUR);
      if (r == -1)
        return false;
    }

  return false;
}


static int abr_rle_decode (FILE   *file, char  *buffer, int  height)
{
  char   ch;
  int    i, j, c;
  short *cscanline_len;
  char  *data = buffer;

  /* read compressed size foreach scanline */
  cscanline_len = new short[height];
  for (i = 0; i < height; i++)
    cscanline_len[i] = abr_read_short (file);

  /* unpack each scanline data */
  for (i = 0; i < height; i++)
    {
      for (j = 0; j < cscanline_len[i];)
        {
          int n = abr_read_char (file);

          j++;

          if (n >= 128)     /* force sign */
            n -= 256;

          if (n < 0)
            {
              /* copy the following char -n + 1 times */

              if (n == -128)  /* it's a nop */
                continue;

              n = -n + 1;
              ch = abr_read_char (file);
              j++;

              for (c = 0; c < n; c++, data++)
                *data = ch;
            }
          else
            {
              /* read the following n + 1 chars (no compr) */

              for (c = 0; c < n + 1; c++, j++, data++)
                *data = abr_read_char (file);
            }
        }
    }

  delete cscanline_len;

  return 0;
}


static GimpBrush *
brush_load_abr_brush_v6 (FILE *file,
                    AbrHeader *abr_hdr,
                       gint32 max_offset,
                         gint index,
                  const gchar *filename,
                       GError **error)
{
  GimpBrush *brush = NULL;
  guchar    *mask;

  gint32     brush_size;
  gint32     brush_end;
  gint32     complement_to_4;
  gint32     next_brush;

  gint32     top, left, bottom, right;
  gint16     depth;
  gchar      compress;

  gint32     width, height;
  gint32     size;

  gchar     *tmp;
  gchar     *name;
  gint       r;

  brush_size = abr_read_long (file);
  brush_end = brush_size;
  /* complement to 4 */
  while (brush_end % 4 != 0) brush_end++;
  complement_to_4 = brush_end - brush_size;
  next_brush = ftell (file) + brush_end;

  if (abr_hdr->count == 1)
    /* discard key and short coordinates and unknown short */
    r = fseek (file, 47, SEEK_CUR);
  else
    /* discard key and unknown bytes */
    r = fseek (file, 301, SEEK_CUR);

  if (r == -1)
    {
      //g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
      //             _("Fatal parse error in brush file '%s': "
      //               "File appears truncated."),
      //             gimp_filename_to_utf8 (filename));
		std::cerr<<"Fatal parse error in brush file '"<<filename<<"': File appears truncated."<<std::endl;
      return NULL;
    }

  top      = abr_read_long (file);
  left     = abr_read_long (file);
  bottom   = abr_read_long (file);
  right    = abr_read_long (file);
  depth    = abr_read_short (file);
  compress = abr_read_char (file);

  width  = right - left;
  height = bottom - top;
  size   = width * (depth >> 3) * height;
/*
  tmp = g_filename_display_basename (filename);
  name = g_strdup_printf ("%s-%03d", tmp, index);
  g_free (tmp);

  brush = g_object_new (GIMP_TYPE_BRUSH,
                        "name",      name,
                        //  FIXME: MIME type!!  
                        "mime-type", "application/x-photoshop-abr",
                        NULL);
*/
brush = new GimpBrush;

 // g_free (name);

  brush->spacing  = 25; /* real value needs 8BIMdesc section parser */
  brush->x_axis.x = width / 2.0;
  brush->x_axis.y = 0.0;
  brush->y_axis.x = 0.0;
  brush->y_axis.y = height / 2.0;
  brush->mask     = temp_buf_new (width, height, 1, 0, 0, NULL);

  mask = temp_buf_data (brush->mask);

  /* data decoding */
  if (! compress)
    /* not compressed - read raw bytes as brush data */
    fread (mask, size, 1, file);
  else
    abr_rle_decode (file, (gchar *) mask, height);

  fseek (file, next_brush, SEEK_SET);

  return brush;
}

TempBuf * temp_buf_new (gint width, gint height, gint bytes, gint x, gint y, const guchar *color)
{
  TempBuf *temp;
  //g_return_val_if_fail (width > 0 && height > 0, NULL);
  //g_return_val_if_fail (bytes > 0, NULL);

  temp = g_slice_new (TempBuf);

  temp->width  = width;
  temp->height = height;
  temp->bytes  = bytes;
  temp->x      = x;
  temp->y      = y;

  temp->data = g_new (guchar, width * height * bytes);

  /*  initialize the data  */
  if (color)
    {
      glong i;

      /* First check if we can save a lot of work */
      for (i = 1; i < bytes; i++)
        {
          if (color[0] != color[i])
            break;
        }

      if (i == bytes)
        {
          memset (temp->data, *color, width * height * bytes);
        }
      else /* No, we cannot */
        {
          guchar *dptr = temp->data;

          /* Fill the first row */
          for (i = width - 1; i >= 0; --i)
            {
              const guchar *c = color;
              gint          j = bytes;

              while (j--)
                *dptr++ = *c++;
            }

          /* Now copy from it (we set bytes to bytes-per-row now) */
          bytes *= width;

          while (--height)
            {
              memcpy (dptr, temp->data, bytes);
              dptr += bytes;
            }
        }
    }

  return temp;
}

guchar * temp_buf_data (TempBuf *temp_buf)
{
  return temp_buf->data;
}


static brush_list_t *
brush_load_abr_v12 (FILE *file, AbrHeader    *abr_hdr, const gchar  *filename, GError **error)
{
  brush_list_t  *brush_list = NULL;
  gint   i;

  brush_list = new brush_list_t;

  for (i = 0; i < abr_hdr->count; i++)
    {
      GimpBrush *brush;
      GError    *my_error = NULL;

      brush = brush_load_abr_brush_v12 (file, abr_hdr, i,
                                             filename, &my_error);

      /*  a NULL brush without an error means an unsupported brush
       *  type was encountered, silently skip it and try the next one
       */

      if (brush)
        {
          //brush_list = g_list_prepend (brush_list, brush);
			brush_list->push_back(brush);
        }
      else if (my_error)
        {
          //g_propagate_error (error, my_error);
			std::cerr<<my_error->message<<std::endl;
          break;
        }
    }

  return brush_list;
}

static GimpBrush *
brush_load_abr_brush_v12 (FILE         *file,
                               AbrHeader    *abr_hdr,
                               gint          index,
                               const gchar  *filename,
                               GError      **error)
{
	
  GimpBrush      *brush = NULL;
  AbrBrushHeader  abr_brush_hdr;

  //g_return_val_if_fail (filename != NULL, NULL);
  //g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  abr_brush_hdr.type = abr_read_short (file);
  abr_brush_hdr.size = abr_read_long (file);

  //*  g_print(" + BRUSH\n | << type: %i  block size: %i bytes\n",
  //*          abr_brush_hdr.type, abr_brush_hdr.size);
  //*

  switch (abr_brush_hdr.type)
    {
    case 1: // computed brush 
      //* FIXME: support it!
      //*
      //* We can probabaly feed the info into the generated brush code
      //* and get a useable brush back. It seems to support the same
      //* types -akl
      //
		std::cerr<<"WARNING: computed brush unsupported, skipping.\n";
      fseek (file, abr_brush_hdr.size, SEEK_CUR);
      break;

    case 2: // sampled brush 
      {
        AbrSampledBrushHeader  abr_sampled_brush_hdr;
        gint                   width, height;
        gint                   bytes;
        gint                   size;
        guchar                *mask;
        gint                   i;
        gchar                 *name;
        gchar                 *sample_name = NULL;
        gchar                 *tmp;
        gshort                 compress;

        abr_sampled_brush_hdr.misc    = abr_read_long (file);
        abr_sampled_brush_hdr.spacing = abr_read_short (file);

        if (abr_hdr->version == 2)
          sample_name = abr_read_ucs2_text (file);

        abr_sampled_brush_hdr.antialiasing = abr_read_char (file);

        for (i = 0; i < 4; i++)
          abr_sampled_brush_hdr.bounds[i] = abr_read_short (file);

        for (i = 0; i < 4; i++)
          abr_sampled_brush_hdr.bounds_long[i] = abr_read_long (file);

        abr_sampled_brush_hdr.depth = abr_read_short (file);

        height = (abr_sampled_brush_hdr.bounds_long[2] -
                  abr_sampled_brush_hdr.bounds_long[0]); // bottom - top 
        width  = (abr_sampled_brush_hdr.bounds_long[3] -
                  abr_sampled_brush_hdr.bounds_long[1]); // right - left 
        bytes  = abr_sampled_brush_hdr.depth >> 3;

        // g_print("width %i  height %i\n", width, height);

        abr_sampled_brush_hdr.wide = height > 16384;

        if (abr_sampled_brush_hdr.wide)
          {
            // FIXME: support wide brushes 
			/*
            g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                         _("Fatal parse error in brush file '%s': "
                           "Wide brushes are not supported."),
                         gimp_filename_to_utf8 (filename));
			*/
			  std::cerr<<"Fatal parse error in brush file '"<<filename<<"': Wide brushes are not supported."<<std::endl;
            return NULL;
          }
/*
        tmp = g_filename_display_basename (filename);
        if (! sample_name)
          {
            // build name from filename and index 
            name = g_strdup_printf ("%s-%03d", tmp, index);
          }
        else
          {
            // build name from filename and sample name 
            name = g_strdup_printf ("%s-%s", tmp, sample_name);
            g_free (sample_name);
          }
        g_free (tmp);

        brush = g_object_new (GIMP_TYPE_BRUSH,
                              "name",      name,
                              //  FIXME: MIME type!!  
                              "mime-type", "application/x-photoshop-abr",
                              NULL);

        g_free (name);
*/
		if (sample_name)//заглушка из g_convert
			g_free (sample_name);

		brush = new GimpBrush;
        brush->spacing  = abr_sampled_brush_hdr.spacing;
        brush->x_axis.x = width / 2.0;
        brush->x_axis.y = 0.0;
        brush->y_axis.x = 0.0;
        brush->y_axis.y = height / 2.0;
        brush->mask     = temp_buf_new (width, height, 1, 0, 0, NULL);

        mask = temp_buf_data (brush->mask);
        size = width * height * bytes;

        compress = abr_read_char (file);

        // g_print(" | << size: %dx%d %d bit (%d bytes) %s\n",
        //         width, height, abr_sampled_brush_hdr.depth, size,
        //         comppres ? "compressed" : "raw");
        //

        if (! compress)
          fread (mask, size, 1, file);
        else
          abr_rle_decode (file, (gchar *) mask, height);
      }
      break;

    default:
		std::cerr<<"WARNING: unknown brush type, skipping.\n";
      fseek (file, abr_brush_hdr.size, SEEK_CUR);
      break;
    }

  return brush;
}

static gchar *
abr_read_ucs2_text (FILE *file)
{
  gchar *name_ucs2;
  gchar *name_utf8;
  gint   len;
  gint   i;

  /* two-bytes characters encoded (UCS-2)
   *  format:
   *   long : number of characters in string
   *   data : zero terminated UCS-2 string
   */

  len = 2 * abr_read_long (file);
  if (len <= 0)
    return NULL;

  name_ucs2 = g_new (gchar, len);

  for (i = 0; i < len; i++)
    name_ucs2[i] = abr_read_char (file);
/*
  name_utf8 = g_convert (name_ucs2, len,
                         "UTF-8", "UCS-2BE",
                         NULL, NULL, NULL);
*/
  //HACK
 name_utf8 = g_new (gchar, len);
 memcpy(name_utf8,name_ucs2,len);

  g_free (name_ucs2);

  return name_utf8;
}
