//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//           INSTITUTO POLIT CNICO DO C VADO E DO AVE
//                          2022/2023
//             ENGENHARIA DE SISTEMAS INFORM TICOS
//                    VIS O POR COMPUTADOR
//
//             [  DUARTE DUQUE - dduque@ipca.pt  ]
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Desabilita (no MSVC++) warnings de fun  es n o seguras (fopen, sscanf, etc...)
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <malloc.h>
#include <stdlib.h>
#include <math.h>
#include "vc.h"

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//            FUNÇÕES: ALOCAR E LIBERTAR UMA IMAGEM
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Alocar mem ria para uma imagem
IVC *vc_image_new(int width, int height, int channels, int levels)
{
	IVC *image = (IVC *)malloc(sizeof(IVC));

	if (image == NULL)
		return NULL;
	if ((levels <= 0) || (levels > 255))
		return NULL;

	image->width = width;
	image->height = height;
	image->channels = channels;
	image->levels = levels;
	image->bytesperline = image->width * image->channels;
	image->data = (unsigned char *)malloc(image->width * image->height * image->channels * sizeof(char));

	if (image->data == NULL)
	{
		return vc_image_free(image);
	}

	return image;
}

// Libertar mem ria de uma imagem
IVC *vc_image_free(IVC *image)
{
	if (image != NULL)
	{
		if (image->data != NULL)
		{
			free(image->data);
			image->data = NULL;
		}

		free(image);
		image = NULL;
	}

	return image;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//    FUN  ES: LEITURA E ESCRITA DE IMAGENS (PBM, PGM E PPM)
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

char *netpbm_get_token(FILE *file, char *tok, int len)
{
	char *t;
	int c;

	for (;;)
	{
		while (isspace(c = getc(file)))
			;
		if (c != '#')
			break;
		do
			c = getc(file);
		while ((c != '\n') && (c != EOF));
		if (c == EOF)
			break;
	}

	t = tok;

	if (c != EOF)
	{
		do
		{
			*t++ = c;
			c = getc(file);
		} while ((!isspace(c)) && (c != '#') && (c != EOF) && (t - tok < len - 1));

		if (c == '#')
			ungetc(c, file);
	}

	*t = 0;

	return tok;
}

long int unsigned_char_to_bit(unsigned char *datauchar, unsigned char *databit, int width, int height)
{
	int x, y;
	int countbits;
	long int pos, counttotalbytes;
	unsigned char *p = databit;

	*p = 0;
	countbits = 1;
	counttotalbytes = 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = width * y + x;

			if (countbits <= 8)
			{
				// Numa imagem PBM:
				// 1 = Preto
				// 0 = Branco
				//*p |= (datauchar[pos] != 0) << (8 - countbits);

				// Na nossa imagem:
				// 1 = Branco
				// 0 = Preto
				*p |= (datauchar[pos] == 0) << (8 - countbits);

				countbits++;
			}
			if ((countbits > 8) || (x == width - 1))
			{
				p++;
				*p = 0;
				countbits = 1;
				counttotalbytes++;
			}
		}
	}

	return counttotalbytes;
}

void bit_to_unsigned_char(unsigned char *databit, unsigned char *datauchar, int width, int height)
{
	int x, y;
	int countbits;
	long int pos;
	unsigned char *p = databit;

	countbits = 1;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = width * y + x;

			if (countbits <= 8)
			{
				// Numa imagem PBM:
				// 1 = Preto
				// 0 = Branco
				// datauchar[pos] = (*p & (1 << (8 - countbits))) ? 1 : 0;

				// Na nossa imagem:
				// 1 = Branco
				// 0 = Preto
				datauchar[pos] = (*p & (1 << (8 - countbits))) ? 0 : 1;

				countbits++;
			}
			if ((countbits > 8) || (x == width - 1))
			{
				p++;
				countbits = 1;
			}
		}
	}
}

IVC *vc_read_image(char *filename)
{
	FILE *file = NULL;
	IVC *image = NULL;
	unsigned char *tmp;
	char tok[20];
	long int size, sizeofbinarydata;
	int width, height, channels;
	int levels = 255;
	int v;

	// Abre o ficheiro
	if ((file = fopen(filename, "rb")) != NULL)
	{
		// Efectua a leitura do header
		netpbm_get_token(file, tok, sizeof(tok));

		if (strcmp(tok, "P4") == 0)
		{
			channels = 1;
			levels = 1;
		} // Se PBM (Binary [0,1])
		else if (strcmp(tok, "P5") == 0)
			channels = 1; // Se PGM (Gray [0,MAX(level,255)])
		else if (strcmp(tok, "P6") == 0)
			channels = 3; // Se PPM (RGB [0,MAX(level,255)])
		else
		{
#ifdef VC_DEBUG
			printf("ERROR -> vc_read_image():\n\tFile is not a valid PBM, PGM or PPM file.\n\tBad magic number!\n");
#endif

			fclose(file);
			return NULL;
		}

		if (levels == 1) // PBM
		{
			if (sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &width) != 1 ||
				sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &height) != 1)
			{
#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tFile is not a valid PBM file.\n\tBad size!\n");
#endif

				fclose(file);
				return NULL;
			}

			// Aloca mem ria para imagem
			image = vc_image_new(width, height, channels, levels);
			if (image == NULL)
				return NULL;

			sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height;
			tmp = (unsigned char *)malloc(sizeofbinarydata);
			if (tmp == NULL)
				return 0;

#ifdef VC_DEBUG
			printf("\nchannels=%d w=%d h=%d levels=%d\n", image->channels, image->width, image->height, levels);
#endif

			if ((v = fread(tmp, sizeof(unsigned char), sizeofbinarydata, file)) != sizeofbinarydata)
			{
#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tPremature EOF on file.\n");
#endif

				vc_image_free(image);
				fclose(file);
				free(tmp);
				return NULL;
			}

			bit_to_unsigned_char(tmp, image->data, image->width, image->height);

			free(tmp);
		}
		else // PGM ou PPM
		{
			if (sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &width) != 1 ||
				sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &height) != 1 ||
				sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &levels) != 1 || levels <= 0 || levels > 255)
			{
#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tFile is not a valid PGM or PPM file.\n\tBad size!\n");
#endif

				fclose(file);
				return NULL;
			}

			// Aloca mem ria para imagem
			image = vc_image_new(width, height, channels, levels);
			if (image == NULL)
				return NULL;

#ifdef VC_DEBUG
			printf("\nchannels=%d w=%d h=%d levels=%d\n", image->channels, image->width, image->height, levels);
#endif

			size = image->width * image->height * image->channels;

			if ((v = fread(image->data, sizeof(unsigned char), size, file)) != size)
			{
#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tPremature EOF on file.\n");
#endif

				vc_image_free(image);
				fclose(file);
				return NULL;
			}
		}

		fclose(file);
	}
	else
	{
#ifdef VC_DEBUG
		printf("ERROR -> vc_read_image():\n\tFile not found.\n");
#endif
	}

	return image;
}

int vc_write_image(char *filename, IVC *image)
{
	FILE *file = NULL;
	unsigned char *tmp;
	long int totalbytes, sizeofbinarydata;

	if (image == NULL)
		return 0;

	if ((file = fopen(filename, "wb")) != NULL)
	{
		if (image->levels == 1)
		{
			sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height + 1;
			tmp = (unsigned char *)malloc(sizeofbinarydata);
			if (tmp == NULL)
				return 0;

			fprintf(file, "%s %d %d\n", "P4", image->width, image->height);

			totalbytes = unsigned_char_to_bit(image->data, tmp, image->width, image->height);
			printf("Total = %ld\n", totalbytes);
			if (fwrite(tmp, sizeof(unsigned char), totalbytes, file) != totalbytes)
			{
#ifdef VC_DEBUG
				fprintf(stderr, "ERROR -> vc_read_image():\n\tError writing PBM, PGM or PPM file.\n");
#endif

				fclose(file);
				free(tmp);
				return 0;
			}

			free(tmp);
		}
		else
		{
			fprintf(file, "%s %d %d 255\n", (image->channels == 1) ? "P5" : "P6", image->width, image->height);

			if (fwrite(image->data, image->bytesperline, image->height, file) != image->height)
			{
#ifdef VC_DEBUG
				fprintf(stderr, "ERROR -> vc_read_image():\n\tError writing PBM, PGM or PPM file.\n");
#endif

				fclose(file);
				return 0;
			}
		}

		fclose(file);

		return 1;
	}

	return 0;
}

// Gerar negativo da imagem gray
int vc_gray_negative(IVC *srcdst)
{
	unsigned char *data = (unsigned char *)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->bytesperline;
	int channels = srcdst->channels;
	int x, y;
	long int pos;

	// Verificação de erros
	if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL))
		return 0;
	if (channels != 1)
		return 0;

	// Investe a imagem gray
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			data[pos] = 255 - data[pos];
		}
	}

	return 1;
}

// Gerar negativo da imagem RGB
int vc_rgb_negative(IVC *srcdst)
{
	unsigned char *data = (unsigned char *)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->bytesperline;
	int channels = srcdst->channels;
	int x, y;
	long int pos;

	// Verificação de erros
	if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL))
		return 0;
	if (channels != 3)
		return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			data[pos] = 255 - data[pos];
			data[pos + 1] = 255 - data[pos + 1];
			data[pos + 2] = 255 - data[pos + 2];
		}
	}

	return 1;
}

// Extrair componente RED da imagem RGB para gray
int vc_rgb_get_red_gray(IVC *srcdst)
{
	unsigned char *data = (unsigned char *)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->bytesperline;
	int channels = srcdst->channels;
	int x, y;
	long int pos;

	// Verificação de erros
	if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL))
		return 0;
	if (channels != 3)
		return 0;

	// Extrai a componente RED
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			data[pos + 1] = data[pos]; // Green
			data[pos + 2] = data[pos]; // Blue
		}
	}

	return 1;
}

// Extrair componente GREEN da imagem RGB para gray
int vc_rgb_get_green_gray(IVC *srcdst)
{
	unsigned char *data = (unsigned char *)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->bytesperline;
	int channels = srcdst->channels;
	int x, y;
	long int pos;

	// Verificação de erros
	if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL))
		return 0;
	if (channels != 3)
		return 0;

	// Extrai a componente RED
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			data[pos] = data[pos + 1];	   // RED
			data[pos + 2] = data[pos + 1]; // BLUE
		}
	}

	return 1;
}

// Extrair componente BLUE da imagem RGB para gray
int vc_rgb_get_blue_gray(IVC *srcdst)
{
	unsigned char *data = (unsigned char *)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->bytesperline;
	int channels = srcdst->channels;
	int x, y;
	long int pos;

	// Verificação de erros
	if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL))
		return 0;
	if (channels != 3)
		return 0;

	// Extrai a componente RED
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			data[pos] = data[pos + 2];	   // RED
			data[pos + 1] = data[pos + 2]; // GREEN
		}
	}

	return 1;
}

// Converter de RGB para Gray
int vc_rgb_to_gray(IVC *src, IVC *dst)
{
	unsigned char *datasrc = (unsigned char *)src->data;
	int bytesperline_src = src->width * src->channels;
	int channels_src = src->channels;
	unsigned char *datadst = (unsigned char *)dst->data;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_dst = dst->channels;
	int width = src->width;
	int height = src->height;
	int x, y;
	long int pos_src, pos_dst;
	float rf, gf, bf;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if ((src->width != dst->width) || (src->height != dst->height))
		return 0;
	if ((src->channels != 3) || (dst->channels != 1))
		return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;

			rf = (float)datasrc[pos_src];
			gf = (float)datasrc[pos_src + 1];
			bf = (float)datasrc[pos_src + 2];

			datadst[pos_dst] = (unsigned char)((rf * 0.299) + (gf * 0.587) + (bf * 0.114));
		}
	}

	return 1;
}

// Converter rgb para hsv
int vc_rgb_to_hsv(IVC *src, IVC *dst)
{
	unsigned char *datasrc = (unsigned char *)src->data;
	int channels_src = src->channels;
	unsigned char *datadst = (unsigned char *)dst->data;
	int width = src->width;
	int height = src->height;
	int x;
	float rf, gf, bf;
	float max, min, delta;
	float h, s, v;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if ((src->width != dst->width) || (src->height != dst->height))
		return 0;
	if ((src->channels != 3) || (dst->channels != 3))
		return 0;

	float size = width * height * channels_src;

	for (x = 0; x < size; x += channels_src)
	{
		rf = (float)datasrc[x];
		gf = (float)datasrc[x + 1];
		bf = (float)datasrc[x + 2];

		max = (rf > gf ? (rf > bf ? rf : bf) : (gf > bf ? gf : bf));
		min = (rf < gf ? (rf < bf ? rf : bf) : (gf < bf ? gf : bf));
		delta = max - min;

		v = max;
		if (v == 0.0f)
		{
			h = 0.0f;
			s = 0.0f;
		}
		else
		{
			// Saturation toma valores entre [0,255]
			s = (delta / max) * 255.0f;

			if (s == 0.0f)
			{
				h = 0.0f;
			}
			else
			{
				// Hue toma valores entre [0,360]
				if ((max == rf) && (gf >= bf))
				{
					h = 60.0f * (gf - bf) / delta;
				}
				else if ((max == rf) && (bf > gf))
				{
					h = 360.0f + 60.0f * (gf - bf) / delta;
				}
				else if (max == gf)
				{
					h = 120.0f + 60.0f * (bf - rf) / delta;
				}
				else
				{
					h = 240.0f + 60.0f * (rf - gf) / delta;
				}
			}
		}

		datadst[x] = (unsigned char)((h / 360.0f) * 255.0f);
		datadst[x + 1] = (unsigned char)(s);
		datadst[x + 2] = (unsigned char)(v);
	}

	return 1;
}

// hmin,hmax = [0, 360]; smin,smax = [0, 100]; vmin,vmax = [0, 100]
int vc_hsv_segmentation(IVC *src, IVC *dst, int hmin, int hmax, int smin, int smax, int vmin, int vmax)
{
	unsigned char *datasrc = (unsigned char *)src->data;
	int bytesperline_src = src->width * src->channels;
	int channels_src = src->channels;
	unsigned char *datadst = (unsigned char *)dst->data;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_dst = dst->channels;
	int width = src->width;
	int height = src->height;
	int x, y;
	float max, min, hue, sat, valor, delta;
	long int pos_src, pos_dst;
	float rf, gf, bf;

	// verificalão de errors
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if ((src->width != dst->width) || (src->height != dst->height))
		return 0;
	if ((src->channels != 3) || (dst->channels != 1))
		return 0;

	// meter em hsv
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;

			rf = (float)datasrc[pos_src];
			gf = (float)datasrc[pos_src + 1];
			bf = (float)datasrc[pos_src + 2];

			max = (rf > gf ? (rf > bf ? rf : bf) : (gf > bf ? gf : bf));
			min = (rf < gf ? (rf < bf ? rf : bf) : (gf < bf ? gf : bf));
			delta = max - min;

			// calcular value
			valor = max;

			// calcular saturação
			if (max == 0 || max == min)
			{
				sat = 0;
				hue = 0;
			}
			else
			{
				sat = delta * 100.0f / valor;
				// calcular hue
				//  Quando o vermelho é o maior, Hue será um ângulo entre 300 e 360 ou entre 0 e 60
				if (rf == max && gf >= bf)
				{
					hue = 60.0f * (gf - bf) / delta;
				}
				else if (rf == max && bf > gf)
				{
					hue = 360 + 60.0f * (gf - bf) / delta;
				}
				else if (gf == max)
				{
					hue = 120 + 60.0f * (bf - rf) / delta;
				}
				else if (max == bf)
				{
					hue = 240 + 60.0f * (rf - gf) / delta;
				}
			}

			// se o hmin for maior que o hmax  entao hmin ate 360 e de 0 ate hmax
			if (hmin > hmax)
			{
				if ((hue >= 0 && hue <= hmax || hue <= 360 && hue >= hmin) && sat <= smax && sat >= smin && valor <= vmax / 100.0f * 255 && valor >= vmin / 100.0f * 255)
					datadst[pos_dst] = 255;
				else
					datadst[pos_dst] = 0;
			}
			else
			{
				if (hue <= hmax && hue >= hmin && sat <= smax && sat >= smin && valor <= vmax / 100.0f * 255 && valor >= vmin / 100.0f * 255)
					datadst[pos_dst] = 255;
				else
					datadst[pos_dst] = 0;
			}
		}
	}

	return 1;
}

int vc_scale_gray_to_color_palette(IVC *src, IVC *dst)
{
	unsigned char *datasrc = (unsigned char *)src->data;
	int bytesperline_src = src->channels * src->width;
	int channels_src = src->channels;
	unsigned char *datadst = (unsigned char *)dst->data;
	int bytesperline_dst = dst->channels * dst->width;
	int channels_dst = dst->channels;
	int width = src->width;
	int height = src->height;
	int x, y;
	long int pos_src, pos_dst;

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if ((src->width != dst->width) || (src->height != dst->height))
		return 0;
	if ((src->channels != 1) || (dst->channels != 3))
		return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;

			if (datasrc[pos_src] < 64)
			{ // Primeiro quarteto -> cores verdes
				datadst[pos_dst] = 0;
				datadst[pos_dst + 1] = datasrc[pos_src] * 4;
				datadst[pos_dst + 2] = 255;
			}
			else if (datasrc[pos_src] < 128)
			{ // Segundo quarteto -> verde a azul
				datadst[pos_dst] = 0;
				datadst[pos_dst + 1] = 255;
				datadst[pos_dst + 2] = 255 - ((datasrc[pos_src] - 64) * 4);
			}
			else if (datasrc[pos_src] < 192)
			{ // Terceiro quarteto -> azul a vermelho
				datadst[pos_dst] = (datasrc[pos_src] - 128) * 4;
				datadst[pos_dst + 1] = 255;
				datadst[pos_dst + 2] = 0;
			}
			else
			{ // Quarto quarteto -> vermelho para verde
				datadst[pos_dst] = 255;
				datadst[pos_dst + 1] = 255 - ((datasrc[pos_src] - 192) * 4);
				;
				datadst[pos_dst + 2] = 0;
			}
		}
	}

	return 1;
}

int vc_gray_to_binary(IVC *src, IVC *dst, int threshold)
{
	unsigned char *datasrc = (unsigned char *)src->data;
	int bytesperline_src = src->channels * src->width;
	int channels_src = src->channels;
	unsigned char *datadst = (unsigned char *)dst->data;
	int bytesperline_dst = dst->channels * dst->width;
	int channels_dst = dst->channels;
	int width = src->width;
	int height = src->height;
	int x, y;
	long int pos_src, pos_dst;

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if ((src->width != dst->width) || (src->height != dst->height))
		return 0;
	if ((src->channels != 1) || (dst->channels != 1))
		return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;

			if (datasrc[pos_src] > threshold)
			{
				datadst[pos_dst] = 255;
			}
			else
			{
				datadst[pos_dst] = 0;
			}
		}
	}

	return 1;
}

int vc_gray_to_binary_global_mean(IVC *src, IVC *dst)
{
	unsigned char *datasrc = (unsigned char *)src->data;
	int bytesperline_src = src->channels * src->width;
	int channels_src = src->channels;
	unsigned char *datadst = (unsigned char *)dst->data;
	int bytesperline_dst = dst->channels * dst->width;
	int channels_dst = dst->channels;
	int width = src->width;
	int height = src->height;
	int x, y;
	long int pos_src, pos_dst;
	int total = 0;
	float threshold;

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if ((src->width != dst->width) || (src->height != dst->height))
		return 0;
	if ((src->channels != 1) || (dst->channels != 1))
		return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x * channels_src;

			total += datasrc[pos_src];
		}
	}

	threshold = (float)total / (width * height);

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;

			if (datasrc[pos_src] > threshold)
			{
				datadst[pos_dst] = 255;
			}
			else
			{
				datadst[pos_dst] = 0;
			}
		}
	}

	return 1;
}

int vc_gray_to_binary_midpoint(IVC *src, IVC *dst, int kernel)
{
	unsigned char *datasrc = (unsigned char *)src->data;
	int bytesperline_src = src->channels * src->width;
	int channels_src = src->channels;
	unsigned char *datadst = (unsigned char *)dst->data;
	int bytesperline_dst = dst->channels * dst->width;
	int channels_dst = dst->channels;
	int width = src->width;
	int height = src->height;
	int x, y, vY, vX;
	long int pos_src, pos_dst;
	float threshold;
	float vMax = 0, vMin = 256;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if ((src->width != dst->width) || (src->height != dst->height))
		return 0;
	if ((src->channels != 1) || (dst->channels != 1))
		return 0;

	// Percorre uma imagem
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;

			vY = kernel / 2;
			vX = kernel / 2;

			int xx, yy;

			for (vMax = 0, vMin = 255, yy = y - vY; yy <= y + vY; yy++)
			{
				for (xx = x - vX; xx <= x + vX; xx++)
				{
					if (yy >= 0 && yy < height && xx >= 0 && xx < width)
					{
						pos_src = yy * bytesperline_src + xx * channels_src;

						if (datasrc[pos_src] > vMax)
						{
							vMax = datasrc[pos_src];
						}

						if (datasrc[pos_src] < vMin)
						{
							vMin = datasrc[pos_src];
						}
					}
				}
			}

			threshold = (vMin + vMax) / 2;

			if (datasrc[pos_src] > threshold)
			{
				datadst[pos_dst] = 255;
			}
			else
			{
				datadst[pos_dst] = 0;
			}
		}
	}

	return 1;
}

int vc_gray_to_binary_niblac(IVC *src, IVC *dst, int kernel, float k)
{
	unsigned char *datasrc = (unsigned char *)src->data;
	int bytesperline_src = src->channels * src->width;
	int channels_src = src->channels;
	unsigned char *datadst = (unsigned char *)dst->data;
	int width = src->width;
	int height = src->height;
	int x, y, vY, vX, counter;
	long int pos, pos_v;
	unsigned char threshold;
	int offset = (kernel - 1) / 2;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if ((src->width != dst->width) || (src->height != dst->height))
		return 0;
	if ((src->channels != 1) || (dst->channels != 1))
		return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline_src + x * channels_src;

			float mean = 0.0f;

			// Vizinhos
			for (counter = 0, vY = -offset; vY <= offset; vY++)
			{
				for (vX = -offset; vX <= offset; vX++)
				{
					if ((y + vY > 0) && (y + vY < height) && (x + vX > 0) && (x + vX < width))
					{
						pos_v = (y + vY) * bytesperline_src + (x + vX) * channels_src;

						mean += (float)datasrc[pos_v];

						counter++;
					}
				}
			}
			mean /= counter;

			float sdeviation = 0.0f;

			// Vizinhos
			for (counter = 0, vY = -offset; vY <= offset; vY++)
			{
				for (vX = -offset; vX <= offset; vX++)
				{
					if ((y + vY > 0) && (y + vY < height) && (x + vX > 0) && (x + vX < width))
					{
						pos_v = (y + vY) * bytesperline_src + (x + vX) * channels_src;

						sdeviation += powf(((float)datasrc[pos_v]) - mean, 2);

						counter++;
					}
				}
			}
			sdeviation = sqrt(sdeviation / counter);

			threshold = mean + k * sdeviation;

			if (datasrc[pos] > threshold)
				datadst[pos] = 255;
			else
				datadst[pos] = 0;
		}
	}

	return 1;
}

int vc_binary_dilate(IVC *src, IVC *dst, int kernel)
{
	unsigned char *datasrc = (unsigned char *)src->data;
	unsigned char *datadst = (unsigned char *)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y;
	int xk, yk;
	int i, j;
	long int pos, posk;
	int s1, s2;
	unsigned char pixel;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels))
		return 0;
	if (channels != 1)
		return 0;

	s2 = (kernel - 1) / 2;
	s1 = -(s2);

	memcpy(datadst, datasrc, bytesperline * height);

	// Cálculo da dilatacao
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			pixel = datasrc[pos];

			for (yk = s1; yk <= s2; yk++)
			{
				j = y + yk;

				if ((j < 0) || (j >= height))
					continue;

				for (xk = s1; xk <= s2; xk++)
				{
					i = x + xk;

					if ((i < 0) || (i >= width))
						continue;

					posk = j * bytesperline + i * channels;
					// aqui a unica diference entre erode ou dilate
					// se encontrar um pixel a branco mete o pixel central a branco

					pixel |= datasrc[posk];
				}
			}

			// Se um qualquer pixel da vizinhança, na imagem de origem, for de plano de fundo, então o pixel central
			// na imagem de destino é também definido como plano de fundo.
			if (pixel == 255)
				datadst[pos] = 255;
		}
	}
	return 1;
}
// Erosão binária
int vc_binary_erode(IVC *src, IVC *dst, int kernel)
{
	unsigned char *datasrc = (unsigned char *)src->data;
	unsigned char *datadst = (unsigned char *)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y;
	int xk, yk;
	int i, j;
	long int pos, posk;
	int s1, s2;
	unsigned char pixel;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels))
		return 0;
	if (channels != 1)
		return 0;

	s2 = (kernel - 1) / 2;
	s1 = -(s2);

	memcpy(datadst, datasrc, bytesperline * height);

	// Cálculo da erosão
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			pixel = datasrc[pos];

			for (yk = s1; yk <= s2; yk++)
			{
				j = y + yk;

				if ((j < 0) || (j >= height))
					continue;

				for (xk = s1; xk <= s2; xk++)
				{
					i = x + xk;

					if ((i < 0) || (i >= width))
						continue;

					posk = j * bytesperline + i * channels;
					// aqui a unica diference entre erode ou dilate
					// se encontrar um pixel a branco mete o pixel central a branco

					pixel &= datasrc[posk];
				}
			}

			// Se um qualquer pixel da vizinhança, na imagem de origem, for de plano de fundo, então o pixel central
			// na imagem de destino é também definido como plano de fundo.
			if (pixel == 0)
				datadst[pos] = 0;
		}
	}

	return 1;
}

int vc_binary_open(IVC *src, IVC *dst, int kernelErode, int kernelDilate)
{

	IVC *temp;
	temp = vc_image_new(src->width, src->height, 1, 255);

	vc_binary_erode(src, temp, kernelErode);

	vc_binary_dilate(temp, dst, kernelDilate);

	vc_image_free(temp);

	return 1;
}

int vc_binary_close(IVC *src, IVC *dst, int kernelErode, int kernelDilate)
{

	IVC *temp;
	temp = vc_image_new(src->width, src->height, 1, 255);

	vc_binary_dilate(src, temp, kernelDilate);

	vc_binary_erode(temp, dst, kernelErode);

	vc_image_free(temp);

	return 1;
}

// Etiquetagem de blobs
// src     : Imagem binária de entrada
// dst     : Imagem grayscale (irá conter as etiquetas)
// nlabels  : Endereço de memória de uma variável, onde será armazenado o numero de etiquetas encontradas.
// OVC*    : Retorna um array de estruturas de blobs (objectos), com respectivas etiquetas. é necessário libertar posteriormente esta memória.
OVC *vc_binary_blob_labelling(IVC *src, IVC *dst, int *nlabels)
{
	unsigned char *datasrc = (unsigned char *)src->data;
	unsigned char *datadst = (unsigned char *)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, a, b;
	long int i, size;
	long int posX, posA, posB, posC, posD;
	int labeltable[1024] = {0};
	int labelarea[1024] = {0};
	int label = 1; // Etiqueta inicial.
	int num, tmplabel;
	OVC *blobs; // Apontador para array de blobs (objectos) que será retornado desta função.

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels))
		return NULL;
	if (channels != 1)
		return NULL;

	// Copia dados da imagem binária para imagem grayscale
	memcpy(datadst, datasrc, bytesperline * height);

	// Todos os pixeis de plano de fundo devem obrigatoriamente ter valor 0
	// Todos os pixeis de primeiro plano devem obrigatoriamente ter valor 255
	// Serão atribuidas etiquetas no intervalo [1,254]
	// Este algoritmo esta assim limitado a 254 labels
	for (i = 0, size = bytesperline * height; i < size; i++)
	{
		if (datadst[i] != 0)
			datadst[i] = 255;
	}

	// Limpa os rebordos da imagem binária
	for (y = 0; y < height; y++)
	{
		datadst[y * bytesperline + 0 * channels] = 0;
		datadst[y * bytesperline + (width - 1) * channels] = 0;
	}

	for (x = 0; x < width; x++)
	{
		datadst[0 * bytesperline + x * channels] = 0;
		datadst[(height - 1) * bytesperline + x * channels] = 0;
	}

	// Efectua a etiquetagem
	for (y = 1; y < height - 1; y++)
	{
		for (x = 1; x < width - 1; x++)
		{
			// Kernel:
			// A B C
			// D X

			posA = (y - 1) * bytesperline + (x - 1) * channels; // A
			posB = (y - 1) * bytesperline + x * channels;		// B
			posC = (y - 1) * bytesperline + (x + 1) * channels; // C
			posD = y * bytesperline + (x - 1) * channels;		// D
			posX = y * bytesperline + x * channels;				// X

			// Se o pixel foi marcado
			if (datadst[posX] != 0)
			{
				if ((datadst[posA] == 0) && (datadst[posB] == 0) && (datadst[posC] == 0) && (datadst[posD] == 0))
				{
					datadst[posX] = label;
					labeltable[label] = label;
					label++;
				}
				else
				{
					num = 255;

					// Se A est  marcado
					if (datadst[posA] != 0)
						num = labeltable[datadst[posA]];
					// Se B est  marcado, e   menor que a etiqueta "num"
					if ((datadst[posB] != 0) && (labeltable[datadst[posB]] < num))
						num = labeltable[datadst[posB]];
					// Se C est  marcado, e   menor que a etiqueta "num"
					if ((datadst[posC] != 0) && (labeltable[datadst[posC]] < num))
						num = labeltable[datadst[posC]];
					// Se D est  marcado, e   menor que a etiqueta "num"
					if ((datadst[posD] != 0) && (labeltable[datadst[posD]] < num))
						num = labeltable[datadst[posD]];

					// Atribui a etiqueta ao pixel
					datadst[posX] = num;
					labeltable[num] = num;

					// Actualiza a tabela de etiquetas
					if (datadst[posA] != 0)
					{
						if (labeltable[datadst[posA]] != num)
						{
							for (tmplabel = labeltable[datadst[posA]], a = 1; a < label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}

					if (datadst[posB] != 0)
					{
						if (labeltable[datadst[posB]] != num)
						{
							for (tmplabel = labeltable[datadst[posB]], a = 1; a < label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}

					if (datadst[posC] != 0)
					{
						if (labeltable[datadst[posC]] != num)
						{
							for (tmplabel = labeltable[datadst[posC]], a = 1; a < label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}

					if (datadst[posD] != 0)
					{
						if (labeltable[datadst[posD]] != num)
						{
							for (tmplabel = labeltable[datadst[posD]], a = 1; a < label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
				}
			}
		}
	}

	// Volta a etiquetar a imagem
	for (y = 1; y < height - 1; y++)
	{
		for (x = 1; x < width - 1; x++)
		{
			posX = y * bytesperline + x * channels; // X

			if (datadst[posX] != 0)
			{
				datadst[posX] = labeltable[datadst[posX]];
			}
		}
	}

	// Contagem do número de blobs
	// Passo 1: Eliminar, da tabela, etiquetas repetidas
	for (a = 1; a < label - 1; a++)
	{
		for (b = a + 1; b < label; b++)
		{
			if (labeltable[a] == labeltable[b])
				labeltable[b] = 0;
		}
	}

	// Passo 2: Conta etiquetas e organiza a tabela de etiquetas, para que não hajam valores vazios (zero) entre etiquetas
	*nlabels = 0;
	for (a = 1; a < label; a++)
	{
		if (labeltable[a] != 0)
		{
			labeltable[*nlabels] = labeltable[a]; // Organiza tabela de etiquetas
			(*nlabels)++;						  // Conta etiquetas
		}
	}

	// Se não ha blobs
	if (*nlabels == 0)
		return NULL;

	// Cria lista de blobs (objectos) e preenche a etiqueta
	blobs = (OVC *)calloc((*nlabels), sizeof(OVC));
	if (blobs != NULL)
	{
		for (a = 0; a < (*nlabels); a++)
			blobs[a].label = labeltable[a];
	}
	else
		return NULL;

	return blobs;
}

int vc_binary_blob_info(IVC *src, OVC *blobs, int nblobs, int areaRelevant)
{
	unsigned char *data = (unsigned char *)src->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, i, j = 0;
	long int pos;
	int xmin, ymin, xmax, ymax;
	long int sumx, sumy;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if (channels != 1)
		return 0;

	// Conta área de cada blob
	for (i = 0; i < nblobs; i++)
	{
		xmin = width - 1;
		ymin = height - 1;
		xmax = 0;
		ymax = 0;

		sumx = 0;
		sumy = 0;

		blobs[i].area = 0;

		for (y = 1; y < height - 1; y++)
		{
			for (x = 1; x < width - 1; x++)
			{
				pos = y * bytesperline + x * channels;

				if (data[pos] == blobs[i].label)
				{
					// área
					blobs[i].area++;

					// Centro de Gravidade
					sumx += x;
					sumy += y;

					// Bounding Box
					if (xmin > x)
						xmin = x;
					if (ymin > y)
						ymin = y;
					if (xmax < x)
						xmax = x;
					if (ymax < y)
						ymax = y;

					// Perímetro
					// Se pelo menos um dos quatro vizinhos não pertence ao mesmo label, então é um pixel de contorno
					if ((data[pos - 1] != blobs[i].label) || (data[pos + 1] != blobs[i].label) || (data[pos - bytesperline] != blobs[i].label) || (data[pos + bytesperline] != blobs[i].label))
					{
						blobs[i].perimeter++;
					}
				}
			}
		}

		if (blobs[i].area > areaRelevant)
		{
			// Bounding Box
			blobs[j].x = xmin;
			blobs[j].y = ymin;
			blobs[j].width = (xmax - xmin) + 1;
			blobs[j].height = (ymax - ymin) + 1;

			// Centro de Gravidade
			blobs[j].xc = sumx / MAX(blobs[i].area, 1);
			blobs[j].yc = sumy / MAX(blobs[i].area, 1);

			blobs[j].area = blobs[i].area;
			blobs[j].perimeter = blobs[i].perimeter;
			j++;
			continue;
		}

		// Se a área não for relevante - 0
		else if (!areaRelevant)
		{
			// Bounding Box
			blobs[i].x = xmin;
			blobs[i].y = ymin;
			blobs[i].width = (xmax - xmin) + 1;
			blobs[i].height = (ymax - ymin) + 1;

			// Centro de Gravidade
			blobs[i].xc = sumx / MAX(blobs[i].area, 1);
			blobs[i].yc = sumy / MAX(blobs[i].area, 1);
		}
	}

	return 1;
}

int vc_blob_to_gray_scale(IVC *src, IVC *dst, int nlabels)
{
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y;
	long int pos;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels))
		return 0;
	if (channels != 1)
		return 0;

	float labelIntens = (float)255 / nlabels;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			dst->data[pos] = src->data[pos] * labelIntens;
		}
	}

	return 0;
}

int vc_draw_center_of_gravity(IVC *img, OVC *blob, int comp)
{
	int c;
	int x, y;

	for (x = blob->xc - comp; x <= blob->xc + comp; x++)
	{
		for (c = 0; c < img->channels; c++)
		{
			img->data[blob->yc * img->bytesperline + x * img->channels + c] = 0;
		}
	}

	for (y = blob->yc - comp; y <= blob->yc + comp; y++)
	{
		for (c = 0; c < img->channels; c++)
		{
			img->data[y * img->bytesperline + blob->xc * img->channels + c] = 0;
		}
	}

	return 1;
}

int vc_draw_bounding_box(IVC *img, OVC *blob)
{
	int c;
	int x, y;

	for (y = blob->y; y < blob->y + blob->height; y++)
	{
		for (c = 0; c < img->channels; c++)
		{
			img->data[y * img->bytesperline + blob->x * img->channels + c] = 255;
			img->data[y * img->bytesperline + (blob->x + blob->width - 1) * img->channels + c] = 255;
		}
	}

	for (x = blob->x; x < blob->x + blob->width; x++)
	{
		for (c = 0; c < img->channels; c++)
		{
			img->data[blob->y * img->bytesperline + x * img->channels + c] = 255;
			img->data[(blob->y + blob->height - 1) * img->bytesperline + x * img->channels + c] = 255;
		}
	}

	return 1;
}

int vc_gray_histogram_show(IVC *src, IVC *dst)
{
	unsigned char *datasrc = (unsigned char *)src->data;
	unsigned char *datadst = (unsigned char *)dst->data;
	int x, y;
	int ni[256] = {0};				  // contador para cada pixel
	float pdf[256];					  // Função de probabilidade da densidade
	float pdfnorm[256];				  // Nomarlização
	float pdfmax = 0;				  // Valor do pixel mais alto
	int n = src->width * src->height; // Número de pixéis na imagem

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if (dst->width != 256 && dst->height != 256)
		return 0;
	if ((src->channels != 1) || (dst->channels != 1))
		return 0;

	// 1º - Contar o brilho dos pixéis
	for (int i = 0; i < n; ni[datasrc[i++]]++)
		;

	// 2º - Calcular a densidade
	for (int i = 0; i < 256; i++)
	{
		pdf[i] = (float)ni[i] / n;
	}

	// 3º - Encontrar o pixel com valor maior
	for (int i = 0; i < 256; i++)
	{
		if (pdf[i] > pdfmax)
		{
			pdfmax = pdf[i];
		}
	}

	// 4º - Normalização
	for (int i = 0; i < 256; i++)
	{
		pdfnorm[i] = pdf[i] / pdfmax;
	}

	// 5º - Gerar o histograma
	// Limpar a imagem para garantir que fica apenas com o histograma
	memset(datadst, 0, 256 * 256 * sizeof(unsigned char));

	// Desenhar
	for (x = 0; x < 256; x++)
	{
		for (y = 256 - 1; y >= (256 - 1) - pdfnorm[x] * 255; y--)
		{
			datadst[y * 256 + x] = 255;
		}
	}

	return 1;
}

int vc_gray_histogram_equalization(IVC *src, IVC *dst)
{
	unsigned char *datasrc = (unsigned char *)src->data;
	int bytesperline_src = src->width * src->channels;
	int channels_src = src->channels;
	unsigned char *datadst = (unsigned char *)dst->data;
	int width = src->width;
	int height = src->height;
	int x, y;
	long int pos_dst;
	int n[256] = {0};
	float pdf[256] = {0}, cdf[256] = {0};
	float size = width * height;
	float min = 0;

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if (dst->width != src->width && dst->height != src->height)
		return 0;
	if ((src->channels != 1) || (dst->channels != 1))
		return 0;

	for (x = 0; x < height * width; x++)
	{
		n[datasrc[x]]++;
	}

	// calcular pdf
	for (x = 0; x <= 255; x++)
	{
		pdf[x] = (((float)n[x]) / size);
	}

	// calcular cdf
	for (y = 0; y <= 255; y++)
	{
		if (y == 0)
		{
			cdf[y] = pdf[y];
		}
		else
		{
			cdf[y] = cdf[y - 1] + pdf[y];
		}
	}

	for (x = 0; x <= 255; x++)
	{
		if (cdf[x] != 0)
		{
			min = pdf[x];
			break;
		}
	}

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_dst = y * bytesperline_src + x * channels_src;
			datadst[pos_dst] = (cdf[datasrc[pos_dst]] - min) / (1.0 - min) * (256 - 1);
		}
	}

	return 1;
}

int vc_gray_edge_prewitt(IVC *src, IVC *dst, float th)
{
	unsigned char *datasrc = (unsigned char *)src->data;
	unsigned char *datadst = (unsigned char *)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y;
	long int posX, posA, posB, posC, posD, posE, posF, posG, posH;
	int i, size;
	float histmax;
	int histthreshold;
	int sumx, sumy;
	float hist[256] = {0.0f};

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels))
		return 0;
	if (channels != 1)
		return 0;

	size = width * height;

	for (y = 1; y < height - 1; y++)
	{
		for (x = 1; x < width - 1; x++)
		{
			posA = (y - 1) * bytesperline + (x - 1) * channels;
			posB = (y - 1) * bytesperline + x * channels;
			posC = (y - 1) * bytesperline + (x + 1) * channels;
			posD = y * bytesperline + (x - 1) * channels;
			posX = y * bytesperline + x * channels;
			posE = y * bytesperline + (x + 1) * channels;
			posF = (y + 1) * bytesperline + (x - 1) * channels;
			posG = (y + 1) * bytesperline + x * channels;
			posH = (y + 1) * bytesperline + (x + 1) * channels;

			sumx = datasrc[posA] * -1;
			sumx += datasrc[posD] * -1;
			sumx += datasrc[posF] * -1;

			sumx += datasrc[posC] * +1;
			sumx += datasrc[posE] * +1;
			sumx += datasrc[posH] * +1;
			sumx = sumx / 3; // 3 = 1 + 1 + 1

			sumy = datasrc[posA] * -1;
			sumy += datasrc[posB] * -1;
			sumy += datasrc[posC] * -1;

			sumy += datasrc[posF] * +1;
			sumy += datasrc[posG] * +1;
			sumy += datasrc[posH] * +1;
			sumy = sumy / 3; // 3 = 1 + 1 + 1

			datadst[posX] = (unsigned char)(sqrt((double)(sumx * sumx + sumy * sumy)) / sqrt(2.0));
			// Explicação:
			// Queremos que no caso do pior cenário, em que sumx = sumy = 255, o resultado
			// da operação se mantenha no intervalo de valores admitido, isto é, entre [0, 255].
			// Se se considerar que:
			// max = 255
			// Então,
			// sqrt(pow(max,2) + pow(max,2)) * k = max <=> sqrt(2*pow(max,2)) * k = max <=> k = max / (sqrt(2) * max) <=>
			// k = 1 / sqrt(2)
		}
	}

	// Calcular o histograma com o valor das magnitudes
	for (i = 0; i < size; i++)
	{
		hist[datadst[i]]++;
	}

	// Definir o threshold.
	// O threshold é definido pelo nível de intensidade (das magnitudes)
	// quando se atinge uma determinada percentagem de pixeis, definida pelo utilizador.
	// Por exemplo, se o parâmetro 'th' tiver valor 0.8, significa the o threshold será o
	// nível de magnitude, abaixo do qual estão pelo menos 80% dos pixeis.
	histmax = 0.0f;
	for (i = 0; i <= 255; i++)
	{
		histmax += hist[i];

		// th = Prewitt Threshold
		if (histmax >= (((float)size) * th))
			break;
	}
	histthreshold = i == 0 ? 1 : i;

	// Aplicada o threshold
	for (i = 0; i < size; i++)
	{
		if (datadst[i] >= (unsigned char)histthreshold)
			datadst[i] = 255;
		else
			datadst[i] = 0;
	}

	return 0;
}

int vc_gray_edge_sobel(IVC *src, IVC *dst, float th)
{
	unsigned char *datasrc = (unsigned char *)src->data;
	unsigned char *datadst = (unsigned char *)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y;
	long int posX, posA, posB, posC, posD, posE, posF, posG, posH;
	int i, size;
	float histmax;
	int histthreshold;
	int sumx, sumy;
	float hist[256] = {0.0f};

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels))
		return 0;
	if (channels != 1)
		return 0;

	size = width * height;

	for (y = 1; y < height - 1; y++)
	{
		for (x = 1; x < width - 1; x++)
		{
			posA = (y - 1) * bytesperline + (x - 1) * channels;
			posB = (y - 1) * bytesperline + x * channels;
			posC = (y - 1) * bytesperline + (x + 1) * channels;
			posD = y * bytesperline + (x - 1) * channels;
			posX = y * bytesperline + x * channels;
			posE = y * bytesperline + (x + 1) * channels;
			posF = (y + 1) * bytesperline + (x - 1) * channels;
			posG = (y + 1) * bytesperline + x * channels;
			posH = (y + 1) * bytesperline + (x + 1) * channels;

			sumx = datasrc[posA] * -1;
			sumx += datasrc[posD] * -2; // Peso dobrado comparado com Prewitt
			sumx += datasrc[posF] * -1;
			sumx += datasrc[posC] * +1;
			sumx += datasrc[posE] * +2; // Peso dobrado comparado com Prewitt
			sumx += datasrc[posH] * +1;

			sumy = datasrc[posA] * -1;
			sumy += datasrc[posB] * -2; // Peso dobrado comparado com Prewitt
			sumy += datasrc[posC] * -1;
			sumy += datasrc[posF] * +1;
			sumy += datasrc[posG] * +2; // Peso dobrado comparado com Prewitt
			sumy += datasrc[posH] * +1;

			datadst[posX] = (unsigned char)(sqrt((double)(sumx * sumx + sumy * sumy)) / sqrt(2.0));
		}
	}

	// Calcular o histograma com o valor das magnitudes
	for (i = 0; i < size; i++)
	{
		hist[datadst[i]]++;
	}

	// Definir o threshold.
	histmax = 0.0f;
	for (i = 0; i <= 255; i++)
	{
		histmax += hist[i];
		if (histmax >= (((float)size) * th))
			break;
	}
	histthreshold = i == 0 ? 1 : i;

	// Aplicar o threshold
	for (i = 0; i < size; i++)
	{
		if (datadst[i] >= (unsigned char)histthreshold)
			datadst[i] = 255;
		else
			datadst[i] = 0;
	}

	return 1;
}

int vc_gray_lowpass_mean_filter(IVC *src, IVC *dst, int kernelsize)
{
	// Verificações iniciais: dimensões e canais
	if (src == NULL || dst == NULL)
		return 0;
	if (src->width != dst->width || src->height != dst->height)
		return 0;
	if (src->channels != 1 || dst->channels != 1)
		return 0;

	int width = src->width;
	int height = src->height;
	unsigned char *datasrc = (unsigned char *)src->data;
	unsigned char *datadst = (unsigned char *)dst->data;
	int offset = kernelsize / 2;
	float sum;
	int count;

	// Processa cada pixel da imagem
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			sum = 0;
			count = 0;

			// Aplica o kernel na vizinhança do pixel
			for (int ky = -offset; ky <= offset; ky++)
			{
				for (int kx = -offset; kx <= offset; kx++)
				{
					int pos_x = x + kx;
					int pos_y = y + ky;

					// Verifica limites da imagem
					if (pos_x >= 0 && pos_x < width && pos_y >= 0 && pos_y < height)
					{
						sum += datasrc[pos_y * width + pos_x];
						count++;
					}
				}
			}

			// Calcula a média e define o valor no destino
			datadst[y * width + x] = (unsigned char)(sum / count);
		}
	}

	return 1; // Sucesso
}

// Função para comparar valores (usada pelo qsort)
int compare(const void *a, const void *b)
{
	unsigned char val1 = *(unsigned char *)a;
	unsigned char val2 = *(unsigned char *)b;
	return (val1 > val2) - (val1 < val2);
}

int vc_gray_lowpass_median_filter(IVC *src, IVC *dst, int kernelsize)
{
	// Verificações iniciais
	if (src == NULL || dst == NULL)
		return 0;
	if (src->width != dst->width || src->height != dst->height)
		return 0;
	if (src->channels != 1 || dst->channels != 1)
		return 0;

	int width = src->width;
	int height = src->height;
	unsigned char *datasrc = (unsigned char *)src->data;
	unsigned char *datadst = (unsigned char *)dst->data;
	int offset = kernelsize / 2;
	unsigned char *neighborhood = malloc(kernelsize * kernelsize * sizeof(unsigned char));
	int count;

	// Processa cada pixel da imagem
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			count = 0;

			// Coleta os valores dos pixels na vizinhança
			for (int ky = -offset; ky <= offset; ky++)
			{
				for (int kx = -offset; kx <= offset; kx++)
				{
					int pos_x = x + kx;
					int pos_y = y + ky;

					// Verifica os limites da imagem
					if (pos_x >= 0 && pos_x < width && pos_y >= 0 && pos_y < height)
					{
						neighborhood[count++] = datasrc[pos_y * width + pos_x];
					}
				}
			}

			// Ordena a vizinhança e seleciona o valor mediano
			qsort(neighborhood, count, sizeof(unsigned char), compare);
			datadst[y * width + x] = neighborhood[count / 2];
		}
	}

	free(neighborhood); // Libera memória alocada
	return 1;			// Sucesso
}

int vc_bgr_to_rgb(IVC *src, IVC *dst)
{
	unsigned char *datasrc = (unsigned char *)src->data;
	int channels_src = src->channels;
	int bytesperline_src = src->bytesperline;
	unsigned char *datadst = (unsigned char *)dst->data;
	int width = src->width;
	int height = src->height;
	long int pos;

	memcpy(datadst, datasrc, width * height * 3);

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			pos = y * bytesperline_src + x * channels_src;
			unsigned char blue = datadst[pos];
			datadst[pos] = datadst[pos + 2];
			datadst[pos + 2] = blue;
		}
	}

	return 0;
}

// dst - corpo das resistencias
// img_colors - estrutura com imagem de cada cor separadamente
int vc_hsv_resistances_segmentation(IVC *src, IVC *dst, ImageColors *img_colors)
{
	// CONF - cores(HSV)
	ColorRange colors[12] = {
		{30, 55, 35, 64, 45, 90},	  // Corpo resistência
		{0, 360, 0, 35, 0, 35},		  // Preto
		{5, 30, 20, 50, 20, 50},	  // Castanho
		{340, 15, 51, 90, 51, 100},	  // Vermelho
		{12, 21, 65, 100, 78, 100},	  // Laranja
		{20, 35, 50, 100, 50, 100},	  // Amarelo
		{75, 165, 30, 100, 30, 100},  // Verde
		{170, 240, 20, 100, 30, 100}, // Azul
		{220, 320, 30, 100, 30, 100}, // Roxo
		{0, 360, 0, 10, 20, 80},	  // Cinza
		{0, 360, 0, 0, 90, 100},	  // Branco
		{7, 12, 70, 100, 82, 100}	  // Laranja v2
	};

	unsigned char *datasrc = (unsigned char *)src->data;
	int bytesperline_src = src->width * src->channels;
	int channels_src = src->channels;
	unsigned char *datadst = (unsigned char *)dst->data;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_dst = dst->channels;
	int width = src->width;
	int height = src->height;
	int x, y;
	float max, min, hue, sat, valor, delta;
	long int pos_src, pos_dst;
	float rf, gf, bf;

	// verificalão de errors
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if ((src->width != dst->width) || (src->height != dst->height))
		return 0;
	if ((src->channels != 3) || (dst->channels != 1))
		return 0;

	// meter em hsv
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;

			rf = (float)datasrc[pos_src];
			gf = (float)datasrc[pos_src + 1];
			bf = (float)datasrc[pos_src + 2];

			max = (rf > gf ? (rf > bf ? rf : bf) : (gf > bf ? gf : bf));
			min = (rf < gf ? (rf < bf ? rf : bf) : (gf < bf ? gf : bf));
			delta = max - min;

			// calcular value
			valor = max;

			// calcular saturação
			if (max == 0 || max == min)
			{
				sat = 0;
				hue = 0;
			}
			else
			{
				sat = delta * 100.0f / valor;
				// calcular hue
				//  Quando o vermelho é o maior, Hue será um ângulo entre 300 e 360 ou entre 0 e 60
				if (rf == max && gf >= bf)
				{
					hue = 60.0f * (gf - bf) / delta;
				}
				else if (rf == max && bf > gf)
				{
					hue = 360 + 60.0f * (gf - bf) / delta;
				}
				else if (gf == max)
				{
					hue = 120 + 60.0f * (bf - rf) / delta;
				}
				else if (max == bf)
				{
					hue = 240 + 60.0f * (rf - gf) / delta;
				}
			}

			if (hue <= colors[0].maxHue && hue >= colors[0].minHue &&
				sat <= colors[0].maxSaturation && sat >= colors[0].minSaturation &&
				valor <= colors[0].maxValue / 100.0f * 255 && valor >= colors[0].minValue / 100.0f * 255)
			{
				datadst[pos_dst] = 255; // Corpo resistência
				img_colors->corpo->data[pos_dst] = 255;
			}
			else if (hue <= colors[2].maxHue && hue >= colors[2].minHue &&
					 sat <= colors[2].maxSaturation && sat >= colors[2].minSaturation &&
					 valor <= colors[2].maxValue / 100.0f * 255 && valor >= colors[2].minValue / 100.0f * 255)
			{
				datadst[pos_dst] = 255; // Castanho
				img_colors->castanho->data[pos_dst] = 255;
			}
			else if (hue <= colors[4].maxHue && hue >= colors[4].minHue &&
					 sat <= colors[4].maxSaturation && sat >= colors[4].minSaturation &&
					 valor <= colors[4].maxValue / 100.0f * 255 && valor >= colors[4].minValue / 100.0f * 255)
			{
				datadst[pos_dst] = 255; // Laranja
				img_colors->laranja->data[pos_dst] = 255;
			}
			else if (hue <= colors[11].maxHue && hue >= colors[11].minHue &&
					 sat <= colors[11].maxSaturation && sat >= colors[11].minSaturation &&
					 valor <= colors[11].maxValue / 100.0f * 255 && valor >= colors[11].minValue / 100.0f * 255)
			{
				datadst[pos_dst] = 255; // Laranja v2
				img_colors->laranja->data[pos_dst] = 255;
			}
			else if (colors[3].minHue > colors[3].maxHue &&
					 ((hue >= 0 && hue <= colors[3].maxHue) || (hue <= 360 && hue >= colors[3].minHue)) &&
					 sat <= colors[3].maxSaturation && sat >= colors[3].minSaturation &&
					 valor <= colors[3].maxValue / 100.0f * 255 && valor >= colors[3].minValue / 100.0f * 255)
			{
				datadst[pos_dst] = 255; // Vermelho - minHue > maxHue
				img_colors->vermelho->data[pos_dst] = 255;
			}
			else if (colors[3].minHue < colors[3].maxHue &&
					 hue <= colors[3].maxHue && hue >= colors[3].minHue &&
					 sat <= colors[3].maxSaturation && sat >= colors[3].minSaturation &&
					 valor <= colors[3].maxValue / 100.0f * 255 && valor >= colors[3].minValue / 100.0f * 255)
			{
				datadst[pos_dst] = 255; // Vermelho - minHue < maxHue
				img_colors->vermelho->data[pos_dst] = 255;
			}
			else if (hue <= colors[5].maxHue && hue >= colors[5].minHue &&
					 sat <= colors[5].maxSaturation && sat >= colors[5].minSaturation &&
					 valor <= colors[5].maxValue / 100.0f * 255 && valor >= colors[5].minValue / 100.0f * 255)
			{
				datadst[pos_dst] = 255; // Amarelo
				img_colors->amarelo->data[pos_dst] = 255;
			}
			else if (hue <= colors[6].maxHue && hue >= colors[6].minHue &&
					 sat <= colors[6].maxSaturation && sat >= colors[6].minSaturation &&
					 valor <= colors[6].maxValue / 100.0f * 255 && valor >= colors[6].minValue / 100.0f * 255)
			{
				datadst[pos_dst] = 255; // Verde
				img_colors->verde->data[pos_dst] = 255;
			}
			else if (hue <= colors[7].maxHue && hue >= colors[7].minHue &&
					 sat <= colors[7].maxSaturation && sat >= colors[7].minSaturation &&
					 valor <= colors[7].maxValue / 100.0f * 255 && valor >= colors[7].minValue / 100.0f * 255)
			{
				datadst[pos_dst] = 255; // Azul
				img_colors->azul->data[pos_dst] = 255;
			}
			/* else if (hue <= colors[8].maxHue && hue >= colors[8].minHue &&
					 sat <= colors[8].maxSaturation && sat >= colors[8].minSaturation &&
					 valor <= colors[8].maxValue / 100.0f * 255 && valor >= colors[8].minValue / 100.0f * 255)
			{
				datadst[pos_dst] = 255; // Roxo
				img_colors->roxo->data[pos_dst] = 255;
			} */
			/* else if (hue <= colors[9].maxHue && hue >= colors[9].minHue &&
					 sat <= colors[9].maxSaturation && sat >= colors[9].minSaturation &&
					 valor <= colors[9].maxValue / 100.0f * 255 && valor >= colors[9].minValue / 100.0f * 255)
			{
				datadst[pos_dst] = 255; // Cinza
				img_colors->cinza->data[pos_dst] = 255;
			}
			else if (hue <= colors[10].maxHue && hue >= colors[10].minHue &&
					 sat <= colors[10].maxSaturation && sat >= colors[10].minSaturation &&
					 valor <= colors[10].maxValue / 100.0f * 255 && valor >= colors[10].minValue / 100.0f * 255)
			{
				datadst[pos_dst] = 255; // Branco
				img_colors->branco->data[pos_dst] = 255;
			} */
			else if (hue <= colors[1].maxHue && hue >= colors[1].minHue &&
					 sat <= colors[1].maxSaturation && sat >= colors[1].minSaturation &&
					 valor <= colors[1].maxValue / 100.0f * 255 && valor >= colors[1].minValue / 100.0f * 255)
			{
				datadst[pos_dst] = 255; // Preto
				img_colors->preto->data[pos_dst] = 255;
			}
			else
			{
				datadst[pos_dst] = 0;
				img_colors->corpo->data[pos_dst] = 0;
				img_colors->preto->data[pos_dst] = 0;
				img_colors->castanho->data[pos_dst] = 0;
				img_colors->vermelho->data[pos_dst] = 0;
				img_colors->laranja->data[pos_dst] = 0;
				img_colors->amarelo->data[pos_dst] = 0;
				img_colors->verde->data[pos_dst] = 0;
				img_colors->azul->data[pos_dst] = 0;
				// img_colors->roxo->data[pos_dst] = 0;
				// img_colors->cinza->data[pos_dst] = 0;
				// img_colors->branco->data[pos_dst] = 0;
			}
		}
	}

	return 1;
}

ResistenceColorList vc_check_resistence_color(int xpos, int ypos, int width, int height, ImageColors *img_colors, int videoWidth)
{
	int nlabel;
	ResistenceColorList ResColors = {0};

	for (int y = ypos; y < ypos + height; y++)
	{
		for (int x = xpos; x < xpos + width; x++)
		{
			if (x > xpos && x < xpos + width && y > ypos && y < ypos + height)
			{
				int pos = y * videoWidth + x;

				if (img_colors->laranja->data[pos] == 255)
					ResColors.lista_laranja++;
				else if (img_colors->vermelho->data[pos] == 255)
					ResColors.lista_vermelho++;
				else if (img_colors->azul->data[pos] == 255)
					ResColors.lista_azul++;
				else if (img_colors->verde->data[pos] == 255)
					ResColors.lista_verde++;
				else if (img_colors->castanho->data[pos] == 255)
					ResColors.lista_castanho++;
				else if (img_colors->preto->data[pos] == 255)
					ResColors.lista_preto++;
			}
		}
	}

	return ResColors;
}

void vc_initialize_colors(int width, int height, ImageColors *img_colors, int channels, int levels)
{
	img_colors->corpo = vc_image_new(width, height, channels, levels);
	img_colors->preto = vc_image_new(width, height, channels, levels);
	img_colors->castanho = vc_image_new(width, height, channels, levels);
	img_colors->vermelho = vc_image_new(width, height, channels, levels);
	img_colors->laranja = vc_image_new(width, height, channels, levels);
	img_colors->amarelo = vc_image_new(width, height, channels, levels);
	img_colors->verde = vc_image_new(width, height, channels, levels);
	img_colors->azul = vc_image_new(width, height, channels, levels);
	img_colors->roxo = vc_image_new(width, height, channels, levels);
	img_colors->cinza = vc_image_new(width, height, channels, levels);
	img_colors->branco = vc_image_new(width, height, channels, levels);
}

void vc_free_images(ImageColors *img_colors)
{
	if (img_colors->corpo)
		vc_image_free(img_colors->corpo);
	if (img_colors->preto)
		vc_image_free(img_colors->preto);
	if (img_colors->castanho)
		vc_image_free(img_colors->castanho);
	if (img_colors->vermelho)
		vc_image_free(img_colors->vermelho);
	if (img_colors->laranja)
		vc_image_free(img_colors->laranja);
	if (img_colors->amarelo)
		vc_image_free(img_colors->amarelo);
	if (img_colors->verde)
		vc_image_free(img_colors->verde);
	if (img_colors->azul)
		vc_image_free(img_colors->azul);
	if (img_colors->roxo)
		vc_image_free(img_colors->roxo);
	if (img_colors->cinza)
		vc_image_free(img_colors->cinza);
	if (img_colors->branco)
		vc_image_free(img_colors->branco);
}

bool vc_check_resistence_body(int xpos, int ypos, int width, int height, IVC *image)
{
	bool result = false;
	if (image->data != NULL)
	{
		IVC *image_blob = vc_image_new(image->width, image->height, 1, 255);
		int nlabel;
		OVC *blobs = vc_binary_blob_labelling(image, image_blob, &nlabel);
		if (blobs != NULL)
		{
			vc_binary_blob_info(image_blob, blobs, nlabel, 0);
		}

		for (int i = 0; i < nlabel; i++)
		{
			if (blobs[i].area < 200)
			{
				continue;
			}
			if (blobs[i].xc > xpos && blobs[i].xc < xpos + width && blobs[i].yc > ypos && blobs[i].yc < ypos + height)
			{
				result = true;
			}
		}

		free(blobs);
		vc_image_free(image_blob);
	}
	return result;
}

void swap_cores(CorContagemImagem *cor1, CorContagemImagem *cor2)
{
	CorContagemImagem temp = *cor1; // Usa uma variável temporária para armazenar o conteúdo de cor1
	*cor1 = *cor2;					// Copia o conteúdo de cor2 para onde cor1 aponta
	*cor2 = temp;					// Copia o conteúdo da variável temporária (original cor1) para onde cor2 aponta
}

OVC *vc_binary_blob_labelling_custom(IVC *src, IVC *dst, int *nlabels, int xpos, int ypos, int blobWidth, int blobHeight)
{
	unsigned char *datasrc = (unsigned char *)src->data;
	unsigned char *datadst = (unsigned char *)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, a, b;
	long int i, size;
	long int posX, posA, posB, posC, posD;
	int labeltable[1024] = {0};
	int labelarea[1024] = {0};
	int label = 1; // Etiqueta inicial.
	int num, tmplabel;
	OVC *blobs; // Apontador para array de blobs (objectos) que será retornado desta função.

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels))
		return NULL;
	if (channels != 1)
		return NULL;

	// Copia dados da imagem binária para imagem grayscale
	memcpy(datadst, datasrc, bytesperline * height);

	// Todos os pixeis de plano de fundo devem obrigatoriamente ter valor 0
	// Todos os pixeis de primeiro plano devem obrigatoriamente ter valor 255
	// Serão atribuidas etiquetas no intervalo [1,254]
	// Este algoritmo esta assim limitado a 254 labels
	for (i = 0, size = bytesperline * height; i < size; i++)
	{
		if (datadst[i] != 0)
			datadst[i] = 255;
	}

	// Limpa os rebordos da imagem binária
	for (y = 0; y < height; y++)
	{
		datadst[y * bytesperline + 0 * channels] = 0;
		datadst[y * bytesperline + (width - 1) * channels] = 0;
	}

	for (x = 0; x < width; x++)
	{
		datadst[0 * bytesperline + x * channels] = 0;
		datadst[(height - 1) * bytesperline + x * channels] = 0;
	}

	// Efectua a etiquetagem
	for (y = ypos + 1; y < blobHeight + ypos - 1; y++)
	{
		for (x = xpos + 1; x < blobWidth + xpos - 1; x++)
		{
			// Kernel:
			// A B C
			// D X

			posA = (y - 1) * bytesperline + (x - 1) * channels; // A
			posB = (y - 1) * bytesperline + x * channels;		// B
			posC = (y - 1) * bytesperline + (x + 1) * channels; // C
			posD = y * bytesperline + (x - 1) * channels;		// D
			posX = y * bytesperline + x * channels;				// X

			// Se o pixel foi marcado
			if (datadst[posX] != 0)
			{
				if ((datadst[posA] == 0) && (datadst[posB] == 0) && (datadst[posC] == 0) && (datadst[posD] == 0))
				{
					datadst[posX] = label;
					labeltable[label] = label;
					label++;
				}
				else
				{
					num = 255;

					// Se A est  marcado
					if (datadst[posA] != 0)
						num = labeltable[datadst[posA]];
					// Se B est  marcado, e   menor que a etiqueta "num"
					if ((datadst[posB] != 0) && (labeltable[datadst[posB]] < num))
						num = labeltable[datadst[posB]];
					// Se C est  marcado, e   menor que a etiqueta "num"
					if ((datadst[posC] != 0) && (labeltable[datadst[posC]] < num))
						num = labeltable[datadst[posC]];
					// Se D est  marcado, e   menor que a etiqueta "num"
					if ((datadst[posD] != 0) && (labeltable[datadst[posD]] < num))
						num = labeltable[datadst[posD]];

					// Atribui a etiqueta ao pixel
					datadst[posX] = num;
					labeltable[num] = num;

					// Actualiza a tabela de etiquetas
					if (datadst[posA] != 0)
					{
						if (labeltable[datadst[posA]] != num)
						{
							for (tmplabel = labeltable[datadst[posA]], a = 1; a < label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}

					if (datadst[posB] != 0)
					{
						if (labeltable[datadst[posB]] != num)
						{
							for (tmplabel = labeltable[datadst[posB]], a = 1; a < label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}

					if (datadst[posC] != 0)
					{
						if (labeltable[datadst[posC]] != num)
						{
							for (tmplabel = labeltable[datadst[posC]], a = 1; a < label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}

					if (datadst[posD] != 0)
					{
						if (labeltable[datadst[posD]] != num)
						{
							for (tmplabel = labeltable[datadst[posD]], a = 1; a < label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
				}
			}
		}
	}

	// Volta a etiquetar a imagem
	for (y = ypos + 1; y < blobHeight + ypos - 1; y++)
	{
		for (x = xpos + 1; x < blobWidth + xpos - 1; x++)
		{
			posX = y * bytesperline + x * channels; // X

			if (datadst[posX] != 0)
			{
				datadst[posX] = labeltable[datadst[posX]];
			}
		}
	}

	// Contagem do número de blobs
	// Passo 1: Eliminar, da tabela, etiquetas repetidas
	for (a = 1; a < label - 1; a++)
	{
		for (b = a + 1; b < label; b++)
		{
			if (labeltable[a] == labeltable[b])
				labeltable[b] = 0;
		}
	}

	// Passo 2: Conta etiquetas e organiza a tabela de etiquetas, para que não hajam valores vazios (zero) entre etiquetas
	*nlabels = 0;
	for (a = 1; a < label; a++)
	{
		if (labeltable[a] != 0)
		{
			labeltable[*nlabels] = labeltable[a]; // Organiza tabela de etiquetas
			(*nlabels)++;						  // Conta etiquetas
		}
	}

	// Se não ha blobs
	if (*nlabels == 0)
		return NULL;

	// Cria lista de blobs (objectos) e preenche a etiqueta
	blobs = (OVC *)calloc((*nlabels), sizeof(OVC));
	if (blobs != NULL)
	{
		for (a = 0; a < (*nlabels); a++)
			blobs[a].label = labeltable[a];
	}
	else
		return NULL;

	return blobs;
}

int vc_binary_blob_info_custom(IVC *src, OVC *blobs, int nblobs, int areaRelevant, int xpos, int ypos, int blobWidth, int blobHeight)
{
	unsigned char *data = (unsigned char *)src->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, i, j = 0;
	long int pos;
	int xmin, ymin, xmax, ymax;
	long int sumx, sumy;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if (channels != 1)
		return 0;

	// Conta área de cada blob
	for (i = 0; i < nblobs; i++)
	{
		xmin = xpos - 1;
		ymin = ypos - 1;
		xmax = 0;
		ymax = 0;

		sumx = 0;
		sumy = 0;

		blobs[i].area = 0;

		for (y = ypos + 1; y < ypos + blobHeight - 1; y++)
		{
			for (x = xpos + 1; x < xpos + blobWidth - 1; x++)
			{
				pos = y * bytesperline + x * channels;

				if (data[pos] == blobs[i].label)
				{
					// área
					blobs[i].area++;

					// Centro de Gravidade
					sumx += x;
					sumy += y;

					// Bounding Box
					if (xmin > x)
						xmin = x;
					if (ymin > y)
						ymin = y;
					if (xmax < x)
						xmax = x;
					if (ymax < y)
						ymax = y;

					// Perímetro
					// Se pelo menos um dos quatro vizinhos não pertence ao mesmo label, então é um pixel de contorno
					if ((data[pos - 1] != blobs[i].label) || (data[pos + 1] != blobs[i].label) || (data[pos - bytesperline] != blobs[i].label) || (data[pos + bytesperline] != blobs[i].label))
					{
						blobs[i].perimeter++;
					}
				}
			}
		}

		if (blobs[i].area > areaRelevant)
		{
			// Bounding Box
			blobs[j].x = xmin;
			blobs[j].y = ymin;
			blobs[j].width = (xmax - xmin) + 1;
			blobs[j].height = (ymax - ymin) + 1;

			// Centro de Gravidade
			blobs[j].xc = sumx / MAX(blobs[i].area, 1);
			blobs[j].yc = sumy / MAX(blobs[i].area, 1);

			blobs[j].area = blobs[i].area;
			blobs[j].perimeter = blobs[i].perimeter;
			j++;
			continue;
		}

		// Se a área não for relevante
		else if (!areaRelevant)
		{
			// Bounding Box
			blobs[i].x = xmin;
			blobs[i].y = ymin;
			blobs[i].width = (xmax - xmin) + 1;
			blobs[i].height = (ymax - ymin) + 1;

			// Centro de Gravidade
			blobs[i].xc = sumx / MAX(blobs[i].area, 1);
			blobs[i].yc = sumy / MAX(blobs[i].area, 1);
		}
	}

	return 1;
}