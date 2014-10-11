// abr.cpp : Defines the entry point for the console application.
//

#include <string>
#include <iostream>

#include "abr_util.h"
#include "PngWrite.h"

using namespace std;

bool invert    = false;  // -i
bool png       = false;  // -png
bool pgm       = false;  // -pgm
int  png_level = 9;      // zlib compression level
//bool raw=false;

bool find_key(string key,int argc, char* argv[]);

void invert_brush(GimpBrush * brush)
{
	long data_size = brush->mask->height * brush->mask->width*brush->mask->bytes;

	for (int i = 0; i < data_size;i++)
		brush->mask->data[i]=255-brush->mask->data[i];
}

int main(int argc, char* argv[])
{
	if(argc<=1)
	{
		cerr
			<<"Name:"<<endl
			<<"    abr2prn - Photoshop Brush ripper"<<endl
			<<endl
			<<"Version:"<<endl
			<<"    abr2png v.2.0 (23-oct-2008)"<<endl
			<<endl
			<<"Description:"<<endl
			<<"    abr2png - tool  for   rip  Photoshop   Brush  (.abr)  and"<<endl
			<<"              PaintShopPro (.jbr) to Portable Grayscale (.pgm)"<<endl
			<<"              or  Portable  Network  Graphics  (.png)  format."<<endl
			<<endl
			<<"Synopsys:"<<endl
			<<"    abr2png [options] [file.abr]"<<endl
			<<endl
			<<"Options:"<<endl
			<<"    -i    invert image"<<endl
			<<"    -pgm  output to pgm file format"<<endl
			<<"    -png  output to png file format (default)"<<endl
			<<"    -c0"<<endl
			<<"    ..."<<endl
			<<"    -c9   compression level. -c0 - no compression"<<endl
			<<"                             -c1 - best speed"<<endl
			<<"                             -c9 - best compression (default)"<<endl
			<<"Exam.:"<<endl
			<<"    abr2png -png -c9 brush.abr "<<endl
			<<"        will be created files (png, compression level = 9): "<<endl
			<<"            brush_0001.png,"<<endl
			<<"            brush_0002.png,"<<endl
			<<"            ...,"<<endl
			<<"            brush_000n.png (n is brush count)" <<endl
			<<endl
			<<"Copyright (c) 2008, D.Gar'kaev aka Dickobraz (dickobraz@mail.ru)"<<endl;

		return 1;
	}

	invert=find_key("-i",argc,argv);
	png=find_key("-png",argc,argv);
	pgm=find_key("-pgm",argc,argv);
	if (png==false && pgm==false) png=true;
	if (find_key("-c0",argc,argv)) png_level=0;
	if (find_key("-c1",argc,argv)) png_level=1;
	if (find_key("-c2",argc,argv)) png_level=2;
	if (find_key("-c3",argc,argv)) png_level=3;
	if (find_key("-c4",argc,argv)) png_level=4;
	if (find_key("-c5",argc,argv)) png_level=5;
	if (find_key("-c6",argc,argv)) png_level=6;
	if (find_key("-c7",argc,argv)) png_level=7;
	if (find_key("-c8",argc,argv)) png_level=8;
	if (find_key("-c9",argc,argv)) png_level=9;


	brush_list_t::iterator list_iter;
	char *file_name=argv[argc-1];
	brush_list_t* b_l = brush_load_abr((const char *)file_name);
	if (!b_l)
		return 1;
	GimpBrush *gbrush;
	int data_size;
	FILE *fout;
	std::string fname;
	char head[128]={0};
	int index=0;
	char s_index[128]={0};

	for(list_iter = b_l->begin(); list_iter != b_l->end(); list_iter++)
	{

		gbrush=*list_iter;
		data_size=gbrush->mask->height*gbrush->mask->width*gbrush->mask->bytes;
		sprintf(s_index,"%03d",index++);
		if(pgm)
		{
			fname=std::string((char *)file_name)+std::string("_")+std::string(s_index)+std::string(".pgm");
			fout=fopen(fname.c_str(),"wb");
			if(fout)
			{
				sprintf(head,"P5 %d %d 255\n",gbrush->mask->width,gbrush->mask->height);
				fwrite(head,strlen(head),1,fout);
				invert_brush(gbrush);
				fwrite(gbrush->mask->data,data_size,1,fout);
				fclose(fout);
			}
		}

		if(png)
		{
			fname=std::string((char *)file_name)+std::string("_")+std::string(s_index)+std::string(".png");
			if (invert)
				invert_brush(gbrush);
			bool r = WritePNG(gbrush->mask->width, gbrush->mask->height, gbrush->mask->data, 1, COLOR_GRAY, png_level, fname.c_str());
		}

	}
	return 0;
}

bool find_key(string key, int argc, char* argv[])
{
	bool key_ok=false;
	for (int i=0;i<argc;i++)
	{
		string par(argv[i]);
		if (par==key)
		{
			key_ok=true;
			break;
		}
	}
	return key_ok;
}
