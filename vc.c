/**
 * Lib fornecida no decorrer da unidade curricular
 * @brief Lib fornecida no decorrer da unidade curricular
 * @file vc.c
 * @date 2011/2012
 * @author Duarte Duque <dduque@ipca.pt>
 */

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//           INSTITUTO POLIT�CNICO DO C�VADO E DO AVE
//                          2011/2012
//             ENGENHARIA DE SISTEMAS INFORM�TICOS
//                    VIS�O POR COMPUTADOR
//
//             [  DUARTE DUQUE -   ]
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Desabilita (no MSVC++) warnings de fun��es n�o seguras (fopen, sscanf, etc...)
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <malloc.h>
#include "vc.h"
#include <math.h>


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//            FUN��ES: ALOCAR E LIBERTAR UMA IMAGEM
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


// Alocar mem�ria para uma imagem
IVC *vc_image_new(int width, int height, int channels, int levels)
{
	IVC *image = (IVC *) malloc(sizeof(IVC));

	if(image == NULL) return NULL;
	if((levels <= 0) || (levels > 255)) return NULL;

	image->width = width;
	image->height = height;
	image->channels = channels;
	image->levels = levels;
	image->bytesperline = image->width * image->channels;
	image->data = (unsigned char *) malloc(image->width * image->height * image->channels * sizeof(char));

	if(image->data == NULL)
	{
		return vc_image_free(image);
	}

	return image;
}


// Libertar mem�ria de uma imagem
IVC *vc_image_free(IVC *image)
{
	if(image != NULL)
	{
		if(image->data != NULL)
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
//    FUN��ES: LEITURA E ESCRITA DE IMAGENS (PBM, PGM E PPM)
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
char *netpbm_get_token(FILE *file, char *tok, int len)
{
	char *t;
	int c;
	
	for(;;)
	{
		while(isspace(c = getc(file)));
		if(c != '#') break;
		do c = getc(file);
		while((c != '\n') && (c != EOF));
		if(c == EOF) break;
	}
	
	t = tok;
	
	if(c != EOF)
	{
		do
		{
			*t++ = c;
			c = getc(file);
		} while((!isspace(c)) && (c != '#') && (c != EOF) && (t - tok < len - 1));
		
		if(c == '#') ungetc(c, file);
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

	for(y=0; y<height; y++)
	{
		for(x=0; x<width; x++)
		{
			pos = width * y + x;

			if(countbits <= 8)
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
			if((countbits > 8) || (x == width - 1))
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

	for(y=0; y<height; y++)
	{
		for(x=0; x<width; x++)
		{
			pos = width * y + x;

			if(countbits <= 8)
			{
				// Numa imagem PBM:
				// 1 = Preto
				// 0 = Branco
				//datauchar[pos] = (*p & (1 << (8 - countbits))) ? 1 : 0;

				// Na nossa imagem:
				// 1 = Branco
				// 0 = Preto
				datauchar[pos] = (*p & (1 << (8 - countbits))) ? 0 : 1;
				
				countbits++;
			}
			if((countbits > 8) || (x == width - 1))
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
	if((file = fopen(filename, "rb")) != NULL)
	{
		// Efectua a leitura do header
		netpbm_get_token(file, tok, sizeof(tok));

		if(strcmp(tok, "P4") == 0) { channels = 1; levels = 1; }	// Se PBM (Binary [0,1])
		else if(strcmp(tok, "P5") == 0) channels = 1;				// Se PGM (Gray [0,MAX(level,255)])
		else if(strcmp(tok, "P6") == 0) channels = 3;				// Se PPM (RGB [0,MAX(level,255)])
		else
		{
			#ifdef VC_DEBUG
			printf("ERROR -> vc_read_image():\n\tFile is not a valid PBM, PGM or PPM file.\n\tBad magic number!\n");
			#endif

			fclose(file);
			return NULL;
		}
		
		if(levels == 1) // PBM
		{
			if(sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &width) != 1 || 
			   sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &height) != 1)
			{
				#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tFile is not a valid PBM file.\n\tBad size!\n");
				#endif

				fclose(file);
				return NULL;
			}

			// Aloca mem�ria para imagem
			image = vc_image_new(width, height, channels, levels);
			if(image == NULL) return NULL;

			sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height;
			tmp = (unsigned char *) malloc(sizeofbinarydata);
			if(tmp == NULL) return 0;

			#ifdef VC_DEBUG
			printf("\nchannels=%d w=%d h=%d levels=%d\n", image->channels, image->width, image->height, levels);
			#endif

			if((v = fread(tmp, sizeof(unsigned char), sizeofbinarydata, file)) != sizeofbinarydata)
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
			if(sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &width) != 1 || 
			   sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &height) != 1 || 
			   sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &levels) != 1 || levels <= 0 || levels > 255)
			{
				#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tFile is not a valid PGM or PPM file.\n\tBad size!\n");
				#endif

				fclose(file);
				return NULL;
			}

			// Aloca mem�ria para imagem
			image = vc_image_new(width, height, channels, levels);
			if(image == NULL) return NULL;

			#ifdef VC_DEBUG
			printf("\nchannels=%d w=%d h=%d levels=%d\n", image->channels, image->width, image->height, levels);
			#endif

			size = image->width * image->height * image->channels;

			if((v = fread(image->data, sizeof(unsigned char), size, file)) != size)
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
	
	if(image == NULL) return 0;

	if((file = fopen(filename, "wb")) != NULL)
	{
		if(image->levels == 1)
		{
			sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height + 1;
			tmp = (unsigned char *) malloc(sizeofbinarydata);
			if(tmp == NULL) return 0;
			
			fprintf(file, "%s %d %d\n", "P4", image->width, image->height);
			
			totalbytes = unsigned_char_to_bit(image->data, tmp, image->width, image->height);
			printf("Total = %ld\n", totalbytes);
			if(fwrite(tmp, sizeof(unsigned char), totalbytes, file) != totalbytes)
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
		
			if(fwrite(image->data, image->bytesperline, image->height, file) != image->height)
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



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//           INSTITUTO POLIT�CNICO DO C�VADO E DO AVE
//                          2019/2020
//             ENGENHARIA DE SISTEMAS INFORM�TICOS
//                    VIS�O POR COMPUTADOR
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Converter de RGB para Gray
int vc_rgb_to_gray(IVC *src, IVC *dst) {

    // Verifica��o de Erros
    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
    if ((src->width != dst->width) || (src->height != dst->height)) return 0;
    if ((src->channels != 3) || (dst->channels != 1)) return 0;

    long int maxpixels = src->width * src->height * src->channels;
    float rf, gf, bf;

    for (int xy = 0; xy < maxpixels; xy = xy + 3) {
        rf = (float)src->data[xy];
        gf = (float)src->data[xy + 1];
        bf = (float)src->data[xy + 2];

        dst->data[xy / 3] = (unsigned char)((rf * 0.299) + (gf * 0.587) + (bf * 0.114));
    }

    return 1;
}

// Segmenta��o por Thresholding
// Convers�o de imagem cinzenta para Bin�ria
int vc_gray_to_binary(IVC* src,IVC* dst, int threshold) {
    unsigned char *datasrc = (unsigned char *)src->data;
    int bytesperline = src->width * src->channels;
    int channels = src->channels;
    unsigned char *datadst = (unsigned char *)dst->data;
    int bytesperline_dst = dst->width * dst->channels;
    int channels_dst = dst->channels;
    int width = src->width;
    int height = src->height;
    int x, y;
    long int pos_src, pos_dst;

    // Verifica��o de Erros
    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
    if ((src->width != dst->width) || (src->height != dst->height)) return 0;
    if ((src->channels != 1) || (dst->channels != 1)) return 0;
    if (channels != 1) return 0;

    // Ciclo que vai percorrer todos os pixeis da imagem
    for (y = 0; y<height; y++)
    {
        for (x = 0; x<width; x++)
        {
            pos_src = y * bytesperline + x * channels;
            pos_dst = y * bytesperline_dst + x * channels_dst;

            if (datasrc[pos_src] > threshold) datadst[pos_dst] = 255;
            else if (datasrc[pos_src] <= threshold) datadst[pos_src] = 0;
        }
    }
    return 1;
}

// Dilata��o de uma imagem em bin�rio
int vc_binary_dilate(IVC * src, IVC * dst, int kernel)
{
	unsigned char *datasrc = (unsigned char *)src->data;
	int bytesperline_src = src->width * src->channels;
	int channels_src = src->channels;
	unsigned char *datadst = (unsigned char *)dst->data;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_dst = dst->channels;
	int width = src->width;
	int height = src->height;
	long int pos;
	int y, x;
	int aux;

	int offset = kernel / 2;
	int ky, kx;
	long int posk;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels)) return 0;
	if (channels_src != 1) return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline_src + x * channels_src;
			aux = 0;

			// NxM Vizinhos
			for (ky = -offset; ky <= offset; ky++)
			{
				for (kx = -offset; kx <= offset; kx++)
				{
					if ((y + ky >= 0) && (y + ky < height) && (x + kx >= 0) && (x + kx < width))
					{
						posk = (y + ky) * bytesperline_src + (x + kx) * channels_src;

						if (datasrc[posk] == 255) {
							aux = 255;
						}
					}
				}
			}

			if (aux == 255) datadst[pos] = 255;
			else datadst[pos] = 0;
		}
	}
	return 1;

}

// Eros�o de uma imagem em Bin�rio
int vc_binary_erode(IVC * src, IVC * dst, int kernel)
{
	unsigned char *datasrc = (unsigned char *)src->data;
	int bytesperline_src = src->width * src->channels;
	int channels_src = src->channels;
	unsigned char *datadst = (unsigned char *)dst->data;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_dst = dst->channels;
	int width = src->width;
	int height = src->height;
	long int pos;
	int y, x;
	int nb;

	int offset = kernel / 2;
	int ky, kx;
	long int posk;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels)) return 0;
	if (channels_src != 1) return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline_src + x * channels_src;
			nb = 255;

			// NxM Vizinhos
			for (ky = -offset; ky <= offset; ky++)
			{
				for (kx = -offset; kx <= offset; kx++)
				{
					if ((y + ky >= 0) && (y + ky < height) && (x + kx >= 0) && (x + kx < width))
					{
						posk = (y + ky) * bytesperline_src + (x + kx) * channels_src;

						if (datasrc[posk] == 0) {
							nb = 0;
						}
					}
				}
			}

			if (nb != 0) datadst[pos] = 255;
			else datadst[pos] = 0;
		}
	}
	return 1;

}

// Fecho de uma imagem em Bin�rio
int vc_binary_close(IVC *src, IVC *dst, int kernel)
{
    int ret = 1;

    IVC *aux = vc_image_new(src->width, src->height, src->channels, src->levels);

    ret &= vc_binary_dilate(src, aux, kernel);
    ret &= vc_binary_erode(aux, dst, kernel);

    vc_image_free(aux);

    return ret;
}



OVC* vc_binary_blob_labelling(IVC *src, IVC *dst, int *nlabels) {

    unsigned char *datasrc = (unsigned char *)src->data;
    unsigned char *datadst = (unsigned char *)dst->data;
    int width = src->width;
    int height = src->height;
    int bytesperline = src->bytesperline;
    int channels = src->channels;
    int x, y, a, b;
    long int i, size;
    long int posX, posA, posB, posC, posD;
    int labeltable[2048] = { 0 };
    int labelarea[2048] = { 0 };
    int label = 1; // Etiqueta inicial.
    int num, tmplabel;
    OVC *blobs; // Apontador para array de blobs (objectos) que ser� retornado desta fun��o.

    // Verifica��o de erros
    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
    if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels)) return NULL;
    if (channels != 1) return NULL;

    // Copia dados da imagem bin�ria para imagem grayscale
    memcpy(datadst, datasrc, bytesperline * height);

    // Todos os pix�is de plano de fundo devem obrigat�riamente ter valor 0
    // Todos os pix�is de primeiro plano devem obrigat�riamente ter valor 255
    // Ser�o atribu�das etiquetas no intervalo [1,254]
    // Este algoritmo est� assim limitado a 255 labels
    for (i = 0, size = bytesperline * height; i<size; i++)
        if (datadst[i] != 0) datadst[i] = 255;

    // Limpa os rebordos da imagem bin�ria
    for (y = 0; y<height; y++) {
        datadst[y * bytesperline + 0 * channels] = 0;
        datadst[y * bytesperline + (width - 1) * channels] = 0;
    }

    for (x = 0; x<width; x++) {
        datadst[0 * bytesperline + x * channels] = 0;
        datadst[(height - 1) * bytesperline + x * channels] = 0;
    }

    // Efectua a etiquetagem
    for (y = 1; y<height - 1; y++) {
        for (x = 1; x<width - 1; x++) {
            // Kernel:
            // A B C
            // D X

            posA = (y - 1) * bytesperline + (x - 1) * channels; // A
            posB = (y - 1) * bytesperline + x * channels; // B
            posC = (y - 1) * bytesperline + (x + 1) * channels; // C
            posD = y * bytesperline + (x - 1) * channels; // D
            posX = y * bytesperline + x * channels; // X

            // Se o pixel foi marcado
            if (datadst[posX] != 0) {
                if ((datadst[posA] == 0) && (datadst[posB] == 0) && (datadst[posC] == 0) && (datadst[posD] == 0)) {
                    datadst[posX] = label;
                    labeltable[label] = label;
                    label++;
                }
                else {
                    num = 255;

                    // Se A est� marcado
                    if (datadst[posA] != 0) num = labeltable[datadst[posA]];
                    // Se B est� marcado, e � menor que a etiqueta "num"
                    if ((datadst[posB] != 0) && (labeltable[datadst[posB]] < num)) num = labeltable[datadst[posB]];
                    // Se C est� marcado, e � menor que a etiqueta "num"
                    if ((datadst[posC] != 0) && (labeltable[datadst[posC]] < num)) num = labeltable[datadst[posC]];
                    // Se D est� marcado, e � menor que a etiqueta "num"
                    if ((datadst[posD] != 0) && (labeltable[datadst[posD]] < num)) num = labeltable[datadst[posD]];

                    // Atribui a etiqueta ao pixel
                    datadst[posX] = num;
                    labeltable[num] = num;

                    // Actualiza a tabela de etiquetas
                    if (datadst[posA] != 0) {
                        if (labeltable[datadst[posA]] != num) {
                            for (tmplabel = labeltable[datadst[posA]], a = 1; a<label; a++) {
                                if (labeltable[a] == tmplabel)
                                    labeltable[a] = num;
                            }
                        }
                    }
                    if (datadst[posB] != 0) {
                        if (labeltable[datadst[posB]] != num) {
                            for (tmplabel = labeltable[datadst[posB]], a = 1; a<label; a++) {
                                if (labeltable[a] == tmplabel)
                                    labeltable[a] = num;
                            }
                        }
                    }
                    if (datadst[posC] != 0) {
                        if (labeltable[datadst[posC]] != num) {
                            for (tmplabel = labeltable[datadst[posC]], a = 1; a<label; a++) {
                                if (labeltable[a] == tmplabel)
                                    labeltable[a] = num;
                            }
                        }
                    }
                    if (datadst[posD] != 0) {
                        if (labeltable[datadst[posD]] != num) {
                            for (tmplabel = labeltable[datadst[posC]], a = 1; a<label; a++) {
                                if (labeltable[a] == tmplabel)
                                    labeltable[a] = num;
                            }
                        }
                    }
                }
            }
        }
    }

    // Volta a etiquetar a imagem
    for (y = 1; y<height - 1; y++) {
        for (x = 1; x<width - 1; x++) {
            posX = y * bytesperline + x * channels; // X

            if (datadst[posX] != 0) {
                datadst[posX] = labeltable[datadst[posX]];
            }
        }
    }

    //printf("\nMax Label = %d\n", label);

    // Contagem do n�mero de blobs
    // Passo 1: Eliminar, da tabela, etiquetas repetidas
    for (a = 1; a<label - 1; a++) {
        for (b = a + 1; b<label; b++)
            if (labeltable[a] == labeltable[b]) labeltable[b] = 0;
    }
    // Passo 2: Conta etiquetas e organiza a tabela de etiquetas, para que n�o hajam valores vazios (zero) entre etiquetas
    *nlabels = 0;
    for (a = 1; a<label; a++) {
        if (labeltable[a] != 0) {
            labeltable[*nlabels] = labeltable[a]; // Organiza tabela de etiquetas
            (*nlabels)++; // Conta etiquetas
        }
    }

    // Se n�o h� blobs
    if (*nlabels == 0) return NULL;

    // Cria lista de blobs (objectos) e preenche a etiqueta
    blobs = (OVC *)calloc((*nlabels), sizeof(OVC));

    if (blobs != NULL)
        for (a = 0; a<(*nlabels); a++) blobs[a].label = labeltable[a];
    else return NULL;

    return blobs;
}

// Extra��o de informa��o referente a Blobs
int vc_binary_blob_info(IVC *src, OVC *blobs, int nblobs) {

    unsigned char *data = (unsigned char *)src->data;
    int width = src->width;
    int height = src->height;
    int bytesperline = src->bytesperline;
    int channels = src->channels;
    int x, y, i;
    long int pos;
    int xmin, ymin, xmax, ymax;
    long int sumx, sumy;

    // Verifica��o de erros
    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
    if (channels != 1) return 0;

    // Conta �rea de cada blob
    for (i = 0; i<nblobs; i++) {
        xmin = width - 1;
        ymin = height - 1;
        xmax = 0;
        ymax = 0;

        sumx = 0;
        sumy = 0;

        blobs[i].area = 0;

        for (y = 1; y<height - 1; y++) {
            for (x = 1; x<width - 1; x++) {
                pos = y * bytesperline + x * channels;

                if (data[pos] == blobs[i].label) {
                    // �rea
                    blobs[i].area++;

                    // Centro de Gravidade
                    sumx += x;
                    sumy += y;

                    // Bounding Box
                    if (xmin > x) xmin = x;
                    if (ymin > y) ymin = y;
                    if (xmax < x) xmax = x;
                    if (ymax < y) ymax = y;

                    // Per�metro
                    // Se pelo menos um dos quatro vizinhos n�o pertence ao mesmo label, ent�o � um pixel de contorno
                    if ((data[pos - 1] != blobs[i].label) || (data[pos + 1] != blobs[i].label) || (data[pos - bytesperline] != blobs[i].label) || (data[pos + bytesperline] != blobs[i].label))
                        blobs[i].perimeter++;
                }
            }
        }

        // Bounding Box
        blobs[i].x = xmin;
        blobs[i].y = ymin;
        blobs[i].width = (xmax - xmin) + 1;
        blobs[i].height = (ymax - ymin) + 1;

        // Centro de Gravidade
        //blobs[i].xc = (xmax - xmin) / 2;
        //blobs[i].yc = (ymax - ymin) / 2;
        blobs[i].xc = sumx / MAX(blobs[i].area, 1);
        blobs[i].yc = sumy / MAX(blobs[i].area, 1);
    }

    return 1;
}

void rgb2bgrinvert(IVC *imagem) {
    int temp;
    long int maxpixels = imagem->width * imagem->height * 3;
    for (int xy = 0; xy < maxpixels; xy = xy + 3) {
        temp = imagem->data[xy];
        imagem->data[xy] = imagem->data[xy+2]; // r
        imagem->data[xy+2] = temp; // b
    }
}

// Construa uma função que exiba o histograma de uma imagem em tons de cinzento
int vc_gray_histogram_show(IVC* src, IVC* dst)
{
    unsigned char* datasrc = (unsigned char*)src->data;
    unsigned char* datadst = (unsigned char*)dst->data;
    int width = src->width;
    int height = src->height;
    int bytesperline = src->bytesperline;
    int channels = src->channels;
    int levels = src->levels;
    int x, y, cnt = 0, w = 255, b = 0;
    long int pos;
    int quantidades[256], maior = 0, step = width / 256, posc, z = 0;

    // Verificação de erros
    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
    if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels)) return 0;
    if (channels != 1) return 0;

    // array a zeros
    for (int i = 0; i < 256; i++) {
        quantidades[i] = 0;
    }

    //contagem
    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            pos = y * bytesperline + x * channels;
            posc = (int)datasrc[pos];
            quantidades[posc]++;
        }
    }

    //maior valor
    for (int i = 0; i < 256; i++) {
        if (maior < quantidades[i]) maior = quantidades[i];
    }

    // calcular pixeis para histograma
    for (int i = 0; i < 256; i++) {
        quantidades[i] = (quantidades[i] * (height - 125)) / maior;
    }

    // inverter quantidades
    for (int i = 0; i < 256; i++) {
        quantidades[i] = (height-75) - quantidades[i];
    }

    // criar histograma
    for (y = 0; y < (height-75); y++)
    {
        for (x = 0; x < (width / step); x++)
        {
            pos = y * bytesperline + x * step;
            for (int q = 0; q < step; q++) {
                if (quantidades[x] != 0) {
                    datadst[pos] = w;
                    pos++;
                }
                else {
                    datadst[pos] = b;
                    pos++;
                }
            }
            if (quantidades[x] != 0)quantidades[x]--;
        }
    }

    // gradiente
    for (y = (height - 75); y < height; y++)
    {
        for (x = 0; x < (width / step); x++)
        {
            pos = y * bytesperline + x * step;

            for (int q = 0; q < step; q++) {
                datadst[pos] = x;
                pos++;
            }
        }
    }

    return 1;
}


// Construa uma função que realize a equalização de uma imagem em tons de cinzento
int vc_gray_histogram_equalization(IVC* srcdst)
{
    unsigned char* data = (unsigned char*)srcdst->data;
    int width = srcdst->width;
    int height = srcdst->height;
    int bytesperline = srcdst->bytesperline;
    int channels = srcdst->channels;
    int levels = srcdst->levels;
    int x, y, cnt = 0, w = 255, b = 0;
    long int pos;
    int quantidades[256], maior = 0, step = width / 256, posc, z = 0;
    float niveis[256];

    // Verificação de erros
    if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL)) return 0;
    if (channels != 1) return 0;

    // array a zeros
    for (int i = 0; i < 256; i++) {
        quantidades[i] = 0;
        niveis[i] = 0;
    }

    //contagem
    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            pos = y * bytesperline + x * channels;
            posc = (int)data[pos];
            quantidades[posc]++;
        }
    }

    // valores acumulados
    for (int i = 1; i < 256; i++) {
        quantidades[i] = quantidades[i] + quantidades[i - 1];
    }

    // calcular percentagens
    for (int i = 0; i < 255; i++) {
        niveis[i] = (float)quantidades[i] / (float)(height * width);
    }

    // realizar equalização
    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            pos = y * bytesperline + x * channels;

            data[pos] = niveis[(int)data[pos]] * levels;
        }
    }

    return 1;
}

float vc_rgb_max(int r, int g, int b) {
	return (r > g ? (r > b ? r : b) : (g > b ? g : b));
}
float vc_rgb_min(int r, int g, int b) {
	return (r < g ? (r < b ? r : b) : (g < b ? g : b));
}

int vc_rgb_to_hsv(IVC* srcdst) {
	float hue, saturation, value;
	float rgb_max, rgb_min;
	float max_min_diff;

	if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL)) return 0;
	if (srcdst->channels != 3) return 0;

	int size = srcdst->width * srcdst->height * srcdst->channels;

	for (int i = 0; i < size; i += srcdst->channels) {
		float r = (float)srcdst->data[i];
		float g = (float)srcdst->data[i + 1];
		float b = (float)srcdst->data[i + 2];

		
		// Max and min RGB
		rgb_max = vc_rgb_max(r,g,b);
		rgb_min = vc_rgb_min(r,g,b);

		// Default values
		hue = 0;
		saturation = 0;
		

		// Value toma valores entre [0,255]
		value = rgb_max;
		if (value != 0) {
			max_min_diff = (rgb_max - rgb_min);

			// Saturation toma valores entre [0,255]
			saturation = (max_min_diff / value) * 255;

			if (saturation != 0) {
				// Hue toma valores entre [0,360]
				if ((rgb_max == r) && (g >= b)) {
					hue = 60 * (g - b) / max_min_diff;
				}
				else if ((rgb_max == r) && (b > g)) {
					hue = 360 + 60 * (g - b) / max_min_diff;
				}
				else if (rgb_max == g) {
					hue = 120 + 60 * (b - r) / max_min_diff;
				}
				else /* rgb_max == b*/ {
					hue = 240 + 60 * (r - g) / max_min_diff;
				}
			}
		}

		// Atribui valores entre [0,255]
		srcdst->data[i] = (unsigned char)(hue / 360 * 255);
		srcdst->data[i + 1] = (unsigned char)(saturation);
		srcdst->data[i + 2] = (unsigned char)(value);
	}

	return 1;
}

// hmin,hmax = [0, 360]; smin,smax = [0, 100]; vmin,vmax = [0, 100]
int vc_hsv_segmentation(IVC* srcdst, int hmin, int hmax, int smin, int smax, int vmin, int vmax) {

	int h, s, v; // h=[0, 360] s=[0, 100] v=[0, 100]

	// Verificação de erros
	if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL)) return 0;
	if (srcdst->channels != 3) return 0;

	int size = srcdst->width * srcdst->height * srcdst->channels;

	for (int i = 0; i < size; i += srcdst->channels) {
		h = ((float)srcdst->data[i]) / 255.0f * 360.0f;
		s = ((float)srcdst->data[i + 1]) / 255.0f * 100.0f;
		v = ((float)srcdst->data[i + 2]) / 255.0f * 100.0f;

		if (
			(h > hmin) && (h <= hmax) && 
			(s >= smin) && (s <= smax) && 
			(v >= vmin) && (v <= vmax)
			) {
			srcdst->data[i] = 255;
			srcdst->data[i + 1] = 255;
			srcdst->data[i + 2] = 255;
		}
		else {
			srcdst->data[i] = 0;
			srcdst->data[i + 1] = 0;
			srcdst->data[i + 2] = 0;
		}
	}

	return 1;
}