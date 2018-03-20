/* ========================================================================
    File: @(#)glaciers_gen2.cpp
   ------------------------------------------------------------------------
    glaciers_gen simple glaciers generalization
    second version with relief taken into account
    Copyright (C) 2013-2016 Christoph Hormann <chris_hormann@gmx.de>
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
      0.4: more efficient processing of trivial data sets
      1.0: modifying process, using relief data, April 2016
      1.1: some smaller improvements, may 2016

   ========================================================================
 */

const char PROGRAM_TITLE[] = "glaciers_gen2 version 1.1";

#include <cstdlib>
#include <algorithm>
#include <cmath>

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
	std::fprintf(stderr,"Copyright (C) 2013-2016 Christoph Hormann\n");
	std::fprintf(stderr,"This program comes with ABSOLUTELY NO WARRANTY;\n");
	std::fprintf(stderr,"This is free software, and you are welcome to redistribute\n");
	std::fprintf(stderr,"it under certain conditions; see COPYING for details.\n");

	cimg_usage("Usage: glaciers_gen2 [options]");

	// --- Read command line parameters ---

	// Files
	const char *file_o = cimg_option("-o",(char*)NULL,"output mask file");
	const char *file_i = cimg_option("-i",(char*)NULL,"input mask file");

	const char *file_d = cimg_option("-d",(char*)NULL,"dem file");

	const char *file_f = cimg_option("-f",(char*)NULL,"fixed mask file");

	const float Level = cimg_option("-l",0.5,"selection threshold");
	const float RSelect = cimg_option("-rs",4.0,"selection radius");
	const float LV = cimg_option("-lv",0.2,"selection range");

	const float IFR = cimg_option("-ifr",0.0,"relief intensity factor");
	const float IFG = cimg_option("-ifg",0.0,"relief gradient intensity factor");
	const float IFE = cimg_option("-ife",0.0,"environment intensity factor");
	const float ID = cimg_option("-id",0.0,"dynamic intensity factor");

	const float SBF = cimg_option("-sbf",0.0,"symmetry balance factor");
	const float BAF = cimg_option("-baf",0.0,"balance asymmetry factor");

	const float ERF = cimg_option("-erf",1.0,"environment radius factor");

	const int FRS = cimg_option("-rfs",5,"fixed mask search radius");
	const float FR = cimg_option("-rf",2.0,"fixed mask buffer radius");

	const float GridSz = cimg_option("-resolution",30.0,"Grid resolution (in m)");

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

	img_d = CImg<unsigned char>(img_m.width(), img_m.height(), 1, 1);

	std::fprintf(stderr,"Measuring ice areas...\n");

	size_t cnt_all = 0;
	size_t cnt_land = 0;

	cimg_forXY(img_m,px,py)
	{
		cnt_all++;
		if (img_m(px,py) == 255)
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

	if (Debug)
		img_d.save("debug-dx.tif");

	CImg<float> img_ldiff;
	CImgList<float> img_grad;

	if (file_d != NULL)
	{
		std::fprintf(stderr,"Loading dem data...\n");
		CImg<float> img_dem = CImg<float>(file_d);

		img_ldiff = CImg<float>(img_dem.width(), img_dem.height(), 1, 1);

		CImg<float> img_dem2a = img_dem.get_blur(RSelect*1.5);
		CImg<float> img_dem2b = img_dem.get_blur(RSelect*2.0);

		std::fprintf(stderr,"Calculating difference...\n");
		cimg_forXY(img_dem,px,py)
		{
			float diff1 = img_dem(px,py) - img_dem2a(px,py);
			float diff2 = 2.5*(img_dem2a(px,py) - img_dem2b(px,py));
			if ((diff1 > 0) && (diff2 > 0)) img_ldiff(px,py) = std::max(diff1, diff2);
			else if ((diff1 < 0) && (diff2 < 0)) img_ldiff(px,py) = std::min(diff1, diff2);
			else img_ldiff(px,py) = img_dem(px,py) - img_dem2a(px,py);
			if (img_ldiff(px,py) < -0.5*GridSz) img_d(px,py) = 64;
			else if (img_ldiff(px,py) > 0.5*GridSz) img_d(px,py) = 200;
			else if (img_ldiff(px,py) < 0) img_d(px,py) = 110;
			else if (img_ldiff(px,py) > 0) img_d(px,py) = 140;
			else img_d(px,py) = 128;
		}

		std::fprintf(stderr,"Calculating gradients...\n");

		img_dem2a = img_dem.get_blur(RSelect*0.25);
		img_grad = img_dem2a.get_gradient("xy");

		if (Debug)
			img_ldiff.save("debug-curv.tif");
	}


	std::fprintf(stderr,"Generating convolution kernels...\n");

	CImg<unsigned char> morph_mask(int(RSelect+0.5)*2 + 1, int(RSelect+0.5)*2 + 1, 1, 1);

	cimg_forXY(morph_mask,px,py)
	{
		int dx = px-(morph_mask.width()/2);
		int dy = py-(morph_mask.height()/2);
		if (std::sqrt(dx*dx+dy*dy) <= RSelect)
			morph_mask(px,py) = 255;
		else
			morph_mask(px,py) = 0;
	}

	std::fprintf(stderr,"Processing mask...\n");
	
	CImg<unsigned char> img_o = img_m.get_dilate(morph_mask);
	CImg<unsigned char> img_i = img_m.get_erode(morph_mask);

	CImg<unsigned char> img_t = img_m.get_blur(RSelect*0.5*ERF);
	CImg<unsigned char> img_t2 = img_m.get_blur(RSelect*2.0*ERF);

	CImg<unsigned char> img_ts;
	if (SBF != 0.0)
		img_ts = img_m.get_blur(RSelect*3.0*ERF);

	if (Debug)
	{
		img_o.save("debug-o.tif");
		img_i.save("debug-i.tif");
		img_t.save("debug-t.tif");
		img_t2.save("debug-t2.tif");
	}

	std::fprintf(stderr,"Post processing mask...\n");

	// post process edges
	for (int j=0; j < 40; j++)
	{
		img_f = img_m;
		cimg_forXY(img_b,px,py)
		{
			// solid ice
			if ((img_o(px,py) == 255) && (img_i(px,py) == 255) )
			{
				img_m(px,py) = 255;
			}
			// solid non-ice
			else if ((img_o(px,py) == 0) && (img_i(px,py) == 0))
			{
				img_m(px,py) = 0;
			}
			else
			{
				float grad_x = 0.0;
				float grad_y = 0.0;

				if (file_d != NULL)
				{
					grad_x = img_grad[0](px,py);
					grad_y = img_grad[1](px,py);
					float grad_l = std::sqrt(grad_x*grad_x + grad_y*grad_y);
					if (grad_l > 0)
					{
						grad_x /= grad_l;
						grad_y /= grad_l;
					}
					else
					{
						grad_x = 0.0;
						grad_y = 0.0;
					}
				}

				float cnt_i = 0.0;
				float cnt_l = 0.0;
				for (int i = 1; i < 9; i++)
				{
					int xn = px + xo[i];
					int yn = py + yo[i];

					if (xn >= 0)
						if (yn >= 0)
							if (xn < img_b.width())
								if (yn < img_b.height())
								{
									float dd;
									if (i%2 == 0)
										dd = grad_x*xo[i] + grad_y*yo[i];
									else
										dd = grad_x*xo[i]/std::sqrt(2.0) + grad_y*yo[i]/std::sqrt(2.0);

									float f = 1.0-IFG+std::abs(dd)*2.0*IFG;

									if (img_f(xn,yn) == 0)
									{
										if (img_ldiff(xn,yn) > 0.6*GridSz) f *= (1.0+IFR*1.2);
										if (img_ldiff(xn,yn) > 0.4*GridSz) f *= (1.0+IFR*0.6);
										else if (img_ldiff(xn,yn) < -0.6*GridSz) f /= (1.0+IFR*1.2);
										else if (img_ldiff(xn,yn) < -0.4*GridSz) f /= (1.0+IFR*0.6);

										if (i%2 == 0)
											cnt_l += f*1.0;
										else
											cnt_l += f*0.5;
									}
									else
									{
										if (img_ldiff(xn,yn) > 0.6*GridSz) f /= (1.0+IFR*1.2);
										else if (img_ldiff(xn,yn) > 0.4*GridSz) f /= (1.0+IFR*0.6);
										else if (img_ldiff(xn,yn) < -0.6*GridSz) f *= (1.0+IFR*1.2);
										else if (img_ldiff(xn,yn) < -0.4*GridSz) f *= (1.0+IFR*0.6);

										if (i%2 == 0)
											cnt_i += f*1.0;
										else
											cnt_i += f*0.5;
									}
								}
				}

				/*
				if (img_ldiff(px,py) > 0.5*GridSz) cnt_l += 0.125;
				if (img_ldiff(px,py) < -0.5*GridSz) cnt_i += 0.125;
				*/

				if (img_ldiff(px,py) > 0.6*GridSz) cnt_i -= 0.6;
				else if (img_ldiff(px,py) > 0.4*GridSz) cnt_i -= 0.3;
				else if (img_ldiff(px,py) < -0.6*GridSz) cnt_l -= 0.6;
				else if (img_ldiff(px,py) < -0.4*GridSz) cnt_l -= 0.3;

				if (img_t(px,py) > 0.75*255) cnt_i -= IFE;
				else if (img_t(px,py) > 0.5*255) cnt_i -= IFE*0.3;
				else if (img_t(px,py) < 0.25*255) cnt_l -= IFE;
				else if (img_t(px,py) < 0.5*255) cnt_l -= IFE*0.3;

				if (img_t(px,py) > 0.25*255)
				{
					if (img_t2(px,py) < 0.1*255) cnt_i += IFE;
					else if (img_t2(px,py) < 0.2*255) cnt_i += IFE*0.3;
				}
				if (img_t(px,py) < 0.75*255)
				{
					if (img_t2(px,py) > 0.9*255) cnt_l += IFE;
					else if (img_t2(px,py) > 0.8*255) cnt_l += IFE*0.3;
				}

				if (SBF != 0.0)
				{
					if (img_ts(px,py) < 0.1*255) cnt_l -= SBF;
					else if (img_ts(px,py) < 0.2*255) cnt_l -= SBF*0.3;
					else if (img_ts(px,py) > 0.9*255) cnt_i -= SBF+BAF;
					else if (img_ts(px,py) > 0.8*255) cnt_i -= (SBF+BAF)*0.3;
				}

				if (img_f(px,py) == 0)
				{
					if (cnt_i > 3.0-ID) { img_m(px,py) = 255;  img_d(px,py) = 0; continue; }
				}
				else
				{
					if (cnt_l > 3.0-ID) { img_m(px,py) = 0;  img_d(px,py) = 0; continue; }
				}
			}
		}
	}



	if (Debug)
		img_d.save("debug-d.tif");

	if (file_o != NULL)
	{
		std::fprintf(stderr,"Writing output...\n");
		img_m.save(file_o);
		std::fprintf(stderr,"generalized mask written to file %s\n", file_o);
	}
}
