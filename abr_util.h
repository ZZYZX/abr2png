#ifndef __ABR_UTIL_H__
#define __ABR_UTIL_H__

#include <list>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

//################################ GIMP + GLib Types ########################################
#define gint int
#define guint unsigned int
#define guchar unsigned char
#define gchar char
#define gdouble double
#define gint16 char
#define guint16 unsigned char
#define gint32 int
#define guint32 unsigned int
#define glong long
#define gshort short
typedef unsigned int gsize;

#define  g_slice_new(type)      ((type*) malloc (sizeof (type)))
#define g_new(struct_type, n_structs)		\
    ((struct_type *) malloc (((gsize) sizeof (struct_type)) * ((gsize) (n_structs))))
#define g_free(val) free(val) 
//############################# GIMP + GLIB Stuff ##################################################

struct  _GObject
{
//  GTypeInstance  g_type_instance;
  
//  volatile guint ref_count;
//  GData         *qdata;
};


typedef struct _GObject                  GObject;

struct _GimpObject
{
  GObject  parent_instance;

  gchar   *name;

  /*<  private  >*/
  gchar   *normalized;
  guint    static_name ;
  guint    disconnected;
};

typedef struct _GimpObject          GimpObject;

struct _GimpViewable
{
  GimpObject  parent_instance;
  /*<  private  >*/
  gchar      *stock_id;
};

typedef struct _GimpViewable        GimpViewable;

typedef guint32 GQuark;

struct _GimpVector2
{
  gdouble x, y;
};

typedef struct _GimpVector2             GimpVector2;

struct _TempBuf
{
  gint    bytes;      /*  the necessary info                             */
  gint    width;
  gint    height;
  gint    x, y;       /*  origin of data source                          */
  guchar *data;       /*  The data buffer. Do never access this field
                          directly, use temp_buf_data() instead !!       */
};

typedef struct _TempBuf             TempBuf;

TempBuf * temp_buf_new (gint, gint, gint, gint , gint , const guchar *);
guchar * temp_buf_data (TempBuf *);


struct _GimpData
{
  GimpViewable  parent_instance;

  gchar        *filename;
  GQuark        mime_type;
  guint         writable  : 1;
  guint         deletable : 1;
  guint         dirty     : 1;
  guint         internal  : 1;
  gint          freeze_count;
  __time_t      mtime;
};

typedef struct _GimpData             GimpData;

struct _GimpBrush
{
  GimpData      parent_instance;

  TempBuf      *mask;       /*  the actual mask                */
  TempBuf      *pixmap;     /*  optional pixmap data           */

  gint          spacing;    /*  brush's spacing                */
  GimpVector2   x_axis;     /*  for calculating brush spacing  */
  GimpVector2   y_axis;     /*  for calculating brush spacing  */
};

typedef struct _GimpBrush GimpBrush;



struct _GError
{
  GQuark       domain;
  gint         code;
  gchar       *message;
};

typedef struct _GError GError;
//######################################################################################################

typedef std::list<GimpBrush*> brush_list_t;

#define ByteSwap16(n) \
( ((((unsigned int) n) << 8) & 0xFF00) | \
((((unsigned int) n) >> 8) & 0x00FF) )

#define ByteSwap32(n) \
( ((((unsigned long) n) << 24) & 0xFF000000) | \
((((unsigned long) n) << 8) & 0x00FF0000) | \
((((unsigned long) n) >> 8) & 0x0000FF00) | \
((((unsigned long) n) >> 24) & 0x000000FF) )

typedef struct _AbrHeader               AbrHeader;
typedef struct _AbrBrushHeader          AbrBrushHeader;
typedef struct _AbrSampledBrushHeader   AbrSampledBrushHeader;

struct _AbrHeader
{
  char version;
  char count;
};

struct _AbrBrushHeader
{
  char type;
  int size;
};

struct _AbrSampledBrushHeader
{
  int   misc;
  char   spacing;
  char		antialiasing;
  char   bounds[4];
  int   bounds_long[4];
  char   depth;
  bool		wide;
};

brush_list_t* brush_load_abr(const char *);

static char abr_read_short (FILE *);
static int abr_read_long (FILE *);
static char abr_read_char (FILE *);

static bool abr_supported (AbrHeader *);
static brush_list_t* brush_load_abr_v6 (FILE*, AbrHeader*, const char*, GError**);
static brush_list_t* brush_load_abr_v12(FILE*, AbrHeader*, const gchar*, GError**);

static bool abr_reach_8bim_section (FILE*, const char *);

static int abr_rle_decode (FILE*, char*, int);

static GimpBrush * brush_load_abr_brush_v6 (FILE*, AbrHeader*, gint32, gint, const gchar*, GError **);
static GimpBrush * brush_load_abr_brush_v12(FILE*, AbrHeader*, gint, const gchar *, GError **);

static gchar * abr_read_ucs2_text (FILE *);

#endif
