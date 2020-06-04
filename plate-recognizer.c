/**
 * Este ficheiro contem as funções que foram criadas a fim de reconhecimento das matriculas propostas no TP2
 * @brief Ficheiro que contem funções criadas para o processamento de uma imagem a fim de extrair a matricula e seus caracteres
 * @file plate-recognizer.c
 * @date 04 Maio de 2020
 * @author Rafael Pereira <a13871@alunos.ipca.pt>
 * @author Óscar Silva <a14383@alunos.ipca.pt>
 * @author Daniel Torres <a17442@alunos.ipca.pt>
 */

#include <limits.h> // PATH_MAX
#include <string.h> // strcpy()
#include <stdlib.h> // EXIT_FAILURE, EXIT_SUCCESS
#include "dirent.h"
#include <sys/stat.h> // para o stat() // S_ISDIR Macro
#include <sys/types.h>
#include <stdio.h> // puts() printf
#include <math.h>
#include "plate-recognizer.h"

int id = 0;


char output_dir[PATH_MAX] = "C:\\IPCA\\VC_TP2\\img";


void debugSave(char* filen, int id, IVC* src) {
	return;
    char fileimagename[PATH_MAX];
    sprintf(fileimagename, "%s/%s_%d.ppm", output_dir, filen, id);
    vc_write_image(fileimagename, src);
}


long int histogram(IVC* src, float histograma[]) {
	int size = src->width * src->height * src->channels;
	int ocorrencias[256];
	unsigned char pixel_value;
	long int total_pixels = 0;
	for (int i = 0; i < 255; i++) ocorrencias[i] = 0;
	for (int i = 0; i < 255; i++) histograma[i] = 0;

	if (src->channels != 1) exit(1);

	for (int i = 0; i < size; i += src->channels) {
		pixel_value = src->data[i];
		ocorrencias[pixel_value] = ocorrencias[pixel_value] + 1;
		total_pixels++;
	}

	for (int i = 0; i <= 255; i++) {
		histograma[i] = (float)ocorrencias[i] / (float)total_pixels;
	}
	return total_pixels;
}

int metodo_otsu(float* histogram, long int total_pixels) {
	double probabilidade[256], media[256];
	double max_entre, entre[256];
	int threshold;


	for (int i = 0; i < 256; i++) {
		probabilidade[i] = 0.0;
		media[i] = 0.0;
		entre[i] = 0.0;
	}

	probabilidade[0] = histogram[0];

	for (int i = 1; i < 256; i++) {
		probabilidade[i] = probabilidade[i - 1] + histogram[i];
		media[i] = media[i - 1] + i * histogram[i];
	}

	threshold = 0;
	max_entre = 0.0;

	for (int i = 0; i < 255; i++) {
		if (probabilidade[i] != 0.0 && probabilidade[i] != 1.0)
			entre[i] = pow(media[255] * probabilidade[i] - media[i], 2) / (probabilidade[i] * (1.0 - probabilidade[i]));
		else
			entre[i] = 0.0;

		if (entre[i] > max_entre) {
			max_entre = entre[i];
			threshold = i;
		}
	}

	return threshold;
}

/**
 * Verifica se o directorio existe e se é um directorio
 * @param path
 * @return 1 se for directorio e existir
 */
int directory_exists(const char *path) {
    struct stat filestats;
    stat(path, &filestats);
    return S_ISDIR(filestats.st_mode);
}

/**
 * Returns if file exist
 * @param path
 * @return
 */
int file_exists(const char *path) {
    struct stat filestats;
    stat(path, &filestats);
    return S_ISREG(filestats.st_mode);
}


/**
 * Convert reg to grayscale from r g b
 * @param r
 * @param g
 * @param b
 * @return
 */
int rgb_to_gray(int r, int g, int b) {
    int grey = (r * 0.299) + (g * 0.587) + (b * 0.114);
    return (unsigned char)(grey>255 ? 255 : grey);
}

/**
 *
 * @param src
 */
void invertImageBinary(IVC *src) {
    int size = src->height * src->width;
    for (int x = 0; x < size;x++) {
        src->data[x] = (src->data[x]== 0 ? 255:0);
    }
}

/**
 * Fill Image with a value
 * @param src
 * @param value
 */
void fillImage(IVC *src, unsigned char value) {
    long int maxpixels = src->width * src->height * 3;
    for (int xy = 0; xy < maxpixels; xy++) {
        src->data[xy] = value;
        src->data[xy+1] = value;
        src->data[xy+2] = value;
    }
}

/**
 * Extract blog from picture and return white ratio with threshold
 * @param src
 * @param dst
 * @param blob
 * @return
 */
float extractBlob(IVC *src, IVC *dst, OVC blob) {

    // To count pixels from this threshold as potential plate
    int threshold = 200;

    int total_white = 0;
    int bytesperline_src = src->width * src->channels;

    // Mete a imagem a branco
    fillImage(dst,128);
    // Percorre a altura do blob para extrair o blob
    for (int yy = blob.y; yy <= blob.y + blob.height;yy++) {
        // Percorre a largura do blog
        for (int xx = blob.x; xx <= blob.x + blob.width;xx++) {
            int pos = yy * bytesperline_src + xx * src->channels;
            dst->data[pos] = (unsigned char)src->data[pos] ;
            dst->data[pos+1] = (unsigned char)src->data[pos+1];
            dst->data[pos+2] = (unsigned char)src->data[pos+2];

            // Faz um clareamente de 50 tambem
            int grey = rgb_to_gray(dst->data[pos]+50,dst->data[pos+1]+50,dst->data[pos+1]+50);
            // Soma 1 se o pixel for maior que threshold
            // Converte para binário onfly para contar pixeis brancos
            total_white = total_white + (grey > threshold ? 1 : 0);
        }
    }
    return ((float)total_white / blob.area);
}

/**
 * Extract blog from picture and return white ratio with threshold
 * @param src
 * @param dst
 * @param blob
 * @return
 */
float extractBlob2(IVC* src, IVC* dst, OVC blob) {

	// To count pixels from this threshold as potential plate
	int threshold = 200;

	int total_white = 0;
	int bytesperline_src = src->width * src->channels;


	int oo = 0;

	// Mete a imagem a branco
	fillImage(dst, 128);
	// Percorre a altura do blob para extrair o blob
	for (int yy = blob.y; yy <= blob.y + blob.height; yy++) {
		// Percorre a largura do blog
		for (int xx = blob.x; xx <= blob.x + blob.width; xx++) {
			int pos = yy * bytesperline_src + xx * src->channels;
			dst->data[oo] = (unsigned char)src->data[pos];
			dst->data[oo + 1] = (unsigned char)src->data[pos + 1];
			dst->data[oo + 2] = (unsigned char)src->data[pos + 2];
			oo++;
			// Faz um clareamente de 50 tambem
			int grey = rgb_to_gray(dst->data[pos] + 50, dst->data[pos + 1] + 50, dst->data[pos + 1] + 50);
			// Soma 1 se o pixel for maior que threshold
			// Converte para binário onfly para contar pixeis brancos
			total_white = total_white + (grey > threshold ? 1 : 0);
		}
	}
	return ((float)total_white / blob.area);
}

/**
 * Extract blog from picture and return white ratio with threshold
 * @param src
 * @param dst
 * @param blob
 * @return
 */
float extractBlobRGB(IVC* src, IVC* dst, OVC* blob) {
	int o = 1;

	if (src->channels != 3) return 0;

	int bytesperline_src = src->width * src->channels;
	int bytesperline_dst = dst->width * dst->channels;

	// Percorre a altura do blob para extrair o blob
	for (int yy = blob->y; yy <= blob->y + blob->height - 1; yy++) {
		// Percorre a largura do blog
		for (int xx = blob->x; xx <= blob->x + blob->width - 1; xx++) {
			int inside_condition =
				(
				(xx >= blob->x) &&  // x tem de ser maior que x do blob da matricula
					((xx) < (blob->x + blob->width)) &&
					(yy >= blob->y) &&
					((yy) < (blob->y + blob->height))
					);

			int pos = yy * bytesperline_src + xx * src->channels;

			if (inside_condition) {
				int pos_dst = (yy - blob->y) * bytesperline_dst + (xx - blob->x) * dst->channels;
				dst->data[pos_dst] = (unsigned char)src->data[pos];
				dst->data[pos_dst+1] = (unsigned char)src->data[pos+1];
				dst->data[pos_dst+2] = (unsigned char)src->data[pos+2];
				o++;
			}

		}
	}
	return 1;
}

/**
 * Extract blog from picture and return white ratio with threshold
 * @param src
 * @param dst
 * @param blob
 * @return
 */
float extractBlobBinary(IVC *src, IVC *dst, OVC blob) {
    int o=1;

    if (src->channels != 1) return 0;

    int bytesperline_src = src->width * src->channels;
    int bytesperline_dst = dst->width * dst->channels;

    // Percorre a altura do blob para extrair o blob
    for (int yy = blob.y; yy <= blob.y + blob.height-1;yy++) {
        // Percorre a largura do blog
        for (int xx = blob.x; xx <= blob.x + blob.width-1;xx++) {
            int inside_condition =
                    (
                            (xx >= blob.x) &&  // x tem de ser maior que x do blob da matricula
                            ((xx) < (blob.x + blob.width)) &&
                            (yy >= blob.y) &&
                            ((yy) < (blob.y + blob.height))
                    );

            int pos = yy * bytesperline_src + xx * src->channels;

            if (inside_condition) {
                int pos_dst = (yy-blob.y) * bytesperline_dst + (xx-blob.x) * dst->channels;
                dst->data[pos_dst] = (unsigned char)src->data[pos];
                o++;
            }

        }
    }
    return 1;
}


/**
 * Processes a probable plate to find if it has 6 numbers or digits
 * devolve os blobs encontrados na matricula
 * @param src
 * @return
 */
int processPlate(IVC* src, OVC* blobs_caracteres, int* numero_blobs, OVC blob, OVC found_plate[1], OVC blobs_matricula[6], int algo) {
	char fileimagename[PATH_MAX];

	OVC* blobs_plate;
	IVC* image = vc_image_new(src->width, src->height, 1, src->levels);
	IVC* image2 = vc_image_new(src->width, src->height, 1, src->levels);
	IVC* image3 = vc_image_new(src->width, src->height, 1, src->levels);
	IVC* image5 = vc_image_new(src->width, src->height, 3, src->levels);
	IVC* image6 = vc_image_new(src->width, src->height, 3, src->levels);
	IVC* orig = vc_image_new(src->width, src->height, 3, src->levels);
	int numero2 = 0;
	
	//debugSave("plate_original", 0, src);
	memcpy(image6->data, src->data, src->width * src->height * 3);
	memcpy(image5->data, src->data, src->width * src->height * 3);


	
	vc_rgb_to_hsv(image6);

	if (algo == 1) {
		vc_hsv_segmentation(image6, 0, 255, 0, 20, 0, 50); // 50 70
	}
	else {
		vc_hsv_segmentation(image6, 0, 255, 0, 20, 0, 70); // 50 70
	}
	
	getChannel(image6, image3, 1);
	vc_binary_close(image3, image2, 2);


	
	//if (image2->height > 0 && image2->width > 0) {
	//	vc_write_image("c:\\IPCA\\image.ppm", image2);
	//}
	*numero_blobs = 0;

	
	// Cria imagem de blobs
	blobs_caracteres = vc_binary_blob_labelling(image2, image, numero_blobs);
	
	// Vai buscar a info dos blobs
	vc_binary_blob_info(image, blobs_caracteres, *numero_blobs);

	
	// Apenas blobs com mais de metade da altura que a matricula
	int min_height = blob.height * (float)0.6;
	float max_racio = 1;

	int encontrados = 0;
	
	for (int e = 0; e < *numero_blobs; e++) {
		int height_condition = 0;
		int racio_condition = 0;
		int inside_condition = 0;

		float wh_racio = (float)(blobs_caracteres[e].width) / blobs_caracteres[e].height;

		// O tamanho nao pode ser maior que o blob da matricula
		height_condition = blobs_caracteres[e].height > min_height && blobs_caracteres[e].height < blob.height;
		racio_condition = wh_racio < max_racio;
		inside_condition = 1;/*
			(
			(blobs_caracteres[e].x >= blob.x) &&  // x tem de ser maior que x do blob da matricula
				((blobs_caracteres[e].x + blobs_caracteres[e].width) <= (blob.x + blob.width)) &&
				(blobs_caracteres[e].y >= blob.y) &&
				((blobs_caracteres[e].y + blobs_caracteres[e].height) <= (blob.y + blob.height))
				);*/
		//printf("-%d%d%d-%f\n", height_condition, racio_condition, inside_condition, min_height);
		if (height_condition && racio_condition && inside_condition) {

			encontrados++;
			if (encontrados > 6) return 0;
			blobs_matricula[encontrados - 1] = blobs_caracteres[e];
			found_plate[0] = blob;

			// Testing
			// Desenha os potenciais blobs
			//desenha_bounding_box(src, &blobs_caracteres[e], 1);

			// To save digits
			IVC* temp_save = vc_image_new(blobs_caracteres[e].width, blobs_caracteres[e].height, 1, src->levels);
			extractBlobBinary(image2, temp_save, blobs_caracteres[e]);
			blobs_matricula[encontrados - 1].data = predict(temp_save);
			//strcpy(blobs_caracteres[e].data, predict(temp_save));
			//strncpy(blobs_caracteres[e].data, predict(temp_save), 1);
			//printf("\n%c\n", blobs_caracteres[e].data);
			//if (blobs_caracteres[e].width > 0 && blobs_caracteres[e].height > 0 )
			//	debugSave("caracteres", id++, temp_save);
			vc_image_free(temp_save);
			
		}
	}
	vc_image_free(image);
	vc_image_free(image2);
	vc_image_free(image3);
	vc_image_free(image5);
	vc_image_free(image6);
	vc_image_free(orig);
	return encontrados;
}

/**
 * Percorre um blog da matricula passo 1 a ver se é +potencial
 * @param src
 * @param blobs
 * @param numeroBlobs
 * @param blob_matricula
 * @param found_blobs_caracteres
 * @return
 */
int potentialBlobs(IVC* src, OVC* blobs, int numeroBlobs, OVC blob_matricula[1], OVC found_blobs_caracteres[6], int algo) {

	float wh_inf = 4, wh_sup = 5;


	// Percentagem de branco a partir da qual é considerado matricula
	// Pode-se mexer
	float white_ideal = 0.3;


	// Verificaçao de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->channels != 3)) return 0;

	//percorre os blobs da imagem
	for (int i = 0; i < numeroBlobs; i++) {

		int wh_potential = 0, area_potential = 0;
		float wh_racio = 0;

		wh_racio = (float)blobs[i].width / blobs[i].height;

		// Potencial wh_racio
		wh_potential = (wh_racio > wh_inf) && (wh_racio < wh_sup);

		if (wh_potential) { // && area_potential) {
			//IVC* plate = vc_image_new(src->width, src->height, 3, src->levels);
			
			// Potential plate extract
			//float white_ratio = extractBlob(src, plate, blobs[i]);
			

			// FOUND THE PLATE ?!?!?
			// Verifica se agora há 6 blobs lá dentro todos catitas com um ratio:D
			OVC* blobs_caracteres = NULL;
			int numero_caracteres = 0, numeros_encontrados = 0;
			// Retira os blobs da imagem da matricula

			IVC* plate2 = vc_image_new(blobs[i].width, blobs[i].height, 3, 255);
			extractBlobRGB(src, plate2, &blobs[i]);

				
			numeros_encontrados = processPlate(plate2, blobs_caracteres, &numero_caracteres, blobs[i], blob_matricula, found_blobs_caracteres, algo);
			//printf("=%d=", numeros_encontrados);
			if (numeros_encontrados == 6) {
				// ENCONTREI UMA MATRICULA têm 6 digitos lá dentro
				blob_matricula[0] = blobs[i];
				for (int car = 0; car <= 5; car++) {
					found_blobs_caracteres[car].x = found_blobs_caracteres[car].x + blob_matricula[0].x;
					found_blobs_caracteres[car].y = found_blobs_caracteres[car].y + blob_matricula[0].y;
				}
					
				// Matricula = blobs[i]
				// Caracteres =
				return numeros_encontrados;
			}
			else {
				for (int car = 0; car <= 5; car++) {
					found_blobs_caracteres[car].x = found_blobs_caracteres[car].x + blob_matricula[0].x;
					found_blobs_caracteres[car].y = found_blobs_caracteres[car].y + blob_matricula[0].y;
				}
			}


		}


	}
	

	return 0;
}




void replaceFrame(IVC* src, IVC* frame) {
	int size = src->width * src->height * frame->channels;

	// Tamanhos diferentes
	if (frame->height != src->height || frame->width != src->width)
		return 0;
	//printf(".");
	for (int i = 0; i < size; i += frame->channels ) {
		if (src->channels == 1) {
			int position = i / 3;
			frame->data[i] = src->data[position];
			frame->data[i+1] = src->data[position];
			frame->data[i+2] = src->data[position];
		}
		else {
			frame->data[i] = src->data[i];
			frame->data[i+1] = src->data[i+1];
			frame->data[i+2] = src->data[i+2];
		}
		

	}

}

void getChannel(IVC* src, IVC* out, int channel) {
	int size = src->width * src->height * src->channels;

	if (channel > 3 || channel <= 0)
		return;
	if (out->channels != 1)
		return;

	//printf(".");
	for (int i = 0; i < size; i += src->channels) {
		int position = i / src->channels;
		out->data[position] = src->data[i + channel - 1];
	}

}

char predict(IVC* image_test) {
	const int charss = 8;
	const int clauses = 5;

	int predictions[8][5] =
	{ 
	//	 Q1  Q2  Q3  Q4  RACIO
		{33, 67, 67, 38, 59}, 
		{66, 45, 54, 53, 63},
		{32, 63, 30, 13, 58},
		{55, 59, 56, 56, 57},
		{54, 68, 31, 56, 58},
		{42, 43, 40, 62, 71},
		{55, 53, 42, 41, 73},
		{40, 41, 48, 51, 71},
	};
	char chars[8] = { '2','6','7','8','9','Q','R','U' };
	
	int diffs[8][5];
	
	int matches[5] = { 0,0,0,0,0 };

	int quadrant1x = image_test->width / 2;
	int quadrant1y = image_test->height / 2;

	long int whitepxq1 = 0;
	long int whitepxq2 = 0;
	long int whitepxq3 = 0;
	long int whitepxq4 = 0;
	long int pixelsq1 = 0;
	long int pixelsq2 = 0;
	long int pixelsq3 = 0;
	long int pixelsq4 = 0;

	// Area
	long int pixels = image_test->height * image_test->width;

	// Calcula dados da imagem passada
	for (int y = 0; y < image_test->height; y++) {
		for (int x = 0; x < image_test->width; x++) {
			int pos = y * image_test->bytesperline + x * image_test->channels;
			/*
			1 2
			3 4
			*/
			if (x <= quadrant1x && y <= quadrant1y) {
				// Quadrante 1
				pixelsq1++;
				if (image_test->data[pos] == 255) {
					whitepxq1++;
				}
			}
			else if (x > quadrant1x&& y <= quadrant1y) {
				// Quadrante 2
				pixelsq2++;
				if (image_test->data[pos] == 255) {
					whitepxq2++;
				}
			}
			else if (x <= quadrant1x && y > quadrant1y) {
				// Quadrante 3
				pixelsq3++;
				if (image_test->data[pos] == 255) {
					whitepxq3++;
				}
			}
			else {
				// Quadrante 4
				pixelsq4++;
				if (image_test->data[pos] == 255) {
					whitepxq4++;
				}
			}

		}
	}
	// Q1
	matches[0] = whitepxq1 / (float)pixelsq1 * 100;
	// Q2
	matches[1] = whitepxq2 / (float)pixelsq2 * 100;
	// Q3
	matches[2] = whitepxq3 / (float)pixelsq3 * 100;
	// Q4
	matches[3] = whitepxq4 / (float)pixelsq4 * 100;
	// Racio
	matches[4] = image_test->width / (float)image_test->height * 100;

	//printf("CHAR\tQ1\tQ2\tQ3\tQ4\tRc\n");
	for (int i = 0; i < charss; i++) {
		
		//printf("%c\t", chars[i]);
		// predictions[i]
		for (int e = 0; e < clauses; e++) {
			// predictions[i][e]
			// matches[e]
			diffs[i][e] = abs(matches[e] - predictions[i][e]);
			//printf("%d\t", diffs[i][e]);
		}
		//printf("\n");
	}

	float desvios[8];
	int soma_difs[8];
	float menor_desvio = 0;
	int menor_desvio_index = 0;
	int menor_index = 0;
	int menor_valor = 0;

	// menor valor é o primeiro
	menor_valor = diffs[0][0] + diffs[0][1] + diffs[0][2] + diffs[0][3] + diffs[0][4];
	menor_desvio = calcula_desvio5(
		diffs[0][0],
		diffs[0][1],
		diffs[0][2],
		diffs[0][3],
		diffs[0][4]
	);

	for (int i = 1; i < charss; i++) {
		desvios[i] = calcula_desvio5(
			diffs[i][0],
			diffs[i][1],
			diffs[i][2],
			diffs[i][3],
			diffs[i][4]
		);
		soma_difs[i] = diffs[i][0] + diffs[i][1] + diffs[i][2] + diffs[i][3] + diffs[i][4];
		if (soma_difs[i] <= menor_valor) {
			menor_index = i;
			menor_valor = soma_difs[i];
		}
		if (desvios[i] <= menor_desvio) {
			menor_desvio_index = i;
			menor_desvio = desvios[i];
		}
	}

	for (int i = 0; i < charss; i++) {
		//printf("%c\t", chars[i]);
		//printf("%d\t", soma_difs[i]);
		//printf("Desvio %f\n", desvios[i]);
	}
	

	if (menor_desvio_index == menor_index) {
		// O menor desvio corresponde ao menor
		//printf("PREDICTED: %c\n", chars[menor_index]);
		return chars[menor_index];
		
		//return 'x';
	}
	else {
		return '?';
	}
	//printf("\n");
	//printf("\n");
	//printf("\n");


}

/**
For learning debuging
*/
void learn() {
	//IVC *working_frame;
	learnChar(vc_read_image("C:\\IPCA\\VC_TP2\\VC_TP2\\characters\\2.ppm"));
	learnChar(vc_read_image("C:\\IPCA\\VC_TP2\\VC_TP2\\characters\\6.ppm"));
	learnChar(vc_read_image("C:\\IPCA\\VC_TP2\\VC_TP2\\characters\\7.ppm"));
	learnChar(vc_read_image("C:\\IPCA\\VC_TP2\\VC_TP2\\characters\\8.ppm"));
	learnChar(vc_read_image("C:\\IPCA\\VC_TP2\\VC_TP2\\characters\\9.ppm"));
	learnChar(vc_read_image("C:\\IPCA\\VC_TP2\\VC_TP2\\characters\\Q.ppm"));
	learnChar(vc_read_image("C:\\IPCA\\VC_TP2\\VC_TP2\\characters\\R.ppm"));
	predict(vc_read_image("C:\\IPCA\\VC_TP2\\VC_TP2\\characters\\U.ppm"));
	exit(20);
};

int learnChar(IVC* image_test) {



	int quadrant1x = image_test->width / 2;
	int quadrant1y = image_test->height / 2;

	long int whitepxq1 = 0;
	long int whitepxq2 = 0;
	long int whitepxq3 = 0;
	long int whitepxq4 = 0;
	long int pixelsq1 = 0;
	long int pixelsq2 = 0;
	long int pixelsq3 = 0;
	long int pixelsq4 = 0;

	// Area
	long int pixels = image_test->height * image_test->width;

	for (int y = 0; y < image_test->height; y++) {
		for (int x = 0; x < image_test->width; x++) {
			int pos = y * image_test->bytesperline + x * image_test->channels;
			/*
			1 2
			3 4
			*/
			if (x <= quadrant1x && y <= quadrant1y) {
				// Quadrante 1
				pixelsq1++;
				if (image_test->data[pos] == 255) {
					whitepxq1++;
				}
			} else if (x > quadrant1x && y <= quadrant1y) {
				// Quadrante 2
				pixelsq2++;
				if (image_test->data[pos] == 255) {
					whitepxq2++;
				}
			} else if (x <= quadrant1x && y > quadrant1y) {
				// Quadrante 3
				pixelsq3++;
				if (image_test->data[pos] == 255) {
					whitepxq3++;
				}
			} else {
				// Quadrante 4
				pixelsq4++;
				if (image_test->data[pos] == 255) {
					whitepxq4++;
				}
			}

		}
	}

	
	printf("Q1 Brancos: %d\n", whitepxq1);
	printf("Q1 Racio: %f\n\n", whitepxq1 / (float)pixelsq1 * 100);
	printf("Q2 Brancos: %d\n", whitepxq2);
	printf("Q2 Racio: %f\n\n", whitepxq2 / (float)pixelsq2 * 100);
	printf("Q3 Brancos: %d\n", whitepxq3);
	printf("Q3 Racio: %f\n\n", whitepxq3 / (float)pixelsq3 * 100);
	printf("Q4 Brancos: %d\n", whitepxq4);
	printf("Q4 Racio: %f\n\n", whitepxq4 / (float)pixelsq4 * 100);
	printf("Racio: %f\n\n", image_test->width / (float)image_test->height );
}

/**
 * Processa uma imagem passada por argumento e faz o output do processamento para a pasta passada
 * @param name nome da imagem a processar
 * @param directorio directorio de output
 */
int processFrame(IVC* frame, IVC *frame_orig, OVC blobs_caracteres[6], OVC blob_matricula[1], int algo) {

	//OVC blob_matricula[1];
	//OVC blobs_caracteres[6];

    IVC *image[7];
    OVC *blobs[2];
    OVC *blobs_plate;
    int numero, numero2;
    int i;

	

    image[0] = vc_image_new(frame->width, frame->height, 3, frame->levels);
   // image[1] = vc_image_new(frame->width, frame->height, 1, frame->levels);
    image[2] = vc_image_new(frame->width, frame->height, 1, frame->levels);
    image[3] = vc_image_new(frame->width, frame->height, 1, frame->levels);
    //image[4] = vc_image_new(frame->width, frame->height, 1, frame->levels);
    image[5] = vc_image_new(frame->width, frame->height, 1, frame->levels);
    image[6] = vc_image_new(frame->width, frame->height, 1, frame->levels);
	image[7] = vc_image_new(frame->width, frame->height, 3, frame->levels);

    memcpy(image[0]->data, frame->data, frame->width * frame->height * 3);

    //PROCESSAMENTOS
    vc_color_remove(image[0], 12, 255);
    //debugSave("color_remove", 1, image[0]);
    vc_rgb_to_gray(image[0], image[2]);

	float histograma[256];
	long int total_pixels = histogram(image[2], histograma);
	int thres = metodo_otsu(histograma, total_pixels) - 60;

	memcpy(image[7]->data, frame->data, frame->width * frame->height * 3);

	// Faz uma segmentação tendo em conta o valor de thres pelo metodo de otsu
	vc_rgb_to_hsv(image[7]);
	vc_hsv_segmentation(image[7], 0, 255, 0, 255, thres , 255);
	
	//thres = vc_medbright(image[0]) -30;
	//vc_hsv_segmentation(image[7], 0, 255, 0, 255, thres, 255);
	

    //vc_gray_to_binary(image[7], image[3], 1);
	getChannel(image[7], image[3], 3);

    debugSave("gray_to_binary", 4, image[3]);
	

    //vc_binary_dilate(image[3], image[5], 2);
	vc_binary_dilate(image[3], image[5], 2);
	vc_binary_dilate(image[5], image[3], 2);
	vc_binary_dilate(image[3], image[5], 5);
	/*vc_binary_close(image[5], image[3], 2);
	vc_binary_close(image[3], image[5], 2);*/

    debugSave("close_and_dilate", 5, image[5]);
	
	replaceFrame(image[5], frame_orig);

    blobs_plate = vc_binary_blob_labelling(image[5], image[6],&numero2);
    
    vc_binary_blob_info(image[6],blobs_plate, numero2);

	int found = potentialBlobs(frame_orig, blobs_plate, numero2, blob_matricula, blobs_caracteres, algo);
    
    if (found == 6) {
		// Orderna os caracteres
		for (int i = 0; i < 6; i++) {
			for (int j = 0; j < (6 - 1 - i); j++) {
				
				if (blobs_caracteres[j].x > blobs_caracteres[j + 1].x) {
					OVC temp = blobs_caracteres[j];
					blobs_caracteres[j] = blobs_caracteres[j + 1];
					blobs_caracteres[j + 1] = temp;
				}
			}
		}

    } else {

		//printf("\n%d\n", blobs_caracteres);
        //desenha_bounding_box(frame, blobs_plate, numero2);

    }
    
    vc_image_free(image[0]);
    //vc_image_free(image[1]);
    vc_image_free(image[2]);
    vc_image_free(image[3]);
    //vc_image_free(image[4]);
    vc_image_free(image[5]);
    vc_image_free(image[6]);
	vc_image_free(image[7]);


    // return frame worked
    // @todo
    return found;
}

/**
 * Clareamento de imagem pela soma
 * @param src
 * @return
 */
int vc_medbright(IVC* src) {
	long int total = 0;
	long int maxpixels = src->width * src->height * src->channels;
	for (int xy = 0; xy < maxpixels; xy++) {
		total += src->data[xy];
	}
	return total / maxpixels;
}

/**
 * Clareamento de imagem pela soma
 * @param src
 * @param value
 * @return
 */
int vc_brigten(IVC *src, int value) {
    int temp;
    long int maxpixels = src->width * src->height * src->channels;
    for (int xy = 0; xy < maxpixels; xy++) {
        temp = src->data[xy] + value;
        src->data[xy] = (temp > 255) ? 255 : temp;
    }
    return 1;
}

/**
 * Calcula o desvio padrão entre rgb
 * @param r
 * @param g
 * @param b
 * @return valor de threshold
 */
int calcula_desvio(int r, int g, int b) {

    int soma = 0;
    float media, SD = 0.0;
    soma = r + g + b;
    media = soma / 3;
    SD = (r - media)*(r - media);
    SD = SD + (g - media)*(g - media);
    SD = SD + (b - media)*(b - media);
    return sqrt(SD / 3);
}

/**
 * Calcula o desvio padrão entre 4
 * @param r
 * @param g
 * @param b
 * @return valor de threshold
 */
int calcula_desvio5(int a, int b, int c, int d, int e) {

	int soma = 0;
	float media, SD = 0.0;
	soma = a + b + c + d + e;
	media = soma / 3;
	SD = (a - media) * (a - media);
	SD = SD + (b - media) * (b - media);
	SD = SD + (c - media) * (c - media);
	SD = SD + (d - media) * (d - media);
	SD = SD + (e - media) * (e - media);
	return sqrt(SD / 5);
}

/**
 * Remove cores de uma imagem tendo em conta o desvio padrão
 * @param image
 * @param threshold
 * @param color
 * @return
 */
int vc_color_remove(IVC *image, int threshold, int color) {
    unsigned char *data = (unsigned char *) image->data;
    int width = image->width;
    int height = image->height;
    int bytesperline = image->bytesperline;
    int channels = image->channels;
    int x,y;
    long int pos;

    // Verificação de erros
    if((image->width <= 0) || (image->height <= 0) || (image->data == NULL)) return 0;
    if(channels != 3) return 0;

    for(y = 0; y < height; y++) {
        for(x = 0; x < width; x++) {
            pos = y * bytesperline + x * channels;
            if (calcula_desvio(data[pos],data[pos+1],data[pos+3]) >= threshold) {
                data[pos] = color;
                data[pos + 1] = color;
                data[pos + 2] = color;
            }

        }
    }

    return 1;
}


/**
 * Desenha uma bounding box ao redor de um blog
 * @param src
 * @param blobs
 * @param numeroBlobs
 * @return
 */
int desenha_bounding_box(IVC *src, OVC* blobs, int numeroBlobs) {

    unsigned char *datasrc = (unsigned char*)src->data;
    int bytesperline_src = src->width * src->channels;

    // Apenas para imagens com 3 canais
    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
    if ((src->channels != 3)) return 0;

    // Percorre os Blobs os blobs da imagem
    for (int i = 0; i < numeroBlobs; i++) {
        //percorre a altura da box
        for (int yy = blobs[i].y; yy <= blobs[i].y + blobs[i].height;yy++) {
            //percorre a largura da box
            for (int xx = blobs[i].x; xx <= blobs[i].x + blobs[i].width;xx++) {

                //condiçao para colocar a (255,0,0) apenas os pixeis do limite da caixa
                int limite_y = (yy == blobs[i].y || yy == blobs[i].y + blobs[i].height);
                int limit_x = (xx == blobs[i].x || xx == blobs[i].x + blobs[i].width);

                if (limite_y || limit_x) {
                    long int  posk = yy * bytesperline_src + xx * src->channels;
                    datasrc[posk] = 255;
                    datasrc[posk+1] = 0;
                    datasrc[posk+2] = 0;
                }
            }
        }
    }
    return 1;
}
