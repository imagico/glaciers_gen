/* ========================================================================
    File: @(#)glaciers_gen.cpp
   ------------------------------------------------------------------------
    glaciers_gen simple glaciers generalization
    Copyright (C) 2013-2014 Christoph Hormann <chris_hormann@gmx.de>
   ------------------------------------------------------------------------

    This file is part of glaciers_gen

    glaciers_gen is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    glaciers_gen is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with glaciers_gen.  If not, see <http://www.gnu.org/licenses/>.

    Version history:

      0.3: initial public version, March 2014

   ========================================================================
 */

const char PROGRAM_TITLE[] = "glaciers_gen version 0.3";

#include <cstdlib>
#include <algorithm>

#define cimg_use_tiff 1
#define cimg_use_png 1
#define cimg_display 0

#include "skeleton.h"
#include "CImg.h"

using namespace cimg_library;


int main(int argc,char **argv)
{
	std::fprintf(stderr,"%s\n", PROGRAM_TITLE);
	std::fprintf(stderr,"-------------------------------------------------------\n");
	std::fprintf(stderr,"Copyright (C) 2013-2014 Christoph Hormann\n");
	std::fprintf(stderr,"This program comes with ABSOLUTELY NO WARRANTY;\n");
	std::fprintf(stderr,"This is free software, and you are welcome to redistribute\n");
	std::fprintf(stderr,"it under certain conditions; see COPYING for details.\n");

	cimg_usage("Usage: glaciers_gen [options]");

	// --- Read command line parameters ---

	// Files
	const char *file_o = cimg_option("-o",(char*)NULL,"output mask file");
	const char *file_i = cimg_option("-i",(char*)NULL,"input mask file");

	const char *file_f = cimg_option("-f",(char*)NULL,"fixed mask file");

	const float Level = cimg_option("-l",0.5,"selection threshold");
	const float RSelect = cimg_option("-rs",4.0,"selection radius");

	const int FRS = cimg_option("-rfs",5,"fixed mask search radius");
	const float FR = cimg_option("-rf",2.0,"fixed mask buffer radius");

	float ORadius[4];
	const char *rad_string = cimg_option("-ro","2.2:2.2:1.2:1.2","open radius (erode:dilate:dilate:erode)");
	std::sscanf(rad_string,"%f:%f:%f:%f",&ORadius[0],&ORadius[1],&ORadius[2],&ORadius[3]);

	float CRadius[4];
	const char *rad_string2 = cimg_option("-rc","2.2:2.2:1.2:1.2","close radius (dilate:erode:erode:dilate)");
	std::sscanf(rad_string2,"%f:%f:%f:%f",&CRadius[0],&CRadius[1],&CRadius[2],&CRadius[3]);

	const bool Debug = cimg_option("-debug",false,"generate debug output");

	const bool helpflag = cimg_option("-h",false,"Display this help");
	if (helpflag) std::exit(0);

	if ((file_i == NULL) || (file_o == NULL))
	{
		std::fprintf(stderr,"You must specify input and output mask images files (try '%s -h').\n\n",argv[0]);
		std::exit(1);
	}

	CImg<unsigned char> img_m;
	CImg<unsigned char> img_b;
	CImg<unsigned char> img_d;
	CImg<unsigned char> img_f;

	std::fprintf(stderr,"Loading mask data...\n");
	img_m = CImg<unsigned char>(file_i);

	if (file_f != NULL)
	{
		std::fprintf(stderr,"Loading fixed mask data...\n");
		img_f = CImg<unsigned char>(file_f);

		if ((img_f.width() != img_m.width()) || (img_f.height() != img_m.height()))
		{
			std::fprintf(stderr,"input (-i) and fixed mask (-f) images need to be the same size.\n\n");
			std::exit(1);
		}

		std::fprintf(stderr,"Preprocessing fixed mask data...\n");

		CImg<unsigned char> img_c(img_m.width(), img_m.height(), 1, 1);
		CImg<unsigned char> img_r(img_m.width(), img_m.height(), 1, 1);

		cimg_forXY(img_m,px,py)
		{
			if (img_m(px,py) != 0)
			{
				// avoid isolated pixels at edge - should not usually occur in a pre-generalized mask
				if (img_f(px,py) == 255)
				{
					bool found = false;
					bool found_e = false;
					for (int i = 0; i < 4; i++)
					{
						int xn = px + x4[i];
						int yn = py + y4[i];
						if (xn >= 0)
							if (yn >= 0)
								if (xn < img_f.width())
									if (yn < img_f.height())
									{
										if (img_m(xn,yn) != 0)
											found = true;
										if (img_f(xn,yn) != 255)
											found_e = true;
									}
					}

					if (!found && found_e)
					{
						img_m(px,py) = 0;
						continue;
					}
				}
			}
		}

		cimg_forXY(img_m,px,py)
		{
			img_c(px,py) = 0;
			img_r(px,py) = 0;

			if (img_m(px,py) != 0)
			{
				// ice over water -> connect unless next to land without ice, then -> remove
				if (img_f(px,py) < 80)
				{
					bool found = false;
					for (int i = 0; i < 4; i++)
					{
						int xn = px + x4[i];
						int yn = py + y4[i];
						if (xn >= 0)
							if (yn >= 0)
								if (xn < img_f.width())
									if (yn < img_f.height())
										if (img_m(xn,yn) == 0)
										if (img_f(xn,yn) == 255)
										{
											found = true;
											break;
										}
					}
					if (found)
						img_r(px,py) = 255;
					else
						img_c(px,py) = 255;
				}
				else if (img_f(px,py) < 255)
				{
					// ice edge pixel on land 'flooded by generalization' -> remove
					for (int i = 1; i < 9; i++)
					{
						int xn = px + xo[i];
						int yn = py + yo[i];
						if (xn >= 0)
							if (yn >= 0)
								if (xn < img_f.width())
									if (yn < img_f.height())
										if (img_m(xn,yn) == 0)
										if (img_f(xn,yn) != 0)
										{
											img_r(px,py) = 255;
											break;
										}
					}
				}
				else
				{
					// ice on land: if near to water and not next to ice free land -> connect
					// if near to water and next to ice free land -> remove free land
					int cw = 0;
					int cl = 0;

					for (int i = 1; i < 9; i++)
					{
						int xn = px + xo[i];
						int yn = py + yo[i];
						if (xn >= 0)
							if (yn >= 0)
								if (xn < img_f.width())
									if (yn < img_f.height())
									{
										if (img_f(xn,yn) < 80)
											cw++;
									}
					}

					if (cw>0)
					{
						for (int i = 0; i < 4; i++)
						{
							int xn = px + x4[i];
							int yn = py + y4[i];
							if (xn >= 0)
								if (yn >= 0)
									if (xn < img_f.width())
										if (yn < img_f.height())
										{
											if ((img_f(xn,yn) == 255) && (img_m(xn,yn) == 0))
											{
												cl++;
												if ((i%2) == 0)
													img_r(xn,yn) = 255;
											}
										}
						}

						if (cl<cw)
							img_c(px,py) = 255;
					}
				}
			}
			else if (img_f(px,py) == 64)
			{
				// no ice with water made land by generalization
				// -> connect if water and ice closer than ice free land


				double d_w = FRS+1.0;
				double d_l = FRS+1.0;
				double d_i = FRS+1.0;
				for (int yn=py-FRS; yn <=py+FRS; yn++)
				{
					for (int xn=px-FRS; xn <=px+FRS; xn++)
						if (xn >= 0)
							if (yn >= 0)
								if (xn < img_f.width())
									if (yn < img_f.height())
									{
										double d = std::sqrt((px-xn)*(px-xn) + (py-yn)*(py-yn));
										if (d <= FRS)
										{
											if (img_f(xn,yn) == 0) d_w = std::min(d_w, d);
											if ((img_f(xn,yn) == 255) && (img_m(xn,yn) == 0)) d_l = std::min(d_l, d);
											if (img_m(xn,yn) != 0) d_i = std::min(d_i, d);
										}
									}
				}

				if ((d_w < d_l) && (d_i <= d_l))
					img_c(px,py) = 255;
				else
					img_r(px,py) = 255;
			}
		}

		if (Debug)
		{
			img_c.save("debug-c.tif");
			img_r.save("debug-r.tif");
		}

		CImg<unsigned char> morph_mask((FR+1.0)*2 + 1, (FR+1.0)*2 + 1, 1, 1);

		cimg_forXY(morph_mask,px,py)
		{
			int dx = px-(morph_mask.width()/2);
			int dy = py-(morph_mask.height()/2);
			if (std::sqrt(dx*dx+dy*dy) <= FR)
				morph_mask(px,py) = 255;
			else
				morph_mask(px,py) = 0;
		}

		img_c.dilate(morph_mask);

		cimg_forXY(img_m,px,py)
		{
			if (img_r(px,py) > 0)
				img_c(px,py) = 0;
			if ((img_f(px,py) == 255) && (img_m(px,py) == 0))
				img_c(px,py) = 0;
		}

		img_r.dilate(morph_mask);

		CImg<unsigned char> img_f2(img_m.width(), img_m.height(), 1, 1);

		if (Debug)
		{
			img_c.save("debug-cd.tif");
			img_r.save("debug-rd.tif");
		}

		img_b = img_m;

		cimg_forXY(img_m,px,py)
		{
			img_f2(px,py) = 128;
			if (img_m(px,py) > 0)
			{
				img_m(px,py) = 255;
			}

			if (img_c(px,py) == 0)
			{
				if (img_r(px,py) > 0)
				{
					img_m(px,py) = 0;
					img_f2(px,py) = 0;
				}
			}
			else
			{
				img_m(px,py) = 255;
				img_f2(px,py) = 255;
			}

			// limit expansion into water next to ice free land
			if (img_f2(px,py) == 255)
			if (img_f(px,py) == 0)
			{
				bool found_l = false;
				bool found_i = false;
				for (int i = 1; i < 9; i++)
				{
					int xn = px + xo[i];
					int yn = py + yo[i];
					if (xn >= 0)
						if (yn >= 0)
							if (xn < img_f.width())
								if (yn < img_f.height())
									if (img_f(xn,yn) > 80)
									{
										if (img_b(xn,yn) == 0)
											found_l = true;
										else
											found_i = true;
									}
				}
				if (found_l && !found_i)
				{
					img_f2(px,py) = 128;
					img_m(px,py) = 0;
				}
			}
		}

		img_f = img_f2;

		if (Debug)
		{
			img_m.save("debug-fm.tif");
		}

		img_b = img_m;
	}
	else
	{
		cimg_forXY(img_m,px,py)
		{
			if (img_m(px,py) > 0)
				img_m(px,py) = 255;
		}
		img_b = img_m;
	}

	img_d = CImg<unsigned char>(img_m.width(), img_m.height(), 1, 1);

	std::fprintf(stderr,"Measuring ice areas...\n");

	size_t cnt_all = 0;
	size_t cnt_land = 0;

	cimg_forXY(img_b,px,py)
	{
		cnt_all++;
		if (img_b(px,py) == 255)
		{
			img_d(px,py) = 48;
			cnt_land++;
		}
		else
		{
			img_d(px,py) = 0;
		}
	}

	if ((cnt_land == cnt_all) || (cnt_land == 0))
	{
		std::fprintf(stderr,"  data is trivial\n");

		if (file_o != NULL)
		{
			std::fprintf(stderr,"Writing output...\n");
			img_m.save(file_o);
			std::fprintf(stderr,"glaciers mask written to file %s\n", file_o);
		}
		return 0;
	}

	if (Debug)
		img_d.save("debug-dx.pgm");

	std::fprintf(stderr,"Generating convolution kernels...\n");

	CImg<unsigned char> morph_mask_o1(int(ORadius[0]+0.5)*2 + 1, int(ORadius[0]+0.5)*2 + 1, 1, 1);

	cimg_forXY(morph_mask_o1,px,py)
	{
		int dx = px-(morph_mask_o1.width()/2);
		int dy = py-(morph_mask_o1.height()/2);
		if (std::sqrt(dx*dx+dy*dy) <= ORadius[0])
			morph_mask_o1(px,py) = 255;
		else
			morph_mask_o1(px,py) = 0;
	}

	if (Debug)
		morph_mask_o1.save("debug-mo1.pgm");

	CImg<unsigned char> morph_mask_o2(int(ORadius[1]+0.5)*2 + 1, int(ORadius[1]+0.5)*2 + 1, 1, 1);

	cimg_forXY(morph_mask_o2,px,py)
	{
		int dx = px-(morph_mask_o2.width()/2);
		int dy = py-(morph_mask_o2.height()/2);
		if (std::sqrt(dx*dx+dy*dy) <= ORadius[1])
			morph_mask_o2(px,py) = 255;
		else
			morph_mask_o2(px,py) = 0;
	}

	if (Debug)
		morph_mask_o2.save("debug-mo2.pgm");

	CImg<unsigned char> morph_mask_o3(int(ORadius[2]+0.5)*2 + 1, int(ORadius[2]+0.5)*2 + 1, 1, 1);

	cimg_forXY(morph_mask_o3,px,py)
	{
		int dx = px-(morph_mask_o3.width()/2);
		int dy = py-(morph_mask_o3.height()/2);
		if (std::sqrt(dx*dx+dy*dy) <= ORadius[2])
			morph_mask_o3(px,py) = 255;
		else
			morph_mask_o3(px,py) = 0;
	}

	if (Debug)
		morph_mask_o3.save("debug-mo3.pgm");

	CImg<unsigned char> morph_mask_o4(int(ORadius[3]+0.5)*2 + 1, int(ORadius[3]+0.5)*2 + 1, 1, 1);

	cimg_forXY(morph_mask_o4,px,py)
	{
		int dx = px-(morph_mask_o4.width()/2);
		int dy = py-(morph_mask_o4.height()/2);
		if (std::sqrt(dx*dx+dy*dy) <= ORadius[3])
			morph_mask_o4(px,py) = 255;
		else
			morph_mask_o4(px,py) = 0;
	}

	CImg<unsigned char> morph_mask_c1(int(CRadius[0]+0.5)*2 + 1, int(CRadius[0]+0.5)*2 + 1, 1, 1);

	cimg_forXY(morph_mask_c1,px,py)
	{
		int dx = px-(morph_mask_c1.width()/2);
		int dy = py-(morph_mask_c1.height()/2);
		if (std::sqrt(dx*dx+dy*dy) <= CRadius[0])
			morph_mask_c1(px,py) = 255;
		else
			morph_mask_c1(px,py) = 0;
	}

	if (Debug)
		morph_mask_o4.save("debug-mo4.pgm");

	CImg<unsigned char> morph_mask_c2(int(CRadius[1]+0.5)*2 + 1, int(CRadius[1]+0.5)*2 + 1, 1, 1);

	cimg_forXY(morph_mask_c2,px,py)
	{
		int dx = px-(morph_mask_c2.width()/2);
		int dy = py-(morph_mask_c2.height()/2);
		if (std::sqrt(dx*dx+dy*dy) <= CRadius[1])
			morph_mask_c2(px,py) = 255;
		else
			morph_mask_c2(px,py) = 0;
	}

	CImg<unsigned char> morph_mask_c3(int(CRadius[2]+0.5)*2 + 1, int(CRadius[2]+0.5)*2 + 1, 1, 1);

	cimg_forXY(morph_mask_c3,px,py)
	{
		int dx = px-(morph_mask_c3.width()/2);
		int dy = py-(morph_mask_c3.height()/2);
		if (std::sqrt(dx*dx+dy*dy) <= CRadius[2])
			morph_mask_c3(px,py) = 255;
		else
			morph_mask_c3(px,py) = 0;
	}

	CImg<unsigned char> morph_mask_c4(int(CRadius[3]+0.5)*2 + 1, int(CRadius[3]+0.5)*2 + 1, 1, 1);

	cimg_forXY(morph_mask_c4,px,py)
	{
		int dx = px-(morph_mask_c4.width()/2);
		int dy = py-(morph_mask_c4.height()/2);
		if (std::sqrt(dx*dx+dy*dy) <= CRadius[3])
			morph_mask_c4(px,py) = 255;
		else
			morph_mask_c4(px,py) = 0;
	}

	std::fprintf(stderr,"Processing mask...\n");

	std::fprintf(stderr,"Opening...\n");

	CImg<unsigned char> img_o = img_m.get_erode(morph_mask_o1);
	std::fprintf(stderr,"  step 1\n");
	img_o.dilate(morph_mask_o2);
	std::fprintf(stderr,"  step 2\n");
	img_o.dilate(morph_mask_o3);
	std::fprintf(stderr,"  step 3\n");
	img_o.erode(morph_mask_o4);
	std::fprintf(stderr,"  step 4\n");

	std::fprintf(stderr,"Closing...\n");

	CImg<unsigned char> img_c = img_m.get_erode(morph_mask_c1);
	std::fprintf(stderr,"  step 1\n");
	img_c.dilate(morph_mask_c2);
	std::fprintf(stderr,"  step 2\n");
	img_c.dilate(morph_mask_c3);
	std::fprintf(stderr,"  step 3\n");
	img_c.erode(morph_mask_c4);
	std::fprintf(stderr,"  step 4\n");

	std::fprintf(stderr,"Generating selection mask...\n");

	CImg<unsigned char> img_cb = img_c.get_blur(3.0);
	CImg<unsigned char> img_ob = img_o.get_blur(3.0);

	img_b.blur(RSelect);

	cimg_forXY(img_b,px,py)
	{
		if (img_b(px,py) > (Level+0.12)*255)
		{
			img_m(px,py) = img_o(px,py);
			img_b(px,py) = 255;
		}
		else if (img_b(px,py) < (Level-0.12)*255)
		{
			img_m(px,py) = img_c(px,py);
			img_b(px,py) = 0;
		}
		else
		{
			float v = (img_b(px,py)-(Level-0.12)*255)/(0.24*255);
			int c = v*img_ob(px,py) + (1.0-v)*img_cb(px,py);
			if (c > 127)
				img_m(px,py) = 255;
			else
				img_m(px,py) = 0;
			img_b(px,py) = 128;
		}
	}

	if (Debug)
	{
		img_b.save("debug-b.tif");
		img_o.save("debug-o.tif");
		img_c.save("debug-c2.tif");
	}

	if (file_f != NULL)
	{
		if (Debug)
			img_f.save("debug-f.tif");

		cimg_forXY(img_f,px,py)
		{
			if (img_f(px,py) < 128)
				img_m(px,py) = 0;
			else if (img_f(px,py) > 128)
				img_m(px,py) = 255;
		}
	}

	if (Debug)
		img_d.save("debug-d.pgm");

	if (file_o != NULL)
	{
		std::fprintf(stderr,"Writing output...\n");
		img_m.save(file_o);
		std::fprintf(stderr,"generalized mask written to file %s\n", file_o);
	}
}
